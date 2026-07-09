<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { createPackage, listPackages } from '$lib/api/packages';
  import { listWorkspaces } from '$lib/api/workspaces';
  import { ApiError, type Package, type Workspace } from '$lib/api/types';
  import { auth } from '$lib/stores/auth';

  let workspaces: Workspace[] = [];
  let selectedWorkspaceId = '';
  let packages: Package[] = [];
  let name = '';
  let description = '';
  let repositoryUrl = '';
  let visibility = 'private';
  let loading = true;
  let saving = false;
  let error = '';

  async function load() {
    const user = $auth.user;
    if (!$auth.session || !user) {
      await goto('/login');
      return;
    }

    loading = true;
    error = '';
    try {
      const workspaceData = await listWorkspaces(user.id);
      workspaces = workspaceData.workspaces;
      selectedWorkspaceId = new URLSearchParams(window.location.search).get('workspace_id') ?? workspaces[0]?.id ?? '';
      await loadPackages();
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to load packages.';
    } finally {
      loading = false;
    }
  }

  async function loadPackages() {
    if (!selectedWorkspaceId) {
      packages = [];
      return;
    }
    const data = await listPackages(selectedWorkspaceId);
    packages = data.packages;
  }

  async function submitPackage() {
    const user = $auth.user;
    if (!user || !selectedWorkspaceId || !name.trim()) return;
    saving = true;
    error = '';
    try {
      const created = await createPackage({ workspaceId: selectedWorkspaceId, ownerUserId: user.id, name: name.trim(), description, repositoryUrl, visibility });
      packages = [created.package, ...packages];
      name = '';
      description = '';
      repositoryUrl = '';
      visibility = 'private';
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to create package.';
    } finally {
      saving = false;
    }
  }

  onMount(load);
</script>

<svelte:head><title>Packages | Softadastra Cloud</title></svelte:head>
<section class="page-header"><div><p class="eyebrow">Registry</p><h1>Packages</h1></div></section>
{#if error}<p class="form-error">{error}</p>{/if}

<section class="dashboard-grid">
  <form class="panel" on:submit|preventDefault={submitPackage}>
    <div class="panel-header"><h2>Create package</h2></div>
    <label>Workspace<select bind:value={selectedWorkspaceId} on:change={loadPackages}>{#each workspaces as workspace}<option value={workspace.id}>{workspace.name}</option>{/each}</select></label>
    <label>Name<input bind:value={name} placeholder="vix/http" required /></label>
    <label>Description<input bind:value={description} /></label>
    <label>Repository URL<input bind:value={repositoryUrl} /></label>
    <label>Visibility<select bind:value={visibility}><option value="private">private</option><option value="public">public</option></select></label>
    <button type="submit" disabled={saving || !selectedWorkspaceId}>{saving ? 'Creating...' : 'Create package'}</button>
  </form>

  <div class="panel span-2">
    <div class="panel-header"><h2>Packages</h2><span>{packages.length}</span></div>
    {#if loading}<p class="muted">Loading...</p>{:else}
      <div class="table-list">
        {#each packages as pkg}
          <a class="row" href="/package-versions?workspace_id={pkg.workspace_id}&package_id={pkg.id}"><span><strong>{pkg.name}</strong><small>{pkg.repository_url || pkg.description || pkg.id}</small></span><small>{pkg.visibility}</small></a>
        {:else}<p class="muted padded">No packages.</p>{/each}
      </div>
    {/if}
  </div>
</section>
