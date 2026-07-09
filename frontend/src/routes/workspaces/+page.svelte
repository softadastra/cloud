<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { listMembers } from '$lib/api/members';
  import { listProjects } from '$lib/api/projects';
  import { listTokens } from '$lib/api/tokens';
  import { listWorkspaces } from '$lib/api/workspaces';
  import { ApiError, type CliToken, type Member, type Project, type Workspace } from '$lib/api/types';
  import { auth } from '$lib/stores/auth';

  let workspaces: Workspace[] = [];
  let selectedWorkspace: Workspace | null = null;
  let projects: Project[] = [];
  let members: Member[] = [];
  let tokens: CliToken[] = [];
  let loading = true;
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
      const requestedId = new URLSearchParams(window.location.search).get('workspace_id');
      selectedWorkspace = workspaces.find((workspace) => workspace.id === requestedId) ?? workspaces[0] ?? null;
      await loadWorkspaceDetail();
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to load workspaces.';
    } finally {
      loading = false;
    }
  }

  async function selectWorkspace(workspace: Workspace) {
    selectedWorkspace = workspace;
    history.replaceState(null, '', `/workspaces?workspace_id=${workspace.id}`);
    await loadWorkspaceDetail();
  }

  async function loadWorkspaceDetail() {
    const user = $auth.user;

    if (!selectedWorkspace || !user) {
      projects = [];
      members = [];
      tokens = [];
      return;
    }

    const [projectData, memberData, tokenData] = await Promise.all([
      listProjects(selectedWorkspace.id),
      listMembers(selectedWorkspace.id),
      listTokens(selectedWorkspace.id, user.id)
    ]);

    projects = projectData.projects;
    members = memberData.members;
    tokens = tokenData.tokens;
  }

  onMount(load);
</script>

<svelte:head><title>Workspaces | Softadastra Cloud</title></svelte:head>

<section class="page-header">
  <div><p class="eyebrow">Workspaces</p><h1>Workspace detail</h1></div>
</section>

{#if error}<p class="form-error">{error}</p>{/if}

<section class="dashboard-grid">
  <div class="panel">
    <div class="panel-header"><h2>Workspaces</h2><span>{workspaces.length}</span></div>
    {#if loading}
      <p class="muted">Loading...</p>
    {:else if workspaces.length === 0}
      <p class="muted">No workspaces yet.</p>
    {:else}
      <div class="table-list">
        {#each workspaces as workspace}
          <button class="row-button" type="button" on:click={() => selectWorkspace(workspace)}>
            <span><strong>{workspace.name}</strong><small>{workspace.slug}</small></span>
            <small>{workspace.id === selectedWorkspace?.id ? 'selected' : 'open'}</small>
          </button>
        {/each}
      </div>
    {/if}
  </div>

  <div class="panel span-2">
    <div class="panel-header"><h2>{selectedWorkspace?.name ?? 'Workspace'}</h2></div>
    {#if selectedWorkspace}
      <div class="meta-grid">
        <span><strong>ID</strong>{selectedWorkspace.id}</span>
        <span><strong>Slug</strong>{selectedWorkspace.slug}</span>
        <span><strong>Status</strong>{selectedWorkspace.active ? 'active' : 'inactive'}</span>
      </div>
    {:else}
      <p class="muted">Select a workspace.</p>
    {/if}
  </div>
</section>

{#if selectedWorkspace}
  <section class="triple-grid">
    <div class="panel">
      <div class="panel-header"><h2>Projects</h2><span>{projects.length}</span></div>
      <div class="table-list compact">
        {#each projects as project}
          <a class="row" href="/projects?workspace_id={selectedWorkspace.id}&project_id={project.id}"><span><strong>{project.name}</strong><small>{project.slug}</small></span></a>
        {:else}<p class="muted padded">No projects.</p>{/each}
      </div>
    </div>

    <div class="panel">
      <div class="panel-header"><h2>Members</h2><span>{members.length}</span></div>
      <div class="table-list compact">
        {#each members as member}
          <div class="row"><span><strong>{member.email}</strong><small>{member.role}</small></span><small>{member.status}</small></div>
        {:else}<p class="muted padded">No members.</p>{/each}
      </div>
    </div>

    <div class="panel">
      <div class="panel-header"><h2>Tokens</h2><span>{tokens.length}</span></div>
      <div class="table-list compact">
        {#each tokens as token}
          <a class="row" href="/tokens?workspace_id={selectedWorkspace.id}"><span><strong>{token.name}</strong><small>{token.revoked ? 'revoked' : 'active'}</small></span></a>
        {:else}<p class="muted padded">No tokens.</p>{/each}
      </div>
    </div>
  </section>
{/if}
