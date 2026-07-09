<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { createPackage, listPackages } from '$lib/api/packages';
  import { listWorkspaces } from '$lib/api/workspaces';
  import { ApiError, type Package, type Workspace } from '$lib/api/types';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import { canCreatePackage } from '$lib/permissions';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

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
  let success = '';

  $: selectedWorkspace = workspaces.find((workspace) => workspace.id === selectedWorkspaceId) ?? null;
  $: currentRole = selectedWorkspace?.current_user_role ?? 'viewer';

  async function load() {
    const user = $auth.user;
    if (!$auth.session || !user) { await goto('/login'); return; }
    loading = true; error = '';
    try {
      const workspaceData = await listWorkspaces(user.id);
      workspaces = workspaceData.workspaces;
      selectedWorkspaceId = new URLSearchParams(window.location.search).get('workspace_id') ?? workspaces[0]?.id ?? '';
      workspaceContext.setWorkspaces(workspaces, selectedWorkspaceId);
      await loadPackages();
    } catch (err) { error = err instanceof ApiError ? err.message : 'Unable to load packages.'; }
    finally { loading = false; }
  }

  async function loadPackages() {
    workspaceContext.setSelectedWorkspace(selectedWorkspaceId);
    if (!selectedWorkspaceId) { packages = []; return; }
    const data = await listPackages(selectedWorkspaceId);
    packages = data.packages;
  }

  async function submitPackage() {
    const user = $auth.user;
    if (!user || !selectedWorkspaceId || !name.trim() || !canCreatePackage(currentRole)) return;
    saving = true; error = ''; success = '';
    try {
      const created = await createPackage({ workspaceId: selectedWorkspaceId, ownerUserId: user.id, name: name.trim(), description, repositoryUrl, visibility });
      packages = [created.package, ...packages];
      success = `Package ${created.package.name} created.`;
      name = ''; description = ''; repositoryUrl = ''; visibility = 'private';
    } catch (err) { error = err instanceof ApiError ? err.message : 'Unable to create package.'; }
    finally { saving = false; }
  }

  onMount(load);
</script>

<svelte:head><title>Packages | Softadastra Cloud</title></svelte:head>
<PageHeader eyebrow="Registry" title="Packages" workspaceName={selectedWorkspace?.name ?? ''} role={currentRole} />
<InlineError message={error} />
{#if success}<p class="success-message">{success}</p>{/if}

<section class="dashboard-grid">
  <div class="panel">
    <div class="panel-header"><h2>Workspace</h2></div>
    <label>Workspace<select bind:value={selectedWorkspaceId} on:change={loadPackages}>{#each workspaces as workspace}<option value={workspace.id}>{workspace.name}</option>{/each}</select></label>
    {#if canCreatePackage(currentRole)}
      <form on:submit|preventDefault={submitPackage}>
        <div class="panel-header"><h2>Create package</h2></div>
        <label>Name<input bind:value={name} placeholder="vix/http" required /></label>
        <label>Description<input bind:value={description} /></label>
        <label>Repository URL<input bind:value={repositoryUrl} /></label>
        <label>Visibility<select bind:value={visibility}><option value="private">private</option><option value="public">public</option></select></label>
        <button type="submit" disabled={saving || !selectedWorkspaceId}>{saving ? 'Creating...' : 'Create package'}</button>
      </form>
    {:else}
      <ReadOnlyNotice message="You can view packages in this workspace, but you cannot create or edit them." />
    {/if}
  </div>

  <div class="panel span-2">
    <div class="panel-header"><h2>Packages</h2><span>{packages.length}</span></div>
    {#if loading}<p class="muted">Loading...</p>{:else}
      <div class="table-list">
        {#each packages as pkg}
          <a class="row" href="/package-versions?workspace_id={pkg.workspace_id}&package_id={pkg.id}"><span><strong>{pkg.name}</strong><small>{pkg.repository_url || pkg.description || pkg.id}</small></span><span class="actions"><StatusBadge status={pkg.active ? 'active' : 'archived'} /><small>{pkg.visibility}</small></span></a>
        {:else}<EmptyState title="No packages yet." body={canCreatePackage(currentRole) ? 'Create the first package.' : 'No packages have been created yet.'} />{/each}
      </div>
    {/if}
  </div>
</section>
