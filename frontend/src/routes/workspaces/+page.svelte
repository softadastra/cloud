<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { listMembers } from '$lib/api/members';
  import { listProjects } from '$lib/api/projects';
  import { listTokens } from '$lib/api/tokens';
  import { listWorkspaces } from '$lib/api/workspaces';
  import { ApiError, type CliToken, type Member, type Project, type Workspace } from '$lib/api/types';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import RoleBadge from '$lib/components/RoleBadge.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import { canCreateProject, canManageMembers, canManageTokens } from '$lib/permissions';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  let workspaces: Workspace[] = []; let selectedWorkspace: Workspace | null = null; let projects: Project[] = []; let members: Member[] = []; let tokens: CliToken[] = [];
  let loading = true; let error = '';
  $: currentRole = selectedWorkspace?.current_user_role ?? 'viewer';

  async function load() { const user = $auth.user; if (!$auth.session || !user) { await goto('/login'); return; } loading = true; error = ''; try { const workspaceData = await listWorkspaces(user.id); workspaces = workspaceData.workspaces; const requestedId = new URLSearchParams(window.location.search).get('workspace_id') ?? ''; selectedWorkspace = workspaces.find((workspace) => workspace.id === requestedId) ?? workspaces[0] ?? null; workspaceContext.setWorkspaces(workspaces, selectedWorkspace?.id ?? ''); await loadWorkspaceDetail(); } catch (err) { error = err instanceof ApiError ? err.message : 'Unable to load workspaces.'; } finally { loading = false; } }
  async function selectWorkspace(workspace: Workspace) { selectedWorkspace = workspace; workspaceContext.setSelectedWorkspace(workspace.id); history.replaceState(null, '', `/workspaces?workspace_id=${workspace.id}`); await loadWorkspaceDetail(); }
  async function loadWorkspaceDetail() { const user = $auth.user; if (!selectedWorkspace || !user) { projects = []; members = []; tokens = []; return; } const [projectData, memberData] = await Promise.all([listProjects(selectedWorkspace.id), listMembers(selectedWorkspace.id)]); projects = projectData.projects; members = memberData.members; if (canManageTokens(currentRole)) { const tokenData = await listTokens(selectedWorkspace.id, user.id); tokens = tokenData.tokens; } else { tokens = []; } }
  onMount(load);
</script>

<svelte:head><title>Workspaces | Softadastra Cloud</title></svelte:head>
<PageHeader eyebrow="Workspaces" title="Workspace detail" workspaceName={selectedWorkspace?.name ?? ''} role={currentRole} />
<InlineError message={error} />
<section class="dashboard-grid">
  <div class="panel"><div class="panel-header"><h2>Workspaces</h2><span>{workspaces.length}</span></div>{#if loading}<p class="muted">Loading...</p>{:else if workspaces.length === 0}<EmptyState title="No workspaces yet." />{:else}<div class="table-list">{#each workspaces as workspace}<button class="row-button" type="button" on:click={() => selectWorkspace(workspace)}><span><strong>{workspace.name}</strong><small>{workspace.slug}</small></span><span class="actions"><RoleBadge role={workspace.current_user_role} /><small>{workspace.id === selectedWorkspace?.id ? 'selected' : 'open'}</small></span></button>{/each}</div>{/if}</div>
  <div class="panel span-2"><div class="panel-header"><h2>{selectedWorkspace?.name ?? 'Workspace'}</h2></div>{#if selectedWorkspace}<p class="readonly-notice">You are viewing this workspace as {currentRole}.</p><div class="meta-grid"><span><strong>ID</strong>{selectedWorkspace.id}</span><span><strong>Slug</strong>{selectedWorkspace.slug}</span><span><strong>Status</strong>{selectedWorkspace.active ? 'active' : 'inactive'}</span><span><strong>Role</strong><RoleBadge role={currentRole} /></span></div><div class="quick-actions">{#if canCreateProject(currentRole)}<a class="inline-link" href="/projects?workspace_id={selectedWorkspace.id}">New project</a>{/if}{#if canManageMembers(currentRole)}<a class="inline-link" href="/members?workspace_id={selectedWorkspace.id}">Invite member</a>{/if}{#if canManageTokens(currentRole)}<a class="inline-link" href="/tokens?workspace_id={selectedWorkspace.id}">Create CLI token</a>{/if}</div>{:else}<p class="muted">Select a workspace.</p>{/if}</div>
</section>
{#if selectedWorkspace}
  <section class="triple-grid">
    <div class="panel"><div class="panel-header"><h2>Projects</h2><span>{projects.length}</span></div><div class="table-list compact">{#each projects as project}<a class="row" href="/projects?workspace_id={selectedWorkspace.id}&project_id={project.id}"><span><strong>{project.name}</strong><small>{project.slug}</small></span></a>{:else}<EmptyState title={canCreateProject(currentRole) ? 'No projects yet.' : 'No projects have been created yet.'} body={canCreateProject(currentRole) ? 'Create the first project.' : ''} />{/each}</div></div>
    <div class="panel"><div class="panel-header"><h2>Members</h2><span>{members.length}</span></div><div class="table-list compact">{#each members as member}<div class="row"><span><strong>{member.email}</strong><small><RoleBadge role={member.role} /></small></span><StatusBadge status={member.status} /></div>{:else}<EmptyState title="No members yet." />{/each}</div></div>
    <div class="panel"><div class="panel-header"><h2>Tokens</h2><span>{tokens.length}</span></div>{#if canManageTokens(currentRole)}<div class="table-list compact">{#each tokens as token}<a class="row" href="/tokens?workspace_id={selectedWorkspace.id}"><span><strong>{token.name}</strong><small>{token.revoked ? 'revoked' : 'active'}</small></span></a>{:else}<EmptyState title="No CLI tokens yet." />{/each}</div>{:else}<ReadOnlyNotice message="Only workspace owners and admins can manage CLI tokens." />{/if}</div>
  </section>
{/if}
