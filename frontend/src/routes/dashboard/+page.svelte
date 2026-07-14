<script lang="ts">
  import { goto } from '$app/navigation';

  import { usePageRefresh } from '$lib/stores/pageRefresh';
  import { auth } from '$lib/stores/auth';
  import { notifications } from '$lib/stores/notifications';
  import { workspaceContext } from '$lib/stores/workspace';

  import {
    createWorkspace,
    listWorkspaces
  } from '$lib/api/workspaces';

  import { listProjects } from '$lib/api/projects';

  import {
    acceptWorkspaceInvite,
    declineWorkspaceInvite,
    listMyWorkspaceInvites
  } from '$lib/api/workspaceInvites';

  import {
    ApiError,
    type Project,
    type Workspace,
    type WorkspaceInvite
  } from '$lib/api/types';

  import {
    canCreateProject,
    canManageMembers,
    canManageTokens
  } from '$lib/permissions';

  import { notificationHref } from '$lib/notifications/notificationLinks';

  import InlineError from '$lib/components/InlineError.svelte';
  import WorkspaceAvatar from '$lib/components/WorkspaceAvatar.svelte';

  type DashboardProject = {
    project: Project;
    workspace: Workspace;
  };

  type QuickLink = {
    label: string;
    href: string;
    icon: 'workspace' | 'projects' | 'packages' | 'members' | 'tokens';
  };

  let workspaces: Workspace[] = [];
  let invitations: WorkspaceInvite[] = [];
  let projectEntries: DashboardProject[] = [];

  let loading = true;
  let saving = false;
  let inviteBusyId = '';

  let error = '';
  let success = '';

  let showWorkspaceForm = false;
  let workspaceName = '';
  let workspaceSlug = '';
  let slugTouched = false;

  $: pendingInvites = invitations.filter(
    (invite) => invite.status === 'pending'
  );

  $: hasWorkspaces = workspaces.length > 0;

  $: selectedWorkspace =
    $workspaceContext.selectedWorkspace ??
    workspaces[0] ??
    null;

  $: currentRole =
    selectedWorkspace?.current_user_role ??
    'viewer';

  $: currentWorkspaceProjects = selectedWorkspace
    ? projectEntries.filter(
        (entry) =>
          entry.workspace.id === selectedWorkspace?.id
      )
    : [];

  $: recentProjects =
    currentWorkspaceProjects.slice(0, 5);

  $: recentNotifications =
    $notifications.notifications.slice(0, 5);

  $: unreadNotifications =
    $notifications.unreadCount;

  $: effectiveSlug = slugTouched
    ? slugify(workspaceSlug)
    : slugify(workspaceName);

  $: quickLinks = buildQuickLinks(
    selectedWorkspace,
    currentRole
  );

  function slugify(value: string) {
    return value
      .trim()
      .toLowerCase()
      .replace(/[^a-z0-9]+/g, '-')
      .replace(/^-|-$/g, '');
  }

  function roleLabel(value?: string) {
    if (!value) {
      return 'Viewer';
    }

    return value.charAt(0).toUpperCase() + value.slice(1);
  }

  function accessLabel(value?: string) {
    return value === 'selected_projects'
      ? 'Selected projects'
      : 'Entire workspace';
  }

  function workspaceStatus(workspace: Workspace) {
    return workspace.active === false
      ? 'Inactive'
      : 'Active';
  }

  function buildQuickLinks(
    workspace: Workspace | null,
    role: string
  ): QuickLink[] {
    if (!workspace) {
      return [];
    }

    const workspaceId =
      encodeURIComponent(workspace.id);

    const links: QuickLink[] = [
      {
        label: 'Workspace',
        href: `/workspaces?workspace_id=${workspaceId}`,
        icon: 'workspace'
      },
      {
        label: 'Projects',
        href: `/projects?workspace_id=${workspaceId}`,
        icon: 'projects'
      },
      {
        label: 'Packages',
        href: `/packages?workspace_id=${workspaceId}`,
        icon: 'packages'
      }
    ];

    if (canManageMembers(role)) {
      links.push({
        label: 'Members',
        href: `/members?workspace_id=${workspaceId}`,
        icon: 'members'
      });
    }

    if (canManageTokens(role)) {
      links.push({
        label: 'CLI tokens',
        href: `/tokens?workspace_id=${workspaceId}`,
        icon: 'tokens'
      });
    }

    return links;
  }

  function resetWorkspaceForm() {
    workspaceName = '';
    workspaceSlug = '';
    slugTouched = false;
  }

  function openWorkspaceForm() {
    showWorkspaceForm = true;
    error = '';
    success = '';
  }

  function closeWorkspaceForm() {
    if (saving) {
      return;
    }

    showWorkspaceForm = false;
    resetWorkspaceForm();
    error = '';
  }

  function toggleWorkspaceForm() {
    if (showWorkspaceForm) {
      closeWorkspaceForm();
      return;
    }

    openWorkspaceForm();
  }

  function handleSlugInput(event: Event) {
    const input =
      event.currentTarget as HTMLInputElement;

    slugTouched = true;
    workspaceSlug = input.value;
  }

  function handleWorkspaceSubmit(
    event: SubmitEvent
  ) {
    event.preventDefault();
    void submitWorkspace();
  }

  async function loadProjectsForWorkspaces(
    sourceWorkspaces: Workspace[]
  ) {
    if (sourceWorkspaces.length === 0) {
      projectEntries = [];
      return;
    }

    const results = await Promise.all(
      sourceWorkspaces.map(async (workspace) => {
        try {
          const data =
            await listProjects(workspace.id);

          return data.projects.map((project) => ({
            project,
            workspace
          }));
        } catch {
          return [] as DashboardProject[];
        }
      })
    );

    projectEntries = results.flat();
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
      const workspaceData =
        await listWorkspaces(user.id);

      workspaces = workspaceData.workspaces;

      const requestedWorkspaceId =
        new URLSearchParams(
          window.location.search
        ).get('workspace_id');

      const storedWorkspaceId =
        $workspaceContext.selectedWorkspace?.id;

      const resolvedWorkspaceId =
        requestedWorkspaceId &&
        workspaces.some(
          (workspace) =>
            workspace.id === requestedWorkspaceId
        )
          ? requestedWorkspaceId
          : storedWorkspaceId &&
              workspaces.some(
                (workspace) =>
                  workspace.id === storedWorkspaceId
              )
            ? storedWorkspaceId
            : workspaces[0]?.id;

      workspaceContext.setWorkspaces(
        workspaces,
        resolvedWorkspaceId
      );

      const [inviteData] = await Promise.all([
        listMyWorkspaceInvites(
          user.id,
          user.email
        ).catch(() => ({ invites: [] })),

        loadProjectsForWorkspaces(workspaces)
      ]);

      invitations = inviteData.invites.filter(
        (invite) => invite.status === 'pending'
      );

      /*
       * A new account immediately sees the creation form.
       * When an invitation exists, the user first chooses
       * whether to join it or create a separate workspace.
       */
      showWorkspaceForm =
        workspaces.length === 0 &&
        invitations.length === 0;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load dashboard.';
    } finally {
      loading = false;
    }
  }

  async function submitWorkspace() {
    const user = $auth.user;
    const name = workspaceName.trim();
    const slug = effectiveSlug.trim();

    if (!user || !name || !slug) {
      return;
    }

    saving = true;
    error = '';
    success = '';

    try {
      const created = await createWorkspace({
        name,
        slug,
        ownerUserId: user.id
      });

      workspaces = [
        created.workspace,
        ...workspaces
      ];

      workspaceContext.setWorkspaces(
        workspaces,
        created.workspace.id
      );

      resetWorkspaceForm();
      showWorkspaceForm = false;

      success =
        `${created.workspace.name} is ready. ` +
        'You can now connect your first Vix project.';

      /*
       * Stay on the dashboard instead of redirecting
       * the user away from the onboarding path.
       */
      window.history.replaceState(
        {},
        '',
        `/dashboard?workspace_id=${encodeURIComponent(
          created.workspace.id
        )}`
      );
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to create workspace.';
    } finally {
      saving = false;
    }
  }

  async function acceptInvite(
    invite: WorkspaceInvite
  ) {
    const user = $auth.user;

    if (!user || inviteBusyId) {
      return;
    }

    inviteBusyId = invite.id;
    error = '';
    success = '';

    try {
      await acceptWorkspaceInvite(
        invite.id,
        user.id,
        user.email
      );

      invitations = invitations.filter(
        (item) => item.id !== invite.id
      );

      const workspaceData =
        await listWorkspaces(user.id);

      workspaces = workspaceData.workspaces;

      workspaceContext.setWorkspaces(
        workspaces,
        invite.workspace_id
      );

      await loadProjectsForWorkspaces(workspaces);

      success =
        `You joined ${
          invite.workspace_name || 'the workspace'
        }.`;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to accept invitation.';
    } finally {
      inviteBusyId = '';
    }
  }

  async function declineInvite(
    invite: WorkspaceInvite
  ) {
    const user = $auth.user;

    if (!user || inviteBusyId) {
      return;
    }

    inviteBusyId = invite.id;
    error = '';
    success = '';

    try {
      await declineWorkspaceInvite(
        invite.id,
        user.id,
        user.email
      );

      invitations = invitations.filter(
        (item) => item.id !== invite.id
      );

      if (
        workspaces.length === 0 &&
        invitations.length === 0
      ) {
        showWorkspaceForm = true;
      }
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to decline invitation.';
    } finally {
      inviteBusyId = '';
    }
  }

  usePageRefresh(load);
</script>

<svelte:head>
  <title>Dashboard | Softadastra Cloud</title>

  <meta
    name="description"
    content="Manage your Softadastra Cloud workspace, Vix projects, private packages, build reports, members, and project activity."
  />
</svelte:head>

<div class="dashboard-page">
  <!-- =====================================================
       PAGE HEADER
       ===================================================== -->
  <header class="dashboard-header">
    <div class="dashboard-heading">
      <p class="dashboard-eyebrow">
        <span aria-hidden="true"></span>
        Softadastra Cloud
      </p>

      <h1>Dashboard</h1>

      <p>
        {#if loading}
          Loading your workspace…
        {:else if selectedWorkspace}
          Your current view is
          <strong>{selectedWorkspace.name}</strong>.
        {:else}
          Create a workspace to begin organizing your
          Vix projects.
        {/if}
      </p>
    </div>

    {#if hasWorkspaces}
      <button
        class:button-secondary={showWorkspaceForm}
        class="header-action"
        type="button"
        onclick={toggleWorkspaceForm}
      >
        {#if !showWorkspaceForm}
          <svg
            viewBox="0 0 24 24"
            aria-hidden="true"
          >
            <path d="M12 5v14M5 12h14"></path>
          </svg>
        {/if}

        {showWorkspaceForm
          ? 'Close'
          : 'New workspace'}
      </button>
    {/if}
  </header>

  <InlineError message={error} />

  {#if success}
    <div class="success-message" role="status">
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

  <!-- =====================================================
       INVITATIONS
       Only shown when action is required
       ===================================================== -->
  {#if pendingInvites.length > 0}
    <section
      class="invitation-panel"
      aria-labelledby="invitation-title"
    >
      <header class="panel-header">
        <div>
          <p class="panel-kicker">Action required</p>

          <h2 id="invitation-title">
            Workspace invitations
          </h2>

          <p>
            Join an existing workspace or create your
            own.
          </p>
        </div>

        <span class="panel-count">
          {pendingInvites.length}
        </span>
      </header>

      <div class="invitation-list">
        {#each pendingInvites as invite (invite.id)}
          <article class="invitation-row">
            <div class="invitation-identity">
              <span class="invitation-mark">
                {(invite.workspace_name ||
                  invite.workspace_id)
                  .charAt(0)
                  .toUpperCase()}
              </span>

              <div>
                <strong>
                  {invite.workspace_name ||
                    invite.workspace_id}
                </strong>

                <span>
                  {roleLabel(invite.role)}
                  ·
                  {accessLabel(invite.access_scope)}
                </span>
              </div>
            </div>

            <div class="invitation-actions">
              <button
                class="button-secondary button-compact"
                type="button"
                disabled={Boolean(inviteBusyId)}
                onclick={() => declineInvite(invite)}
              >
                {inviteBusyId === invite.id
                  ? 'Working…'
                  : 'Decline'}
              </button>

              <button
                class="button-compact"
                type="button"
                disabled={Boolean(inviteBusyId)}
                onclick={() => acceptInvite(invite)}
              >
                {inviteBusyId === invite.id
                  ? 'Working…'
                  : 'Accept invitation'}
              </button>
            </div>
          </article>
        {/each}
      </div>
    </section>
  {/if}

  <!-- =====================================================
       WORKSPACE CREATION
       ===================================================== -->
  {#if showWorkspaceForm}
    <section
      class:first-workspace-form={!hasWorkspaces}
      class="workspace-form-panel"
      aria-labelledby="workspace-form-title"
    >
      <div class="workspace-form-intro">
        <span class="workspace-form-icon" aria-hidden="true">
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

            <path d="M17.5 14v7M14 17.5h7"></path>
          </svg>
        </span>

        <div>
          <p class="panel-kicker">
            {hasWorkspaces
              ? 'New workspace'
              : 'First step'}
          </p>

          <h2 id="workspace-form-title">
            {hasWorkspaces
              ? 'Create another workspace'
              : 'Create your workspace'}
          </h2>

          <p>
            A workspace groups your projects, private
            packages, members, permissions and CLI
            access.
          </p>
        </div>
      </div>

      <form
        class="workspace-form"
        onsubmit={handleWorkspaceSubmit}
      >
        <label>
          <span>Workspace name</span>

          <input
            bind:value={workspaceName}
            placeholder="My engineering team"
            required
            autocomplete="organization"
          />
        </label>

        <label>
          <span>Workspace slug</span>

          <input
            value={effectiveSlug}
            placeholder="my-engineering-team"
            required
            autocomplete="off"
            oninput={handleSlugInput}
          />
        </label>

        <div class="workspace-slug-preview">
          <span>Workspace identifier</span>

          <code>
            {effectiveSlug || 'workspace-slug'}
          </code>
        </div>

        <div class="workspace-form-actions">
          {#if hasWorkspaces}
            <button
              class="button-secondary"
              type="button"
              disabled={saving}
              onclick={closeWorkspaceForm}
            >
              Cancel
            </button>
          {/if}

          <button
            type="submit"
            disabled={
              saving ||
              !workspaceName.trim() ||
              !effectiveSlug
            }
          >
            {saving
              ? 'Creating workspace…'
              : 'Create workspace'}
          </button>
        </div>
      </form>
    </section>
  {/if}

  <!-- =====================================================
       LOADING
       ===================================================== -->
  {#if loading}
    <section class="dashboard-loading">
      <span class="dashboard-loading__mark" aria-hidden="true">
        S
      </span>

      <div>
        <strong>Preparing your dashboard</strong>
        <p>
          Loading workspaces, projects and recent
          activity.
        </p>
      </div>

      <span
        class="dashboard-loading__progress"
        aria-hidden="true"
      ></span>
    </section>

  <!-- =====================================================
       NEW USER
       No empty metrics and no empty dashboard sections
       ===================================================== -->
  {:else if !hasWorkspaces}
    <section class="onboarding">
      <div class="onboarding-copy">
        <p class="panel-kicker">
          Your starting point
        </p>

        <h2>
          Your dashboard begins with
          <span>one workspace.</span>
        </h2>

        <p>
          The workspace becomes the home for your Vix
          projects, private packages, build information,
          members and access rules.
        </p>

        {#if !showWorkspaceForm}
          <button
            class="onboarding-action"
            type="button"
            onclick={openWorkspaceForm}
          >
            Create your workspace

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

      <div class="onboarding-path">
        <article class:active={showWorkspaceForm}>
          <span class="onboarding-step-number">01</span>

          <span class="onboarding-step-icon">
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="1.8"
              stroke-linecap="round"
              stroke-linejoin="round"
              aria-hidden="true"
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

              <path d="M17.5 14v7M14 17.5h7"></path>
            </svg>
          </span>

          <div>
            <strong>Create a workspace</strong>
            <p>
              Define the shared space that owns your
              projects and packages.
            </p>
          </div>
        </article>

        <article>
          <span class="onboarding-step-number">02</span>

          <span class="onboarding-step-icon">
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="1.8"
              stroke-linecap="round"
              stroke-linejoin="round"
              aria-hidden="true"
            >
              <path d="m8 6-5 6 5 6"></path>
              <path d="m16 6 5 6-5 6"></path>
              <path d="M10 19 14 5"></path>
            </svg>
          </span>

          <div>
            <strong>Connect a Vix project</strong>
            <p>
              Run <code>vix cloud init</code> from your
              local project.
            </p>
          </div>
        </article>

        <article>
          <span class="onboarding-step-number">03</span>

          <span class="onboarding-step-icon">
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="1.8"
              stroke-linecap="round"
              stroke-linejoin="round"
              aria-hidden="true"
            >
              <circle cx="8" cy="8" r="3"></circle>
              <circle cx="17" cy="9" r="2.5"></circle>
              <path d="M2.5 20a5.5 5.5 0 0 1 11 0"></path>
              <path d="M13 20a4 4 0 0 1 8 0"></path>
            </svg>
          </span>

          <div>
            <strong>Bring in the team</strong>
            <p>
              Add members only when the project is ready
              to be shared.
            </p>
          </div>
        </article>

        <span
          class="onboarding-path-line"
          aria-hidden="true"
        ></span>
      </div>
    </section>

  <!-- =====================================================
       ACTIVE DASHBOARD
       ===================================================== -->
  {:else if selectedWorkspace}
    <section class="workspace-overview">
      <div class="workspace-overview__main">
        <div class="workspace-overview__identity">
          <WorkspaceAvatar
            workspace={selectedWorkspace}
          />

          <div>
            <p class="panel-kicker">
              Current workspace
            </p>

            <div class="workspace-title-line">
              <h2>{selectedWorkspace.name}</h2>

              <span
                class:active={
                  selectedWorkspace.active !== false
                }
                class="workspace-status"
              >
                <i aria-hidden="true"></i>

                {workspaceStatus(selectedWorkspace)}
              </span>
            </div>

            <code>{selectedWorkspace.slug}</code>
          </div>
        </div>

        <a
          class="workspace-switch"
          href="/workspaces"
        >
          Change workspace

          <svg
            viewBox="0 0 24 24"
            aria-hidden="true"
          >
            <path d="m9 18 6-6-6-6"></path>
          </svg>
        </a>
      </div>

      <div class="workspace-metrics">
        <article>
          <span>Your role</span>
          <strong>{roleLabel(currentRole)}</strong>
          <small>
            {accessLabel(
              selectedWorkspace.access_scope
            )}
          </small>
        </article>

        <article>
          <span>Projects</span>
          <strong>
            {currentWorkspaceProjects.length}
          </strong>
          <small>Visible in this workspace</small>
        </article>

        <article>
          <span>Unread</span>
          <strong>{unreadNotifications}</strong>
          <small>Workspace notifications</small>
        </article>
      </div>

      <nav
        class="workspace-navigation"
        aria-label="Workspace navigation"
      >
        {#each quickLinks as link}
          <a href={link.href}>
            <span class="workspace-navigation__icon">
              {#if link.icon === 'workspace'}
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="1.8"
                  aria-hidden="true"
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
                  <rect
                    x="14"
                    y="14"
                    width="7"
                    height="7"
                    rx="1"
                  ></rect>
                </svg>
              {:else if link.icon === 'projects'}
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="1.8"
                  aria-hidden="true"
                >
                  <path d="M3 6h6l2 2h10v11H3z"></path>
                </svg>
              {:else if link.icon === 'packages'}
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="1.8"
                  aria-hidden="true"
                >
                  <path d="M21 16V8l-9-5-9 5v8l9 5 9-5Z"></path>
                  <path d="m3.3 8.3 8.7 4.8 8.7-4.8"></path>
                  <path d="M12 13v8"></path>
                </svg>
              {:else if link.icon === 'members'}
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="1.8"
                  aria-hidden="true"
                >
                  <circle cx="8" cy="8" r="3"></circle>
                  <circle cx="17" cy="9" r="2.5"></circle>
                  <path d="M2.5 20a5.5 5.5 0 0 1 11 0"></path>
                  <path d="M13 20a4 4 0 0 1 8 0"></path>
                </svg>
              {:else}
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="1.8"
                  aria-hidden="true"
                >
                  <circle cx="8" cy="15" r="3"></circle>
                  <path d="m10.5 13.5 8-8"></path>
                  <path d="m15 5 4 4"></path>
                </svg>
              {/if}
            </span>

            <span>{link.label}</span>

            <svg
              class="workspace-navigation__arrow"
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path d="m9 18 6-6-6-6"></path>
            </svg>
          </a>
        {/each}
      </nav>
    </section>

    <div class="dashboard-content">
      <!-- Projects -->
      <section
        class="dashboard-panel projects-panel"
        aria-labelledby="projects-title"
      >
        <header class="panel-header">
          <div>
            <p class="panel-kicker">
              Development
            </p>

            <h2 id="projects-title">
              Recent projects
            </h2>

            <p>
              Projects connected to
              {selectedWorkspace.name}.
            </p>
          </div>

          {#if currentWorkspaceProjects.length > 0}
            <a
              class="panel-link"
              href={`/projects?workspace_id=${encodeURIComponent(
                selectedWorkspace.id
              )}`}
            >
              View all
            </a>
          {/if}
        </header>

        {#if recentProjects.length === 0}
          <div class="connect-project">
            <span class="connect-project__icon">
              <svg
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="1.8"
                stroke-linecap="round"
                stroke-linejoin="round"
                aria-hidden="true"
              >
                <path d="m8 6-5 6 5 6"></path>
                <path d="m16 6 5 6-5 6"></path>
                <path d="M10 19 14 5"></path>
              </svg>
            </span>

            <div>
              <p class="panel-kicker">Next step</p>

              <h3>Connect your first Vix project</h3>

              <p>
                Run this command from the project you
                want to link to the workspace.
              </p>
            </div>

            <code>
              <span>$</span>
              vix cloud init
            </code>

            {#if canCreateProject(currentRole)}
              <a
                href={`/projects?workspace_id=${encodeURIComponent(
                  selectedWorkspace.id
                )}`}
              >
                Open project setup

                <svg
                  viewBox="0 0 24 24"
                  aria-hidden="true"
                >
                  <path d="M5 12h14"></path>
                  <path d="m13 6 6 6-6 6"></path>
                </svg>
              </a>
            {/if}
          </div>
        {:else}
          <div class="project-list">
            {#each recentProjects as entry (entry.project.id)}
              <a
                class="project-row"
                href={`/projects?workspace_id=${entry.workspace.id}&project_id=${entry.project.id}`}
              >
                <span class="project-row__mark">
                  {entry.project.name
                    .charAt(0)
                    .toUpperCase()}
                </span>

                <div>
                  <strong>
                    {entry.project.name}
                  </strong>

                  <span>
                    <code>{entry.project.slug}</code>
                    <i aria-hidden="true"></i>
                    {entry.workspace.name}
                  </span>
                </div>

                <svg
                  viewBox="0 0 24 24"
                  aria-hidden="true"
                >
                  <path d="m9 18 6-6-6-6"></path>
                </svg>
              </a>
            {/each}
          </div>
        {/if}
      </section>

      <!-- Activity -->
      <section
        class="dashboard-panel activity-panel"
        aria-labelledby="activity-title"
      >
        <header class="panel-header">
          <div>
            <p class="panel-kicker">
              Workspace
            </p>

            <h2 id="activity-title">
              Recent activity
            </h2>

            <p>
              Events that may need your attention.
            </p>
          </div>

          {#if recentNotifications.length > 0}
            <a
              class="panel-link"
              href="/notifications"
            >
              View all
            </a>
          {/if}
        </header>

        {#if $notifications.loading}
          <div class="compact-state">
            Loading recent activity…
          </div>
        {:else if recentNotifications.length === 0}
          <div class="activity-empty">
            <span aria-hidden="true">
              <svg
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="1.8"
                stroke-linecap="round"
                stroke-linejoin="round"
              >
                <path d="M4 19V5"></path>
                <path d="M4 19h16"></path>
                <path d="m7 15 3-4 3 2 4-6"></path>
              </svg>
            </span>

            <strong>No activity yet</strong>

            <p>
              Package publications, project changes and
              build reports will appear here.
            </p>
          </div>
        {:else}
          <div class="activity-list">
            {#each recentNotifications as item (item.id)}
              <a
                class="activity-row"
                href={notificationHref(item)}
              >
                <span
                  class:unread={!item.read_at}
                  class="activity-indicator"
                  aria-hidden="true"
                ></span>

                <div>
                  <strong>{item.title}</strong>
                  <p>{item.message}</p>
                </div>
              </a>
            {/each}
          </div>
        {/if}
      </section>
    </div>
  {/if}
</div>

<style>
  /* =====================================================
     PAGE
     ===================================================== */

  .dashboard-page {
    display: grid;
    gap: 18px;
    max-width: 1180px;
    margin-inline: auto;
    padding-bottom: 42px;
    animation: dashboard-enter 420ms var(--ease) both;
  }

  @keyframes dashboard-enter {
    from {
      opacity: 0;
      transform: translateY(8px);
    }
  }

  /* =====================================================
     HEADER
     ===================================================== */

  .dashboard-header {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 24px;
    padding-bottom: 22px;
    border-bottom: 1px solid var(--line);
  }

  .dashboard-heading {
    display: grid;
    gap: 7px;
  }

  .dashboard-eyebrow {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    margin: 0;
    color: var(--brand-bright);
    font-family: var(--font-mono);
    font-size: 9px;
    font-weight: 700;
    letter-spacing: 0.1em;
    text-transform: uppercase;
  }

  .dashboard-eyebrow > span {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: var(--brand);
    box-shadow: 0 0 0 4px var(--brand-faint);
  }

  .dashboard-heading h1 {
    margin: 0;
    color: var(--text);
    font-size: clamp(28px, 4vw, 42px);
    font-weight: 650;
    line-height: 1;
    letter-spacing: -0.045em;
  }

  .dashboard-heading > p:last-child {
    margin: 0;
    color: var(--text-muted);
    font-size: 12.5px;
    line-height: 1.55;
  }

  .dashboard-heading strong {
    color: var(--text-soft);
    font-weight: 600;
  }

  .header-action {
    display: inline-flex;
    min-height: 38px;
    flex: 0 0 auto;
    align-items: center;
    justify-content: center;
    gap: 7px;
    padding: 0 13px;
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
     SHARED ELEMENTS
     ===================================================== */

  button,
  .onboarding-action {
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
      box-shadow var(--speed) var(--ease),
      transform var(--speed) var(--ease);
  }

  button:hover:not(:disabled),
  .onboarding-action:hover {
    border-color: var(--brand-soft);
    background: var(--brand-soft);
    transform: translateY(-1px);
  }

  button:disabled {
    cursor: not-allowed;
    opacity: 0.55;
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

  .button-compact {
    min-height: 31px;
    padding: 0 11px;
    font-size: 11px;
  }

  .panel-kicker {
    margin: 0;
    color: var(--brand-soft);
    font-family: var(--font-mono);
    font-size: 7px;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }

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
    background: rgba(89, 200, 121, 0.12);
    color: var(--green-soft);
  }

  .success-message svg {
    width: 13px;
    height: 13px;
  }

  .success-message p {
    margin: 0;
    color: var(--green-soft);
    font-size: 11.5px;
  }

  /* =====================================================
     PANEL HEADER
     ===================================================== */

  .panel-header {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 18px;
    padding: 15px 17px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .panel-header > div {
    display: grid;
    gap: 3px;
  }

  .panel-header h2 {
    margin: 0;
    color: var(--text);
    font-size: 14px;
    font-weight: 600;
    letter-spacing: -0.02em;
  }

  .panel-header p:not(.panel-kicker) {
    margin: 0;
    color: var(--text-muted);
    font-size: 10.5px;
    line-height: 1.45;
  }

  .panel-count {
    display: grid;
    min-width: 25px;
    height: 23px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 999px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    padding: 0 7px;
    font-family: var(--font-mono);
    font-size: 9px;
  }

  .panel-link {
    flex: 0 0 auto;
    color: var(--brand-soft);
    font-size: 10.5px;
    font-weight: 600;
    text-decoration: none;
  }

  .panel-link:hover {
    color: var(--brand-bright);
  }

  /* =====================================================
     INVITATIONS
     ===================================================== */

  .invitation-panel {
    overflow: hidden;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    box-shadow: inset 3px 0 0 var(--brand);
  }

  .invitation-list {
    display: grid;
  }

  .invitation-row {
    display: flex;
    min-height: 65px;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    padding: 12px 17px;
    border-bottom: 1px solid var(--line-soft);
  }

  .invitation-row:last-child {
    border-bottom: 0;
  }

  .invitation-identity {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 10px;
  }

  .invitation-mark {
    display: grid;
    width: 34px;
    height: 34px;
    flex: 0 0 auto;
    place-items: center;
    border-radius: 8px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 11px;
    font-weight: 750;
  }

  .invitation-identity > div {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .invitation-identity strong {
    overflow-wrap: anywhere;
    color: var(--text);
    font-size: 12px;
    font-weight: 600;
  }

  .invitation-identity div > span {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .invitation-actions {
    display: flex;
    flex: 0 0 auto;
    gap: 7px;
  }

  /* =====================================================
     WORKSPACE FORM
     ===================================================== */

  .workspace-form-panel {
    display: grid;
    grid-template-columns: minmax(230px, 0.72fr) minmax(0, 1.28fr);
    gap: 26px;
    padding: 22px;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    box-shadow:
      inset 3px 0 0 var(--brand),
      var(--shadow-sm);
  }

  .workspace-form-panel.first-workspace-form {
    padding: clamp(22px, 4vw, 34px);
  }

  .workspace-form-intro {
    display: flex;
    align-items: flex-start;
    gap: 12px;
  }

  .workspace-form-icon {
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

  .workspace-form-icon svg {
    width: 20px;
    height: 20px;
  }

  .workspace-form-intro > div {
    display: grid;
    gap: 5px;
  }

  .workspace-form-intro h2 {
    margin: 0;
    color: var(--text);
    font-size: 18px;
    font-weight: 620;
    letter-spacing: -0.03em;
  }

  .workspace-form-intro p:not(.panel-kicker) {
    max-width: 330px;
    margin: 0;
    color: var(--text-muted);
    font-size: 11px;
    line-height: 1.55;
  }

  .workspace-form {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 13px;
  }

  .workspace-form label {
    display: grid;
    gap: 7px;
    color: var(--text-soft);
    font-size: 10.5px;
    font-weight: 550;
  }

  .workspace-form input {
    width: 100%;
    min-height: 40px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    outline: 0;
    background: var(--bg-ink-soft);
    color: var(--text);
    padding: 0 11px;
    font: inherit;
    font-size: 12px;
    transition:
      border-color var(--speed) var(--ease),
      box-shadow var(--speed) var(--ease);
  }

  .workspace-form input:focus {
    border-color: var(--brand);
    box-shadow: 0 0 0 3px var(--brand-faint);
  }

  .workspace-form input::placeholder {
    color: var(--text-faint);
  }

  .workspace-slug-preview {
    grid-column: 1 / -1;
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 9px;
    min-width: 0;
    padding: 9px 10px;
    border: 1px solid var(--line-soft);
    border-radius: 6px;
    background: var(--bg-ink-soft);
  }

  .workspace-slug-preview > span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 7px;
    text-transform: uppercase;
  }

  .workspace-slug-preview code {
    min-width: 0;
    color: var(--brand-bright);
    font-family: var(--font-mono);
    font-size: 10.5px;
    overflow-wrap: anywhere;
  }

  .workspace-form-actions {
    grid-column: 1 / -1;
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    padding-top: 2px;
  }

  /* =====================================================
     LOADING
     ===================================================== */

  .dashboard-loading {
    position: relative;
    display: flex;
    min-height: 110px;
    align-items: center;
    gap: 13px;
    overflow: hidden;
    padding: 20px;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .dashboard-loading__mark {
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

  .dashboard-loading > div {
    display: grid;
    gap: 4px;
  }

  .dashboard-loading strong {
    color: var(--text);
    font-size: 12.5px;
  }

  .dashboard-loading p {
    margin: 0;
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .dashboard-loading__progress {
    position: absolute;
    right: 0;
    bottom: 0;
    left: 0;
    height: 2px;
    background: var(--brand);
    transform-origin: left;
    animation: dashboard-loading 1.4s ease-in-out infinite;
  }

  @keyframes dashboard-loading {
    0% {
      transform: scaleX(0);
    }

    60% {
      transform: scaleX(1);
    }

    100% {
      transform: scaleX(1);
      opacity: 0;
    }
  }

  /* =====================================================
     NEW USER ONBOARDING
     ===================================================== */

  .onboarding {
    position: relative;
    display: grid;
    grid-template-columns: minmax(0, 0.82fr) minmax(400px, 1.18fr);
    gap: clamp(42px, 7vw, 92px);
    align-items: center;
    overflow: hidden;
    min-height: 430px;
    padding: clamp(32px, 6vw, 64px);
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-lg);
    background: var(--bg-panel);
    box-shadow: var(--shadow-panel);
  }

  .onboarding::before {
    position: absolute;
    top: -120px;
    left: -100px;
    width: 320px;
    height: 320px;
    border-radius: 50%;
    background: var(--brand-faint);
    content: '';
    filter: blur(65px);
    pointer-events: none;
  }

  .onboarding-copy {
    position: relative;
    z-index: 1;
  }

  .onboarding-copy h2 {
    max-width: 520px;
    margin: 10px 0 0;
    color: var(--text);
    font-size: clamp(31px, 4.7vw, 52px);
    font-weight: 640;
    line-height: 1;
    letter-spacing: -0.052em;
  }

  .onboarding-copy h2 span {
    display: block;
    color: var(--brand-soft);
  }

  .onboarding-copy > p:not(.panel-kicker) {
    max-width: 520px;
    margin: 17px 0 0;
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.68;
  }

  .onboarding-action {
    margin-top: 24px;
  }

  .onboarding-action svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2.2;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .onboarding-path {
    position: relative;
    z-index: 1;
    display: grid;
    gap: 10px;
  }

  .onboarding-path article {
    position: relative;
    z-index: 2;
    display: grid;
    grid-template-columns: 28px 42px minmax(0, 1fr);
    align-items: center;
    gap: 11px;
    min-height: 82px;
    padding: 13px;
    border: 1px solid var(--line);
    border-radius: 9px;
    background: var(--bg-ink-soft);
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease),
      transform var(--speed) var(--ease);
  }

  .onboarding-path article.active {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    transform: translateX(4px);
  }

  .onboarding-step-number {
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 8px;
    font-weight: 650;
  }

  .onboarding-step-icon {
    display: grid;
    width: 38px;
    height: 38px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 9px;
    background: var(--bg-panel);
    color: var(--brand-soft);
  }

  .onboarding-step-icon svg {
    width: 18px;
    height: 18px;
  }

  .onboarding-path article > div {
    display: grid;
    gap: 4px;
  }

  .onboarding-path strong {
    color: var(--text);
    font-size: 11.5px;
    font-weight: 600;
  }

  .onboarding-path p {
    margin: 0;
    color: var(--text-muted);
    font-size: 9.5px;
    line-height: 1.5;
  }

  .onboarding-path code {
    color: var(--brand-bright);
    font-family: var(--font-mono);
    font-size: 9px;
  }

  .onboarding-path-line {
    position: absolute;
    z-index: 1;
    top: 42px;
    bottom: 42px;
    left: 61px;
    width: 1px;
    background: var(--brand-line);
  }

  /* =====================================================
     WORKSPACE OVERVIEW
     ===================================================== */

  .workspace-overview {
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    box-shadow: var(--shadow-sm);
  }

  .workspace-overview__main {
    display: flex;
    min-height: 98px;
    align-items: center;
    justify-content: space-between;
    gap: 22px;
    padding: 19px 20px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .workspace-overview__identity {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 13px;
  }

  .workspace-overview__identity > div {
    display: grid;
    min-width: 0;
    gap: 5px;
  }

  .workspace-title-line {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 9px;
  }

  .workspace-title-line h2 {
    margin: 0;
    color: var(--text);
    font-size: 21px;
    font-weight: 620;
    letter-spacing: -0.035em;
    overflow-wrap: anywhere;
  }

  .workspace-status {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    padding: 4px 7px;
    border: 1px solid var(--line);
    border-radius: 999px;
    color: var(--text-muted);
    font-size: 6.5px;
    font-weight: 650;
    text-transform: uppercase;
  }

  .workspace-status i {
    width: 4px;
    height: 4px;
    border-radius: 50%;
    background: currentColor;
  }

  .workspace-status.active {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .workspace-overview__identity code {
    width: fit-content;
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 9px;
  }

  .workspace-switch {
    display: inline-flex;
    flex: 0 0 auto;
    align-items: center;
    gap: 6px;
    color: var(--text-muted);
    font-size: 10px;
    text-decoration: none;
  }

  .workspace-switch:hover {
    color: var(--brand-soft);
  }

  .workspace-switch svg {
    width: 13px;
    height: 13px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.9;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .workspace-metrics {
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 1fr));
  }

  .workspace-metrics article {
    display: grid;
    gap: 4px;
    padding: 15px 20px;
    border-right: 1px solid var(--line-soft);
  }

  .workspace-metrics article:last-child {
    border-right: 0;
  }

  .workspace-metrics span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6.5px;
    letter-spacing: 0.06em;
    text-transform: uppercase;
  }

  .workspace-metrics strong {
    color: var(--text);
    font-size: 17px;
    font-weight: 620;
    line-height: 1.2;
  }

  .workspace-metrics small {
    color: var(--text-faint);
    font-size: 8.5px;
  }

  .workspace-navigation {
    display: flex;
    flex-wrap: wrap;
    gap: 7px;
    padding: 12px 16px;
    border-top: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .workspace-navigation a {
    display: inline-grid;
    min-height: 34px;
    grid-template-columns: auto auto auto;
    align-items: center;
    gap: 7px;
    padding: 0 10px;
    border: 1px solid var(--line);
    border-radius: 7px;
    background: var(--bg-ink-soft);
    color: var(--text-soft);
    font-size: 9.5px;
    font-weight: 550;
    text-decoration: none;
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease),
      color var(--speed) var(--ease),
      transform var(--speed) var(--ease);
  }

  .workspace-navigation a:hover {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-bright);
    transform: translateY(-1px);
  }

  .workspace-navigation__icon {
    display: grid;
    width: 21px;
    height: 21px;
    place-items: center;
    border-radius: 5px;
    background: var(--bg-panel);
  }

  .workspace-navigation__icon svg {
    width: 12px;
    height: 12px;
  }

  .workspace-navigation__arrow {
    width: 11px;
    height: 11px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
    opacity: 0.45;
  }

  /* =====================================================
     DASHBOARD CONTENT
     ===================================================== */

  .dashboard-content {
    display: grid;
    grid-template-columns: minmax(0, 1.35fr) minmax(300px, 0.65fr);
    gap: 16px;
    align-items: start;
  }

  .dashboard-panel {
    overflow: hidden;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  /* =====================================================
     CONNECT PROJECT
     ===================================================== */

  .connect-project {
    display: grid;
    grid-template-columns: auto minmax(0, 1fr);
    gap: 12px;
    padding: 24px;
  }

  .connect-project__icon {
    display: grid;
    width: 43px;
    height: 43px;
    grid-row: 1 / 3;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 10px;
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .connect-project__icon svg {
    width: 20px;
    height: 20px;
  }

  .connect-project > div {
    display: grid;
    gap: 5px;
  }

  .connect-project h3 {
    margin: 0;
    color: var(--text);
    font-size: 16px;
    font-weight: 610;
    letter-spacing: -0.025em;
  }

  .connect-project div > p:not(.panel-kicker) {
    max-width: 520px;
    margin: 0;
    color: var(--text-muted);
    font-size: 10.5px;
    line-height: 1.55;
  }

  .connect-project > code {
    grid-column: 2;
    display: flex;
    min-height: 42px;
    align-items: center;
    gap: 9px;
    margin-top: 7px;
    padding: 0 12px;
    border: 1px solid var(--line-ink);
    border-radius: 7px;
    background: var(--bg-ink);
    color: var(--text-soft);
    font-family: var(--font-mono);
    font-size: 10.5px;
  }

  .connect-project > code span {
    color: var(--brand);
    font-weight: 800;
  }

  .connect-project > a {
    grid-column: 2;
    display: inline-flex;
    width: fit-content;
    align-items: center;
    gap: 6px;
    margin-top: 4px;
    color: var(--brand-soft);
    font-size: 10.5px;
    font-weight: 600;
    text-decoration: none;
  }

  .connect-project > a:hover {
    color: var(--brand-bright);
  }

  .connect-project > a svg {
    width: 12px;
    height: 12px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  /* =====================================================
     PROJECT LIST
     ===================================================== */

  .project-list {
    display: grid;
  }

  .project-row {
    display: grid;
    min-height: 62px;
    grid-template-columns: auto minmax(0, 1fr) auto;
    align-items: center;
    gap: 10px;
    padding: 10px 16px;
    border-bottom: 1px solid var(--line-soft);
    color: inherit;
    text-decoration: none;
    transition: background var(--speed) var(--ease);
  }

  .project-row:last-child {
    border-bottom: 0;
  }

  .project-row:hover {
    background: var(--bg-elevated);
  }

  .project-row__mark {
    display: grid;
    width: 34px;
    height: 34px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 8px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 10px;
    font-weight: 750;
  }

  .project-row > div {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .project-row strong {
    color: var(--text);
    font-size: 11.5px;
    font-weight: 580;
    overflow-wrap: anywhere;
  }

  .project-row div > span {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 6px;
    color: var(--text-muted);
    font-size: 9px;
  }

  .project-row code {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 8.5px;
  }

  .project-row div > span i {
    width: 3px;
    height: 3px;
    border-radius: 50%;
    background: var(--text-faint);
  }

  .project-row > svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: var(--text-faint);
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
    transition:
      stroke var(--speed) var(--ease),
      transform var(--speed) var(--ease);
  }

  .project-row:hover > svg {
    stroke: var(--brand);
    transform: translateX(2px);
  }

  /* =====================================================
     ACTIVITY
     ===================================================== */

  .activity-list {
    display: grid;
  }

  .activity-row {
    display: grid;
    min-height: 63px;
    grid-template-columns: 7px minmax(0, 1fr);
    gap: 9px;
    align-items: start;
    padding: 12px 16px;
    border-bottom: 1px solid var(--line-soft);
    color: inherit;
    text-decoration: none;
    transition: background var(--speed) var(--ease);
  }

  .activity-row:last-child {
    border-bottom: 0;
  }

  .activity-row:hover {
    background: var(--bg-elevated);
  }

  .activity-indicator {
    width: 6px;
    height: 6px;
    margin-top: 5px;
    border: 1px solid var(--line-strong);
    border-radius: 50%;
  }

  .activity-indicator.unread {
    border-color: var(--brand);
    background: var(--brand);
    box-shadow: 0 0 0 3px var(--brand-faint);
  }

  .activity-row > div {
    display: grid;
    min-width: 0;
    gap: 4px;
  }

  .activity-row strong {
    color: var(--text-soft);
    font-size: 10.5px;
    font-weight: 600;
    overflow-wrap: anywhere;
  }

  .activity-row p {
    display: -webkit-box;
    margin: 0;
    overflow: hidden;
    color: var(--text-muted);
    font-size: 9.5px;
    line-height: 1.45;
    -webkit-box-orient: vertical;
    -webkit-line-clamp: 2;
    line-clamp: 2;
  }

  .activity-empty,
  .compact-state {
    display: grid;
    min-height: 220px;
    align-content: center;
    justify-items: center;
    padding: 25px;
    text-align: center;
  }

  .activity-empty > span {
    display: grid;
    width: 42px;
    height: 42px;
    place-items: center;
    border: 1px solid var(--line);
    border-radius: 10px;
    background: var(--bg-ink-soft);
    color: var(--text-muted);
  }

  .activity-empty svg {
    width: 19px;
    height: 19px;
  }

  .activity-empty strong {
    margin-top: 11px;
    color: var(--text-soft);
    font-size: 11.5px;
  }

  .activity-empty p {
    max-width: 260px;
    margin: 5px 0 0;
    color: var(--text-muted);
    font-size: 9.5px;
    line-height: 1.5;
  }

  .compact-state {
    min-height: 150px;
    color: var(--text-muted);
    font-size: 10.5px;
  }

  /* =====================================================
     REDUCED MOTION
     ===================================================== */

  @media (prefers-reduced-motion: reduce) {
    .dashboard-page,
    .dashboard-loading__progress {
      animation: none;
    }
  }

  /* =====================================================
     RESPONSIVE
     ===================================================== */

  @media (max-width: 940px) {
    .workspace-form-panel,
    .onboarding {
      grid-template-columns: 1fr;
    }

    .onboarding {
      min-height: auto;
    }

    .dashboard-content {
      grid-template-columns: 1fr;
    }

    .activity-empty {
      min-height: 170px;
    }
  }

  @media (max-width: 720px) {
    .dashboard-header {
      align-items: stretch;
      flex-direction: column;
    }

    .header-action {
      width: 100%;
    }

    .workspace-form {
      grid-template-columns: 1fr;
    }

    .workspace-slug-preview,
    .workspace-form-actions {
      grid-column: auto;
    }

    .workspace-form-actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .workspace-form-actions button {
      width: 100%;
    }

    .workspace-overview__main {
      align-items: flex-start;
      flex-direction: column;
    }

    .workspace-metrics {
      grid-template-columns: 1fr;
    }

    .workspace-metrics article {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .workspace-metrics article:last-child {
      border-bottom: 0;
    }
  }

  @media (max-width: 600px) {
    .invitation-row {
      align-items: stretch;
      flex-direction: column;
    }

    .invitation-actions {
      width: 100%;
    }

    .invitation-actions button {
      flex: 1 1 0;
    }

    .workspace-form-panel {
      padding: 17px;
    }

    .onboarding {
      padding: 26px 19px;
    }

    .onboarding-copy h2 {
      font-size: clamp(30px, 10vw, 42px);
    }

    .onboarding-path article {
      grid-template-columns: 24px 38px minmax(0, 1fr);
    }

    .onboarding-path-line {
      left: 54px;
    }

    .workspace-navigation {
      display: grid;
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }

    .workspace-navigation a {
      grid-template-columns: auto minmax(0, 1fr) auto;
    }

    .connect-project {
      grid-template-columns: 1fr;
    }

    .connect-project__icon {
      grid-row: auto;
    }

    .connect-project > code,
    .connect-project > a {
      grid-column: auto;
    }
  }

  @media (max-width: 430px) {
    .workspace-navigation {
      grid-template-columns: 1fr;
    }

    .invitation-actions {
      flex-direction: column-reverse;
    }

    .invitation-actions button {
      width: 100%;
    }
  }
</style>
