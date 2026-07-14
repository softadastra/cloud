<script lang="ts">
  import { goto } from '$app/navigation';
  import { tick } from 'svelte';
  import { get } from 'svelte/store';

  import { usePageRefresh } from '$lib/stores/pageRefresh';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

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

  import InlineError from '$lib/components/InlineError.svelte';
  import LimitedAccessNotice from '$lib/components/LimitedAccessNotice.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import WorkspaceAvatar from '$lib/components/WorkspaceAvatar.svelte';

  import { canManageMembersContext } from '$lib/permissions';

  type MemberAction =
    | 'suspend'
    | 'reactivate'
    | 'remove';

  type DirectoryView =
    | 'active'
    | 'pending'
    | 'inactive';

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

  const roleOptions = [
    {
      value: 'admin',
      label: 'Admin',
      description:
        'Can manage members, projects and workspace operations.'
    },
    {
      value: 'member',
      label: 'Member',
      description:
        'Can work with projects, packages and shared development resources.'
    },
    {
      value: 'viewer',
      label: 'Viewer',
      description:
        'Can inspect workspace resources without changing them.'
    }
  ];

  const accessOptions = [
    {
      value: 'entire_workspace',
      label: 'Entire workspace',
      description:
        'Access all current and future projects in this workspace.'
    },
    {
      value: 'selected_projects',
      label: 'Selected projects',
      description:
        'Access only the projects explicitly selected below.'
    }
  ];

  let workspaces: Workspace[] = [];
  let projects: Project[] = [];
  let members: Member[] = [];
  let invites: WorkspaceInvite[] = [];

  let selectedWorkspaceId = '';
  let activeView: DirectoryView = 'active';

  let memberQuery = '';

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
    workspaces.find(
      (workspace) =>
        workspace.id === selectedWorkspaceId
    ) ?? null;

  $: permissionContext =
    getPermissionContext(selectedWorkspaceId);

  $: currentRole =
    permissionContext.role;

  $: canManage =
    canManageMembersContext(permissionContext);

  $: globalWorkspaceId =
    $workspaceContext.selectedWorkspace?.id ?? '';

  $: if (
    initialized &&
    globalWorkspaceId &&
    globalWorkspaceId !== selectedWorkspaceId &&
    workspaces.some(
      (workspace) =>
        workspace.id === globalWorkspaceId
    )
  ) {
    void loadForWorkspace(globalWorkspaceId, {
      updateUrl: true
    });
  }

  $: pendingInvites = invites.filter(
    (invite) =>
      invite.status === 'pending'
  );

  $: inviteHistory = invites.filter(
    (invite) =>
      invite.status !== 'pending'
  );

  $: activeMembers = members.filter(
    (member) =>
      member.status === 'active'
  );

  $: suspendedMembers = members.filter(
    (member) =>
      member.status === 'suspended'
  );

  $: removedMembers = members.filter(
    (member) =>
      member.status === 'removed'
  );

  $: inactiveMembers = [
    ...suspendedMembers,
    ...removedMembers
  ];

  $: managers = activeMembers.filter(
    (member) =>
      member.role === 'owner' ||
      member.role === 'admin'
  );

  $: visibleActiveMembers =
    activeMembers.filter((member) => {
      const query =
        memberQuery.trim().toLowerCase();

      if (!query) {
        return true;
      }

      return (
        member.email
          ?.toLowerCase()
          .includes(query) ||
        member.role
          .toLowerCase()
          .includes(query)
      );
    });

  $: selectedRoleOption =
    roleOptions.find(
      (option) => option.value === role
    ) ?? roleOptions[1];

  $: selectedAccessOption =
    accessOptions.find(
      (option) =>
        option.value === accessScope
    ) ?? accessOptions[0];

  $: isSoloWorkspace =
    canManage &&
    activeMembers.length <= 1 &&
    pendingInvites.length === 0;

  $: if (
    activeView === 'pending' &&
    (
      !canManage ||
      pendingInvites.length === 0
    )
  ) {
    activeView = 'active';
  }

  $: if (
    activeView === 'inactive' &&
    inactiveMembers.length === 0
  ) {
    activeView = 'active';
  }

  function getPermissionContext(
    workspaceId: string
  ) {
    const workspace =
      workspaces.find(
        (item) =>
          item.id === workspaceId
      );

    return {
      role:
        workspace?.current_user_role ??
        'viewer',

      status:
        workspace?.current_user_status ??
        'active',

      access_scope:
        workspace?.access_scope ??
        'entire_workspace',

      project_ids_json:
        workspace?.project_ids_json ??
        ''
    };
  }

  function parseProjectIds(
    value?: string
  ) {
    if (!value) {
      return [];
    }

    try {
      return JSON.parse(value) as string[];
    } catch {
      return [];
    }
  }

  function projectNames(
    value?: string
  ) {
    const ids =
      parseProjectIds(value);

    if (ids.length === 0) {
      return 'All projects';
    }

    const names = ids.map(
      (id) =>
        projects.find(
          (project) =>
            project.id === id
        )?.name ?? id
    );

    if (names.length <= 2) {
      return names.join(', ');
    }

    return `${names
      .slice(0, 2)
      .join(', ')} +${names.length - 2}`;
  }

  function selectedProjectNames() {
    if (
      accessScope !==
        'selected_projects' ||
      selectedProjectIds.length === 0
    ) {
      return 'All workspace projects';
    }

    return selectedProjectIds
      .map(
        (id) =>
          projects.find(
            (project) =>
              project.id === id
          )?.name ?? id
      )
      .join(', ');
  }

  function accessLabel(
    scope?: string
  ) {
    return scope === 'selected_projects'
      ? 'Selected projects'
      : 'Entire workspace';
  }

  function roleLabel(
    value: string
  ) {
    return (
      value.charAt(0).toUpperCase() +
      value.slice(1)
    );
  }

  function statusLabel(
    value: string
  ) {
    return (
      value.charAt(0).toUpperCase() +
      value.slice(1)
    );
  }

  function memberInitial(
    value?: string
  ) {
    return (
      value?.trim().charAt(0).toUpperCase() ||
      'M'
    );
  }

  function isCurrentUser(
    member: Member
  ) {
    return (
      member.user_id ===
      $auth.user?.id
    );
  }

  function canEditMember(
    member: Member
  ) {
    return (
      canManage &&
      member.role !== 'owner' &&
      !isCurrentUser(member)
    );
  }

  function toggleProject(
    projectId: string
  ) {
    selectedProjectIds =
      selectedProjectIds.includes(projectId)
        ? selectedProjectIds.filter(
            (id) => id !== projectId
          )
        : [
            ...selectedProjectIds,
            projectId
          ];
  }

  function resetInviteForm() {
    email = '';
    role = 'member';
    accessScope =
      'entire_workspace';
    selectedProjectIds = [];
  }

  function openInviteForm() {
    showInviteForm = true;
    error = '';
    success = '';
  }

  function closeInviteForm() {
    if (saving) {
      return;
    }

    showInviteForm = false;
    resetInviteForm();
    error = '';
  }

  function handleRoleChange(
    member: Member,
    event: Event
  ) {
    const select =
      event.currentTarget as HTMLSelectElement;

    void changeRole(
      member,
      select.value
    );
  }

  function memberErrorMessage(
    err: unknown
  ) {
    return err instanceof ApiError
      ? err.message
      : 'Unable to load members.';
  }

  function resolveWorkspaceId(
    requestedWorkspaceId: string | null
  ) {
    const storedWorkspaceId =
      globalWorkspaceId ||
      get(workspaceContext)
        .selectedWorkspace?.id ||
      '';

    if (
      requestedWorkspaceId &&
      workspaces.some(
        (workspace) =>
          workspace.id ===
          requestedWorkspaceId
      )
    ) {
      return requestedWorkspaceId;
    }

    if (
      storedWorkspaceId &&
      workspaces.some(
        (workspace) =>
          workspace.id ===
          storedWorkspaceId
      )
    ) {
      return storedWorkspaceId;
    }

    return workspaces[0]?.id ?? '';
  }

  function updateWorkspaceUrl(
    workspaceId: string
  ) {
    const url =
      new URL(window.location.href);

    if (workspaceId) {
      url.searchParams.set(
        'workspace_id',
        workspaceId
      );
    } else {
      url.searchParams.delete(
        'workspace_id'
      );
    }

    history.replaceState(
      null,
      '',
      `${url.pathname}${url.search}`
    );
  }

  async function load() {
    if (!$auth.session) {
      await goto('/login');
      return;
    }

    if (!$auth.user) {
      const status =
        await auth.ensureAuthLoaded();

      if (
        status === 'missing' ||
        status === 'invalid'
      ) {
        await goto('/login');
        return;
      }
    }

    const user = $auth.user;

    if (!user) {
      error =
        'Unable to load the current account.';

      loading = false;
      return;
    }

    const seq = ++loadSeq;

    loading = true;
    initialized = false;

    error = '';
    success = '';

    members = [];
    invites = [];
    projects = [];

    try {
      const workspaceData =
        await listWorkspaces(user.id);

      if (seq !== loadSeq) {
        return;
      }

      workspaces =
        workspaceData.workspaces;

      const requestedWorkspaceId =
        new URLSearchParams(
          window.location.search
        ).get('workspace_id');

      const workspaceId =
        resolveWorkspaceId(
          requestedWorkspaceId
        );

      workspaceContext.setWorkspaces(
        workspaces,
        workspaceId
      );

      initialized = true;

      if (!workspaceId) {
        selectedWorkspaceId = '';
        loading = false;
        return;
      }

      await loadForWorkspace(
        workspaceId,
        { seq }
      );
    } catch (err) {
      if (seq !== loadSeq) {
        return;
      }

      error =
        memberErrorMessage(err);

      loading = false;
      initialized = true;
    }
  }

  async function loadForWorkspace(
    workspaceId: string,
    options: {
      updateUrl?: boolean;
      seq?: number;
    } = {}
  ) {
    const seq =
      options.seq ?? ++loadSeq;

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

    selectedWorkspaceId =
      workspaceId;

    members = [];
    invites = [];
    projects = [];

    selectedProjectIds = [];
    memberQuery = '';

    showInviteForm = false;
    activeView = 'active';

    resetInviteForm();

    confirmation = null;

    error = '';
    success = '';
    loading = true;

    workspaceContext.setSelectedWorkspace(
      workspaceId
    );

    if (options.updateUrl) {
      updateWorkspaceUrl(
        workspaceId
      );
    }

    const context =
      getPermissionContext(workspaceId);

    const canLoadInvites =
      canManageMembersContext(context);

    try {
      const [
        memberData,
        projectData,
        inviteData
      ] = await Promise.all([
        listMembers(workspaceId),

        listProjects(workspaceId),

        canLoadInvites
          ? listWorkspaceInvites(
              workspaceId
            )
          : Promise.resolve({
              invites:
                [] as WorkspaceInvite[]
            })
      ]);

      const currentWorkspaceId =
        get(workspaceContext)
          .selectedWorkspace?.id ?? '';

      if (
        seq !== loadSeq ||
        selectedWorkspaceId !==
          workspaceId ||
        currentWorkspaceId !==
          workspaceId
      ) {
        return;
      }

      members =
        memberData.members;

      projects =
        projectData.projects;

      invites =
        inviteData.invites;
    } catch (err) {
      if (
        seq !== loadSeq ||
        selectedWorkspaceId !==
          workspaceId
      ) {
        return;
      }

      error =
        memberErrorMessage(err);
    } finally {
      if (
        seq === loadSeq &&
        selectedWorkspaceId ===
          workspaceId
      ) {
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
      accessScope ===
        'selected_projects' &&
      selectedProjectIds.length === 0
    ) {
      error =
        'Choose at least one project for selected project access.';

      return;
    }

    saving = true;
    error = '';
    success = '';

    const invitedEmail =
      email.trim();

    try {
      const invited =
        await createWorkspaceInvite({
          workspaceId:
            selectedWorkspaceId,

          invitedEmail,

          role,

          invitedByUserId:
            user.id,

          accessScope,

          projectIdsJson:
            accessScope ===
            'selected_projects'
              ? JSON.stringify(
                  selectedProjectIds
                )
              : ''
        });

      invites = [
        invited.invite,
        ...invites
      ];

      success =
        `Invitation sent to ${invitedEmail}.`;

      resetInviteForm();
      showInviteForm = false;
      activeView = 'pending';
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
      const updated =
        await updateMemberRole({
          workspaceId:
            member.workspace_id,

          userId:
            member.user_id,

          role:
            nextRole,

          actorUserId:
            user.id
        });

      members = members.map(
        (item) =>
          item.id === member.id
            ? updated.member
            : item
      );

      success =
        `Role updated for ${member.email}.`;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to update role.';
    } finally {
      busyId = '';
    }
  }

  async function openConfirmation(
    next: Confirmation
  ) {
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

  function confirmationTitle(
    value: Confirmation
  ) {
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

  function confirmationMessage(
    value: Confirmation
  ) {
    if (value.kind === 'invite') {
      return (
        `The invitation sent to ` +
        `${value.invite.invited_email} ` +
        `will no longer be usable.`
      );
    }

    if (value.action === 'suspend') {
      return (
        `${value.member.email} will lose ` +
        `access until the account is reactivated.`
      );
    }

    if (value.action === 'remove') {
      return (
        `${value.member.email} will be removed ` +
        `from this workspace and lose access.`
      );
    }

    return (
      `${value.member.email} will regain access ` +
      `using the current role and project scope.`
    );
  }

  function confirmationButtonLabel(
    value: Confirmation
  ) {
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

  function confirmationIsDanger(
    value: Confirmation
  ) {
    return (
      value.kind === 'invite' ||
      value.action === 'suspend' ||
      value.action === 'remove'
    );
  }

  async function confirmAction() {
    const user = $auth.user;
    const pending = confirmation;

    if (
      !user ||
      !pending ||
      !canManage
    ) {
      return;
    }

    error = '';
    success = '';

    if (pending.kind === 'invite') {
      busyId = pending.invite.id;

      try {
        const revoked =
          await revokeWorkspaceInvite({
            workspaceId:
              pending.invite.workspace_id,

            inviteId:
              pending.invite.id,

            revokedByUserId:
              user.id
          });

        invites = invites.map(
          (invite) =>
            invite.id ===
            pending.invite.id
              ? revoked.invite
              : invite
        );

        const remainingPending =
          invites.filter(
            (invite) =>
              invite.id !==
                pending.invite.id &&
              invite.status === 'pending'
          ).length;

        if (remainingPending === 0) {
          activeView = 'active';
        }

        success =
          'Invitation revoked.';

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

    const member =
      pending.member;

    const action =
      pending.action;

    busyId = member.id;

    try {
      const request = {
        workspaceId:
          member.workspace_id,

        userId:
          member.user_id,

        actorUserId:
          user.id
      };

      const updated =
        action === 'suspend'
          ? await suspendMember(request)
          : action === 'reactivate'
            ? await reactivateMember(request)
            : await removeMember(request);

      members = members.map(
        (item) =>
          item.id === member.id
            ? updated.member
            : item
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

  function handleModalKeydown(
    event: KeyboardEvent
  ) {
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
  <title>
    Members | Softadastra Cloud
  </title>

  <meta
    name="description"
    content="Invite teammates and manage roles, project access, workspace invitations and inactive accounts in Softadastra Cloud."
  />
</svelte:head>

<svelte:window
  onkeydown={handleModalKeydown}
/>

<div class="members-page">
  <!-- ===================================================
       PAGE HEADER
       =================================================== -->
  <header class="page-header">
    <div class="page-heading">
      <p class="page-eyebrow">
        <span aria-hidden="true"></span>
        Access
      </p>

      <h1>Members</h1>

      <p>
        {#if loading}
          Loading workspace access…
        {:else if selectedWorkspace}
          Manage access to
          <strong>
            {selectedWorkspace.name}
          </strong>.
        {:else}
          Create a workspace before adding
          members.
        {/if}
      </p>
    </div>

    <div class="page-header__actions">
      {#if selectedWorkspace}
        <a
          class="workspace-context"
          href={`/workspaces?workspace_id=${encodeURIComponent(
            selectedWorkspace.id
          )}`}
        >
          <WorkspaceAvatar
            workspace={selectedWorkspace}
            size="sm"
          />

          <span>
            <strong>
              {selectedWorkspace.name}
            </strong>

            <small>
              {roleLabel(currentRole)}
            </small>
          </span>
        </a>
      {/if}

      {#if
        !loading &&
        selectedWorkspace &&
        canManage
      }
        <button
          class:button-secondary={
            showInviteForm
          }
          class="header-action"
          type="button"
          onclick={() => {
            if (showInviteForm) {
              closeInviteForm();
            } else {
              openInviteForm();
            }
          }}
        >
          {#if !showInviteForm}
            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path d="M12 5v14"></path>
              <path d="M5 12h14"></path>
            </svg>
          {/if}

          {showInviteForm
            ? 'Close'
            : isSoloWorkspace
              ? 'Invite first member'
              : 'Invite member'}
        </button>
      {/if}
    </div>
  </header>

  {#if
    selectedWorkspaceId &&
    permissionContext.access_scope ===
      'selected_projects'
  }
    <LimitedAccessNotice />
  {/if}

  <InlineError message={error} />

  {#if success}
    <div
      class="success-message"
      role="status"
    >
      <span aria-hidden="true">
        <svg
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="2.2"
          stroke-linecap="round"
          stroke-linejoin="round"
        >
          <path d="m5 12 4 4L19 6"></path>
        </svg>
      </span>

      <p>{success}</p>
    </div>
  {/if}

  <!-- ===================================================
       LOADING
       =================================================== -->
  {#if loading}
    <section class="page-loading">
      <span class="page-loading__mark">
        M
      </span>

      <div>
        <strong>
          Loading workspace access
        </strong>

        <p>
          Preparing members, invitations and
          project permissions.
        </p>
      </div>

      <span
        class="page-loading__bar"
        aria-hidden="true"
      ></span>
    </section>

  <!-- ===================================================
       NO WORKSPACE
       =================================================== -->
  {:else if !selectedWorkspace}
    <section class="workspace-required">
      <span
        class="workspace-required__icon"
        aria-hidden="true"
      >
        <svg
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="1.8"
          stroke-linecap="round"
          stroke-linejoin="round"
        >
          <rect
            x="3"
            y="3"
            width="7"
            height="7"
            rx="1"
          ></rect>

          <rect
            x="14"
            y="3"
            width="7"
            height="7"
            rx="1"
          ></rect>

          <rect
            x="3"
            y="14"
            width="7"
            height="7"
            rx="1"
          ></rect>

          <path d="M17.5 14v7"></path>
          <path d="M14 17.5h7"></path>
        </svg>
      </span>

      <div>
        <p class="panel-kicker">
          Workspace required
        </p>

        <h2>
          Members belong to a workspace.
        </h2>

        <p>
          Create the workspace that will own your
          projects, packages, members and access
          rules.
        </p>
      </div>

      <a
        class="primary-link"
        href="/workspaces"
      >
        Create a workspace

        <svg
          viewBox="0 0 24 24"
          aria-hidden="true"
        >
          <path d="M5 12h14"></path>
          <path d="m13 6 6 6-6 6"></path>
        </svg>
      </a>
    </section>

  {:else}
    <!-- =================================================
         WORKSPACE ACCESS OVERVIEW
         ================================================= -->
    <section class="access-overview">
      <div class="access-overview__identity">
        <WorkspaceAvatar
          workspace={selectedWorkspace}
        />

        <div>
          <p class="panel-kicker">
            Workspace access
          </p>

          <h2>
            {selectedWorkspace.name}
          </h2>

          <code>
            {selectedWorkspace.slug}
          </code>
        </div>
      </div>

      <div class="access-metrics">
        <article>
          <span>Active members</span>

          <strong>
            {activeMembers.length}
          </strong>

          <small>
            Current workspace access
          </small>
        </article>

        <article>
          <span>Managers</span>

          <strong>
            {managers.length}
          </strong>

          <small>
            Owners and administrators
          </small>
        </article>

        <article>
          {#if canManage}
            <span>Pending invitations</span>

            <strong>
              {pendingInvites.length}
            </strong>

            <small>
              Waiting for acceptance
            </small>
          {:else}
            <span>Your role</span>

            <strong>
              {roleLabel(currentRole)}
            </strong>

            <small>
              {accessLabel(
                permissionContext.access_scope
              )}
            </small>
          {/if}
        </article>
      </div>
    </section>

    <!-- =================================================
         INVITE FORM
         ================================================= -->
    {#if showInviteForm && canManage}
      <section
        class="invite-panel"
        aria-labelledby="invite-panel-title"
      >
        <header class="invite-panel__header">
          <span
            class="invite-panel__icon"
            aria-hidden="true"
          >
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="1.8"
              stroke-linecap="round"
              stroke-linejoin="round"
            >
              <circle
                cx="9"
                cy="8"
                r="4"
              ></circle>

              <path d="M3 21a6 6 0 0 1 12 0"></path>
              <path d="M19 8v6"></path>
              <path d="M16 11h6"></path>
            </svg>
          </span>

          <div>
            <p class="panel-kicker">
              New invitation
            </p>

            <h2 id="invite-panel-title">
              Invite a teammate
            </h2>

            <p>
              Choose what this person can do and
              which projects they can access.
            </p>
          </div>
        </header>

        <form
          class="invite-form"
          onsubmit={(event) => {
            event.preventDefault();
            void submitInvite();
          }}
        >
          <div class="invite-form__fields">
            <label class="email-field">
              <span>Email address</span>

              <input
                bind:value={email}
                type="email"
                placeholder="teammate@example.com"
                required
                autocomplete="email"
              />
            </label>

            <fieldset>
              <legend>Role</legend>

              <div class="option-grid option-grid--roles">
                {#each roleOptions as option}
                  <button
                    class:selected={
                      role === option.value
                    }
                    class="choice-option"
                    type="button"
                    aria-pressed={
                      role === option.value
                    }
                    onclick={() => {
                      role = option.value;
                    }}
                  >
                    <span
                      class="choice-option__indicator"
                      aria-hidden="true"
                    ></span>

                    <span>
                      <strong>
                        {option.label}
                      </strong>

                      <small>
                        {option.description}
                      </small>
                    </span>
                  </button>
                {/each}
              </div>
            </fieldset>

            <fieldset>
              <legend>Project access</legend>

              <div class="option-grid">
                {#each accessOptions as option}
                  <button
                    class:selected={
                      accessScope ===
                      option.value
                    }
                    class="choice-option"
                    type="button"
                    aria-pressed={
                      accessScope ===
                      option.value
                    }
                    disabled={
                      option.value ===
                        'selected_projects' &&
                      projects.length === 0
                    }
                    onclick={() => {
                      accessScope =
                        option.value;

                      if (
                        option.value ===
                        'entire_workspace'
                      ) {
                        selectedProjectIds = [];
                      }
                    }}
                  >
                    <span
                      class="choice-option__indicator"
                      aria-hidden="true"
                    ></span>

                    <span>
                      <strong>
                        {option.label}
                      </strong>

                      <small>
                        {option.description}
                      </small>
                    </span>
                  </button>
                {/each}
              </div>
            </fieldset>

            {#if
              accessScope ===
              'selected_projects'
            }
              <fieldset class="project-selector">
                <legend>
                  Select projects
                </legend>

                <div class="project-selector__header">
                  <span>
                    Choose at least one project.
                  </span>

                  <strong>
                    {selectedProjectIds.length}
                    selected
                  </strong>
                </div>

                <div class="project-list">
                  {#each projects as project (project.id)}
                    <label class="project-option">
                      <input
                        type="checkbox"
                        checked={selectedProjectIds.includes(
                          project.id
                        )}
                        onchange={() =>
                          toggleProject(
                            project.id
                          )}
                      />

                      <span>
                        <strong>
                          {project.name}
                        </strong>

                        <code>
                          {project.slug}
                        </code>
                      </span>
                    </label>
                  {/each}
                </div>
              </fieldset>
            {/if}
          </div>

          <aside class="invite-preview">
            <p class="panel-kicker">
              Access preview
            </p>

            <span class="invite-preview__avatar">
              {memberInitial(email)}
            </span>

            <div class="invite-preview__identity">
              <strong>
                {email.trim() ||
                  'teammate@example.com'}
              </strong>

              <span>
                Invitation pending
              </span>
            </div>

            <dl>
              <div>
                <dt>Role</dt>
                <dd>
                  {selectedRoleOption.label}
                </dd>
              </div>

              <div>
                <dt>Access</dt>
                <dd>
                  {selectedAccessOption.label}
                </dd>
              </div>

              <div>
                <dt>Projects</dt>
                <dd>
                  {selectedProjectNames()}
                </dd>
              </div>
            </dl>

            <p class="invite-preview__note">
              The invitation becomes active only
              after the recipient accepts it.
            </p>
          </aside>

          <div class="invite-actions">
            <button
              class="button-secondary"
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
                (
                  accessScope ===
                    'selected_projects' &&
                  selectedProjectIds.length === 0
                )
              }
            >
              {saving
                ? 'Sending invitation…'
                : 'Send invitation'}
            </button>
          </div>
        </form>
      </section>
    {/if}

    {#if !canManage}
      <ReadOnlyNotice
        message="Your role can inspect workspace members, but only owners and administrators can manage access."
      />
    {/if}

    <!-- =================================================
         FIRST MEMBER ONBOARDING
         ================================================= -->
    {#if isSoloWorkspace}
      <section class="team-onboarding">
        <div class="team-onboarding__copy">
          <p class="panel-kicker">
            Build the team
          </p>

          <h2>
            Bring the first teammate into
            <span>
              {selectedWorkspace.name}.
            </span>
          </h2>

          <p>
            Start with one person who needs real
            project access. Choose their role and
            limit access to selected projects when
            the whole workspace is not required.
          </p>

          {#if !showInviteForm}
            <button
              type="button"
              onclick={openInviteForm}
            >
              Invite the first member

              <svg
                viewBox="0 0 24 24"
                aria-hidden="true"
              >
                <path d="M5 12h14"></path>
                <path d="m13 6 6 6-6 6"></path>
              </svg>
            </button>
          {/if}
        </div>

        <div class="team-onboarding__path">
          <article class="active">
            <span>01</span>

            <div>
              <strong>
                Choose the role
              </strong>

              <p>
                Admin, member or viewer.
              </p>
            </div>
          </article>

          <article>
            <span>02</span>

            <div>
              <strong>
                Choose project access
              </strong>

              <p>
                Entire workspace or selected
                projects.
              </p>
            </div>
          </article>

          <article>
            <span>03</span>

            <div>
              <strong>
                Send the invitation
              </strong>

              <p>
                Access begins after acceptance.
              </p>
            </div>
          </article>
        </div>

        {#if activeMembers[0]}
          <div class="current-owner">
            <span class="member-avatar">
              {memberInitial(
                activeMembers[0].email
              )}
            </span>

            <div>
              <strong>
                {activeMembers[0].email ||
                  'Workspace owner'}
              </strong>

              <span>
                Current workspace owner
              </span>
            </div>

            <span class="role-badge role-badge--owner">
              Owner
            </span>
          </div>
        {/if}
      </section>

    <!-- =================================================
         MEMBER DIRECTORY
         ================================================= -->
    {:else}
      <section class="member-directory">
        <header class="directory-header">
          <div>
            <p class="panel-kicker">
              Workspace directory
            </p>

            <h2>
              Team access
            </h2>

            <p>
              Review members, pending invitations
              and inactive accounts from one place.
            </p>
          </div>

          {#if
            activeView === 'active' &&
            activeMembers.length > 5
          }
            <label class="member-search">
              <span class="visually-hidden">
                Find a member
              </span>

              <svg
                viewBox="0 0 24 24"
                aria-hidden="true"
              >
                <circle
                  cx="11"
                  cy="11"
                  r="7"
                ></circle>

                <path d="m20 20-4-4"></path>
              </svg>

              <input
                bind:value={memberQuery}
                type="search"
                placeholder="Find a member"
              />
            </label>
          {/if}
        </header>

        <nav
          class="directory-tabs"
          aria-label="Member directory sections"
        >
          <button
            class:active={
              activeView === 'active'
            }
            type="button"
            aria-pressed={
              activeView === 'active'
            }
            onclick={() => {
              activeView = 'active';
            }}
          >
            Active

            <span>
              {activeMembers.length}
            </span>
          </button>

          {#if
            canManage &&
            pendingInvites.length > 0
          }
            <button
              class:active={
                activeView === 'pending'
              }
              type="button"
              aria-pressed={
                activeView === 'pending'
              }
              onclick={() => {
                activeView = 'pending';
              }}
            >
              Invitations

              <span>
                {pendingInvites.length}
              </span>
            </button>
          {/if}

          {#if inactiveMembers.length > 0}
            <button
              class:active={
                activeView === 'inactive'
              }
              type="button"
              aria-pressed={
                activeView === 'inactive'
              }
              onclick={() => {
                activeView = 'inactive';
              }}
            >
              Inactive

              <span>
                {inactiveMembers.length}
              </span>
            </button>
          {/if}
        </nav>

        <!-- Active members -->
        {#if activeView === 'active'}
          {#if visibleActiveMembers.length === 0}
            <div class="directory-empty">
              <strong>
                No member matches your search.
              </strong>

              <p>
                Try another email address or role.
              </p>
            </div>
          {:else}
            <div class="member-list">
              {#each visibleActiveMembers as member (member.id)}
                <article class="member-row">
                  <span class="member-avatar">
                    {memberInitial(member.email)}
                  </span>

                  <div class="member-identity">
                    <div>
                      <strong>
                        {member.email || 'Member'}
                      </strong>

                      {#if isCurrentUser(member)}
                        <span class="you-badge">
                          You
                        </span>
                      {/if}
                    </div>

                    <small>
                      {member.role === 'owner'
                        ? 'Workspace owner'
                        : accessLabel(
                            member.access_scope
                          )}
                    </small>
                  </div>

                  <div class="member-access">
                    <span>
                      Project access
                    </span>

                    <strong>
                      {accessLabel(
                        member.access_scope
                      )}
                    </strong>

                    {#if
                      member.access_scope ===
                      'selected_projects'
                    }
                      <small>
                        {projectNames(
                          member.project_ids_json
                        )}
                      </small>
                    {/if}
                  </div>

                  <div class="member-role">
                    <span>Role</span>

                    {#if canEditMember(member)}
                      <select
                        value={member.role}
                        disabled={
                          busyId === member.id
                        }
                        aria-label={`Change role for ${member.email}`}
                        onchange={(event) =>
                          handleRoleChange(
                            member,
                            event
                          )}
                      >
                        <option value="admin">
                          Admin
                        </option>

                        <option value="member">
                          Member
                        </option>

                        <option value="viewer">
                          Viewer
                        </option>
                      </select>
                    {:else}
                      <span
                        class:role-badge--owner={
                          member.role === 'owner'
                        }
                        class="role-badge"
                      >
                        {roleLabel(member.role)}
                      </span>
                    {/if}
                  </div>

                  <div class="member-actions">
                    {#if canEditMember(member)}
                      <button
                        class="button-secondary compact-button"
                        type="button"
                        disabled={
                          busyId === member.id
                        }
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
                        disabled={
                          busyId === member.id
                        }
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
                      <span class="protected-label">
                        {member.role === 'owner'
                          ? 'Protected owner'
                          : 'Current account'}
                      </span>
                    {/if}
                  </div>
                </article>
              {/each}
            </div>
          {/if}

        <!-- Pending invitations -->
        {:else if activeView === 'pending'}
          <div class="member-list">
            {#each pendingInvites as invite (invite.id)}
              <article class="member-row invitation-row">
                <span class="member-avatar member-avatar--pending">
                  {memberInitial(
                    invite.invited_email
                  )}
                </span>

                <div class="member-identity">
                  <strong>
                    {invite.invited_email}
                  </strong>

                  <small>
                    Waiting for acceptance
                  </small>
                </div>

                <div class="member-access">
                  <span>
                    Project access
                  </span>

                  <strong>
                    {accessLabel(
                      invite.access_scope
                    )}
                  </strong>

                  {#if
                    invite.access_scope ===
                    'selected_projects'
                  }
                    <small>
                      {projectNames(
                        invite.project_ids_json
                      )}
                    </small>
                  {/if}
                </div>

                <div class="member-role">
                  <span>Role</span>

                  <span class="role-badge">
                    {roleLabel(invite.role)}
                  </span>
                </div>

                <div class="member-actions">
                  <button
                    class="danger-button compact-button"
                    type="button"
                    disabled={
                      busyId === invite.id
                    }
                    onclick={() =>
                      openConfirmation({
                        kind: 'invite',
                        invite
                      })}
                  >
                    Revoke
                  </button>
                </div>
              </article>
            {/each}
          </div>

        <!-- Inactive members -->
        {:else}
          <div class="member-list">
            {#each inactiveMembers as member (member.id)}
              <article class="member-row inactive-row">
                <span class="member-avatar member-avatar--inactive">
                  {memberInitial(member.email)}
                </span>

                <div class="member-identity">
                  <strong>
                    {member.email || 'Member'}
                  </strong>

                  <small>
                    No current workspace access
                  </small>
                </div>

                <div class="member-access">
                  <span>Status</span>

                  <span class="status-badge">
                    {statusLabel(member.status)}
                  </span>
                </div>

                <div class="member-role">
                  <span>Previous role</span>

                  <span class="role-badge">
                    {roleLabel(member.role)}
                  </span>
                </div>

                <div class="member-actions">
                  {#if canManage}
                    <button
                      class="reactivate-button compact-button"
                      type="button"
                      disabled={
                        busyId === member.id
                      }
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
              </article>
            {/each}
          </div>
        {/if}

        {#if
          canManage &&
          inviteHistory.length > 0
        }
          <details class="invitation-history">
            <summary>
              <span>
                Invitation history
              </span>

              <span class="history-count">
                {inviteHistory.length}
              </span>

              <svg
                viewBox="0 0 24 24"
                aria-hidden="true"
              >
                <path d="m6 9 6 6 6-6"></path>
              </svg>
            </summary>

            <div class="history-list">
              {#each inviteHistory as invite (invite.id)}
                <div class="history-row">
                  <span class="member-avatar member-avatar--small">
                    {memberInitial(
                      invite.invited_email
                    )}
                  </span>

                  <div>
                    <strong>
                      {invite.invited_email}
                    </strong>

                    <small>
                      {roleLabel(invite.role)}
                      ·
                      {accessLabel(
                        invite.access_scope
                      )}
                    </small>
                  </div>

                  <span class="history-status">
                    {statusLabel(
                      invite.status
                    )}
                  </span>
                </div>
              {/each}
            </div>
          </details>
        {/if}
      </section>
    {/if}
  {/if}
</div>

<!-- =====================================================
     CONFIRMATION MODAL
     ===================================================== -->
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
      <span
        class:confirm-modal__icon--danger={
          confirmationIsDanger(
            confirmation
          )
        }
        class="confirm-modal__icon"
        aria-hidden="true"
      >
        {#if
          confirmationIsDanger(
            confirmation
          )
        }
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="1.9"
            stroke-linecap="round"
            stroke-linejoin="round"
          >
            <path d="M12 9v4"></path>
            <path d="M12 17h.01"></path>
            <path d="m10.3 3.7-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.7-3.3l-8-14a2 2 0 0 0-3.4 0Z"></path>
          </svg>
        {:else}
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="1.9"
            stroke-linecap="round"
            stroke-linejoin="round"
          >
            <path d="m5 12 4 4L19 6"></path>
          </svg>
        {/if}
      </span>

      <div class="confirm-modal__content">
        <h2 id="confirmation-title">
          {confirmationTitle(
            confirmation
          )}
        </h2>

        <p id="confirmation-description">
          {confirmationMessage(
            confirmation
          )}
        </p>
      </div>

      <div class="confirm-modal__actions">
        <button
          class="button-secondary"
          type="button"
          disabled={Boolean(busyId)}
          onclick={closeConfirmation}
        >
          Cancel
        </button>

        <button
          class:danger-button={
            confirmationIsDanger(
              confirmation
            )
          }
          type="button"
          disabled={Boolean(busyId)}
          onclick={confirmAction}
        >
          {busyId
            ? 'Working…'
            : confirmationButtonLabel(
                confirmation
              )}
        </button>
      </div>
    </dialog>
  </div>
{/if}

<style>
  /* =====================================================
     PAGE
     ===================================================== */

  .members-page {
    display: grid;
    max-width: 1180px;
    gap: 18px;
    margin-inline: auto;
    padding-bottom: 44px;
    animation: page-enter 420ms var(--ease) both;
  }

  @keyframes page-enter {
    from {
      opacity: 0;
      transform: translateY(7px);
    }
  }

  .visually-hidden {
    position: absolute;
    width: 1px;
    height: 1px;
    overflow: hidden;
    clip: rect(0 0 0 0);
    white-space: nowrap;
  }

  /* =====================================================
     HEADER
     ===================================================== */

  .page-header {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 24px;
    padding-bottom: 22px;
    border-bottom: 1px solid var(--line);
  }

  .page-heading {
    display: grid;
    gap: 7px;
  }

  .page-eyebrow {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    color: var(--brand-bright);
    font-family: var(--font-mono);
    font-size: 9px;
    font-weight: 700;
    letter-spacing: 0.1em;
    text-transform: uppercase;
  }

  .page-eyebrow > span {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: var(--brand);
    box-shadow: 0 0 0 4px var(--brand-faint);
  }

  .page-heading h1 {
    color: var(--text);
    font-size: clamp(28px, 4vw, 42px);
    font-weight: 650;
    line-height: 1;
    letter-spacing: -0.045em;
  }

  .page-heading > p:last-child {
    color: var(--text-muted);
    font-size: 12.5px;
    line-height: 1.55;
  }

  .page-heading strong {
    color: var(--text-soft);
    font-weight: 600;
  }

  .page-header__actions {
    display: flex;
    flex: 0 0 auto;
    align-items: center;
    gap: 9px;
  }

  .workspace-context {
    display: flex;
    min-height: 40px;
    align-items: center;
    gap: 9px;
    padding: 5px 10px;
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
    background: var(--bg-panel);
    color: inherit;
    text-decoration: none;
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease);
  }

  .workspace-context:hover {
    border-color: var(--brand-line);
    background: var(--brand-faint);
  }

  .workspace-context > span {
    display: grid;
    min-width: 0;
    line-height: 1.25;
  }

  .workspace-context strong {
    color: var(--text-soft);
    font-size: 9px;
    font-weight: 600;
  }

  .workspace-context small {
    color: var(--text-muted);
    font-size: 7px;
  }

  .header-action svg {
    width: 13px;
    height: 13px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
  }

  /* =====================================================
     BUTTONS
     ===================================================== */

  button,
  .primary-link {
    display: inline-flex;
    min-height: 38px;
    align-items: center;
    justify-content: center;
    gap: 7px;
    border: 1px solid var(--brand);
    border-radius: var(--radius-sm);
    background: var(--brand);
    color: var(--brand-ink);
    padding: 0 14px;
    font: inherit;
    font-size: 12px;
    font-weight: 650;
    cursor: pointer;
    text-decoration: none;
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease),
      color var(--speed) var(--ease),
      transform var(--speed) var(--ease);
  }

  button:hover:not(:disabled),
  .primary-link:hover {
    border-color: var(--brand-soft);
    background: var(--brand-soft);
    transform: translateY(-1px);
  }

  button:disabled {
    cursor: not-allowed;
    opacity: 0.5;
  }

  .button-secondary {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-soft);
  }

  .button-secondary:hover:not(:disabled) {
    border-color: rgba(255, 255, 255, 0.17);
    background: var(--bg-elevated);
    color: var(--text);
  }

  .danger-button {
    border-color: var(--danger-line);
    background: var(--danger-faint);
    color: var(--danger);
  }

  .danger-button:hover:not(:disabled) {
    border-color: var(--danger);
    background: rgba(248, 113, 113, 0.15);
    color: var(--danger);
  }

  .reactivate-button {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .reactivate-button:hover:not(:disabled) {
    border-color: var(--green-soft);
    background: rgba(34, 197, 94, 0.14);
    color: var(--green-soft);
  }

  .compact-button {
    min-height: 30px;
    padding: 0 10px;
    font-size: 10px;
  }

  .primary-link svg,
  .team-onboarding__copy button svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .panel-kicker {
    color: var(--brand-soft);
    font-family: var(--font-mono);
    font-size: 7px;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }

  /* =====================================================
     MESSAGES
     ===================================================== */

  .success-message {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 11px 13px;
    border: 1px solid var(--green-line);
    border-radius: var(--radius-sm);
    background: var(--green-faint);
  }

  .success-message > span {
    display: grid;
    width: 25px;
    height: 25px;
    flex: 0 0 auto;
    place-items: center;
    border-radius: 6px;
    background: rgba(34, 197, 94, 0.1);
    color: var(--green-soft);
  }

  .success-message svg {
    width: 13px;
    height: 13px;
  }

  .success-message p {
    color: var(--green-soft);
    font-size: 11.5px;
  }

  /* =====================================================
     LOADING
     ===================================================== */

  .page-loading {
    position: relative;
    display: flex;
    min-height: 112px;
    align-items: center;
    gap: 13px;
    overflow: hidden;
    padding: 20px;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .page-loading__mark {
    display: grid;
    width: 42px;
    height: 42px;
    flex: 0 0 auto;
    place-items: center;
    border-radius: 10px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 13px;
    font-weight: 800;
  }

  .page-loading > div {
    display: grid;
    gap: 4px;
  }

  .page-loading strong {
    color: var(--text);
    font-size: 12.5px;
  }

  .page-loading p {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .page-loading__bar {
    position: absolute;
    right: 0;
    bottom: 0;
    left: 0;
    height: 2px;
    background: var(--brand);
    transform-origin: left;
    animation: loading-bar 1.4s ease-in-out infinite;
  }

  @keyframes loading-bar {
    0% {
      transform: scaleX(0);
    }

    65% {
      transform: scaleX(1);
    }

    100% {
      opacity: 0;
      transform: scaleX(1);
    }
  }

  /* =====================================================
     WORKSPACE REQUIRED
     ===================================================== */

  .workspace-required {
    display: grid;
    max-width: 760px;
    min-height: 310px;
    align-content: center;
    justify-items: start;
    gap: 15px;
    padding: clamp(28px, 6vw, 56px);
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-lg);
    background: var(--bg-panel);
    box-shadow: var(--shadow-panel);
  }

  .workspace-required__icon {
    display: grid;
    width: 49px;
    height: 49px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 11px;
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .workspace-required__icon svg {
    width: 22px;
    height: 22px;
  }

  .workspace-required > div {
    display: grid;
    gap: 7px;
  }

  .workspace-required h2 {
    color: var(--text);
    font-size: clamp(27px, 4vw, 42px);
    font-weight: 630;
    letter-spacing: -0.045em;
  }

  .workspace-required div > p:not(.panel-kicker) {
    max-width: 570px;
    color: var(--text-muted);
    font-size: 12px;
    line-height: 1.65;
  }

  /* =====================================================
     ACCESS OVERVIEW
     ===================================================== */

  .access-overview {
    display: grid;
    grid-template-columns:
      minmax(250px, 0.72fr)
      minmax(0, 1.28fr);
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .access-overview__identity {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 12px;
    padding: 18px 19px;
    border-right: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .access-overview__identity > div {
    display: grid;
    min-width: 0;
    gap: 5px;
  }

  .access-overview__identity h2 {
    overflow-wrap: anywhere;
    color: var(--text);
    font-size: 18px;
    font-weight: 620;
    letter-spacing: -0.03em;
  }

  .access-overview__identity code {
    color: var(--text-muted);
    font-size: 9px;
  }

  .access-metrics {
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
  }

  .access-metrics article {
    display: grid;
    min-width: 0;
    align-content: center;
    gap: 4px;
    padding: 15px 18px;
    border-right: 1px solid var(--line-soft);
  }

  .access-metrics article:last-child {
    border-right: 0;
  }

  .access-metrics span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6.5px;
    letter-spacing: 0.06em;
    text-transform: uppercase;
  }

  .access-metrics strong {
    color: var(--text);
    font-size: 18px;
    font-weight: 620;
  }

  .access-metrics small {
    overflow: hidden;
    color: var(--text-faint);
    font-size: 8px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  /* =====================================================
     INVITE PANEL
     ===================================================== */

  .invite-panel {
    overflow: hidden;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    box-shadow: inset 3px 0 0 var(--brand);
    animation: invite-enter 300ms var(--ease) both;
  }

  @keyframes invite-enter {
    from {
      opacity: 0;
      transform: translateY(-6px);
    }
  }

  .invite-panel__header {
    display: flex;
    align-items: flex-start;
    gap: 12px;
    padding: 17px 19px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .invite-panel__icon {
    display: grid;
    width: 42px;
    height: 42px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 10px;
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .invite-panel__icon svg {
    width: 20px;
    height: 20px;
  }

  .invite-panel__header > div {
    display: grid;
    gap: 4px;
  }

  .invite-panel__header h2 {
    color: var(--text);
    font-size: 16px;
    font-weight: 620;
    letter-spacing: -0.025em;
  }

  .invite-panel__header p:not(.panel-kicker) {
    color: var(--text-muted);
    font-size: 10px;
  }

  .invite-form {
    display: grid;
    grid-template-columns:
      minmax(0, 1.35fr)
      minmax(260px, 0.65fr);
  }

  .invite-form__fields {
    display: grid;
    gap: 18px;
    padding: 20px;
    border-right: 1px solid var(--line-soft);
  }

  .invite-form label,
  .invite-form fieldset {
    min-width: 0;
  }

  .email-field {
    display: grid;
    gap: 7px;
  }

  .email-field > span,
  .invite-form legend {
    color: var(--text-soft);
    font-size: 10.5px;
    font-weight: 550;
  }

  .invite-form fieldset {
    display: grid;
    gap: 9px;
    margin: 0;
    border: 0;
    padding: 0;
  }

  .email-field input {
    width: 100%;
    min-height: 41px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    outline: 0;
    background: var(--bg-ink-soft);
    color: var(--text);
    padding: 0 11px;
    font: inherit;
    font-size: 12px;
  }

  .email-field input:focus {
    border-color: var(--brand);
    box-shadow: 0 0 0 3px var(--brand-faint);
  }

  .option-grid {
    display: grid;
    grid-template-columns:
      repeat(2, minmax(0, 1fr));
    gap: 8px;
  }

  .option-grid--roles {
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
  }

  .choice-option {
    display: grid;
    min-height: 82px;
    grid-template-columns:
      auto minmax(0, 1fr);
    align-items: start;
    gap: 9px;
    border-color: var(--line);
    background: var(--bg-ink-soft);
    color: var(--text);
    padding: 11px;
    text-align: left;
  }

  .choice-option:hover:not(:disabled) {
    border-color: var(--line-strong);
    background: var(--bg-elevated);
    transform: none;
  }

  .choice-option.selected {
    border-color: var(--brand-line);
    background: var(--brand-faint);
  }

  .choice-option__indicator {
    width: 11px;
    height: 11px;
    margin-top: 2px;
    border: 1px solid var(--line-strong);
    border-radius: 50%;
    background: transparent;
  }

  .choice-option.selected
    .choice-option__indicator {
    border: 3px solid var(--bg-panel);
    background: var(--brand);
    box-shadow: 0 0 0 1px var(--brand);
  }

  .choice-option > span:last-child {
    display: grid;
    gap: 4px;
  }

  .choice-option strong {
    color: var(--text-soft);
    font-size: 10px;
    font-weight: 600;
  }

  .choice-option.selected strong {
    color: var(--brand-bright);
  }

  .choice-option small {
    color: var(--text-muted);
    font-size: 8px;
    line-height: 1.45;
  }

  .project-selector {
    padding-top: 2px !important;
  }

  .project-selector__header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    color: var(--text-muted);
    font-size: 9px;
  }

  .project-selector__header strong {
    color: var(--brand-soft);
    font-size: 8px;
  }

  .project-list {
    display: grid;
    grid-template-columns:
      repeat(2, minmax(0, 1fr));
    gap: 7px;
  }

  .project-option {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 9px;
    padding: 9px 10px;
    border: 1px solid var(--line-soft);
    border-radius: 7px;
    background: var(--bg-ink-soft);
    color: var(--text);
    cursor: pointer;
  }

  .project-option input {
    width: 15px;
    height: 15px;
    flex: 0 0 auto;
    accent-color: var(--brand);
  }

  .project-option > span {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .project-option strong {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 9.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .project-option code {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 8px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .invite-preview {
    display: grid;
    align-content: start;
    justify-items: start;
    gap: 12px;
    padding: 20px;
    background: var(--bg-ink-soft);
  }

  .invite-preview__avatar {
    display: grid;
    width: 48px;
    height: 48px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 12px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 15px;
    font-weight: 750;
  }

  .invite-preview__identity {
    display: grid;
    max-width: 100%;
    gap: 3px;
  }

  .invite-preview__identity strong {
    max-width: 100%;
    overflow-wrap: anywhere;
    color: var(--text);
    font-size: 11px;
  }

  .invite-preview__identity span {
    color: var(--text-muted);
    font-size: 8.5px;
  }

  .invite-preview dl {
    display: grid;
    width: 100%;
    margin: 3px 0 0;
    border: 1px solid var(--line-soft);
    border-radius: 8px;
  }

  .invite-preview dl > div {
    display: grid;
    gap: 4px;
    padding: 10px;
    border-bottom: 1px solid var(--line-soft);
  }

  .invite-preview dl > div:last-child {
    border-bottom: 0;
  }

  .invite-preview dt {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6px;
    text-transform: uppercase;
  }

  .invite-preview dd {
    overflow-wrap: anywhere;
    margin: 0;
    color: var(--text-soft);
    font-size: 8.5px;
    line-height: 1.4;
  }

  .invite-preview__note {
    color: var(--text-muted);
    font-size: 8px;
    line-height: 1.5;
  }

  .invite-actions {
    grid-column: 1 / -1;
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    padding: 13px 20px;
    border-top: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  /* =====================================================
     TEAM ONBOARDING
     ===================================================== */

  .team-onboarding {
    display: grid;
    grid-template-columns:
      minmax(0, 0.9fr)
      minmax(340px, 1.1fr);
    gap: clamp(38px, 7vw, 80px);
    align-items: center;
    padding: clamp(30px, 5vw, 54px);
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-lg);
    background: var(--bg-panel);
    box-shadow: var(--shadow-panel);
  }

  .team-onboarding__copy {
    display: grid;
    align-content: center;
  }

  .team-onboarding__copy h2 {
    max-width: 560px;
    margin-top: 10px;
    color: var(--text);
    font-size: clamp(31px, 4.5vw, 50px);
    font-weight: 640;
    line-height: 1;
    letter-spacing: -0.052em;
  }

  .team-onboarding__copy h2 span {
    display: block;
    color: var(--brand-soft);
  }

  .team-onboarding__copy > p:not(.panel-kicker) {
    max-width: 540px;
    margin-top: 17px;
    color: var(--text-muted);
    font-size: 12.5px;
    line-height: 1.7;
  }

  .team-onboarding__copy > button {
    width: fit-content;
    margin-top: 24px;
  }

  .team-onboarding__path {
    display: grid;
    gap: 9px;
  }

  .team-onboarding__path article {
    display: grid;
    min-height: 75px;
    grid-template-columns:
      30px minmax(0, 1fr);
    align-items: center;
    gap: 11px;
    padding: 12px 14px;
    border: 1px solid var(--line);
    border-radius: 8px;
    background: var(--bg-ink-soft);
  }

  .team-onboarding__path article.active {
    border-color: var(--brand-line);
    background: var(--brand-faint);
  }

  .team-onboarding__path article > span {
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 8px;
    font-weight: 700;
  }

  .team-onboarding__path article.active > span {
    color: var(--brand-soft);
  }

  .team-onboarding__path article > div {
    display: grid;
    gap: 4px;
  }

  .team-onboarding__path strong {
    color: var(--text-soft);
    font-size: 10px;
  }

  .team-onboarding__path p {
    color: var(--text-muted);
    font-size: 8.5px;
  }

  .current-owner {
    grid-column: 1 / -1;
    display: grid;
    grid-template-columns:
      auto minmax(0, 1fr) auto;
    align-items: center;
    gap: 10px;
    padding-top: 22px;
    border-top: 1px solid var(--line);
  }

  .current-owner > div {
    display: grid;
    gap: 3px;
  }

  .current-owner strong {
    color: var(--text-soft);
    font-size: 10.5px;
  }

  .current-owner div > span {
    color: var(--text-muted);
    font-size: 8.5px;
  }

  /* =====================================================
     MEMBER DIRECTORY
     ===================================================== */

  .member-directory {
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .directory-header {
    display: flex;
    min-height: 79px;
    align-items: center;
    justify-content: space-between;
    gap: 20px;
    padding: 14px 17px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .directory-header > div {
    display: grid;
    gap: 3px;
  }

  .directory-header h2 {
    color: var(--text);
    font-size: 14px;
    font-weight: 610;
  }

  .directory-header p:not(.panel-kicker) {
    color: var(--text-muted);
    font-size: 9.5px;
  }

  .member-search {
    position: relative;
    width: min(240px, 100%);
  }

  .member-search svg {
    position: absolute;
    top: 50%;
    left: 11px;
    width: 14px;
    height: 14px;
    fill: none;
    stroke: var(--text-muted);
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
    transform: translateY(-50%);
    pointer-events: none;
  }

  .member-search input {
    width: 100%;
    min-height: 35px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    outline: 0;
    background: var(--bg-ink-soft);
    color: var(--text);
    padding: 0 10px 0 33px;
    font: inherit;
    font-size: 10.5px;
  }

  .member-search input:focus {
    border-color: var(--brand);
  }

  .directory-tabs {
    display: flex;
    gap: 4px;
    padding: 9px 13px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .directory-tabs button {
    min-height: 31px;
    gap: 7px;
    border-color: transparent;
    background: transparent;
    color: var(--text-muted);
    padding: 0 10px;
    font-size: 9.5px;
  }

  .directory-tabs button:hover:not(:disabled) {
    border-color: var(--line);
    background: var(--bg-elevated);
    color: var(--text-soft);
    transform: none;
  }

  .directory-tabs button.active {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-bright);
  }

  .directory-tabs button > span {
    display: grid;
    min-width: 19px;
    height: 18px;
    place-items: center;
    border-radius: 999px;
    background: var(--bg-panel);
    color: inherit;
    padding-inline: 5px;
    font-family: var(--font-mono);
    font-size: 7px;
  }

  .member-list {
    display: grid;
  }

  .member-row {
    display: grid;
    min-height: 74px;
    grid-template-columns:
      38px minmax(185px, 1.2fr)
      minmax(160px, 0.9fr)
      minmax(120px, 0.55fr)
      minmax(170px, auto);
    align-items: center;
    gap: 12px;
    padding: 11px 15px;
    border-bottom: 1px solid var(--line-soft);
    transition: background var(--speed) var(--ease);
  }

  .member-row:last-child {
    border-bottom: 0;
  }

  .member-row:hover {
    background: rgba(255, 255, 255, 0.018);
  }

  .member-avatar {
    display: grid;
    width: 36px;
    height: 36px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 9px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 10px;
    font-weight: 750;
  }

  .member-avatar--pending {
    border-color: var(--warning-line);
    background: var(--warning-faint);
    color: var(--warning);
  }

  .member-avatar--inactive {
    border-color: var(--line-strong);
    background: var(--bg-elevated);
    color: var(--text-muted);
  }

  .member-avatar--small {
    width: 30px;
    height: 30px;
    border-radius: 7px;
    font-size: 8px;
  }

  .member-identity,
  .member-access,
  .member-role {
    display: grid;
    min-width: 0;
    gap: 4px;
  }

  .member-identity > div {
    display: flex;
    min-width: 0;
    flex-wrap: wrap;
    align-items: center;
    gap: 6px;
  }

  .member-identity strong,
  .member-access strong {
    overflow-wrap: anywhere;
    color: var(--text-soft);
    font-size: 10.5px;
    font-weight: 600;
  }

  .member-identity small,
  .member-access small {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 8.5px;
    line-height: 1.4;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .member-access > span:first-child,
  .member-role > span:first-child {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6px;
    letter-spacing: 0.05em;
    text-transform: uppercase;
  }

  .member-role select {
    width: 100%;
    max-width: 125px;
    min-height: 31px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    outline: 0;
    background: var(--bg-ink-soft);
    color: var(--text-soft);
    padding: 0 8px;
    font: inherit;
    font-size: 9.5px;
  }

  .member-role select:focus {
    border-color: var(--brand);
  }

  .member-actions {
    display: flex;
    min-width: 0;
    flex-wrap: wrap;
    align-items: center;
    justify-content: flex-end;
    gap: 6px;
  }

  .you-badge {
    padding: 2px 6px;
    border: 1px solid var(--brand-line);
    border-radius: 999px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 6px;
    font-weight: 700;
    text-transform: uppercase;
  }

  .role-badge,
  .status-badge,
  .history-status {
    display: inline-flex;
    width: fit-content;
    min-height: 22px;
    align-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    background: var(--bg-elevated);
    color: var(--text-soft);
    padding: 1px 8px;
    font-size: 8.5px;
    white-space: nowrap;
  }

  .role-badge--owner {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .status-badge {
    border-color: var(--danger-line);
    background: var(--danger-faint);
    color: var(--danger);
  }

  .protected-label {
    color: var(--text-faint);
    font-size: 8.5px;
  }

  .directory-empty {
    display: grid;
    min-height: 160px;
    place-content: center;
    justify-items: center;
    gap: 5px;
    padding: 24px;
    text-align: center;
  }

  .directory-empty strong {
    color: var(--text-soft);
    font-size: 11px;
  }

  .directory-empty p {
    color: var(--text-muted);
    font-size: 9px;
  }

  /* =====================================================
     INVITATION HISTORY
     ===================================================== */

  .invitation-history {
    border-top: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .invitation-history summary {
    display: grid;
    min-height: 51px;
    grid-template-columns:
      minmax(0, 1fr) auto auto;
    align-items: center;
    gap: 10px;
    padding: 11px 15px;
    color: var(--text-muted);
    font-size: 9.5px;
    font-weight: 600;
    cursor: pointer;
    list-style: none;
  }

  .invitation-history summary::-webkit-details-marker {
    display: none;
  }

  .invitation-history summary > svg {
    width: 13px;
    height: 13px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    transition: transform var(--speed) var(--ease);
  }

  .invitation-history[open]
    summary > svg {
    transform: rotate(180deg);
  }

  .history-count {
    display: grid;
    min-width: 22px;
    height: 20px;
    place-items: center;
    border: 1px solid var(--line);
    border-radius: 999px;
    background: var(--bg-elevated);
    padding-inline: 6px;
    font-family: var(--font-mono);
    font-size: 7px;
  }

  .history-list {
    display: grid;
    border-top: 1px solid var(--line-soft);
  }

  .history-row {
    display: grid;
    min-height: 55px;
    grid-template-columns:
      auto minmax(0, 1fr) auto;
    align-items: center;
    gap: 10px;
    padding: 10px 15px;
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
    overflow-wrap: anywhere;
    color: var(--text-soft);
    font-size: 9.5px;
  }

  .history-row small {
    color: var(--text-muted);
    font-size: 8px;
  }

  /* =====================================================
     CONFIRMATION MODAL
     ===================================================== */

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

  .modal-backdrop:hover:not(:disabled) {
    background: rgba(0, 0, 0, 0.72);
    transform: none;
  }

  .confirm-modal {
    position: relative;
    z-index: 1;
    display: grid;
    width: min(100%, 430px);
    max-width: none;
    justify-items: start;
    gap: 13px;
    margin: 0;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    color: var(--text);
    padding: 20px;
    box-shadow: var(--shadow-deep);
  }

  .confirm-modal:focus {
    outline: none;
  }

  .confirm-modal::backdrop {
    background: transparent;
  }

  .confirm-modal__icon {
    display: grid;
    width: 38px;
    height: 38px;
    place-items: center;
    border: 1px solid var(--green-line);
    border-radius: 9px;
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .confirm-modal__icon--danger {
    border-color: var(--danger-line);
    background: var(--danger-faint);
    color: var(--danger);
  }

  .confirm-modal__icon svg {
    width: 18px;
    height: 18px;
  }

  .confirm-modal__content {
    display: grid;
    gap: 7px;
  }

  .confirm-modal__content h2 {
    color: var(--text);
    font-size: 16px;
    font-weight: 620;
  }

  .confirm-modal__content p {
    color: var(--text-muted);
    font-size: 11px;
    line-height: 1.6;
  }

  .confirm-modal__actions {
    display: flex;
    width: 100%;
    justify-content: flex-end;
    gap: 8px;
    padding-top: 15px;
    border-top: 1px solid var(--line-soft);
  }

  /* =====================================================
     REDUCED MOTION
     ===================================================== */

  @media (prefers-reduced-motion: reduce) {
    .members-page,
    .invite-panel,
    .page-loading__bar {
      animation: none;
    }
  }

  /* =====================================================
     RESPONSIVE
     ===================================================== */

  @media (max-width: 1000px) {
    .invite-form {
      grid-template-columns: 1fr;
    }

    .invite-form__fields {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .invite-preview {
      grid-template-columns:
        auto minmax(0, 1fr);
      align-items: center;
    }

    .invite-preview > .panel-kicker,
    .invite-preview dl,
    .invite-preview__note {
      grid-column: 1 / -1;
    }

    .member-row {
      grid-template-columns:
        38px minmax(170px, 1.2fr)
        minmax(145px, 0.8fr)
        minmax(110px, 0.5fr);
    }

    .member-actions {
      grid-column: 2 / -1;
      justify-content: flex-start;
      padding-top: 8px;
      border-top: 1px solid var(--line-soft);
    }
  }

  @media (max-width: 820px) {
    .access-overview {
      grid-template-columns: 1fr;
    }

    .access-overview__identity {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .team-onboarding {
      grid-template-columns: 1fr;
    }
  }

  @media (max-width: 720px) {
    .page-header {
      align-items: stretch;
      flex-direction: column;
    }

    .page-header__actions {
      align-items: stretch;
      flex-direction: column;
    }

    .workspace-context,
    .header-action {
      width: 100%;
    }

    .access-metrics {
      grid-template-columns: 1fr;
    }

    .access-metrics article {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .access-metrics article:last-child {
      border-bottom: 0;
    }

    .directory-header {
      align-items: stretch;
      flex-direction: column;
    }

    .member-search {
      width: 100%;
    }

    .member-row {
      grid-template-columns:
        38px minmax(0, 1fr);
      gap: 10px;
      padding: 14px;
    }

    .member-access,
    .member-role,
    .member-actions {
      grid-column: 2;
    }

    .member-access,
    .member-role {
      padding-top: 7px;
      border-top: 1px solid var(--line-soft);
    }

    .member-actions {
      padding-top: 9px;
    }

    .member-role select {
      max-width: none;
    }
  }

  @media (max-width: 620px) {
    .option-grid,
    .option-grid--roles,
    .project-list {
      grid-template-columns: 1fr;
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

    .team-onboarding {
      padding: 26px 19px;
    }

    .team-onboarding__copy h2 {
      font-size: clamp(31px, 10vw, 43px);
    }

    .directory-tabs {
      overflow-x: auto;
    }

    .directory-tabs button {
      flex: 0 0 auto;
    }
  }

  @media (max-width: 460px) {
    .invite-form__fields,
    .invite-preview {
      padding: 16px;
    }

    .current-owner {
      grid-template-columns:
        auto minmax(0, 1fr);
    }

    .current-owner > .role-badge {
      grid-column: 2;
    }

    .member-actions {
      align-items: stretch;
      flex-direction: column;
    }

    .member-actions button {
      width: 100%;
    }

    .history-row {
      grid-template-columns:
        auto minmax(0, 1fr);
    }

    .history-status {
      grid-column: 2;
    }
  }
</style>
