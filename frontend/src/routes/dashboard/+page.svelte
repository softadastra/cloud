<script lang="ts">
  import { usePageRefresh } from '$lib/stores/pageRefresh';
  import { goto } from '$app/navigation';
  import { createWorkspace, listWorkspaces } from '$lib/api/workspaces';
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
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import WorkspaceAvatar from '$lib/components/WorkspaceAvatar.svelte';
  import {
    canCreateProject,
    canManageMembers,
    canManageTokens
  } from '$lib/permissions';
  import { auth } from '$lib/stores/auth';
  import { notifications } from '$lib/stores/notifications';
  import { notificationHref } from '$lib/notifications/notificationLinks';
  import { workspaceContext } from '$lib/stores/workspace';

  type DashboardProject = {
    project: Project;
    workspace: Workspace;
  };

  type QuickLink = {
    label: string;
    href: string;
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

  $: selectedWorkspace =
    $workspaceContext.selectedWorkspace ??
    workspaces[0] ??
    null;

  $: currentRole =
    selectedWorkspace?.current_user_role ?? 'viewer';

  $: currentWorkspaceProjects = selectedWorkspace
    ? projectEntries.filter(
        (entry) =>
          entry.workspace.id === selectedWorkspace?.id
      )
    : [];

  $: recentProjects = currentWorkspaceProjects.slice(0, 6);

  $: recentNotifications =
    $notifications.notifications.slice(0, 6);

  $: unreadNotifications = $notifications.unreadCount;

  $: activeWorkspaceCount = workspaces.filter(
    (workspace) => workspace.active !== false
  ).length;

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

    const workspaceId = encodeURIComponent(workspace.id);

    const links: QuickLink[] = [
      {
        label: 'Open workspace',
        href: `/workspaces?workspace_id=${workspaceId}`
      },
      {
        label: 'Projects',
        href: `/projects?workspace_id=${workspaceId}`
      },
      {
        label: 'Packages',
        href: `/packages?workspace_id=${workspaceId}`
      }
    ];

    if (canManageMembers(role)) {
      links.push({
        label: 'Members',
        href: `/members?workspace_id=${workspaceId}`
      });
    }

    if (canManageTokens(role)) {
      links.push({
        label: 'CLI tokens',
        href: `/tokens?workspace_id=${workspaceId}`
      });
    }

    return links;
  }

  function resetWorkspaceForm() {
    workspaceName = '';
    workspaceSlug = '';
    slugTouched = false;
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
    showWorkspaceForm = !showWorkspaceForm;
    error = '';
    success = '';

    if (!showWorkspaceForm) {
      resetWorkspaceForm();
    }
  }

  function handleSlugInput(event: Event) {
    const input = event.currentTarget as HTMLInputElement;

    slugTouched = true;
    workspaceSlug = input.value;
  }

  function handleWorkspaceSubmit(event: SubmitEvent) {
    event.preventDefault();
    void submitWorkspace();
  }

  async function loadProjectsForWorkspaces(
    sourceWorkspaces: Workspace[]
  ) {
    const results = await Promise.all(
      sourceWorkspaces.map(async (workspace) => {
        try {
          const data = await listProjects(workspace.id);

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
      const workspaceData = await listWorkspaces(user.id);

      workspaces = workspaceData.workspaces;

      const requestedWorkspaceId =
        new URLSearchParams(window.location.search).get(
          'workspace_id'
        );

      const storedWorkspaceId =
        $workspaceContext.selectedWorkspace?.id;

      const resolvedWorkspaceId =
        (
          requestedWorkspaceId &&
          workspaces.some(
            (workspace) =>
              workspace.id === requestedWorkspaceId
          )
        )
          ? requestedWorkspaceId
          : (
              storedWorkspaceId &&
              workspaces.some(
                (workspace) =>
                  workspace.id === storedWorkspaceId
              )
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

      await goto(
        `/workspaces?workspace_id=${created.workspace.id}`
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

      success = `You joined ${
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
</svelte:head>

<header class="dashboard-header">
  <div class="dashboard-heading">
    <p class="eyebrow">Overview</p>
    <h1>Dashboard</h1>

    <p class="dashboard-context">
      {#if loading}
        Loading your workspaces…
      {:else if selectedWorkspace}
        Signed in as
        <strong>{roleLabel(currentRole)}</strong>
        in
        <strong>{selectedWorkspace.name}</strong>
      {:else}
        Create a workspace to get started.
      {/if}
    </p>
  </div>

  <button
    class:secondary-button={showWorkspaceForm}
    class="workspace-button"
    type="button"
    onclick={toggleWorkspaceForm}
  >
    {#if !showWorkspaceForm}
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path d="M12 5v14M5 12h14" />
      </svg>
    {/if}
    {showWorkspaceForm ? 'Close' : 'New workspace'}
  </button>
</header>

<InlineError message={error} />

{#if success}
  <p class="success-message" role="status">
    {success}
  </p>
{/if}

{#if showWorkspaceForm}
  <section
    class="workspace-form-section"
    aria-labelledby="new-workspace-title"
  >
    <div class="section-heading">
      <div>
        <h2 id="new-workspace-title">
          Create a workspace
        </h2>

        <p>
          A workspace groups projects, packages, members and
          CLI access.
        </p>
      </div>
    </div>

    <form
      class="workspace-form"
      onsubmit={handleWorkspaceSubmit}
    >
      <label>
        Workspace name

        <input
          bind:value={workspaceName}
          placeholder="Vix Runtime"
          required
          autocomplete="off"
        />
      </label>

      <label>
        Slug

        <input
          value={effectiveSlug}
          placeholder="vix-runtime"
          required
          autocomplete="off"
          oninput={handleSlugInput}
        />
      </label>

      <div class="slug-preview">
        <span class="eyebrow">Identifier</span>
        <code>{effectiveSlug || 'workspace-slug'}</code>
      </div>

      <div class="workspace-form-actions">
        <button
          class="secondary-button"
          type="button"
          disabled={saving}
          onclick={closeWorkspaceForm}
        >
          Cancel
        </button>

        <button
          type="submit"
          disabled={
            saving ||
            !workspaceName.trim() ||
            !effectiveSlug
          }
        >
          {saving ? 'Creating…' : 'Create workspace'}
        </button>
      </div>
    </form>
  </section>
{/if}

{#if pendingInvites.length > 0}
  <section
    class="dashboard-section invitations-section"
    aria-labelledby="invitations-title"
  >
    <div class="section-heading section-heading--bordered">
      <div>
        <h2 id="invitations-title">
          Workspace invitations
        </h2>

        <p>
          Invitations waiting for your response.
        </p>
      </div>

      <span class="section-count">
        {pendingInvites.length}
      </span>
    </div>

    <div class="invitation-list">
      {#each pendingInvites as invite (invite.id)}
        <div class="invitation-row">
          <div class="invitation-details">
            <strong>
              {invite.workspace_name || invite.workspace_id}
            </strong>

            <span>
              {roleLabel(invite.role)}
              ·
              {accessLabel(invite.access_scope)}
            </span>
          </div>

          <div class="invitation-actions">
            <button
              class="secondary-button compact-button"
              type="button"
              disabled={Boolean(inviteBusyId)}
              onclick={() => declineInvite(invite)}
            >
              {inviteBusyId === invite.id
                ? 'Working…'
                : 'Decline'}
            </button>

            <button
              class="compact-button"
              type="button"
              disabled={Boolean(inviteBusyId)}
              onclick={() => acceptInvite(invite)}
            >
              {inviteBusyId === invite.id
                ? 'Working…'
                : 'Accept'}
            </button>
          </div>
        </div>
      {/each}
    </div>
  </section>
{/if}

<section
  class="overview-strip"
  aria-label="Dashboard summary"
>
  <div class="overview-item">
    <span class="eyebrow">Workspaces</span>
    <strong>{workspaces.length}</strong>

    <small>
      {activeWorkspaceCount} active
    </small>
  </div>

  <div class="overview-item">
    <span class="eyebrow">Projects</span>
    <strong>{currentWorkspaceProjects.length}</strong>

    <small>
      {selectedWorkspace?.name ?? 'No workspace selected'}
    </small>
  </div>

  <div class="overview-item">
    <span class="eyebrow">Invitations</span>
    <strong>{pendingInvites.length}</strong>

    <small>Pending response</small>
  </div>

  <div class="overview-item">
    <span class="eyebrow">Unread</span>
    <strong>{unreadNotifications}</strong>

    <small>Notifications</small>
  </div>
</section>

<section
  class="dashboard-section current-workspace-section"
  aria-labelledby="current-workspace-title"
>
  <h2 class="visually-hidden" id="current-workspace-title">
    Current workspace
  </h2>

  {#if loading}
    <p class="loading-state">Loading workspace…</p>
  {:else if !selectedWorkspace}
    <div class="empty-wrapper">
      <EmptyState
        title="No workspace selected"
        body="Create a workspace to start organising your cloud resources."
      />
    </div>
  {:else}
    <div class="current-workspace">
      <div class="workspace-identity">
        <span class="eyebrow">Current workspace</span>

        <div class="workspace-identity__row">
          <WorkspaceAvatar workspace={selectedWorkspace} />

          <div>
            <div class="workspace-name-line">
              <strong>{selectedWorkspace.name}</strong>

              <span
                class:status-active={
                  selectedWorkspace.active !== false
                }
                class="badge"
              >
                {workspaceStatus(selectedWorkspace)}
              </span>
            </div>

            <code>{selectedWorkspace.slug}</code>
          </div>
        </div>
      </div>

      <dl class="workspace-details">
        <div>
          <dt class="eyebrow">Your role</dt>
          <dd>
            {roleLabel(
              selectedWorkspace.current_user_role
            )}
          </dd>
        </div>

        <div>
          <dt class="eyebrow">Access</dt>
          <dd>
            {accessLabel(selectedWorkspace.access_scope)}
          </dd>
        </div>

        <div>
          <dt class="eyebrow">Projects</dt>
          <dd>{currentWorkspaceProjects.length}</dd>
        </div>
      </dl>

      <nav
        class="workspace-links"
        aria-label="Current workspace shortcuts"
      >
        {#each quickLinks as link}
          <a href={link.href}>
            {link.label}

            <svg viewBox="0 0 24 24" aria-hidden="true">
              <path d="m9 18 6-6-6-6" />
            </svg>
          </a>
        {/each}
      </nav>
    </div>
  {/if}
</section>

<div class="dashboard-grid">
  <section
    class="dashboard-section projects-section"
    aria-labelledby="recent-projects-title"
  >
    <div class="section-heading section-heading--bordered">
      <div>
        <h2 id="recent-projects-title">
          Recent projects
        </h2>

        <p>
          Projects available across your workspaces.
        </p>
      </div>

      <a class="section-link" href="/projects">
        View all
      </a>
    </div>

    {#if loading}
      <p class="loading-state">Loading projects…</p>
    {:else if recentProjects.length === 0}
      <div class="empty-wrapper">
        <EmptyState
          title="No projects yet"
          body={canCreateProject(currentRole)
            ? 'Run vix cloud init to connect a local project.'
            : 'No projects are currently visible to your account.'}
        />
      </div>
    {:else}
      <div class="resource-list">
        {#each recentProjects as entry (entry.project.id)}
          <a
            class="resource-row"
            href={`/projects?workspace_id=${entry.workspace.id}&project_id=${entry.project.id}`}
          >
            <div>
              <strong>{entry.project.name}</strong>

              <span>
                <code>{entry.project.slug}</code>
                <span aria-hidden="true">·</span>
                {entry.workspace.name}
              </span>
            </div>

            <svg viewBox="0 0 24 24" aria-hidden="true">
              <path d="m9 18 6-6-6-6" />
            </svg>
          </a>
        {/each}
      </div>
    {/if}
  </section>

  <section
    class="dashboard-section activity-section"
    aria-labelledby="recent-activity-title"
  >
    <div class="section-heading section-heading--bordered">
      <div>
        <h2 id="recent-activity-title">
          Recent activity
        </h2>

        <p>Important workspace events.</p>
      </div>

      <a class="section-link" href="/notifications">
        View all
      </a>
    </div>

    {#if $notifications.loading}
      <p class="loading-state">Loading activity…</p>
    {:else if recentNotifications.length === 0}
      <div class="empty-wrapper">
        <EmptyState
          title="No activity yet"
          body="Important workspace events will appear here."
        />
      </div>
    {:else}
      <div class="activity-list">
        {#each recentNotifications as item (item.id)}
          <a class="activity-row" href={notificationHref(item)}>
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

<section
  class="dashboard-section workspaces-section"
  aria-labelledby="workspaces-title"
>
  <div class="section-heading section-heading--bordered">
    <div>
      <h2 id="workspaces-title">
        Your workspaces
      </h2>

      <p>
        Workspaces your account can currently access.
      </p>
    </div>

    <a class="section-link" href="/workspaces">
      Manage
    </a>
  </div>

  {#if loading}
    <p class="loading-state">Loading workspaces…</p>
  {:else if workspaces.length === 0}
    <div class="empty-wrapper">
      <EmptyState
        title="No workspaces yet"
        body="Create your first workspace to start organising C++ projects."
      />
    </div>
  {:else}
    <div class="workspace-list">
      {#each workspaces as workspace (workspace.id)}
        <a
          class:current={
            workspace.id === selectedWorkspace?.id
          }
          class="workspace-row"
          href={`/workspaces?workspace_id=${workspace.id}`}
        >
          <div class="workspace-row__identity">
            <WorkspaceAvatar {workspace} size="sm" />

            <div>
              <strong>{workspace.name}</strong>
              <code>{workspace.slug}</code>
            </div>
          </div>

          <div class="workspace-row__meta">
            <span>{roleLabel(workspace.current_user_role)}</span>

            <span
              class:meta-active={workspace.active !== false}
            >
              {workspaceStatus(workspace)}
            </span>
          </div>

          <svg viewBox="0 0 24 24" aria-hidden="true">
            <path d="m9 18 6-6-6-6" />
          </svg>
        </a>
      {/each}
    </div>
  {/if}
</section>

<style>
  /* ── Header ─────────────────────────────────────────── */

  .dashboard-header {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 20px;
    margin-bottom: 22px;
    padding-bottom: 18px;
    border-bottom: 1px solid var(--line);
  }

  .dashboard-heading {
    display: grid;
    gap: 6px;
  }

  .dashboard-context {
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.6;
  }

  .dashboard-context strong {
    color: var(--text-soft);
    font-weight: 550;
  }

  .workspace-button {
    flex: 0 0 auto;
  }

  .workspace-button svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
  }

  .workspace-button.secondary-button {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-soft);
  }

  .workspace-button.secondary-button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
  }

  .visually-hidden {
    position: absolute;
    width: 1px;
    height: 1px;
    overflow: hidden;
    clip: rect(0 0 0 0);
    white-space: nowrap;
  }

  /* ── Sections (shared shell) ────────────────────────── */

  .dashboard-section,
  .workspace-form-section {
    margin-bottom: 16px;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    overflow: hidden;
  }

  .section-heading {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 16px;
  }

  .section-heading--bordered {
    padding: 13px 16px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .section-heading > div {
    display: grid;
    gap: 3px;
  }

  .section-heading h2 {
    font-size: 13.5px;
  }

  .section-heading p {
    color: var(--text-muted);
    font-size: 11.5px;
    line-height: 1.5;
  }

  .section-count {
    display: inline-grid;
    min-width: 24px;
    height: 22px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 999px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    padding: 0 7px;
    font-family: var(--font-mono);
    font-size: 10.5px;
    font-weight: 600;
  }

  .section-link {
    flex: 0 0 auto;
    color: var(--link);
    font-size: 11.5px;
    font-weight: 600;
    transition: color var(--speed) var(--ease);
  }

  .section-link:hover {
    color: var(--link-hover);
    text-decoration: underline;
    text-underline-offset: 3px;
  }

  /* ── Workspace creation ─────────────────────────────── */

  .workspace-form-section {
    display: grid;
    gap: 17px;
    border-color: var(--brand-line);
    padding: 18px;
    background: var(--bg-panel);
    box-shadow: inset 3px 0 0 var(--brand);
  }

  .workspace-form {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 14px;
  }

  .slug-preview {
    grid-column: 1 / -1;
    display: flex;
    align-items: center;
    gap: 10px;
    min-width: 0;
  }

  .slug-preview code {
    min-width: 0;
    padding: 3px 8px;
    border: 1px solid var(--line-ink);
    border-radius: 4px;
    background: var(--bg-ink);
    color: var(--brand-bright);
    font-size: 11.5px;
    overflow-wrap: anywhere;
  }

  .workspace-form-actions {
    grid-column: 1 / -1;
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    border-top: 1px solid var(--line-soft);
    padding-top: 14px;
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

  .compact-button {
    min-height: 30px;
    padding: 0 11px;
    font-size: 12px;
  }

  /* ── Invitations — the one urgent block ─────────────── */

  .invitations-section {
    border-color: var(--brand-line);
    box-shadow: inset 3px 0 0 var(--brand);
  }

  .invitation-list {
    display: grid;
  }

  .invitation-row {
    display: flex;
    min-height: 58px;
    align-items: center;
    justify-content: space-between;
    gap: 16px;
    padding: 11px 16px;
    border-bottom: 1px solid var(--line-soft);
  }

  .invitation-row:last-child {
    border-bottom: 0;
  }

  .invitation-details {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .invitation-details strong {
    color: var(--text);
    font-size: 12.5px;
    font-weight: 600;
    overflow-wrap: anywhere;
  }

  .invitation-details span {
    color: var(--text-muted);
    font-size: 11px;
  }

  .invitation-actions {
    display: flex;
    flex: 0 0 auto;
    gap: 7px;
  }

  /* ── Overview strip — the metrics rail ──────────────── */

  .overview-strip {
    display: grid;
    grid-template-columns: repeat(4, minmax(0, 1fr));
    margin-bottom: 16px;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    overflow: hidden;
  }

  .overview-item {
    position: relative;
    display: grid;
    min-width: 0;
    gap: 4px;
    padding: 15px 16px 13px;
    border-right: 1px solid var(--line-soft);
    transition: background var(--speed) var(--ease);
  }

  .overview-item::before {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    height: 2px;
    background: var(--brand);
    transform: scaleX(0);
    transform-origin: left;
    transition: transform var(--speed) var(--ease);
  }

  .overview-item:hover {
    background: var(--bg-panel-strong);
  }

  .overview-item:hover::before {
    transform: scaleX(1);
  }

  .overview-item:last-child {
    border-right: 0;
  }

  .overview-item > strong {
    color: var(--text);
    font-family: var(--font-mono);
    font-size: 22px;
    font-weight: 600;
    font-variant-numeric: tabular-nums;
    line-height: 1.1;
  }

  .overview-item > small {
    min-width: 0;
    color: var(--text-faint);
    font-size: 10.5px;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  /* ── Current workspace — command panel ──────────────── */

  .current-workspace-section {
    border-color: var(--line-strong);
  }

  .current-workspace {
    display: grid;
    grid-template-columns:
      minmax(240px, 1.1fr)
      minmax(0, 1.4fr);
    align-items: stretch;
  }

  .workspace-identity {
    display: grid;
    align-content: center;
    gap: 12px;
    padding: 18px 20px;
    border-right: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .workspace-identity__row {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 12px;
  }

  .workspace-identity__row > div:last-child {
    display: grid;
    min-width: 0;
    gap: 4px;
  }

  .workspace-name-line {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 9px;
    flex-wrap: wrap;
  }

  .workspace-name-line strong {
    color: var(--text);
    font-size: 15px;
    font-weight: 600;
    letter-spacing: -0.01em;
    overflow-wrap: anywhere;
  }

  .workspace-identity code {
    width: fit-content;
    padding: 2px 7px;
    border: 1px solid var(--line-ink);
    border-radius: 4px;
    background: var(--bg-ink);
    color: var(--text-muted);
    font-size: 10.5px;
    overflow-wrap: anywhere;
  }

  .workspace-details {
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 1fr));
    gap: 14px;
    align-content: center;
    margin: 0;
    padding: 18px 20px;
  }

  .workspace-details div {
    display: grid;
    min-width: 0;
    gap: 5px;
  }

  .workspace-details dd {
    margin: 0;
    color: var(--text);
    font-size: 12.5px;
    font-weight: 550;
    overflow-wrap: anywhere;
  }

  .workspace-links {
    grid-column: 1 / -1;
    display: flex;
    flex-wrap: wrap;
    gap: 7px;
    padding: 12px 20px;
    border-top: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .workspace-links a {
    display: inline-flex;
    min-height: 28px;
    align-items: center;
    gap: 6px;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    background: var(--bg-elevated);
    color: var(--text-soft);
    padding: 0 11px;
    font-size: 11.5px;
    font-weight: 550;
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease),
      color var(--speed) var(--ease);
  }

  .workspace-links a:hover {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-bright);
  }

  .workspace-links svg {
    width: 13px;
    height: 13px;
    flex: 0 0 auto;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
    opacity: 0.7;
  }

  /* ── Main grid ──────────────────────────────────────── */

  .dashboard-grid {
    display: grid;
    grid-template-columns: minmax(0, 1.45fr) minmax(280px, 0.85fr);
    gap: 16px;
    align-items: start;
  }

  .resource-list,
  .activity-list,
  .workspace-list {
    display: grid;
  }

  .resource-row,
  .activity-row,
  .workspace-row {
    border-bottom: 1px solid var(--line-soft);
    transition: background var(--speed) var(--ease);
  }

  .resource-row:last-child,
  .activity-row:last-child,
  .workspace-row:last-child {
    border-bottom: 0;
  }

  .resource-row:hover,
  .activity-row:hover,
  .workspace-row:hover {
    background: var(--bg-elevated);
  }

  .resource-row > svg,
  .workspace-row > svg {
    width: 15px;
    height: 15px;
    flex: 0 0 auto;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
    color: var(--text-faint);
    transition:
      color var(--speed) var(--ease),
      transform var(--speed) var(--ease);
  }

  .resource-row:hover > svg,
  .workspace-row:hover > svg {
    color: var(--brand);
    transform: translateX(2px);
  }

  .resource-row {
    display: flex;
    min-height: 58px;
    align-items: center;
    justify-content: space-between;
    gap: 14px;
    padding: 10px 16px;
    color: var(--text-muted);
  }

  .resource-row > div {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .resource-row strong {
    color: var(--text);
    font-size: 12.5px;
    font-weight: 550;
    overflow-wrap: anywhere;
    transition: color var(--speed) var(--ease);
  }

  .resource-row:hover strong {
    color: var(--brand-bright);
  }

  .resource-row span {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 6px;
    color: var(--text-muted);
    font-size: 10.5px;
    flex-wrap: wrap;
  }

  .resource-row code {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  /* ── Activity ───────────────────────────────────────── */

  .activity-row {
    display: grid;
    grid-template-columns: 7px minmax(0, 1fr);
    gap: 10px;
    min-height: 58px;
    align-items: start;
    padding: 11px 16px;
  }

  .activity-indicator {
    width: 6px;
    height: 6px;
    margin-top: 6px;
    border: 1px solid var(--line-strong);
    border-radius: 50%;
    background: transparent;
  }

  .activity-indicator.unread {
    border-color: var(--brand);
    background: var(--brand);
    box-shadow: 0 0 0 3px var(--brand-faint);
  }

  .activity-row > div {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .activity-row strong {
    color: var(--text-soft);
    font-size: 11.5px;
    font-weight: 600;
    overflow-wrap: anywhere;
    transition: color var(--speed) var(--ease);
  }

  .activity-row:hover strong {
    color: var(--text);
  }

  .activity-row p {
    display: -webkit-box;
    color: var(--text-muted);
    font-size: 10.5px;
    line-height: 1.45;
    overflow: hidden;
    line-clamp: 2;
    -webkit-line-clamp: 2;
    -webkit-box-orient: vertical;
  }

  /* ── Workspace list ─────────────────────────────────── */

  .workspace-row {
    display: grid;
    grid-template-columns:
      minmax(230px, 1fr)
      minmax(180px, auto)
      18px;
    gap: 18px;
    align-items: center;
    min-height: 60px;
    padding: 10px 16px;
    color: var(--text-muted);
  }

  .workspace-row.current {
    background: var(--brand-faint);
    box-shadow: inset 2px 0 0 var(--brand);
  }

  .workspace-row.current:hover {
    background: var(--brand-glow);
  }

  .workspace-row__identity {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 10px;
  }

  .workspace-row__identity > div {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .workspace-row__identity strong {
    color: var(--text);
    font-size: 12.5px;
    font-weight: 550;
    overflow-wrap: anywhere;
    transition: color var(--speed) var(--ease);
  }

  .workspace-row:hover .workspace-row__identity strong {
    color: var(--brand-bright);
  }

  .workspace-row__identity code {
    color: var(--text-muted);
    font-size: 10.5px;
    overflow-wrap: anywhere;
  }

  .workspace-row__meta {
    display: flex;
    justify-content: flex-end;
    gap: 18px;
    color: var(--text-muted);
    font-size: 11px;
  }

  .workspace-row__meta .meta-active {
    color: var(--green-soft);
  }

  /* ── States ─────────────────────────────────────────── */

  .loading-state {
    min-height: 76px;
    padding: 20px 16px;
    color: var(--text-muted);
    font-size: 12px;
  }

  .empty-wrapper {
    padding: 14px;
  }

  /* ── Responsive ─────────────────────────────────────── */

  @media (max-width: 980px) {
    .overview-strip {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }

    .overview-item:nth-child(2) {
      border-right: 0;
    }

    .overview-item:nth-child(-n + 2) {
      border-bottom: 1px solid var(--line-soft);
    }

    .current-workspace {
      grid-template-columns: 1fr;
    }

    .workspace-identity {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }
  }

  @media (max-width: 780px) {
    .dashboard-grid {
      grid-template-columns: 1fr;
    }

    .workspace-row {
      grid-template-columns: minmax(0, 1fr) auto;
    }

    .workspace-row > svg {
      display: none;
    }
  }

  @media (max-width: 620px) {
    .dashboard-header {
      align-items: stretch;
      flex-direction: column;
    }

    .workspace-button {
      width: 100%;
    }

    .workspace-form {
      grid-template-columns: 1fr;
    }

    .slug-preview,
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

    .workspace-details {
      grid-template-columns: 1fr;
      gap: 12px;
    }

    .workspace-links {
      padding: 12px 16px;
    }

    .workspace-row {
      grid-template-columns: 1fr;
      gap: 8px;
    }

    .workspace-row__meta {
      justify-content: flex-start;
      padding-left: 40px;
    }
  }

  @media (max-width: 440px) {
    .overview-strip {
      grid-template-columns: 1fr;
    }

    .overview-item {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .overview-item:last-child {
      border-bottom: 0;
    }
  }
</style>
