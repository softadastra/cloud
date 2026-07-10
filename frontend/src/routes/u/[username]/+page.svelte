<script lang="ts">
  import { page } from '$app/stores';
  import { browser } from '$app/environment';
  import { onMount } from 'svelte';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import { getPublicProfile } from '$lib/api/publicProfiles';
  import { ApiError } from '$lib/api/types';
  import type { PublicProfileResponse } from '$lib/api/types';

  const API_BASE_URL = import.meta.env.VITE_API_BASE_URL ?? (browser ? `${window.location.protocol}//${window.location.hostname}:8080` : '');

  let data: PublicProfileResponse | null = null;
  let loading = true;
  let error = '';
  let privateProfile = false;

  $: username = $page.params.username ?? '';
  $: profile = data?.profile;
  $: initial = (profile?.display_name || profile?.username || 'U').slice(0, 1).toUpperCase();
  $: avatarUrl = profile?.avatar_url
    ? profile.avatar_url.startsWith('http')
      ? profile.avatar_url
      : `${API_BASE_URL}${profile.avatar_url}`
    : '';

  function formatDate(value: number) {
    if (!value) {
      return '';
    }

    return new Intl.DateTimeFormat(undefined, {
      dateStyle: 'medium'
    }).format(new Date(value < 1_000_000_000_000 ? value * 1000 : value));
  }

  async function loadProfile() {
    loading = true;
    error = '';
    privateProfile = false;

    try {
      data = await getPublicProfile(username);
    } catch (err) {
      data = null;

      if (err instanceof ApiError && (err.code === 'profile_not_found' || err.code === 'profile_private')) {
        privateProfile = true;
        return;
      }

      error = err instanceof ApiError ? err.message : 'Unable to load this public profile.';
    } finally {
      loading = false;
    }
  }

  onMount(() => {
    void loadProfile();
  });
</script>

<svelte:head>
  <title>{username} | Softadastra Cloud</title>
</svelte:head>

{#if loading}
  <section class="public-profile loading-state">
    <p>Loading profile…</p>
  </section>
{:else if privateProfile}
  <section class="public-profile private-state">
    <h1>This profile is not public.</h1>
    <p>The developer may not have enabled their public profile yet.</p>
  </section>
{:else if error}
  <InlineError message={error} />
{:else if profile && data}
  <section class="public-profile">
    <header class="profile-header">
      <div class="profile-avatar" aria-hidden="true">
        {#if avatarUrl}
          <img src={avatarUrl} alt="" />
        {:else}
          <span>{initial}</span>
        {/if}
      </div>

      <div class="profile-identity">
        <span class="profile-badge">Public profile</span>
        <h1>{profile.display_name || profile.username}</h1>
        <p class="username">@{profile.username}</p>

        {#if profile.bio}
          <p class="bio">{profile.bio}</p>
        {/if}

        <div class="profile-links">
          {#if profile.website_url}
            <a href={profile.website_url} rel="noreferrer" target="_blank">Website</a>
          {/if}

          {#if profile.github_url}
            <a href={profile.github_url} rel="noreferrer" target="_blank">GitHub</a>
          {/if}
        </div>
      </div>
    </header>

    <div class="profile-stats">
      <div>
        <strong>{data.stats.public_packages_count}</strong>
        <span>Public packages</span>
      </div>

      <div>
        <strong>{data.stats.public_contributions_count}</strong>
        <span>Public contributions</span>
      </div>
    </div>

    <section class="public-section">
      <div class="section-heading">
        <h2>Public packages</h2>
      </div>

      {#if data.public_packages.length === 0}
        <EmptyState title="No public packages" body="This developer has not published public packages yet." />
      {:else}
        <div class="package-list">
          {#each data.public_packages as packageItem (packageItem.id)}
            <article class="package-card">
              <div>
                <h3>{packageItem.name}</h3>
                {#if packageItem.description}
                  <p>{packageItem.description}</p>
                {/if}
              </div>

              <div class="package-meta">
                <span>public</span>
                <time datetime={String(packageItem.updated_at)}>{formatDate(packageItem.updated_at)}</time>
              </div>

              {#if packageItem.repository_url}
                <a href={packageItem.repository_url} rel="noreferrer" target="_blank">Repository</a>
              {/if}
            </article>
          {/each}
        </div>
      {/if}
    </section>

    <section class="public-section">
      <div class="section-heading">
        <h2>Public activity</h2>
      </div>

      {#if data.public_activity.length === 0}
        <EmptyState title="No public activity" body="Public package activity will appear here when it is available." />
      {:else}
        <ol class="activity-list">
          {#each data.public_activity as event (event.id)}
            <li>
              <div>
                <strong>{event.title}</strong>
                <span>{event.type.replaceAll('_', ' ')}</span>
              </div>
              <time datetime={String(event.created_at)}>{formatDate(event.created_at)}</time>
            </li>
          {/each}
        </ol>
      {/if}
    </section>
  </section>
{/if}

<style>
  .public-profile {
    display: grid;
    max-width: 980px;
    margin: 0 auto;
    gap: 18px;
  }

  .loading-state,
  .private-state {
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    padding: 28px;
  }

  .private-state h1 {
    color: var(--text);
    font-size: 22px;
  }

  .private-state p,
  .loading-state p {
    margin-top: 8px;
    color: var(--text-muted);
    font-size: 13px;
  }

  .profile-header {
    display: grid;
    grid-template-columns: 96px minmax(0, 1fr);
    gap: 20px;
    align-items: center;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    padding: 22px;
  }

  .profile-avatar {
    display: grid;
    width: 88px;
    height: 88px;
    place-items: center;
    overflow: hidden;
    border: 1px solid var(--brand-line);
    border-radius: 50%;
    background: var(--brand-faint);
    color: var(--brand-bright);
    font-size: 30px;
    font-weight: 700;
  }

  .profile-avatar img {
    width: 100%;
    height: 100%;
    object-fit: cover;
  }

  .profile-identity {
    display: grid;
    min-width: 0;
    gap: 7px;
  }

  .profile-badge {
    width: fit-content;
    border: 1px solid var(--brand-line);
    border-radius: 999px;
    background: var(--brand-faint);
    color: var(--brand-bright);
    padding: 3px 8px;
    font-size: 10.5px;
    font-weight: 650;
  }

  .profile-identity h1 {
    color: var(--text);
    font-size: 26px;
    line-height: 1.15;
  }

  .username,
  .bio {
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.6;
  }

  .bio {
    max-width: 680px;
    color: var(--text-soft);
  }

  .profile-links {
    display: flex;
    flex-wrap: wrap;
    gap: 10px;
  }

  .profile-links a,
  .package-card a {
    color: var(--link);
    font-size: 12px;
    font-weight: 600;
  }

  .profile-stats {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 12px;
  }

  .profile-stats > div,
  .public-section {
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .profile-stats > div {
    display: grid;
    gap: 3px;
    padding: 16px;
  }

  .profile-stats strong {
    color: var(--text);
    font-size: 22px;
  }

  .profile-stats span {
    color: var(--text-muted);
    font-size: 12px;
  }

  .section-heading {
    border-bottom: 1px solid var(--line-soft);
    padding: 14px 16px;
  }

  .section-heading h2 {
    color: var(--text);
    font-size: 14px;
  }

  .package-list {
    display: grid;
  }

  .package-card {
    display: grid;
    gap: 10px;
    border-bottom: 1px solid var(--line-soft);
    padding: 15px 16px;
  }

  .package-card:last-child {
    border-bottom: 0;
  }

  .package-card h3 {
    color: var(--text);
    font-size: 14px;
  }

  .package-card p,
  .package-meta {
    color: var(--text-muted);
    font-size: 11.5px;
    line-height: 1.5;
  }

  .package-meta {
    display: flex;
    flex-wrap: wrap;
    gap: 9px;
  }

  .activity-list {
    display: grid;
    margin: 0;
    padding: 0;
    list-style: none;
  }

  .activity-list li {
    display: flex;
    justify-content: space-between;
    gap: 14px;
    border-bottom: 1px solid var(--line-soft);
    padding: 14px 16px;
  }

  .activity-list li:last-child {
    border-bottom: 0;
  }

  .activity-list div {
    display: grid;
    gap: 3px;
  }

  .activity-list strong {
    color: var(--text-soft);
    font-size: 12px;
  }

  .activity-list span,
  .activity-list time {
    color: var(--text-muted);
    font-size: 11px;
  }

  @media (max-width: 640px) {
    .profile-header,
    .profile-stats {
      grid-template-columns: 1fr;
    }

    .activity-list li {
      flex-direction: column;
    }
  }
</style>
