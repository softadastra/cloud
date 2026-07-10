<script lang="ts">
  import { onMount } from 'svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import { getAdminModules, getAdminOverview } from '$lib/api/admin';
  import { ApiError } from '$lib/api/types';
  import type { AdminModule, AdminOverview } from '$lib/api/types';

  let overview: AdminOverview | null = null;
  let modules: AdminModule[] = [];
  let error = '';

  onMount(async () => {
    try {
      [overview, { modules }] = await Promise.all([getAdminOverview(), getAdminModules()]);
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to load administration.';
    }
  });
</script>

<svelte:head><title>Administration | Softadastra Cloud</title></svelte:head>
<section class="admin-page">
  <header class="admin-header"><div><h1>Administration</h1><p>Manage platform modules, supporter records, feedback, and public moderation.</p></div><nav class="admin-nav"><a href="/admin">Overview</a><a href="/admin/users">Users</a><a href="/admin/packages">Packages</a><a href="/admin/feedback">Feedback</a><a href="/admin/supporters">Supporters</a><a href="/admin/audit">Audit</a></nav></header>
  <InlineError message={error} />
  {#if overview}
    <section class="grid">{#each Object.entries(overview.stats) as [key, value]}<article class="card"><span>{key.replaceAll('_', ' ')}</span><strong>{value}</strong></article>{/each}</section>
    <section class="grid">{#each modules as module}<article class="panel"><h2>{module.name}</h2><p>{module.description}</p><small>{module.actions.join(', ')}</small></article>{/each}</section>
  {/if}
</section>
