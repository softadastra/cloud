<script lang="ts">
  import { onMount } from 'svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import SupporterBadge from '$lib/components/SupporterBadge.svelte';
  import PublicHeader from '$lib/components/PublicHeader.svelte';
  import { listPublicSupporters } from '$lib/api/support';
  import { ApiError } from '$lib/api/types';
  import type { PublicSupporter } from '$lib/api/types';

  let supporters: PublicSupporter[] = [];
  let loading = true;
  let error = '';

  function formatDate(value: number) {
    if (!value) return '';
    return new Intl.DateTimeFormat(undefined, { dateStyle: 'medium' }).format(new Date(value < 1_000_000_000_000 ? value * 1000 : value));
  }

  onMount(async () => {
    try {
      const data = await listPublicSupporters();
      supporters = data.supporters;
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to load supporters.';
    } finally {
      loading = false;
    }
  });
</script>

<svelte:head><title>Supporters | Softadastra Cloud</title></svelte:head>

<main class="supporters-page">
  <PublicHeader showPlatform={false} />

  <header class="hero">
    <p>Founding Supporters</p>
    <h1>The first supporters of Softadastra Cloud</h1>
    <span>The first supporters of Softadastra Cloud will be part of the early story of the project.</span>
  </header>

  <InlineError message={error} />

  {#if loading}
    <section class="empty">Loading supporters...</section>
  {:else if supporters.length === 0}
    <section class="empty">No public supporters yet.</section>
  {:else}
    <section class="supporter-grid">
      {#each supporters as supporter (`${supporter.tier}-${supporter.display_name}-${supporter.started_at}`)}
        <article class:builder={supporter.tier === 'founding_builder'}>
          <SupporterBadge tier={supporter.tier} />
          <h2>{supporter.display_name}</h2>
          {#if supporter.project_name}<p>{supporter.project_name}</p>{/if}
          <div class="links">
            {#if supporter.username}<a href={`/u/${supporter.username}`}>@{supporter.username}</a>{/if}
            {#if supporter.website_url}<a href={supporter.website_url} rel="noreferrer" target="_blank">Website</a>{/if}
            {#if supporter.github_url}<a href={supporter.github_url} rel="noreferrer" target="_blank">GitHub</a>{/if}
          </div>
          {#if supporter.started_at}<time datetime={String(supporter.started_at)}>Since {formatDate(supporter.started_at)}</time>{/if}
        </article>
      {/each}
    </section>
  {/if}
</main>

<style>
  .supporters-page { min-height: 100vh; background: var(--bg); color: var(--text); padding: 24px; }
  .hero, .supporter-grid, .empty { max-width: 1120px; margin-left: auto; margin-right: auto; }
  .links a { color: var(--link); font-size: 13px; font-weight: 650; }
  .hero { border-bottom: 1px solid var(--line); padding-bottom: 24px; }
  .hero p { color: var(--brand-bright); font-size: 11px; font-weight: 800; text-transform: uppercase; }
  .hero h1 { margin-top: 8px; color: var(--text); font-size: clamp(30px, 5vw, 52px); }
  .hero span { display: block; max-width: 680px; margin-top: 12px; color: var(--text-muted); line-height: 1.7; }
  .supporter-grid { display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); gap: 14px; margin-top: 24px; }
  .supporter-grid article, .empty { border: 1px solid var(--line); border-radius: var(--radius-md); background: var(--bg-panel); padding: 18px; }
  .supporter-grid article.builder { border-color: rgba(245, 158, 11, 0.34); }
  .supporter-grid h2 { margin-top: 12px; color: var(--text); font-size: 17px; }
  .supporter-grid p, .supporter-grid time, .empty { color: var(--text-muted); font-size: 13px; line-height: 1.6; }
  .links { display: flex; flex-wrap: wrap; gap: 10px; margin: 12px 0; }
  @media (max-width: 860px) { .supporter-grid { grid-template-columns: 1fr; } }
</style>
