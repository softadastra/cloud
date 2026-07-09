<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { createWorkspace, listWorkspaces } from '$lib/api/workspaces';
  import { listProjects } from '$lib/api/projects';
  import { acceptWorkspaceInvite, declineWorkspaceInvite, listMyWorkspaceInvites } from '$lib/api/workspaceInvites';
  import { ApiError, type Project, type Workspace, type WorkspaceInvite } from '$lib/api/types';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import RoleBadge from '$lib/components/RoleBadge.svelte';
  import { canCreateProject, canManageMembers, canManageTokens, canPublishPackageVersion, canSubmitBuildReport, canUploadLockfile } from '$lib/permissions';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  let workspaces: Workspace[] = [];
  let invitations: WorkspaceInvite[] = [];
  let recentProjects: Project[] = [];
  let success = '';
  let loading = true;
  let saving = false;
  let error = '';
  let workspaceName = '';
  let workspaceSlug = '';

  $: pendingInvitations = invitations.filter((invite) => invite.status === 'pending');
  $: primaryWorkspace = workspaces[0] ?? null;
  $: primaryRole = primaryWorkspace?.current_user_role ?? 'viewer';
  $: nextSteps = roleNextSteps(primaryRole);

  $: workspaceSlug = workspaceSlug || slugify(workspaceName);

  function roleNextSteps(role: string) {
    if (canManageMembers(role) || canManageTokens(role)) {
      return ['Invite team members', 'Create CLI token', 'Create project', 'Publish package'];
    }

    if (canCreateProject(role)) {
      return ['Create project', 'Upload lockfile', 'Submit build report'];
    }

    return ['View projects', 'View packages', 'View build reports'];
  }

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
      const workspaceData = await listWorkspaces(user.id);
      workspaces = workspaceData.workspaces;
      workspaceContext.setWorkspaces(workspaces);

      try {
        const invitationData = await listMyWorkspaceInvites(user.id, user.email);
        invitations = invitationData.invites.filter((invite) => invite.status === 'pending');
      } catch {
        invitations = [];
      }
      const projectLists = await Promise.all(workspaces.slice(0, 3).map((workspace) => listProjects(workspace.id).catch(() => ({ projects: [] }))));
      recentProjects = projectLists.flatMap((item) => item.projects).slice(0, 5);
    } catch (err) {
      error = err instanceof ApiError ? err.message : err instanceof Error ? err.message : 'Unable to load dashboard.';
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
      workspaceContext.setWorkspaces(workspaces, created.workspace.id);
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
      workspaceContext.setWorkspaces(workspaces, invite.workspace_id);
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

{#if pendingInvitations.length > 0}
  <section class="panel invitations-panel">
    <div class="panel-header"><h2>Pending invitations</h2><span>{pendingInvitations.length}</span></div>
    <div class="table-list">
      {#each pendingInvitations as invite}
        <div class="row">
          <span><strong>You have been invited to join {invite.workspace_name || invite.workspace_id}</strong><small>Role: <RoleBadge role={invite.role} /></small></span>
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
            <span class="actions"><RoleBadge role={workspace.current_user_role} /><small>{workspace.active ? 'active' : 'inactive'}</small></span>
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
      {:else}<EmptyState title="No recent projects yet." body={canCreateProject(primaryRole) ? 'Create a project to start tracking activity.' : 'No projects are visible yet.'} />{/each}
    </div>
  </div>

  <div class="panel">
    <div class="panel-header"><h2>CLI state</h2></div>
    {#if canManageTokens(primaryRole)}<p class="muted">Create a CLI token, then connect locally with Vix.</p><a class="inline-link" href="/tokens">Open tokens</a>{:else}<p class="muted">CLI tokens are managed by workspace owners and admins.</p>{/if}
  </div>

  <div class="panel">
    <div class="panel-header"><h2>Next steps</h2></div>
    <ol class="steps-list">
      {#each nextSteps as step}
        <li>{step}</li>
      {/each}
    </ol>
  </div>
</section>
