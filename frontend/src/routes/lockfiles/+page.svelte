<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { listLockfiles, uploadLockfile } from '$lib/api/lockfiles';
  import { listProjects } from '$lib/api/projects';
  import { listWorkspaces } from '$lib/api/workspaces';
  import { ApiError, type Lockfile, type Project, type Workspace } from '$lib/api/types';
  import { auth } from '$lib/stores/auth';

  let workspaces: Workspace[] = [];
  let projects: Project[] = [];
  let lockfiles: Lockfile[] = [];
  let selectedWorkspaceId = '';
  let selectedProjectId = '';
  let checksumSha256 = '';
  let lockfileJson = '{}';
  let source = 'dashboard';
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
      const params = new URLSearchParams(window.location.search);
      const workspaceData = await listWorkspaces(user.id);
      workspaces = workspaceData.workspaces;
      selectedWorkspaceId = params.get('workspace_id') ?? workspaces[0]?.id ?? '';
      await loadProjects(params.get('project_id'));
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to load lockfiles.';
    } finally {
      loading = false;
    }
  }

  async function loadProjects(projectId?: string | null) {
    if (!selectedWorkspaceId) {
      projects = [];
      lockfiles = [];
      return;
    }
    const data = await listProjects(selectedWorkspaceId);
    projects = data.projects;
    selectedProjectId = projectId ?? projects[0]?.id ?? '';
    await loadLockfiles();
  }

  async function loadLockfiles() {
    if (!selectedWorkspaceId || !selectedProjectId) {
      lockfiles = [];
      return;
    }
    const data = await listLockfiles(selectedWorkspaceId, selectedProjectId);
    lockfiles = data.lockfiles;
  }

  async function submitLockfile() {
    const user = $auth.user;
    if (!user || !selectedWorkspaceId || !selectedProjectId || !checksumSha256.trim()) return;
    saving = true;
    error = '';
    try {
      const uploaded = await uploadLockfile({ workspaceId: selectedWorkspaceId, projectId: selectedProjectId, uploadedByUserId: user.id, checksumSha256, lockfileJson, source });
      lockfiles = [uploaded.lockfile, ...lockfiles];
      checksumSha256 = '';
      lockfileJson = '{}';
      source = 'dashboard';
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to upload lockfile.';
    } finally {
      saving = false;
    }
  }

  onMount(load);
</script>

<svelte:head><title>Lockfiles | Softadastra Cloud</title></svelte:head>
<section class="page-header"><div><p class="eyebrow">Lockfiles</p><h1>Lockfile history</h1></div></section>
{#if error}<p class="form-error">{error}</p>{/if}

<section class="dashboard-grid">
  <form class="panel" on:submit|preventDefault={submitLockfile}>
    <div class="panel-header"><h2>Upload lockfile</h2></div>
    <label>Workspace<select bind:value={selectedWorkspaceId} on:change={() => loadProjects()}>{#each workspaces as workspace}<option value={workspace.id}>{workspace.name}</option>{/each}</select></label>
    <label>Project<select bind:value={selectedProjectId} on:change={loadLockfiles}>{#each projects as project}<option value={project.id}>{project.name}</option>{/each}</select></label>
    <label>Checksum SHA256<input bind:value={checksumSha256} maxlength="64" required /></label>
    <label>Source<input bind:value={source} /></label>
    <label>Lockfile JSON<textarea bind:value={lockfileJson} rows="8"></textarea></label>
    <button type="submit" disabled={saving || !selectedProjectId}>{saving ? 'Uploading...' : 'Upload lockfile'}</button>
  </form>

  <div class="panel span-2">
    <div class="panel-header"><h2>History</h2><span>{lockfiles.length}</span></div>
    {#if loading}<p class="muted">Loading...</p>{:else}
      <div class="table-list">
        {#each lockfiles as lockfile}
          <div class="row"><span><strong>{lockfile.checksum_sha256}</strong><small>{lockfile.source}</small></span><small>{new Date(lockfile.created_at * 1000).toLocaleString()}</small></div>
        {:else}<p class="muted padded">No lockfiles.</p>{/each}
      </div>
    {/if}
  </div>
</section>
