<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { createWorkspace, listWorkspaces } from '$lib/api/workspaces';
  import { listProjects } from '$lib/api/projects';
  import { acceptWorkspaceInvite, declineWorkspaceInvite, listMyWorkspaceInvites } from '$lib/api/workspaceInvites';
  import { ApiError, type Project, type Workspace, type WorkspaceInvite } from '$lib/api/types';
  import { auth } from '$lib/stores/auth';

  let workspaces: Workspace[] = [];
  let invitations: WorkspaceInvite[] = [];
  let recentProjects: Project[] = [];
  let success = '';
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
      const [workspaceData, invitationData] = await Promise.all([listWorkspaces(user.id), listMyWorkspaceInvites(user.id, user.email)]);
      workspaces = workspaceData.workspaces;
      invitations = invitationData.invites;
      const projectLists = await Promise.all(workspaces.slice(0, 3).map((workspace) => listProjects(workspace.id).catch(() => ({ projects: [] }))));
      recentProjects = projectLists.flatMap((item) => item.projects).slice(0, 5);
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
      await goto(`/workspaces?workspace_id=${created.workspace.id}`);
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to create workspace.';
    } finally {
      saving = false;
    }
  }

  async function acceptInvite(invite: WorkspaceInvite) {
    const user = $auth.user;
    if (!user) return;
    error = '';
    success = '';
    try {
      await acceptWorkspaceInvite(invite.id, user.id, user.email);
      invitations = invitations.filter((item) => item.id !== invite.id);
      const data = await listWorkspaces(user.id);
      workspaces = data.workspaces;
      success = `You joined ${invite.workspace_name || 'the workspace'}.`;
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to accept invitation.';
    }
  }

  async function declineInvite(invite: WorkspaceInvite) {
    const user = $auth.user;
    if (!user) return;
    error = '';
    try {
      await declineWorkspaceInvite(invite.id, user.id, user.email);
      invitations = invitations.filter((item) => item.id !== invite.id);
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to decline invitation.';
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

{#if success}
  <p class="success-message">{success}</p>
{/if}

{#if invitations.length > 0}
  <section class="panel invitations-panel">
    <div class="panel-header"><h2>Invitations</h2><span>{invitations.length}</span></div>
    <div class="table-list">
      {#each invitations as invite}
        <div class="row">
          <span><strong>You have been invited to join {invite.workspace_name || invite.workspace_id}</strong><small>Role: {invite.role}</small></span>
          <span class="actions horizontal"><button class="small" type="button" on:click={() => acceptInvite(invite)}>Accept</button><button class="small danger" type="button" on:click={() => declineInvite(invite)}>Decline</button></span>
        </div>
      {/each}
    </div>
  </section>
{/if}

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


<section class="triple-grid">
  <div class="panel">
    <div class="panel-header"><h2>Recent projects</h2><span>{recentProjects.length}</span></div>
    <div class="table-list compact">
      {#each recentProjects as project}
        <a class="row" href="/projects?workspace_id={project.workspace_id}&project_id={project.id}"><span><strong>{project.name}</strong><small>{project.slug}</small></span></a>
      {:else}<p class="muted padded">No projects yet.</p>{/each}
    </div>
  </div>

  <div class="panel">
    <div class="panel-header"><h2>CLI state</h2></div>
    <p class="muted">Create a CLI token, then connect locally with Vix.</p>
    <a class="inline-link" href="/tokens">Open tokens</a>
  </div>

  <div class="panel">
    <div class="panel-header"><h2>Next steps</h2></div>
    <ol class="steps-list">
      <li>Create a workspace</li>
      <li>Create a project</li>
      <li>Create a CLI token</li>
      <li>Connect Vix locally</li>
      <li>Publish your first package</li>
      <li>Upload a lockfile</li>
      <li>Submit a build report</li>
    </ol>
  </div>
</section>
