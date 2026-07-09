<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { createWorkspace, listWorkspaces } from '$lib/api/workspaces';
  import { ApiError, type Workspace } from '$lib/api/types';
  import { auth } from '$lib/stores/auth';

  let workspaces: Workspace[] = [];
  let loading = true;
  let saving = false;
  let error = '';
  let workspaceName = '';
  let workspaceSlug = '';

  $: workspaceSlug = workspaceSlug || slugify(workspaceName);

  function slugify(value: string) {
    return value
      .trim()
      .toLowerCase()
      .replace(/[^a-z0-9]+/g, '-')
      .replace(/^-|-$/g, '');
  }

  async function load() {
    const user = $auth.user;

    if (!$auth.session || !user) {
      await goto('/login');
      return;
    }

    loading = true;
    error = '';

    try {
      const data = await listWorkspaces(user.id);
      workspaces = data.workspaces;
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to load dashboard.';
    } finally {
      loading = false;
    }
  }

  async function submitWorkspace() {
    const user = $auth.user;

    if (!user || !workspaceName.trim()) {
      return;
    }

    saving = true;
    error = '';

    try {
      const created = await createWorkspace({
        name: workspaceName.trim(),
        slug: workspaceSlug || slugify(workspaceName),
        ownerUserId: user.id
      });
      workspaces = [created.workspace, ...workspaces];
      workspaceName = '';
      workspaceSlug = '';
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to create workspace.';
    } finally {
      saving = false;
    }
  }

  onMount(load);
</script>

<svelte:head>
  <title>Dashboard | Softadastra Cloud</title>
</svelte:head>

<section class="page-header">
  <div>
    <p class="eyebrow">Dashboard</p>
    <h1>Project cloud overview</h1>
  </div>
</section>

<section class="dashboard-grid">
  <div class="panel span-2">
    <div class="panel-header">
      <h2>Workspaces</h2>
      <span>{workspaces.length}</span>
    </div>

    {#if loading}
      <p class="muted">Loading...</p>
    {:else if workspaces.length === 0}
      <p class="muted">No workspaces yet.</p>
    {:else}
      <div class="table-list">
        {#each workspaces as workspace}
          <a class="row" href="/workspaces?workspace_id={workspace.id}">
            <span>
              <strong>{workspace.name}</strong>
              <small>{workspace.slug}</small>
            </span>
            <small>{workspace.active ? 'active' : 'inactive'}</small>
          </a>
        {/each}
      </div>
    {/if}
  </div>

  <form class="panel" on:submit|preventDefault={submitWorkspace}>
    <div class="panel-header">
      <h2>New workspace</h2>
    </div>

    <label>
      Name
      <input bind:value={workspaceName} type="text" required />
    </label>

    <label>
      Slug
      <input bind:value={workspaceSlug} type="text" required />
    </label>

    {#if error}
      <p class="form-error">{error}</p>
    {/if}

    <button type="submit" disabled={saving}>{saving ? 'Creating...' : 'Create workspace'}</button>
  </form>
</section>
