<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { listLockfiles, uploadLockfile } from '$lib/api/lockfiles';
  import { listProjects } from '$lib/api/projects';
  import { listWorkspaces } from '$lib/api/workspaces';
  import { ApiError, type Lockfile, type Project, type Workspace } from '$lib/api/types';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import { canUploadLockfile } from '$lib/permissions';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  let workspaces: Workspace[] = []; let projects: Project[] = []; let lockfiles: Lockfile[] = [];
  let selectedWorkspaceId = ''; let selectedProjectId = ''; let checksumSha256 = ''; let lockfileJson = '{}'; let source = 'dashboard';
  let loading = true; let saving = false; let error = ''; let success = ''; let openJsonId = '';
  $: selectedWorkspace = workspaces.find((workspace) => workspace.id === selectedWorkspaceId) ?? null;
  $: currentRole = selectedWorkspace?.current_user_role ?? 'viewer';

  async function load() { const user = $auth.user; if (!$auth.session || !user) { await goto('/login'); return; } loading = true; error = ''; try { const params = new URLSearchParams(window.location.search); const workspaceData = await listWorkspaces(user.id); workspaces = workspaceData.workspaces; selectedWorkspaceId = params.get('workspace_id') ?? workspaces[0]?.id ?? ''; workspaceContext.setWorkspaces(workspaces, selectedWorkspaceId); await loadProjects(params.get('project_id')); } catch (err) { error = err instanceof ApiError ? err.message : 'Unable to load lockfiles.'; } finally { loading = false; } }
  async function loadProjects(projectId?: string | null) { workspaceContext.setSelectedWorkspace(selectedWorkspaceId); if (!selectedWorkspaceId) { projects = []; lockfiles = []; return; } const data = await listProjects(selectedWorkspaceId); projects = data.projects; selectedProjectId = projectId ?? projects[0]?.id ?? ''; await loadLockfiles(); }
  async function loadLockfiles() { if (!selectedWorkspaceId || !selectedProjectId) { lockfiles = []; return; } const data = await listLockfiles(selectedWorkspaceId, selectedProjectId); lockfiles = data.lockfiles; }
  async function submitLockfile() { const user = $auth.user; if (!user || !selectedWorkspaceId || !selectedProjectId || !checksumSha256.trim() || !canUploadLockfile(currentRole)) return; saving = true; error = ''; success = ''; try { const uploaded = await uploadLockfile({ workspaceId: selectedWorkspaceId, projectId: selectedProjectId, uploadedByUserId: user.id, checksumSha256, lockfileJson, source }); lockfiles = [uploaded.lockfile, ...lockfiles]; success = 'Lockfile uploaded.'; checksumSha256 = ''; lockfileJson = '{}'; source = 'dashboard'; } catch (err) { error = err instanceof ApiError ? err.message : 'Unable to upload lockfile.'; } finally { saving = false; } }
  onMount(load);
</script>

<svelte:head><title>Lockfiles | Softadastra Cloud</title></svelte:head>
<PageHeader eyebrow="Lockfiles" title="Lockfile history" workspaceName={selectedWorkspace?.name ?? ''} role={currentRole} />
<InlineError message={error} />{#if success}<p class="success-message">{success}</p>{/if}
<section class="dashboard-grid">
  <div class="panel">
    <div class="panel-header"><h2>Project</h2></div>
    <label>Workspace<select bind:value={selectedWorkspaceId} on:change={() => loadProjects()}>{#each workspaces as workspace}<option value={workspace.id}>{workspace.name}</option>{/each}</select></label>
    <label>Project<select bind:value={selectedProjectId} on:change={loadLockfiles}>{#each projects as project}<option value={project.id}>{project.name}</option>{/each}</select></label>
    {#if canUploadLockfile(currentRole)}
      <form on:submit|preventDefault={submitLockfile}><div class="panel-header"><h2>Upload lockfile</h2></div><label>Checksum SHA256<input bind:value={checksumSha256} maxlength="64" required /></label><label>Source<input bind:value={source} /></label><label>Lockfile JSON<textarea bind:value={lockfileJson} rows="8"></textarea></label><button type="submit" disabled={saving || !selectedProjectId}>{saving ? 'Uploading...' : 'Upload lockfile'}</button></form>
    {:else}<ReadOnlyNotice message="You can view lockfile history, but you cannot upload lockfiles." />{/if}
  </div>
  <div class="panel span-2"><div class="panel-header"><h2>History</h2><span>{lockfiles.length}</span></div>{#if loading}<p class="muted">Loading...</p>{:else}<div class="table-list">{#each lockfiles as lockfile}<div class="row"><span><strong>{lockfile.checksum_sha256}</strong><small>{lockfile.source} · {new Date(lockfile.created_at * 1000).toLocaleString()}</small>{#if openJsonId === lockfile.id}<pre class="json-preview">{lockfile.lockfile_json}</pre>{/if}</span><button class="small" type="button" on:click={() => openJsonId = openJsonId === lockfile.id ? '' : lockfile.id}>View JSON</button></div>{:else}<EmptyState title="No lockfiles uploaded yet." body={canUploadLockfile(currentRole) ? 'Upload the first lockfile.' : 'No lockfile has been uploaded yet.'} />{/each}</div>{/if}</div>
</section>
