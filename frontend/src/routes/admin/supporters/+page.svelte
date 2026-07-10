<script lang="ts">
  import { onMount } from 'svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import SupporterBadge from '$lib/components/SupporterBadge.svelte';
  import { createAdminSupporter, deactivateAdminSupporter, listAdminSupporters } from '$lib/api/admin';
  import { ApiError } from '$lib/api/types';
  import type { AdminSupporterSummary } from '$lib/api/types';

  let supporters: AdminSupporterSummary[] = [];
  let error = '';
  let message = '';
  let displayName = '';
  let userId = '';
  let tier = 'founding_supporter';
  let username = '';
  let projectName = '';
  let websiteUrl = '';
  let githubUrl = '';
  let publicVisible = true;

  async function load() {
    try { supporters = (await listAdminSupporters()).supporters; }
    catch (err) { error = err instanceof ApiError ? err.message : 'Unable to load supporters.'; }
  }

  async function createSupporter() {
    error = ''; message = '';
    try {
      await createAdminSupporter({ display_name: displayName, user_id: userId, tier, username, project_name: projectName, website_url: websiteUrl, github_url: githubUrl, public_visible: publicVisible });
      displayName = ''; userId = ''; username = ''; projectName = ''; websiteUrl = ''; githubUrl = ''; publicVisible = true; tier = 'founding_supporter';
      message = 'Supporter created.';
      await load();
    } catch (err) { error = err instanceof ApiError ? err.message : 'Unable to create supporter.'; }
  }

  async function deactivate(id: string) {
    if (!confirm('Deactivate this supporter record?')) return;
    await deactivateAdminSupporter(id);
    await load();
  }

  onMount(load);
</script>

<section class="admin-page">
  <header class="admin-header"><div><h1>Founding Supporters</h1><p>Manually confirm supporters after external payment confirmation. Do not store payment details here.</p></div><nav class="admin-nav"><a href="/admin">Overview</a><a href="/admin/users">Users</a><a href="/admin/packages">Packages</a><a href="/admin/feedback">Feedback</a><a href="/admin/supporters">Supporters</a><a href="/admin/audit">Audit</a></nav></header>
  <InlineError message={error} />
  {#if message}<p>{message}</p>{/if}
  <section class="panel">
    <h2>Create supporter</h2>
    <div class="form-grid">
      <label>Display name<input bind:value={displayName} /></label>
      <label>User ID optional<input bind:value={userId} /></label>
      <label>Tier<select bind:value={tier}><option value="founding_supporter">Founding Supporter</option><option value="founding_builder">Founding Builder</option></select></label>
      <label>Username<input bind:value={username} /></label>
      <label>Project name<input bind:value={projectName} /></label>
      <label>Website URL<input bind:value={websiteUrl} /></label>
      <label>GitHub URL<input bind:value={githubUrl} /></label>
      <label class="checkbox"><input type="checkbox" bind:checked={publicVisible} /> Public visible</label>
    </div>
    <button type="button" onclick={createSupporter} disabled={!displayName}>Create supporter</button>
  </section>
  <section class="panel"><table><thead><tr><th>Supporter</th><th>Status</th><th>Public</th><th>User</th><th></th></tr></thead><tbody>{#each supporters as supporter}<tr><td><SupporterBadge tier={supporter.tier} /><br/>{supporter.display_name}<br/><small>{supporter.project_name}</small></td><td>{supporter.status}</td><td>{supporter.public_visible ? 'yes' : 'no'}</td><td>{supporter.user_id || 'not linked'}</td><td>{#if supporter.status === 'active'}<button class="danger" onclick={() => deactivate(supporter.id)}>Deactivate</button>{/if}</td></tr>{/each}</tbody></table></section>
</section>
