<script lang="ts">
  import { usePageRefresh } from '$lib/stores/pageRefresh';
  import { goto } from '$app/navigation';
  import { tick } from 'svelte';
  import { get } from 'svelte/store';
  import {
    listMembers,
    reactivateMember,
    removeMember,
    suspendMember,
    updateMemberRole
  } from '$lib/api/members';
  import { listProjects } from '$lib/api/projects';
  import {
    createWorkspaceInvite,
    listWorkspaceInvites,
    revokeWorkspaceInvite
  } from '$lib/api/workspaceInvites';
  import { listWorkspaces } from '$lib/api/workspaces';
  import {
    ApiError,
    type Member,
    type Project,
    type Workspace,
    type WorkspaceInvite
  } from '$lib/api/types';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import LimitedAccessNotice from '$lib/components/LimitedAccessNotice.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import { canManageMembersContext } from '$lib/permissions';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  type MemberAction = 'suspend' | 'reactivate' | 'remove';

  type Confirmation =
    | {
        kind: 'invite';
        invite: WorkspaceInvite;
      }
    | {
        kind: 'member';
        member: Member;
        action: MemberAction;
      };

  let workspaces: Workspace[] = [];
  let projects: Project[] = [];
  let members: Member[] = [];
  let invites: WorkspaceInvite[] = [];

  let selectedWorkspaceId = '';
  let showInviteForm = false;

  let email = '';
  let role = 'member';
  let accessScope = 'entire_workspace';
  let selectedProjectIds: string[] = [];

  let loading = true;
  let saving = false;
  let initialized = false;
  let loadSeq = 0;
  let busyId = '';
  let error = '';
  let success = '';

  let confirmation: Confirmation | null = null;
  let confirmationDialog: HTMLDialogElement | null = null;

  $: selectedWorkspace =
    workspaces.find((workspace) => workspace.id === selectedWorkspaceId) ??
    null;

  $: permissionContext = getPermissionContext(selectedWorkspaceId);
  $: currentRole = permissionContext.role;
  $: canManage = canManageMembersContext(permissionContext);

  $: globalWorkspaceId =
    $workspaceContext.selectedWorkspace?.id ?? '';

  $: if (
    initialized &&
    globalWorkspaceId &&
    globalWorkspaceId !== selectedWorkspaceId &&
    workspaces.some((workspace) => workspace.id === globalWorkspaceId)
  ) {
    void loadForWorkspace(globalWorkspaceId, { updateUrl: true });
  }

  $: pendingInvites = invites.filter(
    (invite) => invite.status === 'pending'
  );

  $: inviteHistory = invites.filter(
    (invite) => invite.status !== 'pending'
  );

  $: activeMembers = members.filter(
    (member) => member.status === 'active'
  );

  $: suspendedMembers = members.filter(
    (member) => member.status === 'suspended'
  );

  $: removedMembers = members.filter(
    (member) => member.status === 'removed'
  );

  $: inactiveMembers = [
    ...suspendedMembers,
    ...removedMembers
  ];

  function getPermissionContext(workspaceId: string) {
    const workspace = workspaces.find(
      (item) => item.id === workspaceId
    );

    return {
      role: workspace?.current_user_role ?? 'viewer',
      status: workspace?.current_user_status ?? 'active',
      access_scope: workspace?.access_scope ?? 'entire_workspace',
      project_ids_json: workspace?.project_ids_json ?? ''
    };
  }

  function parseProjectIds(value?: string) {
    if (!value) {
      return [];
    }

    try {
      return JSON.parse(value) as string[];
    } catch {
      return [];
    }
  }

  function projectNames(value?: string) {
    const ids = parseProjectIds(value);

    if (ids.length === 0) {
      return 'All projects';
    }

    return ids
      .map(
        (id) =>
          projects.find((project) => project.id === id)?.name ?? id
      )
      .join(', ');
  }

  function accessLabel(scope?: string) {
    return scope === 'selected_projects'
      ? 'Selected projects'
      : 'Entire workspace';
  }
  function roleLabel(value: string) {
    return value.charAt(0).toUpperCase() + value.slice(1);
  }

  function statusLabel(value: string) {
    return value.charAt(0).toUpperCase() + value.slice(1);
  }

  function isCurrentUser(member: Member) {
    return member.user_id === $auth.user?.id;
  }

  function canEditMember(member: Member) {
    return (
      canManage &&
      member.role !== 'owner' &&
      !isCurrentUser(member)
    );
  }

  function toggleProject(projectId: string) {
    selectedProjectIds = selectedProjectIds.includes(projectId)
      ? selectedProjectIds.filter((id) => id !== projectId)
      : [...selectedProjectIds, projectId];
  }

  function resetInviteForm() {
    email = '';
    role = 'member';
    accessScope = 'entire_workspace';
    selectedProjectIds = [];
  }

  function closeInviteForm() {
    if (saving) {
      return;
    }

    showInviteForm = false;
    resetInviteForm();
    error = '';
  }

  function handleInviteSubmit(event: SubmitEvent) {
    event.preventDefault();
    void submitInvite();
  }

  function handleRoleChange(member: Member, event: Event) {
    const select = event.currentTarget as HTMLSelectElement;
    void changeRole(member, select.value);
  }

  function memberErrorMessage(err: unknown) {
    return err instanceof ApiError
      ? err.message
      : 'Unable to load members.';
  }

  function resolveWorkspaceId(requestedWorkspaceId: string | null) {
    const storedWorkspaceId =
      globalWorkspaceId ||
      get(workspaceContext).selectedWorkspace?.id ||
      '';

    if (
      requestedWorkspaceId &&
      workspaces.some((workspace) => workspace.id === requestedWorkspaceId)
    ) {
      return requestedWorkspaceId;
    }

    if (
      storedWorkspaceId &&
      workspaces.some((workspace) => workspace.id === storedWorkspaceId)
    ) {
      return storedWorkspaceId;
    }

    return workspaces[0]?.id ?? '';
  }

  function updateWorkspaceUrl(workspaceId: string) {
    const url = new URL(window.location.href);

    if (workspaceId) {
      url.searchParams.set('workspace_id', workspaceId);
    } else {
      url.searchParams.delete('workspace_id');
    }

    history.replaceState(null, '', `${url.pathname}${url.search}`);
  }

  async function load() {
    if (!$auth.session) {
      await goto('/login');
      return;
    }

    if (!$auth.user) {
      const status = await auth.ensureAuthLoaded();
      if (status === 'missing' || status === 'invalid') {
        await goto('/login');
        return;
      }
    }

    const user = $auth.user;

    if (!user) {
      error = 'Unable to load the current account.';
      loading = false;
      return;
    }

    const seq = ++loadSeq;
    loading = true;
    error = '';
    success = '';
    initialized = false;
    members = [];
    invites = [];
    projects = [];

    try {
      const workspaceData = await listWorkspaces(user.id);

      if (seq !== loadSeq) {
        return;
      }

      workspaces = workspaceData.workspaces;
      const requestedWorkspaceId = new URLSearchParams(
        window.location.search
      ).get('workspace_id');
      const workspaceId = resolveWorkspaceId(requestedWorkspaceId);

      workspaceContext.setWorkspaces(workspaces, workspaceId);
      initialized = true;

      if (!workspaceId) {
        selectedWorkspaceId = '';
        loading = false;
        return;
      }

      await loadForWorkspace(workspaceId, { seq });
    } catch (err) {
      if (seq !== loadSeq) {
        return;
      }

      error = memberErrorMessage(err);
      loading = false;
      initialized = true;
    }
  }

  async function loadForWorkspace(
    workspaceId: string,
    options: { updateUrl?: boolean; seq?: number } = {}
  ) {
    const seq = options.seq ?? ++loadSeq;

    if (!workspaceId) {
      if (seq === loadSeq) {
        selectedWorkspaceId = '';
        members = [];
        invites = [];
        projects = [];
        loading = false;
      }
      return;
    }

    selectedWorkspaceId = workspaceId;
    members = [];
    invites = [];
    projects = [];
    selectedProjectIds = [];
    showInviteForm = false;
    resetInviteForm();
    confirmation = null;
    error = '';
    success = '';
    loading = true;

    workspaceContext.setSelectedWorkspace(workspaceId);

    if (options.updateUrl) {
      updateWorkspaceUrl(workspaceId);
    }

    const context = getPermissionContext(workspaceId);
    const canLoadInvites = canManageMembersContext(context);

    if (import.meta.env.DEV) {
      console.debug('[members] loading workspace', workspaceId);
    }

    try {
      const [memberData, projectData, inviteData] = await Promise.all([
        listMembers(workspaceId),
        listProjects(workspaceId),
        canLoadInvites
          ? listWorkspaceInvites(workspaceId)
          : Promise.resolve({ invites: [] })
      ]);

      const currentWorkspaceId = get(workspaceContext).selectedWorkspace?.id ?? '';

      if (seq !== loadSeq || selectedWorkspaceId !== workspaceId || currentWorkspaceId !== workspaceId) {
        if (import.meta.env.DEV) {
          console.debug('[members] ignored stale response', workspaceId);
        }
        return;
      }

      if (import.meta.env.DEV) {
        console.debug('[members] response workspace', workspaceId, {
          members: memberData.members.length,
          invites: inviteData.invites.length,
          projects: projectData.projects.length
        });
      }

      members = memberData.members;
      projects = projectData.projects;
      invites = inviteData.invites;
    } catch (err) {
      if (seq !== loadSeq || selectedWorkspaceId !== workspaceId) {
        return;
      }

      error = memberErrorMessage(err);
    } finally {
      if (seq === loadSeq && selectedWorkspaceId === workspaceId) {
        loading = false;
      }
    }
  }

  async function submitInvite() {
    const user = $auth.user;

    if (
      !user ||
      !selectedWorkspaceId ||
      !email.trim() ||
      !canManage
    ) {
      return;
    }

    if (
      accessScope === 'selected_projects' &&
      selectedProjectIds.length === 0
    ) {
      error =
        'Choose at least one project for selected project access.';
      return;
    }

    saving = true;
    error = '';
    success = '';

    try {
      const invited = await createWorkspaceInvite({
        workspaceId: selectedWorkspaceId,
        invitedEmail: email.trim(),
        role,
        invitedByUserId: user.id,
        accessScope,
        projectIdsJson:
          accessScope === 'selected_projects'
            ? JSON.stringify(selectedProjectIds)
            : ''
      });

      invites = [invited.invite, ...invites];

      success = `Invitation sent to ${email.trim()}.`;

      resetInviteForm();
      showInviteForm = false;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to send invitation.';
    } finally {
      saving = false;
    }
  }

  async function changeRole(
    member: Member,
    nextRole: string
  ) {
    const user = $auth.user;

    if (
      !user ||
      !canEditMember(member) ||
      nextRole === member.role
    ) {
      return;
    }

    busyId = member.id;
    error = '';
    success = '';

    try {
      const updated = await updateMemberRole({
        workspaceId: member.workspace_id,
        userId: member.user_id,
        role: nextRole,
        actorUserId: user.id
      });

      members = members.map((item) =>
        item.id === member.id ? updated.member : item
      );

      success = `Role updated for ${member.email}.`;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to update role.';
    } finally {
      busyId = '';
    }
  }

  async function openConfirmation(next: Confirmation) {
    confirmation = next;

    await tick();
    confirmationDialog?.focus();
  }

  function closeConfirmation() {
    if (busyId) {
      return;
    }

    confirmation = null;
  }

  function confirmationTitle(value: Confirmation) {
    if (value.kind === 'invite') {
      return 'Revoke invitation?';
    }

    if (value.action === 'suspend') {
      return 'Suspend member?';
    }

    if (value.action === 'remove') {
      return 'Remove member?';
    }

    return 'Reactivate member?';
  }

  function confirmationMessage(value: Confirmation) {
    if (value.kind === 'invite') {
      return `The invitation sent to ${value.invite.invited_email} will no longer be usable.`;
    }

    if (value.action === 'suspend') {
      return `${value.member.email} will lose access until the account is reactivated.`;
    }

    if (value.action === 'remove') {
      return `${value.member.email} will be removed from this workspace and lose access.`;
    }

    return `${value.member.email} will regain access using the current role and project scope.`;
  }

  function confirmationButtonLabel(value: Confirmation) {
    if (value.kind === 'invite') {
      return 'Revoke invitation';
    }

    if (value.action === 'suspend') {
      return 'Suspend member';
    }

    if (value.action === 'remove') {
      return 'Remove member';
    }

    return 'Reactivate member';
  }

  function confirmationIsDanger(value: Confirmation) {
    return (
      value.kind === 'invite' ||
      value.action === 'suspend' ||
      value.action === 'remove'
    );
  }

  async function confirmAction() {
    const user = $auth.user;
    const pending = confirmation;

    if (!user || !pending || !canManage) {
      return;
    }

    error = '';
    success = '';

    if (pending.kind === 'invite') {
      busyId = pending.invite.id;

      try {
        const revoked = await revokeWorkspaceInvite({
          workspaceId: pending.invite.workspace_id,
          inviteId: pending.invite.id,
          revokedByUserId: user.id
        });

        invites = invites.map((invite) =>
          invite.id === pending.invite.id
            ? revoked.invite
            : invite
        );

        success = 'Invitation revoked.';
        confirmation = null;
      } catch (err) {
        error =
          err instanceof ApiError
            ? err.message
            : 'Unable to revoke invitation.';
      } finally {
        busyId = '';
      }

      return;
    }

    const member = pending.member;
    const action = pending.action;

    busyId = member.id;

    try {
      const request = {
        workspaceId: member.workspace_id,
        userId: member.user_id,
        actorUserId: user.id
      };

      const updated =
        action === 'suspend'
          ? await suspendMember(request)
          : action === 'reactivate'
            ? await reactivateMember(request)
            : await removeMember(request);

      members = members.map((item) =>
        item.id === member.id ? updated.member : item
      );

      success =
        action === 'suspend'
          ? 'Member suspended.'
          : action === 'reactivate'
            ? 'Member reactivated.'
            : 'Member removed.';

      confirmation = null;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : `Unable to ${action} member.`;
    } finally {
      busyId = '';
    }
  }

  function handleModalKeydown(event: KeyboardEvent) {
    if (
      event.key === 'Escape' &&
      confirmation &&
      !busyId
    ) {
      event.preventDefault();
      closeConfirmation();
    }
  }

  usePageRefresh(load);
</script>

<svelte:head>
  <title>Members | Softadastra Cloud</title>
</svelte:head>

<svelte:window onkeydown={handleModalKeydown} />

<PageHeader
  eyebrow="Access"
  title="Members"
  workspaceName={selectedWorkspace?.name ?? ''}
  role={currentRole}
>
  {#if canManage}
    <button
      class="invite-member-button"
      type="button"
      onclick={() => {
        showInviteForm = !showInviteForm;
        error = '';
        success = '';

        if (!showInviteForm) {
          resetInviteForm();
        }
      }}
    >
      {showInviteForm ? 'Close' : 'Invite member'}
    </button>
  {/if}
</PageHeader>

<div class="page-introduction">
  <p>
    Manage who can access this workspace and which projects they can see.
  </p>

  {#if !loading && selectedWorkspaceId}
    <div class="member-summary" aria-label="Workspace access summary">
      <span>
        <strong>{activeMembers.length}</strong>
        active
      </span>

      {#if canManage}
        <span>
          <strong>{pendingInvites.length}</strong>
          pending
        </span>
      {/if}

      <span>
        <strong>{inactiveMembers.length}</strong>
        inactive
      </span>
    </div>
  {/if}
</div>

{#if selectedWorkspaceId && permissionContext.access_scope === 'selected_projects'}
  <LimitedAccessNotice />
{/if}

<InlineError message={error} />

{#if success}
  <p class="success-message" role="status">
    {success}
  </p>
{/if}

{#if showInviteForm && canManage}
  <section class="invite-panel">
    <div class="invite-panel__header">
      <div>
        <h2>Invite a member</h2>
        <p>Set the role and project access before sending the invitation.</p>
      </div>
    </div>

    <form
      class="invite-form"
      onsubmit={handleInviteSubmit}
    >
      <label class="email-field">
        Email address

        <input
          bind:value={email}
          type="email"
          placeholder="teammate@example.com"
          required
          autocomplete="off"
        />
      </label>

      <label>
        Role

        <select bind:value={role}>
          <option value="admin">Admin</option>
          <option value="member">Member</option>
          <option value="viewer">Viewer</option>
        </select>
      </label>

      <label>
        Access

        <select bind:value={accessScope}>
          <option value="entire_workspace">
            Entire workspace
          </option>

          <option value="selected_projects">
            Selected projects
          </option>
        </select>
      </label>

      {#if accessScope === 'selected_projects'}
        <div class="project-selector">
          <div class="project-selector__header">
            <strong>Projects</strong>
            <span>
              {selectedProjectIds.length} selected
            </span>
          </div>

          {#if projects.length === 0}
            <p class="project-empty">
              Create a project before using selected project access.
            </p>
          {:else}
            <div class="project-list">
              {#each projects as project (project.id)}
                <label class="project-option">
                  <input
                    type="checkbox"
                    checked={selectedProjectIds.includes(project.id)}
                    onchange={() => toggleProject(project.id)}
                  />

                  <span>{project.name}</span>
                </label>
              {/each}
            </div>
          {/if}
        </div>
      {/if}

      <div class="invite-actions">
        <button
          class="secondary-button"
          type="button"
          disabled={saving}
          onclick={closeInviteForm}
        >
          Cancel
        </button>

        <button
          type="submit"
          disabled={
            saving ||
            !email.trim() ||
            !selectedWorkspaceId ||
            (
              accessScope === 'selected_projects' &&
              selectedProjectIds.length === 0
            )
          }
        >
          {saving ? 'Sending…' : 'Send invitation'}
        </button>
      </div>
    </form>
  </section>
{/if}

{#if !loading && selectedWorkspaceId && !canManage}
  <ReadOnlyNotice
    message="Your role can view members, but only owners and admins can manage access."
  />
{/if}

{#if loading}
  <section class="directory-section">
    <div class="directory-header">
      <div>
        <h2>Workspace members</h2>
      </div>
    </div>

    <p class="loading-state">Loading members…</p>
  </section>
{:else if !selectedWorkspaceId}
  <section class="directory-section">
    <div class="directory-header">
      <div>
        <h2>Workspace members</h2>
      </div>
    </div>

    <div class="empty-wrapper">
      <EmptyState title="Select a workspace" body="Choose a workspace before reviewing members and invitations." />
    </div>
  </section>
{:else}
  {#if canManage && pendingInvites.length > 0}
    <section class="directory-section">
      <div class="directory-header">
        <div>
          <h2>Pending invitations</h2>
          <p>Invitations that have not been accepted yet.</p>
        </div>

        <span class="section-count">
          {pendingInvites.length}
        </span>
      </div>

      <div class="directory-table">
        <div class="table-heading invitation-grid">
          <span>Invitee</span>
          <span>Access</span>
          <span>Role</span>
          <span class="align-right">Action</span>
        </div>

        {#each pendingInvites as invite (invite.id)}
          <div class="directory-row invitation-grid">
            <div class="identity-cell">
              <strong>{invite.invited_email}</strong>
              <small>Invitation pending</small>
            </div>

            <div class="information-cell" data-label="Access">
              <strong>{accessLabel(invite.access_scope)}</strong>

              {#if invite.access_scope === 'selected_projects'}
                <small>{projectNames(invite.project_ids_json)}</small>
              {/if}
            </div>

            <div class="information-cell" data-label="Role">
              <span class="neutral-badge">
                {roleLabel(invite.role)}
              </span>
            </div>

            <div class="row-actions" data-label="Action">
              <button
                class="danger-button"
                type="button"
                disabled={busyId === invite.id}
                onclick={() =>
                  openConfirmation({
                    kind: 'invite',
                    invite
                  })}
              >
                Revoke
              </button>
            </div>
          </div>
        {/each}
      </div>
    </section>
  {/if}

  <section class="directory-section">
    <div class="directory-header">
      <div>
        <h2>Active members</h2>
        <p>People who currently have access to this workspace.</p>
      </div>

      <span class="section-count">
        {activeMembers.length}
      </span>
    </div>

    {#if activeMembers.length === 0}
      <div class="empty-wrapper">
        <EmptyState title="No active members" />
      </div>
    {:else}
      <div class="directory-table">
        <div class="table-heading member-grid">
          <span>Member</span>
          <span>Access</span>
          <span>Role</span>
          <span class="align-right">Actions</span>
        </div>

        {#each activeMembers as member (member.id)}
          <div class="directory-row member-grid">
            <div class="identity-cell">
              <div class="identity-line">
                <strong>{member.email || 'Member'}</strong>

                {#if isCurrentUser(member)}
                  <span class="you-label">You</span>
                {/if}
              </div>

              <small>
                {member.role === 'owner'
                  ? 'Workspace owner'
                  : 'Workspace member'}
              </small>
            </div>

            <div class="information-cell" data-label="Access">
              <strong>{accessLabel(member.access_scope)}</strong>

              {#if member.access_scope === 'selected_projects'}
                <small>
                  {projectNames(member.project_ids_json)}
                </small>
              {/if}
            </div>

            <div class="role-cell" data-label="Role">
              {#if canEditMember(member)}
                <select
                  value={member.role}
                  disabled={busyId === member.id}
                  aria-label="Change role for {member.email}"
                  onchange={(event) =>
                    handleRoleChange(member, event)}
                >
                  <option value="admin">Admin</option>
                  <option value="member">Member</option>
                  <option value="viewer">Viewer</option>
                </select>
              {:else}
                <span class="neutral-badge">
                  {roleLabel(member.role)}
                </span>
              {/if}
            </div>

            <div class="row-actions" data-label="Actions">
              {#if canEditMember(member)}
                <button
                  class="secondary-button compact-button"
                  type="button"
                  disabled={busyId === member.id}
                  onclick={() =>
                    openConfirmation({
                      kind: 'member',
                      member,
                      action: 'suspend'
                    })}
                >
                  Suspend
                </button>

                <button
                  class="danger-button compact-button"
                  type="button"
                  disabled={busyId === member.id}
                  onclick={() =>
                    openConfirmation({
                      kind: 'member',
                      member,
                      action: 'remove'
                    })}
                >
                  Remove
                </button>
              {:else}
                <span class="no-actions">
                  {member.role === 'owner'
                    ? 'Owner'
                    : 'Current account'}
                </span>
              {/if}
            </div>
          </div>
        {/each}
      </div>
    {/if}
  </section>

  {#if inactiveMembers.length > 0}
    <section class="directory-section">
      <div class="directory-header">
        <div>
          <h2>Inactive members</h2>
          <p>Suspended or removed workspace accounts.</p>
        </div>

        <span class="section-count">
          {inactiveMembers.length}
        </span>
      </div>

      <div class="directory-table">
        <div class="table-heading inactive-grid">
          <span>Member</span>
          <span>Status</span>
          <span>Role</span>
          <span class="align-right">Action</span>
        </div>

        {#each inactiveMembers as member (member.id)}
          <div class="directory-row inactive-grid">
            <div class="identity-cell">
              <strong>{member.email || 'Member'}</strong>
              <small>No current workspace access</small>
            </div>

            <div class="information-cell" data-label="Status">
              <span class="status-badge">
                {statusLabel(member.status)}
              </span>
            </div>

            <div class="information-cell" data-label="Role">
              <span class="neutral-badge">
                {roleLabel(member.role)}
              </span>
            </div>

            <div class="row-actions" data-label="Action">
              {#if canManage}
                <button
                  class="reactivate-button compact-button"
                  type="button"
                  disabled={busyId === member.id}
                  onclick={() =>
                    openConfirmation({
                      kind: 'member',
                      member,
                      action: 'reactivate'
                    })}
                >
                  Reactivate
                </button>
              {/if}
            </div>
          </div>
        {/each}
      </div>
    </section>
  {/if}

  {#if canManage && inviteHistory.length > 0}
    <details class="history-section">
      <summary>
        <span>Invitation history</span>
        <span class="section-count">
          {inviteHistory.length}
        </span>
      </summary>

      <div class="history-list">
        {#each inviteHistory as invite (invite.id)}
          <div class="history-row">
            <div>
              <strong>{invite.invited_email}</strong>

              <small>
                {roleLabel(invite.role)}
                ·
                {accessLabel(invite.access_scope)}
              </small>
            </div>

            <span class="neutral-badge">
              {statusLabel(invite.status)}
            </span>
          </div>
        {/each}
      </div>
    </details>
  {/if}
{/if}

{#if confirmation}
  <div class="modal-layer">
    <button
      class="modal-backdrop"
      type="button"
      aria-label="Close confirmation"
      onclick={closeConfirmation}
    ></button>

    <dialog
      bind:this={confirmationDialog}
      class="confirm-modal"
      open
      aria-modal="true"
      aria-labelledby="confirmation-title"
      aria-describedby="confirmation-description"
      tabindex="-1"
    >
      <div class="confirm-modal__content">
        <h2 id="confirmation-title">
          {confirmationTitle(confirmation)}
        </h2>

        <p id="confirmation-description">
          {confirmationMessage(confirmation)}
        </p>
      </div>

      <div class="confirm-modal__actions">
        <button
          class="modal-cancel-button"
          type="button"
          disabled={Boolean(busyId)}
          onclick={closeConfirmation}
        >
          Cancel
        </button>

        <button
          class:modal-danger-button={confirmationIsDanger(confirmation)}
          class:modal-primary-button={!confirmationIsDanger(confirmation)}
          type="button"
          disabled={Boolean(busyId)}
          onclick={confirmAction}
        >
          {busyId
            ? 'Working…'
            : confirmationButtonLabel(confirmation)}
        </button>
      </div>
    </dialog>
  </div>
{/if}

<style>
  .page-introduction {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    margin: -8px 0 18px;
  }

  .page-introduction > p {
    max-width: 680px;
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.6;
  }

  .member-summary {
    display: flex;
    align-items: center;
    gap: 14px;
    flex: 0 0 auto;
    color: var(--text-muted);
    font-size: 12px;
  }

  .member-summary span {
    display: inline-flex;
    align-items: baseline;
    gap: 4px;
  }

  .member-summary strong {
    color: var(--text);
    font-family: var(--font-mono);
    font-size: 12px;
    font-weight: 600;
  }

  .invite-member-button {
    border-color: var(--brand);
    background: var(--brand);
    color: var(--brand-ink);
  }

  .invite-member-button:hover:not(:disabled) {
    border-color: var(--brand-soft);
    background: var(--brand-soft);
  }

  /* Invite form */

  .invite-panel {
    display: grid;
    gap: 17px;
    margin-bottom: 18px;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    padding: 18px;
  }

  .invite-panel__header {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 16px;
  }

  .invite-panel__header > div {
    display: grid;
    gap: 4px;
  }

  .invite-panel__header h2 {
    font-size: 14px;
  }

  .invite-panel__header p {
    color: var(--text-muted);
    font-size: 12.5px;
  }

  .invite-form {
    display: grid;
    grid-template-columns: minmax(240px, 1.4fr) minmax(140px, 0.7fr) minmax(180px, 0.9fr);
    gap: 14px;
    align-items: start;
  }

  .project-selector,
  .invite-actions {
    grid-column: 1 / -1;
  }

  .project-selector {
    display: grid;
    gap: 10px;
    border: 1px solid var(--line-soft);
    border-radius: var(--radius-sm);
    background: var(--bg-ink-soft);
    padding: 13px;
  }

  .project-selector__header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
  }

  .project-selector__header strong {
    color: var(--text-soft);
    font-size: 12px;
    font-weight: 600;
  }

  .project-selector__header span {
    color: var(--text-muted);
    font-size: 11px;
  }

  .project-list {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 7px;
  }

  .project-option {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 8px;
    border: 1px solid var(--line-soft);
    border-radius: var(--radius-sm);
    background: var(--bg-ink);
    padding: 8px 10px;
    color: var(--text-soft);
    cursor: pointer;
  }

  .project-option input {
    width: 15px;
    height: 15px;
    min-height: 0;
    flex: 0 0 auto;
    accent-color: var(--brand);
  }

  .project-option span {
    min-width: 0;
    overflow-wrap: anywhere;
  }

  .project-empty {
    color: var(--text-muted);
    font-size: 12px;
  }

  .invite-actions {
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    border-top: 1px solid var(--line-soft);
    padding-top: 14px;
  }

  /* Sections */

  .directory-section,
  .history-section {
    margin-bottom: 16px;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    overflow: hidden;
  }

  .directory-header {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 16px;
    padding: 14px 16px;
    border-bottom: 1px solid var(--line-soft);
  }

  .directory-header > div {
    display: grid;
    gap: 3px;
  }

  .directory-header h2 {
    font-size: 13.5px;
  }

  .directory-header p {
    color: var(--text-muted);
    font-size: 11.5px;
  }

  .section-count {
    display: inline-grid;
    min-width: 24px;
    height: 22px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    background: var(--bg-elevated);
    color: var(--text-muted);
    padding: 0 7px;
    font-family: var(--font-mono);
    font-size: 10.5px;
  }

  .directory-table {
    min-width: 0;
  }

  .table-heading,
  .directory-row {
    display: grid;
    gap: 16px;
    align-items: center;
    min-width: 0;
  }

  .member-grid,
  .invitation-grid {
    grid-template-columns:
      minmax(210px, 1.25fr)
      minmax(180px, 1fr)
      minmax(130px, 0.55fr)
      minmax(180px, auto);
  }

  .inactive-grid {
    grid-template-columns:
      minmax(220px, 1.3fr)
      minmax(120px, 0.6fr)
      minmax(120px, 0.6fr)
      minmax(120px, auto);
  }

  .table-heading {
    min-height: 34px;
    padding: 7px 16px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 9.5px;
    font-weight: 600;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }

  .directory-row {
    min-height: 62px;
    padding: 11px 16px;
    border-bottom: 1px solid var(--line-soft);
    transition: background var(--speed) var(--ease);
  }

  .directory-row:last-child {
    border-bottom: 0;
  }

  .directory-row:hover {
    background: rgba(255, 255, 255, 0.018);
  }

  .align-right {
    text-align: right;
  }

  .identity-cell,
  .information-cell {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .identity-line {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 7px;
    flex-wrap: wrap;
  }

  .identity-cell strong,
  .information-cell strong {
    color: var(--text);
    font-size: 12.5px;
    font-weight: 550;
    overflow-wrap: anywhere;
  }

  .identity-cell small,
  .information-cell small {
    color: var(--text-muted);
    font-size: 11px;
    line-height: 1.45;
    overflow-wrap: anywhere;
  }

  .you-label {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .neutral-badge,
  .status-badge {
    display: inline-flex;
    width: fit-content;
    min-height: 21px;
    align-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    background: var(--bg-elevated);
    color: var(--text-soft);
    padding: 1px 8px;
    font-size: 10.5px;
    white-space: nowrap;
  }

  .status-badge {
    border-color: var(--danger-line);
    background: var(--danger-faint);
    color: var(--danger);
  }

  .role-cell {
    min-width: 0;
  }

  .role-cell select {
    width: 100%;
    max-width: 130px;
    min-height: 31px;
    font-size: 12px;
  }

  .row-actions {
    display: flex;
    align-items: center;
    justify-content: flex-end;
    gap: 7px;
    min-width: 0;
    flex-wrap: wrap;
  }

  .compact-button,
  .danger-button {
    min-height: 30px;
    padding: 0 11px;
    font-size: 12px;
  }

  .secondary-button {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-soft);
  }

  .secondary-button:hover:not(:disabled) {
    border-color: rgba(255, 255, 255, 0.18);
    background: var(--bg-elevated);
    color: var(--text);
  }

  .danger-button {
    border-color: var(--danger-line);
    background: transparent;
    color: var(--danger);
  }

  .danger-button:hover:not(:disabled) {
    border-color: var(--danger-line);
    background: var(--danger-faint);
    color: var(--danger);
  }

  .reactivate-button {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-bright);
  }

  .reactivate-button:hover:not(:disabled) {
    border-color: var(--brand);
    background: rgba(249, 115, 22, 0.17);
    color: var(--brand-bright);
  }

  .no-actions {
    color: var(--text-faint);
    font-size: 11px;
  }

  .empty-wrapper {
    padding: 14px;
  }

  .loading-state {
    min-height: 72px;
    padding: 20px 16px;
    color: var(--text-muted);
    font-size: 12.5px;
  }

  /* Invitation history */

  .history-section summary {
    display: flex;
    min-height: 48px;
    align-items: center;
    justify-content: space-between;
    gap: 16px;
    padding: 12px 16px;
    color: var(--text-soft);
    font-size: 12.5px;
    font-weight: 600;
    cursor: pointer;
    list-style: none;
  }

  .history-section summary::-webkit-details-marker {
    display: none;
  }

  .history-section summary:hover {
    background: rgba(255, 255, 255, 0.018);
  }

  .history-list {
    border-top: 1px solid var(--line-soft);
  }

  .history-row {
    display: flex;
    min-height: 52px;
    align-items: center;
    justify-content: space-between;
    gap: 16px;
    padding: 10px 16px;
    border-bottom: 1px solid var(--line-soft);
  }

  .history-row:last-child {
    border-bottom: 0;
  }

  .history-row > div {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .history-row strong {
    color: var(--text-soft);
    font-size: 12px;
    font-weight: 550;
    overflow-wrap: anywhere;
  }

  .history-row small {
    color: var(--text-muted);
    font-size: 11px;
  }

  /* Confirmation modal */

  .modal-layer {
    position: fixed;
    inset: 0;
    z-index: 100;
    display: grid;
    place-items: center;
    padding: 20px;
  }

  .modal-backdrop {
    position: absolute;
    inset: 0;
    width: 100%;
    min-height: 100%;
    border: 0;
    border-radius: 0;
    background: rgba(0, 0, 0, 0.72);
    padding: 0;
    cursor: default;
    backdrop-filter: blur(3px);
  }

  .modal-backdrop:hover:not(:disabled),
  .modal-backdrop:active {
    background: rgba(0, 0, 0, 0.72);
    transform: none;
  }

  .modal-backdrop:focus,
  .modal-backdrop:focus-visible {
    outline: none;
    box-shadow: none;
  }

  .confirm-modal {
    position: relative;
    z-index: 1;
    display: grid;
    width: min(100%, 430px);
    max-width: none;
    margin: 0;
    gap: 18px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    color: var(--text);
    padding: 20px;
    box-shadow: var(--shadow-deep);
  }

  .confirm-modal:focus,
  .confirm-modal:focus-visible {
    outline: none;
  }

  .confirm-modal::backdrop {
    background: transparent;
  }

  .confirm-modal__content {
    display: grid;
    gap: 8px;
  }

  .confirm-modal__content h2 {
    color: var(--text);
    font-size: 16px;
  }

  .confirm-modal__content p {
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.65;
  }

  .confirm-modal__actions {
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    border-top: 1px solid var(--line-soft);
    padding-top: 16px;
  }

  .modal-cancel-button {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-soft);
  }

  .modal-cancel-button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
  }

  .modal-danger-button {
    border-color: var(--danger);
    background: var(--danger);
    color: #240707;
  }

  .modal-danger-button:hover:not(:disabled) {
    border-color: #fca5a5;
    background: #fca5a5;
  }

  .modal-primary-button {
    border-color: var(--brand);
    background: var(--brand);
    color: var(--brand-ink);
  }

  .modal-primary-button:hover:not(:disabled) {
    border-color: var(--brand-soft);
    background: var(--brand-soft);
  }

  @media (max-width: 860px) {
    .invite-form {
      grid-template-columns: 1fr 1fr;
    }

    .email-field {
      grid-column: 1 / -1;
    }

    .member-grid,
    .invitation-grid,
    .inactive-grid {
      grid-template-columns:
        minmax(190px, 1fr)
        minmax(150px, 0.8fr)
        minmax(115px, 0.55fr)
        minmax(150px, auto);
    }
  }

  @media (max-width: 720px) {
    .page-introduction {
      align-items: flex-start;
      flex-direction: column;
    }

    .member-summary {
      width: 100%;
      justify-content: flex-start;
    }

    .invite-form {
      grid-template-columns: 1fr;
    }

    .email-field,
    .project-selector,
    .invite-actions {
      grid-column: auto;
    }

    .project-list {
      grid-template-columns: 1fr;
    }

    .table-heading {
      display: none;
    }

    .member-grid,
    .invitation-grid,
    .inactive-grid {
      grid-template-columns: 1fr;
    }

    .directory-row {
      gap: 10px;
      padding: 14px;
    }

    .information-cell[data-label],
    .role-cell[data-label],
    .row-actions[data-label] {
      display: grid;
      grid-template-columns: 92px minmax(0, 1fr);
      align-items: start;
      gap: 10px;
    }

    .information-cell[data-label]::before,
    .role-cell[data-label]::before,
    .row-actions[data-label]::before {
      content: attr(data-label);
      color: var(--text-muted);
      font-size: 10.5px;
    }

    .role-cell select {
      max-width: none;
    }

    .row-actions {
      justify-content: flex-start;
    }

    .row-actions button {
      width: auto;
    }
  }

  @media (max-width: 520px) {
    .invite-panel {
      padding: 14px;
    }

    .invite-actions,
    .confirm-modal__actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .invite-actions button,
    .confirm-modal__actions button {
      width: 100%;
    }

    .member-summary {
      gap: 10px;
      flex-wrap: wrap;
    }

    .information-cell[data-label],
    .role-cell[data-label],
    .row-actions[data-label] {
      grid-template-columns: 1fr;
      gap: 4px;
    }

    .row-actions button {
      width: 100%;
    }
  }
</style>
