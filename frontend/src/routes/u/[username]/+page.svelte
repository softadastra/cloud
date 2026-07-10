<script lang="ts">
  import { browser } from '$app/environment';
  import { page } from '$app/stores';
  import { onMount } from 'svelte';
  import ContributionGrid from '$lib/components/ContributionGrid.svelte';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import { updateProfilePins } from '$lib/api/profilePins';
  import { getPublicProfile } from '$lib/api/publicProfiles';
  import { ApiError } from '$lib/api/types';
  import { auth } from '$lib/stores/auth';
  import type { PublicActivityEvent, PublicPackageSummary, PublicProfileResponse } from '$lib/api/types';

  const API_BASE_URL = import.meta.env.VITE_API_BASE_URL ?? (browser ? window.location.protocol + '//' + window.location.hostname + ':8080' : '');
  const validTabs = new Set(['overview', 'packages', 'activity']);

  let data: PublicProfileResponse | null = null;
  let loading = true;
  let error = '';
  let privateProfile = false;
  let pinsOpen = false;
  let selectedPinIds: string[] = [];
  let pinMessage = '';
  let pinError = '';
  let savingPins = false;

  $: username = $page.params.username ?? '';
  $: requestedTab = $page.url.searchParams.get('tab') ?? 'overview';
  $: activeTab = validTabs.has(requestedTab) ? requestedTab : 'overview';
  $: profile = data?.profile;
  $: initial = (profile?.display_name || profile?.username || 'U').slice(0, 1).toUpperCase();
  $: avatarUrl = profile?.avatar_url ? (profile.avatar_url.startsWith('http') ? profile.avatar_url : API_BASE_URL + profile.avatar_url) : '';
  $: isOwner = Boolean($auth.user?.username && profile?.username && $auth.user.username === profile.username);
  $: canCustomizePins = isOwner && Boolean($auth.user?.public_profile_enabled && $auth.user?.username);
  $: packagePreview = (data?.public_packages ?? []).slice(0, 6);
  $: activityPreview = (data?.recent_activity ?? []).slice(0, 6);

  function tabHref(tab: string) {
    return tab === 'overview' ? `/u/${encodeURIComponent(username)}` : `/u/${encodeURIComponent(username)}?tab=${tab}`;
  }

  function formatDate(value: number) {
    if (!value) return '';
    return new Intl.DateTimeFormat(undefined, { dateStyle: 'medium' }).format(new Date(value < 1_000_000_000_000 ? value * 1000 : value));
  }

  function activityLabel(event: PublicActivityEvent) {
    let parsed: Record<string, string> = {};
    try {
      parsed = JSON.parse(event.data_json || '{}') as Record<string, string>;
    } catch {
      parsed = {};
    }

    if (event.type === 'public_package_created' && parsed.package) return 'Published package ' + parsed.package;
    if (event.type === 'public_package_version_published' && parsed.package && parsed.version) return 'Published version ' + parsed.version + ' of ' + parsed.package;
    if (event.type === 'profile_updated') return 'Updated public profile';
    return event.title;
  }

  function packageHref(packageItem: PublicPackageSummary) {
    return '/packages?package_id=' + encodeURIComponent(packageItem.id);
  }

  function openPins() {
    selectedPinIds = [...(data?.pinned_packages ?? []).map((item) => item.id)];
    pinMessage = '';
    pinError = '';
    pinsOpen = true;
  }

  function closePins() {
    pinsOpen = false;
  }

  function togglePin(packageId: string) {
    pinError = '';
    if (selectedPinIds.includes(packageId)) {
      selectedPinIds = selectedPinIds.filter((id) => id !== packageId);
      return;
    }

    if (selectedPinIds.length >= 6) {
      pinError = 'You can pin up to 6 public packages.';
      return;
    }

    selectedPinIds = [...selectedPinIds, packageId];
  }

  async function savePins() {
    savingPins = true;
    pinError = '';
    pinMessage = '';

    try {
      await updateProfilePins(selectedPinIds);
      pinMessage = 'Pinned packages updated.';
      await loadProfile(false);
    } catch (err) {
      pinError = err instanceof ApiError ? err.message : 'Unable to update pinned packages.';
    } finally {
      savingPins = false;
    }
  }

  async function loadProfile(showLoading = true) {
    if (showLoading) loading = true;
    error = '';
    privateProfile = false;

    try {
      data = await getPublicProfile(username);
      selectedPinIds = data.pinned_packages.map((item) => item.id);

      if (browser && new URLSearchParams(window.location.search).get('pins') === '1') {
        setTimeout(() => {
          if (canCustomizePins) openPins();
        }, 0);
      }
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

{#snippet packageCard(packageItem: PublicPackageSummary, pinned = false)}
  <article class:package-card={true} class:pinned={pinned}>
    <div class="package-title-row">
      <h3><a href={packageHref(packageItem)}>{packageItem.name}</a></h3>
      <span class="visibility-pill">Public</span>
    </div>
    <p>{packageItem.description || 'No package description yet.'}</p>
    <div class="package-footer">
      <span class="package-kind">Vix package</span>
      {#if packageItem.latest_version}<span>Latest {packageItem.latest_version}</span>{/if}
      <span>{packageItem.versions_count ?? 0} versions</span>
      {#if packageItem.updated_at}<time datetime={String(packageItem.updated_at)}>Updated {formatDate(packageItem.updated_at)}</time>{/if}
    </div>
    {#if packageItem.repository_url}
      <a class="repo-link" href={packageItem.repository_url} rel="noreferrer" target="_blank">Repository</a>
    {/if}
  </article>
{/snippet}

{#snippet activityList(events: PublicActivityEvent[])}
  {#if events.length === 0}
    <EmptyState title="No public activity yet" body="Public package and profile activity will appear here." />
  {:else}
    <ol class="activity-list">
      {#each events as event (event.id)}
        <li>
          <span class="activity-dot" aria-hidden="true"></span>
          <div>
            <strong>{activityLabel(event)}</strong>
            <div class="activity-meta"><span>{event.type.replaceAll('_', ' ')}</span><time datetime={String(event.created_at)}>{formatDate(event.created_at)}</time></div>
          </div>
        </li>
      {/each}
    </ol>
  {/if}
{/snippet}

<svelte:head>
  <title>{username} | Softadastra Cloud</title>
</svelte:head>

{#if loading}
  <section class="public-profile loading-state"><p>Loading profile...</p></section>
{:else if privateProfile}
  <section class="public-profile private-state">
    <h1>This profile is not public.</h1>
    <p>The developer may not have enabled their public profile yet.</p>
  </section>
{:else if error}
  <InlineError message={error} />
{:else if profile && data}
  <div class="profile-page">
    <header class="topbar">
      <div>
        <p>Softadastra Cloud developer</p>
        <h1>{profile.display_name || profile.username}</h1>
      </div>
      {#if canCustomizePins}
        <button type="button" class="pin-button" onclick={openPins}>Customize pins</button>
      {/if}
    </header>

    <nav class="profile-tabs" aria-label="Public profile sections">
      <a class:active={activeTab === 'overview'} href={tabHref('overview')}>Overview</a>
      <a class:active={activeTab === 'packages'} href={tabHref('packages')}>Packages <span>{data.stats.public_packages_count}</span></a>
      <a class:active={activeTab === 'activity'} href={tabHref('activity')}>Activity</a>
    </nav>

    <section class="public-profile">
      <aside class="profile-column">
        <div class="profile-avatar" aria-hidden="true">
          {#if avatarUrl}<img src={avatarUrl} alt="" />{:else}<span>{initial}</span>{/if}
        </div>

        <div class="profile-name">
          <h2>{profile.display_name || profile.username}</h2>
          <p>@{profile.username}</p>
        </div>

        {#if profile.bio}<p class="bio">{profile.bio}</p>{/if}

        <div class="profile-links">
          {#if profile.website_url}<a href={profile.website_url} rel="noreferrer" target="_blank">Website</a>{/if}
          {#if profile.github_url}<a href={profile.github_url} rel="noreferrer" target="_blank">GitHub</a>{/if}
        </div>

        <dl class="profile-stats">
          <div><dt>Public packages</dt><dd>{data.stats.public_packages_count}</dd></div>
          <div><dt>Public contributions</dt><dd>{data.stats.public_contributions_count}</dd></div>
          <div><dt>Pinned packages</dt><dd>{data.stats.pinned_packages_count}</dd></div>
        </dl>
      </aside>

      <main class="profile-main">
        {#if activeTab === 'overview'}
          {#if data.pinned_packages.length > 0}
            <section class="profile-block">
              <div class="block-heading">
                <h2>Pinned packages</h2>
                {#if canCustomizePins}<button type="button" class="link-button" onclick={openPins}>Customize pins</button>{/if}
              </div>
              <div class="pinned-grid">
                {#each data.pinned_packages as packageItem (packageItem.id)}
                  {@render packageCard(packageItem, true)}
                {/each}
              </div>
            </section>
          {:else if canCustomizePins}
            <section class="owner-pin-callout">
              <div>
                <h2>Pinned packages</h2>
                <p>Select public packages to highlight on your profile.</p>
              </div>
              <button type="button" class="link-button" onclick={openPins}>Customize pins</button>
            </section>
          {/if}

          <section class="profile-block">
            <div class="block-heading">
              <h2>Public contributions</h2>
              <span>Public Softadastra Cloud activity, not Git commits.</span>
            </div>
            <ContributionGrid days={data.contribution_grid} />
          </section>

          <section class="profile-block two-column-block">
            <div>
              <div class="block-heading inline-heading">
                <h2>Recent activity</h2>
                <a href={tabHref('activity')}>View all</a>
              </div>
              {@render activityList(activityPreview)}
            </div>
            <div>
              <div class="block-heading inline-heading">
                <h2>Public packages</h2>
                <a href={tabHref('packages')}>View all</a>
              </div>
              {#if packagePreview.length === 0}
                <EmptyState title="No public packages" body="This developer has not published public packages yet." />
              {:else}
                <div class="compact-package-list">
                  {#each packagePreview.slice(0, 3) as packageItem (packageItem.id)}
                    <a href={packageHref(packageItem)}>{packageItem.name}<span>{packageItem.latest_version || 'No version yet'}</span></a>
                  {/each}
                </div>
              {/if}
            </div>
          </section>
        {:else if activeTab === 'packages'}
          <section class="profile-block">
            <div class="block-heading">
              <h2>Public packages</h2>
              <span>{data.stats.public_packages_count} published publicly</span>
            </div>
            {#if data.public_packages.length === 0}
              <EmptyState title="No public packages" body="This developer has not published public packages yet." />
            {:else}
              <div class="package-list">
                {#each data.public_packages as packageItem (packageItem.id)}
                  {@render packageCard(packageItem)}
                {/each}
              </div>
            {/if}
          </section>
        {:else if activeTab === 'activity'}
          <section class="profile-block">
            <div class="block-heading">
              <h2>Public contributions</h2>
              <span>Public package and profile events from the last year.</span>
            </div>
            <ContributionGrid days={data.contribution_grid} />
          </section>
          <section class="profile-block">
            <div class="block-heading">
              <h2>Recent public activity</h2>
              <span>{data.stats.public_contributions_count} public contributions</span>
            </div>
            {@render activityList(data.recent_activity)}
          </section>
        {/if}
      </main>
    </section>
  </div>

  {#if pinsOpen}
    <div class="modal-backdrop" role="presentation" onclick={closePins}></div>
    <div class="pins-modal" role="dialog" aria-modal="true" aria-labelledby="pins-title">
      <div class="modal-header">
        <div>
          <h2 id="pins-title">Customize pinned packages</h2>
          <p>Choose up to 6 public packages to show on your profile.</p>
        </div>
        <button type="button" class="icon-close" aria-label="Close" onclick={closePins}>x</button>
      </div>

      {#if !$auth.user?.public_profile_enabled}
        <p class="pin-note">Enable your public profile before customizing pins.</p>
      {:else if data.public_packages.length === 0}
        <p class="pin-note">You do not have public packages to pin yet.</p>
      {:else}
        <div class="pin-list">
          {#each data.public_packages as packageItem (packageItem.id)}
            <label class="pin-row">
              <input type="checkbox" checked={selectedPinIds.includes(packageItem.id)} onchange={() => togglePin(packageItem.id)} />
              <span><strong>{packageItem.name}</strong><small>{packageItem.description || 'No description'}</small></span>
            </label>
          {/each}
        </div>
      {/if}

      {#if pinError}<InlineError message={pinError} />{/if}
      {#if pinMessage}<p class="pin-success">{pinMessage}</p>{/if}

      <div class="modal-actions">
        <button type="button" class="secondary" onclick={closePins}>Cancel</button>
        <button type="button" onclick={savePins} disabled={savingPins || !$auth.user?.public_profile_enabled}>{savingPins ? 'Saving...' : 'Save pins'}</button>
      </div>
    </div>
  {/if}
{/if}

<style>
  .profile-page { max-width: 1180px; margin: 0 auto; }
  .loading-state, .private-state { border: 1px solid var(--line); border-radius: var(--radius-md); background: var(--bg-panel); padding: 28px; }
  .private-state h1 { color: var(--text); font-size: 22px; }
  .private-state p, .loading-state p { margin-top: 8px; color: var(--text-muted); font-size: 13px; }
  .topbar { display: flex; align-items: center; justify-content: space-between; gap: 16px; border-bottom: 1px solid var(--line); padding: 8px 0 18px; }
  .topbar p { color: var(--brand-bright); font-size: 11px; font-weight: 700; text-transform: uppercase; }
  .topbar h1 { margin-top: 4px; color: var(--text); font-size: 24px; line-height: 1.2; }
  .pin-button { white-space: nowrap; }
  .profile-tabs { display: flex; gap: 22px; border-bottom: 1px solid var(--line); margin-bottom: 22px; }
  .profile-tabs a { display: flex; align-items: center; gap: 6px; border-bottom: 2px solid transparent; color: var(--text-muted); padding: 13px 2px 11px; font-size: 13px; font-weight: 650; }
  .profile-tabs a.active { border-bottom-color: var(--brand-bright); color: var(--text); }
  .profile-tabs span { border: 1px solid var(--line-soft); border-radius: 999px; padding: 1px 7px; color: var(--text-muted); font-size: 10px; }
  .public-profile { display: grid; grid-template-columns: 280px minmax(0, 1fr); gap: 26px; align-items: start; }
  .profile-column { position: sticky; top: 22px; display: grid; gap: 14px; min-width: 0; }
  .profile-avatar { display: grid; width: 184px; height: 184px; place-items: center; overflow: hidden; border: 1px solid var(--line); border-radius: 50%; background: var(--brand-faint); color: var(--brand-bright); font-size: 58px; font-weight: 700; }
  .profile-avatar img { width: 100%; height: 100%; object-fit: cover; }
  .profile-name h2 { color: var(--text); font-size: 24px; line-height: 1.15; overflow-wrap: anywhere; }
  .profile-name p, .bio { color: var(--text-muted); font-size: 14px; line-height: 1.6; }
  .bio { color: var(--text-soft); }
  .profile-links { display: grid; gap: 7px; }
  .profile-links a, .repo-link, .block-heading a, .package-title-row a { color: var(--link); font-weight: 650; }
  .profile-links a { font-size: 13px; }
  .profile-stats { display: grid; gap: 8px; margin: 0; border-top: 1px solid var(--line-soft); padding-top: 12px; }
  .profile-stats div { display: flex; justify-content: space-between; gap: 10px; }
  .profile-stats dt { color: var(--text-muted); font-size: 12px; }
  .profile-stats dd { margin: 0; color: var(--text); font-size: 12px; font-weight: 700; }
  .profile-main { display: grid; gap: 18px; min-width: 0; }
  .profile-block, .owner-pin-callout { border: 1px solid var(--line); border-radius: var(--radius-md); background: var(--bg-panel); overflow: hidden; }
  .owner-pin-callout { display: flex; align-items: center; justify-content: space-between; gap: 16px; padding: 15px 16px; }
  .owner-pin-callout h2, .block-heading h2 { color: var(--text); font-size: 15px; }
  .owner-pin-callout p, .block-heading span { color: var(--text-muted); font-size: 12px; }
  .block-heading { display: flex; align-items: center; justify-content: space-between; gap: 12px; border-bottom: 1px solid var(--line-soft); padding: 14px 16px; }
  .inline-heading { border-bottom: 0; padding: 0 0 12px; }
  .link-button { min-height: auto; border: 0; background: transparent; color: var(--link); padding: 0; font-size: 12px; font-weight: 700; }
  .pinned-grid { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 12px; padding: 14px; }
  .package-list { display: grid; gap: 10px; padding: 14px; }
  .package-card { display: grid; gap: 10px; min-width: 0; border: 1px solid var(--line-soft); border-radius: var(--radius-sm); background: var(--bg-ink-soft); padding: 13px; }
  .package-card.pinned { background: var(--brand-faint); border-color: var(--brand-line); }
  .package-title-row { display: flex; align-items: flex-start; justify-content: space-between; gap: 12px; }
  .package-title-row h3 { font-size: 14px; overflow-wrap: anywhere; }
  .visibility-pill { border: 1px solid var(--line-soft); border-radius: 999px; color: var(--text-muted); padding: 2px 7px; font-size: 10px; }
  .package-card p { color: var(--text-muted); font-size: 12px; line-height: 1.5; }
  .package-footer { display: flex; flex-wrap: wrap; gap: 8px; color: var(--text-muted); font-size: 10.5px; }
  .package-kind { color: var(--brand-bright); font-weight: 700; }
  .repo-link { width: fit-content; font-size: 12px; }
  .two-column-block { display: grid; grid-template-columns: minmax(0, 1.1fr) minmax(260px, 0.9fr); gap: 18px; padding: 16px; }
  .activity-list { display: grid; margin: 0; padding: 0; list-style: none; }
  .activity-list li { display: grid; grid-template-columns: 12px minmax(0, 1fr); gap: 10px; border-bottom: 1px solid var(--line-soft); padding: 12px 0; }
  .activity-list li:first-child { padding-top: 0; }
  .activity-list li:last-child { border-bottom: 0; padding-bottom: 0; }
  .activity-dot { width: 8px; height: 8px; margin-top: 5px; border-radius: 50%; background: var(--brand-bright); }
  .activity-list strong { color: var(--text-soft); font-size: 12px; }
  .activity-meta { display: flex; flex-wrap: wrap; gap: 8px; margin-top: 3px; color: var(--text-muted); font-size: 10.5px; }
  .compact-package-list { display: grid; gap: 8px; }
  .compact-package-list a { display: flex; justify-content: space-between; gap: 12px; border: 1px solid var(--line-soft); border-radius: var(--radius-sm); color: var(--link); padding: 10px; font-size: 12px; font-weight: 700; }
  .compact-package-list span { color: var(--text-muted); font-weight: 500; }
  .modal-backdrop { position: fixed; inset: 0; z-index: 80; background: rgba(0, 0, 0, 0.42); }
  .pins-modal { position: fixed; z-index: 90; top: 50%; left: 50%; display: grid; width: min(620px, calc(100vw - 28px)); max-height: calc(100vh - 48px); overflow: auto; transform: translate(-50%, -50%); border: 1px solid var(--line); border-radius: var(--radius-md); background: var(--bg-panel); box-shadow: var(--shadow-lg); }
  .modal-header { display: flex; justify-content: space-between; gap: 14px; border-bottom: 1px solid var(--line-soft); padding: 15px 16px; }
  .modal-header h2 { color: var(--text); font-size: 15px; }
  .modal-header p, .pin-note { color: var(--text-muted); font-size: 11.5px; }
  .icon-close { min-width: 30px; min-height: 30px; padding: 0; }
  .pin-list { display: grid; gap: 8px; padding: 14px 16px; }
  .pin-row { display: grid; grid-template-columns: 18px minmax(0, 1fr); gap: 10px; align-items: start; border: 1px solid var(--line-soft); border-radius: var(--radius-sm); padding: 10px; }
  .pin-row strong { display: block; color: var(--text); font-size: 12px; }
  .pin-row small { display: block; margin-top: 3px; color: var(--text-muted); font-size: 11px; }
  .pin-note, .pin-success { margin: 14px 16px; }
  .pin-success { color: var(--success); font-size: 11.5px; }
  .modal-actions { display: flex; justify-content: flex-end; gap: 10px; border-top: 1px solid var(--line-soft); padding: 14px 16px; }
  .secondary { background: transparent; color: var(--text-soft); }
  @media (max-width: 900px) { .public-profile { grid-template-columns: 1fr; } .profile-column { position: static; } .profile-avatar { width: 132px; height: 132px; font-size: 42px; } .two-column-block, .pinned-grid { grid-template-columns: 1fr; } }
  @media (max-width: 560px) { .topbar, .owner-pin-callout, .modal-actions { align-items: stretch; flex-direction: column; } .profile-tabs { overflow-x: auto; } }
</style>
