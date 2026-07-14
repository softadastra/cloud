<script lang="ts">
  import { browser } from '$app/environment';
  import { page } from '$app/stores';
  import { onMount } from 'svelte';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import PublicHeader from '$lib/components/PublicHeader.svelte';
  import SupporterBadge from '$lib/components/SupporterBadge.svelte';
  import { showPublicPackage } from '$lib/api/publicPackages';
  import { ApiError } from '$lib/api/types';
  import type { PublicPackageActivityEvent, PublicPackageDetail } from '$lib/api/types';

  const API_BASE_URL = import.meta.env.VITE_API_BASE_URL ?? (browser ? window.location.protocol + '//' + window.location.hostname + ':8080' : '');
  const SITE_URL = import.meta.env.VITE_PUBLIC_SITE_URL ?? 'https://business.softadastra.com';
  const OG_IMAGE_URL = SITE_URL + '/og-logo.png';

  let packageDetail: PublicPackageDetail | null = null;
  let loading = true;
  let error = '';
  let notFound = false;

  $: namespace = $page.params.namespace ?? '';
  $: name = $page.params.name ?? '';
  $: fullName = packageDetail ? `${packageDetail.namespace}/${packageDetail.name}` : `${namespace}/${name}`;
  $: owner = packageDetail?.owner;
  $: ownerProfileHref = owner?.username ? `/u/${encodeURIComponent(owner.username)}` : '';
  $: avatarUrl = owner?.avatar_url
    ? (owner.avatar_url.startsWith('http') ? owner.avatar_url : API_BASE_URL + owner.avatar_url)
    : '';
  $: ownerInitial = (owner?.display_name || owner?.username || 'S').slice(0, 1).toUpperCase();
  $: packageDescription =
    packageDetail?.description ||
    'Public Vix package on Softadastra Cloud.';
  $: packageUrl =
    SITE_URL +
    '/p/' +
    encodeURIComponent(namespace) +
    '/' +
    encodeURIComponent(name);

  function formatDate(value: number) {
    if (!value) return '';
    return new Intl.DateTimeFormat(undefined, { dateStyle: 'medium' }).format(new Date(value < 1_000_000_000_000 ? value * 1000 : value));
  }

  function formatBytes(value: number) {
    if (!value) return '0 B';
    const units = ['B', 'KB', 'MB', 'GB'];
    let size = value;
    let unit = 0;
    while (size >= 1024 && unit < units.length - 1) {
      size = size / 1024;
      unit += 1;
    }
    return `${size >= 10 || unit === 0 ? size.toFixed(0) : size.toFixed(1)} ${units[unit]}`;
  }

  function shortChecksum(value: string) {
    return value ? value.slice(0, 12) : '';
  }

  function activityLabel(event: PublicPackageActivityEvent) {
    let parsed: Record<string, string> = {};
    try {
      parsed = JSON.parse(event.data_json || '{}') as Record<string, string>;
    } catch {
      parsed = {};
    }

    if (event.type === 'public_package_created') return 'Public package created';
    if (event.type === 'public_package_version_published' && parsed.version) return `Version ${parsed.version} published`;
    if (event.type === 'package_visibility_changed_to_public') return 'Package made public';
    return event.title;
  }

  async function loadPackage() {
    loading = true;
    error = '';
    notFound = false;
    packageDetail = null;

    try {
      const response = await showPublicPackage(namespace, name);
      packageDetail = response.package;
    } catch (err) {
      if (err instanceof ApiError && err.code === 'package_not_found') {
        notFound = true;
        return;
      }
      error = err instanceof ApiError ? err.message : 'Unable to load this public package.';
    } finally {
      loading = false;
    }
  }

  onMount(() => {
    void loadPackage();
  });
</script>

<svelte:head>
  <title>{fullName} | Softadastra Cloud</title>
  <meta name="description" content={packageDescription} />
  <link rel="canonical" href={packageUrl} />

  <meta property="og:type" content="article" />
  <meta property="og:title" content={fullName + ' | Softadastra Cloud'} />
  <meta property="og:description" content={packageDescription} />
  <meta property="og:url" content={packageUrl} />
  <meta property="og:image" content={OG_IMAGE_URL} />

  <meta name="twitter:title" content={fullName + ' | Softadastra Cloud'} />
  <meta name="twitter:description" content={packageDescription} />
  <meta name="twitter:image" content={OG_IMAGE_URL} />
</svelte:head>

<main class="package-page">
  <PublicHeader showPlatform={false} />

  {#if loading}
    <section class="state-panel"><p>Loading package...</p></section>
  {:else if notFound}
    <section class="state-panel not-found">
      <h1>Package not found.</h1>
      <p>The package may not exist or may not be public.</p>
    </section>
  {:else if error}
    <InlineError message={error} />
  {:else if packageDetail}
    <section class="package-hero">
      <div class="hero-copy">
        <div class="badge-row">
          <span>Vix package</span>
          <span>Public package</span>
          <span class:archived={packageDetail.status === 'archived'}>{packageDetail.status}</span>
        </div>
        <h1>{fullName}</h1>
        <p>{packageDetail.description || 'No package description yet.'}</p>

        <div class="owner-line">
          <a class="owner-avatar" href={ownerProfileHref || undefined} aria-label="View owner profile">
            {#if avatarUrl}<img src={avatarUrl} alt="" />{:else}<span>{ownerInitial}</span>{/if}
          </a>
          <div>
            <span>Owner</span>
            {#if ownerProfileHref}
              <a href={ownerProfileHref}>{owner?.display_name || owner?.username}</a>
            {:else}
              <strong>{owner?.display_name || 'Softadastra developer'}</strong>
            {/if}
            {#if owner?.username}<small>@{owner.username}</small>{/if}
            {#if owner?.supporter?.tier}<SupporterBadge tier={owner.supporter.tier} />{/if}
          </div>
        </div>
      </div>

      <aside class="quick-panel" aria-label="Package quick actions">
        <a href={ownerProfileHref || undefined} class:disabled={!ownerProfileHref}>View owner profile</a>
        {#if packageDetail.repository_url}
          <a href={packageDetail.repository_url} rel="noreferrer" target="_blank">Repository</a>
        {/if}
        <p>Package installation command will be available when public registry resolution is enabled.</p>
      </aside>
    </section>

    <section class="meta-grid" aria-label="Package metadata">
      <article><span>Latest version</span><strong>{packageDetail.stats.latest_version || 'No public versions'}</strong></article>
      <article><span>Published versions</span><strong>{packageDetail.stats.versions_count}</strong></article>
      <article><span>Created</span><strong>{formatDate(packageDetail.created_at)}</strong></article>
      <article><span>Updated</span><strong>{formatDate(packageDetail.updated_at)}</strong></article>
    </section>

    <section class="content-grid">
      <div class="main-column">
        <section class="panel">
          <div class="panel-heading">
            <h2>Versions</h2>
            <span>{packageDetail.stats.active_versions_count} active</span>
          </div>

          {#if packageDetail.versions.length === 0}
            <EmptyState title="No public versions" body="This package does not have public versions yet." />
          {:else}
            <div class="version-list">
              {#each packageDetail.versions as version (version.id)}
                <article class="version-row">
                  <div class="version-main">
                    <div>
                      <strong>{version.version}</strong>
                      <span class={`status ${version.status}`}>{version.status}</span>
                    </div>
                    <small>Published {formatDate(version.created_at)} · {formatBytes(version.archive_size)}</small>
                    {#if version.checksum}<code>{shortChecksum(version.checksum)}</code>{/if}
                  </div>

                  {#if version.status === 'yanked'}
                    <p class="warning">This version was yanked and is excluded from normal resolution.</p>
                  {:else if version.status === 'deprecated'}
                    <p class="warning">This version is deprecated.{version.deprecation_message ? ` ${version.deprecation_message}` : ''}</p>
                  {/if}
                </article>
              {/each}
            </div>
          {/if}
        </section>

        <section class="panel">
          <div class="panel-heading"><h2>Recent public activity</h2></div>
          {#if packageDetail.recent_activity.length === 0}
            <EmptyState title="No public activity" body="Public package activity will appear here." />
          {:else}
            <ol class="activity-list">
              {#each packageDetail.recent_activity as event (event.id)}
                <li>
                  <strong>{activityLabel(event)}</strong>
                  <time datetime={String(event.created_at)}>{formatDate(event.created_at)}</time>
                </li>
              {/each}
            </ol>
          {/if}
        </section>
      </div>

      <aside class="about-panel">
        <h2>About</h2>
        <dl>
          <div><dt>Description</dt><dd>{packageDetail.description || 'No package description yet.'}</dd></div>
          {#if packageDetail.repository_url}<div><dt>Repository</dt><dd><a href={packageDetail.repository_url} rel="noreferrer" target="_blank">{packageDetail.repository_url}</a></dd></div>{/if}
          <div><dt>Owner</dt><dd>{owner?.display_name || owner?.username || 'Softadastra developer'}</dd></div>
          <div><dt>Created</dt><dd>{formatDate(packageDetail.created_at)}</dd></div>
          <div><dt>Latest version</dt><dd>{packageDetail.stats.latest_version || 'None'}</dd></div>
        </dl>
      </aside>
    </section>
  {/if}
</main>

<style>
  .package-page { min-height: 100vh; background: var(--bg); color: var(--text); padding: 24px; }
  .package-hero, .meta-grid, .content-grid, .state-panel { width: min(1120px, 100%); margin: 0 auto; }
  .state-panel { border: 1px solid var(--line); border-radius: var(--radius-md); background: var(--bg-panel); padding: 28px; }
  .state-panel h1 { color: var(--text); font-size: 24px; }
  .state-panel p { margin-top: 8px; color: var(--text-muted); font-size: 13px; }
  .package-hero { display: grid; grid-template-columns: minmax(0, 1fr) 320px; gap: 24px; align-items: start; padding: 18px 0 24px; }
  .hero-copy { min-width: 0; }
  .badge-row { display: flex; flex-wrap: wrap; gap: 8px; }
  .badge-row span, .status { border: 1px solid var(--line-soft); border-radius: 999px; color: var(--text-muted); padding: 3px 8px; font-size: 10.5px; font-weight: 800; text-transform: capitalize; }
  .badge-row span:first-child { border-color: var(--brand-line); background: var(--brand-faint); color: var(--brand-bright); }
  .badge-row .archived, .status.deprecated { border-color: rgba(245, 158, 11, 0.38); background: rgba(245, 158, 11, 0.1); color: #fbbf24; }
  .status.yanked { border-color: rgba(248, 113, 113, 0.38); background: rgba(248, 113, 113, 0.1); color: #fca5a5; }
  .status.active { border-color: rgba(52, 211, 153, 0.34); background: rgba(52, 211, 153, 0.1); color: #6ee7b7; }
  h1 { margin-top: 14px; color: var(--text); font-size: clamp(32px, 5vw, 56px); line-height: 1.05; overflow-wrap: anywhere; }
  .hero-copy > p { max-width: 760px; margin-top: 14px; color: var(--text-muted); font-size: 15px; line-height: 1.7; }
  .owner-line { display: flex; align-items: center; gap: 12px; margin-top: 22px; }
  .owner-avatar { display: grid; width: 46px; height: 46px; place-items: center; overflow: hidden; border: 1px solid var(--line); border-radius: 50%; background: var(--brand-faint); color: var(--brand-bright); font-weight: 800; }
  .owner-avatar img { width: 100%; height: 100%; object-fit: cover; }
  .owner-line div { display: flex; flex-wrap: wrap; align-items: center; gap: 7px; min-width: 0; }
  .owner-line span, .owner-line small { color: var(--text-muted); font-size: 11px; }
  .owner-line a, .about-panel a { color: var(--link); font-weight: 700; overflow-wrap: anywhere; }
  .quick-panel, .panel, .about-panel, .meta-grid article { border: 1px solid var(--line); border-radius: var(--radius-md); background: var(--bg-panel); }
  .quick-panel { display: grid; gap: 10px; padding: 16px; }
  .quick-panel a { display: grid; min-height: 38px; place-items: center; border: 1px solid var(--brand-line); border-radius: var(--radius-sm); background: var(--brand-faint); color: var(--brand-bright); font-size: 12px; font-weight: 800; }
  .quick-panel a.disabled { pointer-events: none; opacity: .55; }
  .quick-panel p { color: var(--text-muted); font-size: 12px; line-height: 1.6; }
  .meta-grid { display: grid; grid-template-columns: repeat(4, minmax(0, 1fr)); gap: 12px; margin-bottom: 18px; }
  .meta-grid article { display: grid; gap: 5px; padding: 14px; }
  .meta-grid span, dt, .panel-heading span { color: var(--text-muted); font-size: 11px; }
  .meta-grid strong { color: var(--text); font-size: 14px; overflow-wrap: anywhere; }
  .content-grid { display: grid; grid-template-columns: minmax(0, 1fr) 320px; gap: 18px; align-items: start; }
  .main-column { display: grid; gap: 18px; min-width: 0; }
  .panel { overflow: hidden; }
  .panel-heading { display: flex; align-items: center; justify-content: space-between; gap: 12px; border-bottom: 1px solid var(--line-soft); padding: 14px 16px; }
  .panel-heading h2, .about-panel h2 { color: var(--text); font-size: 15px; }
  .version-list { display: grid; gap: 10px; padding: 14px; }
  .version-row { display: grid; gap: 10px; border: 1px solid var(--line-soft); border-radius: var(--radius-sm); background: var(--bg-ink-soft); padding: 12px; }
  .version-main { display: grid; gap: 7px; min-width: 0; }
  .version-main > div { display: flex; flex-wrap: wrap; align-items: center; gap: 8px; }
  .version-main strong { color: var(--text); font-size: 14px; }
  .version-main small, .warning, .activity-list time { color: var(--text-muted); font-size: 11px; }
  code { width: fit-content; border: 1px solid var(--line-soft); border-radius: var(--radius-sm); background: var(--bg-elevated); color: var(--text-soft); padding: 3px 6px; font-size: 11px; }
  .warning { border-left: 2px solid var(--brand-bright); padding-left: 10px; line-height: 1.5; }
  .about-panel { position: sticky; top: 22px; padding: 16px; }
  dl { display: grid; gap: 14px; margin: 14px 0 0; }
  dt { margin-bottom: 4px; }
  dd { margin: 0; color: var(--text-soft); font-size: 12px; line-height: 1.55; overflow-wrap: anywhere; }
  .activity-list { display: grid; gap: 8px; margin: 0; padding: 14px; list-style: none; }
  .activity-list li { display: flex; align-items: center; justify-content: space-between; gap: 12px; border: 1px solid var(--line-soft); border-radius: var(--radius-sm); background: var(--bg-ink-soft); padding: 10px 12px; }
  .activity-list strong { color: var(--text-soft); font-size: 12px; }
  @media (max-width: 900px) { .package-hero, .content-grid { grid-template-columns: 1fr; } .about-panel { position: static; } .meta-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); } }
  @media (max-width: 560px) { .package-page { padding: 18px; } .meta-grid { grid-template-columns: 1fr; } .activity-list li { align-items: flex-start; flex-direction: column; } }
</style>
