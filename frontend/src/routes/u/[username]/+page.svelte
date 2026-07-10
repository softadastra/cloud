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

  function isEmailLike(value?: string) {
    if (!value) {
      return false;
    }

    const trimmed = value.trim();
    const at = trimmed.indexOf('@');

    return (
      at > 0 &&
      at < trimmed.length - 1 &&
      trimmed.indexOf('.', at + 1) > at + 1
    );
  }

  let data: PublicProfileResponse | null = null;
  let loading = true;
  let error = '';
  let privateProfile = false;
  let pinsOpen = false;
  let selectedPinIds: string[] = [];
  let pinMessage = '';
  let pinError = '';
  let savingPins = false;
  let selectedYear = new Date().getUTCFullYear();
  let contributionLoading = false;

  $: username = $page.params.username ?? '';
  $: requestedTab = $page.url.searchParams.get('tab') ?? 'overview';
  $: activeTab = validTabs.has(requestedTab) ? requestedTab : 'overview';
  $: profile = data?.profile;

  $: publicUsername =
    profile?.username &&
    !isEmailLike(profile.username)
      ? profile.username
      : '';

  $: publicDisplayName =
    profile?.display_name &&
    !isEmailLike(profile.display_name)
      ? profile.display_name
      : publicUsername || 'Softadastra developer';

  $: initial = publicDisplayName
    .slice(0, 1)
    .toUpperCase();

  $: avatarUrl = profile?.avatar_url
    ? (
        profile.avatar_url.startsWith('http')
          ? profile.avatar_url
          : API_BASE_URL + profile.avatar_url
      )
    : '';
$: isOwner = Boolean(
  $auth.session &&
  $auth.user?.username &&
  username &&
  $auth.user.username.toLowerCase() ===
    username.toLowerCase()
);

$: canCustomizePins = isOwner;
  $: packagePreview = (data?.public_packages ?? []).slice(0, 6);
  $: contributionYears = data?.contribution_years ?? [selectedYear];

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

  function activityKind(event: PublicActivityEvent) {
    if (event.type === 'public_package_version_published') return 'Version';
    if (event.type === 'public_package_created') return 'Package';
    if (event.type === 'profile_updated') return 'Profile';
    return 'Activity';
  }

  function activityIcon(event: PublicActivityEvent) {
    if (event.type === 'public_package_version_published') return 'tag';
    if (event.type === 'public_package_created') return 'box';
    return 'profile';
  }

  function activityDay(value: number) {
    const date = new Date(value < 1_000_000_000_000 ? value * 1000 : value);
    return new Intl.DateTimeFormat(undefined, {
      weekday: 'short', day: 'numeric', month: 'short', year: 'numeric'
    }).format(date);
  }

  function groupedActivity(events: PublicActivityEvent[]) {
    const groups = new Map<string, PublicActivityEvent[]>();
    for (const event of events) {
      const key = activityDay(event.created_at);
      groups.set(key, [...(groups.get(key) ?? []), event]);
    }
    return [...groups.entries()].map(([date, items]) => ({ date, items }));
  }

  async function changeContributionYear(year: number) {
    if (!data || year === selectedYear) return;
    contributionLoading = true;
    try {
      const next = await getPublicProfile(username, year);
      data = { ...data, contribution_grid: next.contribution_grid,
        contribution_years: next.contribution_years,
        selected_contribution_year: next.selected_contribution_year };
      selectedYear = next.selected_contribution_year;
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to load contribution activity.';
    } finally {
      contributionLoading = false;
    }
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
      selectedYear = data.selected_contribution_year ?? new Date().getUTCFullYear();
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
    <div class="activity-timeline">
      {#each groupedActivity(events) as group (group.date)}
        <section class="activity-group">
          <h3>{group.date}</h3>
          <ol>
            {#each group.items as event (event.id)}
              <li>
                <span class={`event-icon ${activityIcon(event)}`} aria-hidden="true">
                  {#if activityIcon(event) === 'tag'}#
                  {:else if activityIcon(event) === 'box'}□
                  {:else}●{/if}
                </span>
                <div class="event-copy">
                  <strong>{activityLabel(event)}</strong>
                  <span>{activityKind(event)} · {formatDate(event.created_at)}</span>
                </div>
              </li>
            {/each}
          </ol>
        </section>
      {/each}
    </div>
  {/if}
{/snippet}

<svelte:head>
  <title>{publicDisplayName} | Softadastra Cloud</title>
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
        <h2>{publicDisplayName}</h2>

        {#if publicUsername}
          <p>@{publicUsername}</p>
        {/if}
      </div>

        <span class="profile-label">Softadastra Cloud builder</span>

        {#if profile.bio}<p class="bio">{profile.bio}</p>{/if}

       <div class="profile-links">
    {#if profile.website_url}
      <a
        href={profile.website_url}
        rel="noreferrer"
        target="_blank"
      >
        {profile.website_url}
      </a>
    {/if}

    {#if profile.github_url}
      <a
        href={profile.github_url}
        rel="noreferrer"
        target="_blank"
      >
        {profile.github_url}
      </a>
    {/if}
  </div>

        {#if isOwner}
          <a class="edit-profile" href="/account">Edit public profile</a>
        {/if}
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
            <ContributionGrid days={data.contribution_grid} year={selectedYear}
              years={contributionYears} loading={contributionLoading}
              onYearChange={changeContributionYear} />
          </section>

          {#if data.pinned_packages.length === 0 && packagePreview.length > 0}
            <section class="profile-block">
              <div class="block-heading inline-action">
                <div><h2>Published packages</h2><span>Latest public work from this builder</span></div>
                <a href={tabHref('packages')}>Explore all</a>
              </div>
              <div class="pinned-grid">
                {#each packagePreview.slice(0, 4) as packageItem (packageItem.id)}
                  {@render packageCard(packageItem)}
                {/each}
              </div>
            </section>
          {/if}
        {:else if activeTab === 'packages'}
          <section class="profile-block">
            <div class="block-heading">
              <h2>Public packages</h2>
              <span>Packages shared with the Softadastra ecosystem</span>
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
      <h2>Recent public activity</h2>
      <span>A readable history of public work</span>
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

      {#if data.public_packages.length === 0}
    <p class="pin-note">
      You do not have public packages to pin yet.
    </p>
  {:else}
    <div class="pin-list">
      {#each data.public_packages as packageItem (packageItem.id)}
        <label class="pin-row">
          <input
            type="checkbox"
            checked={selectedPinIds.includes(packageItem.id)}
            onchange={() => togglePin(packageItem.id)}
          />

          <span>
            <strong>{packageItem.name}</strong>
            <small>
              {packageItem.description || 'No description'}
            </small>
          </span>
        </label>
      {/each}
    </div>
  {/if}

      {#if pinError}<InlineError message={pinError} />{/if}
      {#if pinMessage}<p class="pin-success">{pinMessage}</p>{/if}

      <div class="modal-actions">
        <button type="button" class="secondary" onclick={closePins}>Cancel</button>
        <button
  type="button"
  onclick={savePins}
  disabled={savingPins}
>{savingPins ? 'Saving...' : 'Save pins'}</button>
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
  .profile-label { width: fit-content; border: 1px solid var(--brand-line); border-radius: 999px; background: var(--brand-faint); color: var(--brand-bright); padding: 5px 9px; font-size: 10.5px; font-weight: 700; letter-spacing: .02em; }
  .edit-profile { display: grid; min-height: 34px; place-items: center; border: 1px solid var(--line); border-radius: var(--radius-sm); background: var(--bg-elevated); color: var(--text-soft); font-size: 11.5px; font-weight: 650; }
  .edit-profile:hover { border-color: var(--brand-line); color: var(--brand-bright); }
  .profile-links { display: grid; gap: 7px; }
  .profile-links a, .repo-link, .block-heading a, .package-title-row a { color: var(--link); font-weight: 650; }
.profile-links a {
  width: fit-content;
  max-width: 100%;
  color: var(--link);
  font-size: 13px;
  overflow-wrap: anywhere;
  word-break: break-word;
}

.profile-links a:hover {
  color: var(--link-hover);
  text-decoration: underline;
  text-underline-offset: 3px;
}
  .profile-main { display: grid; gap: 18px; min-width: 0; }
  .profile-block, .owner-pin-callout { border: 1px solid var(--line); border-radius: var(--radius-md); background: var(--bg-panel); overflow: hidden; }
  .owner-pin-callout { display: flex; align-items: center; justify-content: space-between; gap: 16px; padding: 15px 16px; }
  .owner-pin-callout h2, .block-heading h2 { color: var(--text); font-size: 15px; }
  .owner-pin-callout p, .block-heading span { color: var(--text-muted); font-size: 12px; }
  .block-heading { display: flex; align-items: center; justify-content: space-between; gap: 12px; border-bottom: 1px solid var(--line-soft); padding: 14px 16px; }
  .block-heading.inline-action > div { display: grid; gap: 3px; }
  .inline-heading { border-bottom: 0; padding: 0 0 12px; }
  .link-button { min-height: auto; border: 0; background: transparent; color: var(--link); padding: 0; font-size: 12px; font-weight: 700; }
  .pinned-grid { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 12px; padding: 14px; }
  .package-list { display: grid; gap: 10px; padding: 14px; }
  .package-card { display: grid; gap: 10px; min-width: 0; border: 1px solid var(--line-soft); border-radius: var(--radius-sm); background: var(--bg-ink-soft); padding: 13px; }
.package-card.pinned {
  background: var(--bg-ink-soft);
  border-color: var(--line-soft);
}
.link-button:hover:not(:disabled) {
  background: transparent;
  color: var(--link-hover);
  text-decoration: underline;
  text-underline-offset: 3px;
  transform: none;
}
  .package-title-row { display: flex; align-items: flex-start; justify-content: space-between; gap: 12px; }
  .package-title-row h3 { font-size: 14px; overflow-wrap: anywhere; }
  .visibility-pill { border: 1px solid var(--line-soft); border-radius: 999px; color: var(--text-muted); padding: 2px 7px; font-size: 10px; }
  .package-card p { color: var(--text-muted); font-size: 12px; line-height: 1.5; }
  .package-footer { display: flex; flex-wrap: wrap; gap: 8px; color: var(--text-muted); font-size: 10.5px; }
  .package-kind { color: var(--brand-bright); font-weight: 700; }
  .repo-link { width: fit-content; font-size: 12px; }
  .two-column-block { display: grid; grid-template-columns: minmax(0, 1.1fr) minmax(260px, 0.9fr); gap: 18px; padding: 16px; }
  .activity-timeline { display: grid; padding: 4px 18px 20px; }
  .activity-group { display: grid; grid-template-columns: 126px minmax(0, 1fr); gap: 18px; padding: 18px 0; border-bottom: 1px solid var(--line-soft); }
  .activity-group:last-child { border-bottom: 0; }
  .activity-group h3 { color: var(--text-muted); font-size: 10.5px; font-weight: 650; line-height: 1.5; }
  .activity-group ol { display: grid; gap: 8px; margin: 0; padding: 0; list-style: none; }
  .activity-group li { display: grid; grid-template-columns: 34px minmax(0, 1fr); gap: 11px; align-items: center; min-height: 54px; border: 1px solid var(--line-soft); border-radius: var(--radius-sm); background: var(--bg-ink-soft); padding: 9px 11px; }
  .event-icon { display: grid; width: 32px; height: 32px; place-items: center; border: 1px solid var(--brand-line); border-radius: 9px; background: var(--brand-faint); color: var(--brand-bright); font-size: 12px; font-weight: 800; }
  .event-icon.tag { border-radius: 50%; }
  .event-icon.profile { color: var(--text-soft); border-color: var(--line); background: var(--bg-elevated); }
  .event-copy { display: grid; min-width: 0; gap: 4px; }
  .event-copy strong { color: var(--text-soft); font-size: 12px; line-height: 1.45; overflow-wrap: anywhere; }
  .event-copy span { color: var(--text-muted); font-size: 10.5px; }
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
  @media (max-width: 560px) { .topbar, .owner-pin-callout, .modal-actions { align-items: stretch; flex-direction: column; } .profile-tabs { overflow-x: auto; } .activity-group { grid-template-columns: 1fr; gap: 9px; } }
</style>
