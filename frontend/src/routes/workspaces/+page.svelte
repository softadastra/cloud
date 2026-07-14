<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';

  import { usePageRefresh } from '$lib/stores/pageRefresh';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  import { listMembers } from '$lib/api/members';
  import { listProjects } from '$lib/api/projects';
  import { listTokens } from '$lib/api/tokens';

  import {
    createWorkspace,
    deleteWorkspace,
    deleteWorkspaceAvatar,
    listWorkspaces,
    reactivateWorkspace,
    suspendWorkspace,
    updateWorkspace,
    uploadWorkspaceAvatar
  } from '$lib/api/workspaces';

  import {
    ApiError,
    type CliToken,
    type Member,
    type Project,
    type Workspace
  } from '$lib/api/types';

  import {
    canCreateProject,
    canManageMembers,
    canManageTokens
  } from '$lib/permissions';

  import InlineError from '$lib/components/InlineError.svelte';
  import WorkspaceAvatar from '$lib/components/WorkspaceAvatar.svelte';

  type WorkspaceView = 'overview' | 'settings';

  type DangerAction =
    | 'suspend'
    | 'reactivate'
    | 'delete';

  let workspaces: Workspace[] = [];
  let selectedWorkspace: Workspace | null = null;

  let projects: Project[] = [];
  let members: Member[] = [];
  let tokens: CliToken[] = [];

  let activeView: WorkspaceView = 'overview';

  let workspaceQuery = '';

  let loading = true;
  let loadingDetail = false;
  let creatingWorkspace = false;
  let savingSettings = false;
  let uploadingAvatar = false;

  let showCreateForm = false;

  let copiedWorkspaceId = false;
  let copyResetTimer:
    | ReturnType<typeof setTimeout>
    | undefined;

  let avatarPreviewUrl = '';

  let error = '';
  let success = '';

  let newWorkspaceName = '';
  let newWorkspaceSlug = '';
  let newSlugTouched = false;

  let editName = '';
  let editSlug = '';

  let pendingDangerAction:
    | DangerAction
    | null = null;

  let dangerConfirmation = '';

  $: currentRole =
    selectedWorkspace?.current_user_role ??
    'viewer';

  $: canProjects =
    canCreateProject(currentRole);

  $: canMembers =
    canManageMembers(currentRole);

  $: canTokens =
    canManageTokens(currentRole);

  $: canManage =
    currentRole === 'owner' ||
    currentRole === 'admin';

  $: canOwn =
    currentRole === 'owner';

  $: visibleWorkspaces = workspaces.filter(
    (workspace) => {
      const query =
        workspaceQuery.trim().toLowerCase();

      if (!query) {
        return true;
      }

      return (
        workspace.name
          .toLowerCase()
          .includes(query) ||
        workspace.slug
          .toLowerCase()
          .includes(query)
      );
    }
  );

  $: activeMembers = members.filter(
    (member) => member.status === 'active'
  ).length;

  $: activeTokens = tokens.filter(
    (token) => !token.revoked
  ).length;

  $: effectiveNewSlug = newSlugTouched
    ? slugify(newWorkspaceSlug)
    : slugify(newWorkspaceName);

  $: workspaceSettingsChanged =
    Boolean(selectedWorkspace) &&
    (
      editName.trim() !==
        selectedWorkspace?.name ||
      editSlug.trim() !==
        selectedWorkspace?.slug
    );

  $: dangerNeedsSlug =
    pendingDangerAction === 'delete';

  $: dangerCanContinue =
    Boolean(pendingDangerAction) &&
    (
      !dangerNeedsSlug ||
      dangerConfirmation ===
        selectedWorkspace?.slug
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

    return (
      value.charAt(0).toUpperCase() +
      value.slice(1)
    );
  }

  function accessLabel(value?: string) {
    return value === 'selected_projects'
      ? 'Selected projects'
      : 'Entire workspace';
  }

  function workspaceStatus(
    workspace: Workspace
  ) {
    return (
      workspace.status ||
      (
        workspace.active === false
          ? 'inactive'
          : 'active'
      )
    );
  }

  function resetCreateForm() {
    newWorkspaceName = '';
    newWorkspaceSlug = '';
    newSlugTouched = false;
  }

  function openCreateForm() {
    showCreateForm = true;
    error = '';
    success = '';
  }

  function closeCreateForm() {
    if (
      creatingWorkspace ||
      workspaces.length === 0
    ) {
      return;
    }

    showCreateForm = false;
    resetCreateForm();
    error = '';
  }

  function handleNewSlugInput(
    event: Event
  ) {
    const input =
      event.currentTarget as HTMLInputElement;

    newSlugTouched = true;
    newWorkspaceSlug = input.value;
  }

  function clearAvatarPreview() {
    if (!avatarPreviewUrl) {
      return;
    }

    URL.revokeObjectURL(
      avatarPreviewUrl
    );

    avatarPreviewUrl = '';
  }

  function updateWorkspaceUrl(
    workspaceId?: string
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

  function syncWorkspace(
    updated: Workspace | null
  ) {
    if (!updated) {
      return;
    }

    workspaces = workspaces.map(
      (workspace) =>
        workspace.id === updated.id
          ? {
              ...workspace,
              ...updated
            }
          : workspace
    );

    if (
      selectedWorkspace?.id ===
      updated.id
    ) {
      selectedWorkspace = {
        ...selectedWorkspace,
        ...updated
      };

      workspaceContext.updateWorkspace(
        selectedWorkspace
      );
    }
  }

  async function copyWorkspaceId() {
    if (!selectedWorkspace) {
      return;
    }

    try {
      await navigator.clipboard.writeText(
        selectedWorkspace.id
      );

      copiedWorkspaceId = true;

      if (copyResetTimer) {
        clearTimeout(copyResetTimer);
      }

      copyResetTimer = setTimeout(() => {
        copiedWorkspaceId = false;
      }, 1800);
    } catch {
      error =
        'Unable to copy the workspace ID.';
    }
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

      workspaces =
        workspaceData.workspaces;

      const requestedWorkspaceId =
        new URLSearchParams(
          window.location.search
        ).get('workspace_id');

      const storedWorkspaceId =
        $workspaceContext
          .selectedWorkspace?.id;

      selectedWorkspace =
        (
          requestedWorkspaceId
            ? workspaces.find(
                (workspace) =>
                  workspace.id ===
                  requestedWorkspaceId
              )
            : null
        ) ??
        (
          storedWorkspaceId
            ? workspaces.find(
                (workspace) =>
                  workspace.id ===
                  storedWorkspaceId
              )
            : null
        ) ??
        workspaces[0] ??
        null;

      workspaceContext.setWorkspaces(
        workspaces,
        selectedWorkspace?.id ?? ''
      );

      showCreateForm =
        workspaces.length === 0;

      if (selectedWorkspace) {
        editName =
          selectedWorkspace.name;

        editSlug =
          selectedWorkspace.slug;

        updateWorkspaceUrl(
          selectedWorkspace.id
        );
      }

      await loadDetail(
        selectedWorkspace
      );
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load workspaces.';
    } finally {
      loading = false;
    }
  }

  async function loadDetail(
    workspace: Workspace | null
  ) {
    const user = $auth.user;

    if (!workspace || !user) {
      projects = [];
      members = [];
      tokens = [];
      return;
    }

    const workspaceId = workspace.id;
    const role =
      workspace.current_user_role ??
      'viewer';

    loadingDetail = true;
    error = '';

    projects = [];
    members = [];
    tokens = [];

    try {
      const tokenRequest =
        canManageTokens(role)
          ? listTokens(
              workspace.id,
              user.id
            )
          : Promise.resolve({
              tokens: [] as CliToken[]
            });

      const [
        projectData,
        memberData,
        tokenData
      ] = await Promise.all([
        listProjects(workspace.id),
        listMembers(workspace.id),
        tokenRequest
      ]);

      if (
        selectedWorkspace?.id !==
        workspaceId
      ) {
        return;
      }

      projects = projectData.projects;
      members = memberData.members;
      tokens = tokenData.tokens;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load workspace details.';
    } finally {
      loadingDetail = false;
    }
  }

  async function selectWorkspace(
    workspace: Workspace
  ) {
    if (
      workspace.id ===
        selectedWorkspace?.id ||
      loadingDetail
    ) {
      return;
    }

    clearAvatarPreview();

    selectedWorkspace = workspace;

    editName = workspace.name;
    editSlug = workspace.slug;

    activeView = 'overview';

    copiedWorkspaceId = false;
    pendingDangerAction = null;
    dangerConfirmation = '';

    error = '';
    success = '';

    workspaceContext.setSelectedWorkspace(
      workspace.id
    );

    updateWorkspaceUrl(
      workspace.id
    );

    await loadDetail(workspace);
  }

  async function createNewWorkspace() {
    const user = $auth.user;

    const name =
      newWorkspaceName.trim();

    const slug =
      effectiveNewSlug.trim();

    if (!user || !name || !slug) {
      return;
    }

    creatingWorkspace = true;
    error = '';
    success = '';

    try {
      const created =
        await createWorkspace({
          name,
          slug,
          ownerUserId: user.id
        });

      workspaces = [
        created.workspace,
        ...workspaces
      ];

      selectedWorkspace =
        created.workspace;

      workspaceContext.setWorkspaces(
        workspaces,
        created.workspace.id
      );

      editName =
        created.workspace.name;

      editSlug =
        created.workspace.slug;

      showCreateForm = false;
      activeView = 'overview';

      resetCreateForm();

      updateWorkspaceUrl(
        created.workspace.id
      );

      await loadDetail(
        created.workspace
      );

      success =
        `${created.workspace.name} is ready. ` +
        'Connect a Vix project when you are ready.';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to create workspace.';
    } finally {
      creatingWorkspace = false;
    }
  }

  async function saveWorkspaceSettings() {
    if (
      !selectedWorkspace ||
      !canManage
    ) {
      return;
    }

    savingSettings = true;
    error = '';
    success = '';

    try {
      const updated =
        await updateWorkspace({
          id: selectedWorkspace.id,
          name: editName.trim(),
          slug: editSlug.trim()
        });

      syncWorkspace(
        updated.workspace
      );

      editName =
        updated.workspace.name;

      editSlug =
        updated.workspace.slug;

      success =
        'Workspace identity updated.';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to update workspace.';
    } finally {
      savingSettings = false;
    }
  }

  async function handleAvatarUpload(
    event: Event
  ) {
    const input =
      event.currentTarget as HTMLInputElement;

    const file =
      input.files?.[0];

    if (
      !selectedWorkspace ||
      !file ||
      !canManage
    ) {
      return;
    }

    if (
      ![
        'image/jpeg',
        'image/png',
        'image/webp'
      ].includes(file.type)
    ) {
      error =
        'Workspace logo must be a JPG, PNG or WebP image.';

      input.value = '';
      return;
    }

    if (
      file.size >
      2 * 1024 * 1024
    ) {
      error =
        'Choose a workspace logo smaller than 2MB.';

      input.value = '';
      return;
    }

    clearAvatarPreview();

    avatarPreviewUrl =
      URL.createObjectURL(file);

    uploadingAvatar = true;
    error = '';
    success = '';

    try {
      const updated =
        await uploadWorkspaceAvatar(
          selectedWorkspace.id,
          file
        );

      syncWorkspace(
        updated.workspace
      );

      success =
        'Workspace logo updated.';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to upload workspace logo.';
    } finally {
      uploadingAvatar = false;
      input.value = '';

      clearAvatarPreview();
    }
  }

  async function removeAvatar() {
    if (
      !selectedWorkspace ||
      !canManage
    ) {
      return;
    }

    uploadingAvatar = true;
    error = '';
    success = '';

    try {
      clearAvatarPreview();

      await deleteWorkspaceAvatar(
        selectedWorkspace.id
      );

      syncWorkspace({
        ...selectedWorkspace,
        avatar_url: '',
        avatar_storage_path: '',
        avatar_updated_at: 0
      });

      success =
        'Workspace logo removed.';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to remove workspace logo.';
    } finally {
      uploadingAvatar = false;
    }
  }

  function requestDangerAction(
    action: DangerAction
  ) {
    pendingDangerAction = action;
    dangerConfirmation = '';
    error = '';
    success = '';
  }

  function cancelDangerAction() {
    pendingDangerAction = null;
    dangerConfirmation = '';
  }

  async function confirmDangerAction() {
    if (
      !selectedWorkspace ||
      !canOwn ||
      !pendingDangerAction ||
      !dangerCanContinue
    ) {
      return;
    }

    const action =
      pendingDangerAction;

    const workspaceId =
      selectedWorkspace.id;

    savingSettings = true;
    error = '';
    success = '';

    try {
      if (action === 'delete') {
        await deleteWorkspace(
          workspaceId
        );

        workspaces =
          workspaces.filter(
            (workspace) =>
              workspace.id !== workspaceId
          );

        selectedWorkspace =
          workspaces[0] ?? null;

        workspaceContext.setWorkspaces(
          workspaces,
          selectedWorkspace?.id ?? ''
        );

        if (selectedWorkspace) {
          editName =
            selectedWorkspace.name;

          editSlug =
            selectedWorkspace.slug;

          updateWorkspaceUrl(
            selectedWorkspace.id
          );

          await loadDetail(
            selectedWorkspace
          );
        } else {
          projects = [];
          members = [];
          tokens = [];

          showCreateForm = true;

          updateWorkspaceUrl();
        }

        activeView = 'overview';
        success =
          'Workspace deleted.';
      } else if (
        action === 'suspend'
      ) {
        const updated =
          await suspendWorkspace(
            workspaceId
          );

        syncWorkspace(
          updated.workspace
        );

        success =
          'Workspace suspended.';
      } else {
        const updated =
          await reactivateWorkspace(
            workspaceId
          );

        syncWorkspace(
          updated.workspace
        );

        success =
          'Workspace reactivated.';
      }

      cancelDangerAction();
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : `Unable to ${action} workspace.`;
    } finally {
      savingSettings = false;
    }
  }

  usePageRefresh(load);

  onMount(() => {
    return () => {
      clearAvatarPreview();

      if (copyResetTimer) {
        clearTimeout(copyResetTimer);
      }
    };
  });
</script>

<svelte:head>
  <title>
    Workspaces | Softadastra Cloud
  </title>

  <meta
    name="description"
    content="Create, select and manage Softadastra Cloud workspaces for Vix projects, private packages, members and CLI access."
  />
</svelte:head>

<div class="workspace-page">
  <!-- ===================================================
       PAGE HEADER
       =================================================== -->
  <header class="page-header">
    <div>
      <p class="page-eyebrow">
        <span aria-hidden="true"></span>
        Organization
      </p>

      <h1>Workspaces</h1>

      <p>
        {#if loading}
          Loading your workspaces…
        {:else if selectedWorkspace}
          Manage
          <strong>
            {selectedWorkspace.name}
          </strong>
          or choose another workspace.
        {:else}
          Create the shared space that will own
          your Vix projects.
        {/if}
      </p>
    </div>

    {#if !loading && workspaces.length > 0}
      <button
        class:button-secondary={showCreateForm}
        class="header-action"
        type="button"
        onclick={() => {
          if (showCreateForm) {
            closeCreateForm();
          } else {
            openCreateForm();
          }
        }}
      >
        {#if !showCreateForm}
          <svg
            viewBox="0 0 24 24"
            aria-hidden="true"
          >
            <path d="M12 5v14"></path>
            <path d="M5 12h14"></path>
          </svg>
        {/if}

        {showCreateForm
          ? 'Close'
          : 'New workspace'}
      </button>
    {/if}
  </header>

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
       CREATE WORKSPACE
       The only content for a new account
       =================================================== -->
  {#if !loading && showCreateForm}
    <section
      class:first-workspace={
        workspaces.length === 0
      }
      class="create-workspace"
      aria-labelledby="create-workspace-title"
    >
      <div class="create-workspace__intro">
        <span
          class="create-workspace__icon"
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
            {workspaces.length === 0
              ? 'First step'
              : 'New workspace'}
          </p>

          <h2 id="create-workspace-title">
            {workspaces.length === 0
              ? 'Create your first workspace'
              : 'Create another workspace'}
          </h2>

          <p>
            A workspace owns projects, private packages,
            members, permissions and CLI access.
          </p>
        </div>
      </div>

      <form
        class="create-workspace__form"
        onsubmit={(event) => {
          event.preventDefault();
          void createNewWorkspace();
        }}
      >
        <label>
          <span>Workspace name</span>

          <input
            bind:value={newWorkspaceName}
            placeholder="My engineering team"
            required
            autocomplete="organization"
          />
        </label>

        <label>
          <span>Workspace slug</span>

          <input
            value={effectiveNewSlug}
            placeholder="my-engineering-team"
            required
            autocomplete="off"
            oninput={handleNewSlugInput}
          />
        </label>

        <div class="slug-preview">
          <span>Workspace identifier</span>

          <code>
            {effectiveNewSlug ||
              'workspace-slug'}
          </code>
        </div>

        <div class="create-workspace__actions">
          {#if workspaces.length > 0}
            <button
              class="button-secondary"
              type="button"
              disabled={creatingWorkspace}
              onclick={closeCreateForm}
            >
              Cancel
            </button>
          {/if}

          <button
            type="submit"
            disabled={
              creatingWorkspace ||
              !newWorkspaceName.trim() ||
              !effectiveNewSlug
            }
          >
            {creatingWorkspace
              ? 'Creating workspace…'
              : 'Create workspace'}
          </button>
        </div>
      </form>

      {#if workspaces.length === 0}
        <div class="create-workspace__path">
          <span>
            <strong>01</strong>
            Create the workspace
          </span>

          <i aria-hidden="true"></i>

          <span>
            <strong>02</strong>
            Connect a Vix project
          </span>

          <i aria-hidden="true"></i>

          <span>
            <strong>03</strong>
            Add the team
          </span>
        </div>
      {/if}
    </section>
  {/if}

  <!-- ===================================================
       LOADING
       =================================================== -->
  {#if loading}
    <section class="page-loading">
      <span class="page-loading__mark">
        S
      </span>

      <div>
        <strong>
          Loading your workspaces
        </strong>

        <p>
          Preparing workspace access and resources.
        </p>
      </div>

      <span
        class="page-loading__bar"
        aria-hidden="true"
      ></span>
    </section>

  <!-- ===================================================
       EXISTING WORKSPACES
       =================================================== -->
  {:else if workspaces.length > 0}
    <div class="workspace-layout">
      <!-- Directory -->
      <aside
        class="workspace-directory"
        aria-label="Workspace directory"
      >
        <header class="directory-header">
          <div>
            <p class="panel-kicker">
              Directory
            </p>

            <h2>Your workspaces</h2>
          </div>

          <span class="directory-count">
            {workspaces.length}
          </span>
        </header>

        {#if workspaces.length > 4}
          <label class="workspace-search">
            <span class="visually-hidden">
              Find a workspace
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
              bind:value={workspaceQuery}
              type="search"
              placeholder="Find a workspace"
            />
          </label>
        {/if}

        {#if visibleWorkspaces.length === 0}
          <p class="directory-message">
            No workspace matches your search.
          </p>
        {:else}
          <div class="workspace-options">
            {#each visibleWorkspaces as workspace (workspace.id)}
              <button
                class:selected={
                  workspace.id ===
                  selectedWorkspace?.id
                }
                class="workspace-option"
                type="button"
                aria-pressed={
                  workspace.id ===
                  selectedWorkspace?.id
                }
                onclick={() =>
                  selectWorkspace(workspace)}
              >
                <WorkspaceAvatar
                  {workspace}
                  size="sm"
                />

                <span class="workspace-option__copy">
                  <strong>
                    {workspace.name}
                  </strong>

                  <code>
                    {workspace.slug}
                  </code>
                </span>

                <span class="workspace-option__role">
                  {roleLabel(
                    workspace.current_user_role
                  )}
                </span>
              </button>
            {/each}
          </div>
        {/if}
      </aside>

      <!-- Selected workspace -->
      <main class="workspace-control">
        {#if selectedWorkspace}
          {@const currentWorkspace = selectedWorkspace}
          <section class="workspace-card">
            <header class="workspace-card__header">
              <div class="workspace-identity">
                <WorkspaceAvatar
                  workspace={selectedWorkspace}
                />

                <div>
                  <p class="panel-kicker">
                    Current workspace
                  </p>

                  <div class="workspace-title">
                    <h2>
                      {selectedWorkspace.name}
                    </h2>

                    <span
                      class:active={
                        workspaceStatus(
                              currentWorkspace
                            ) === 'active'
                      }
                      class="workspace-state"
                    >
                      <i aria-hidden="true"></i>

                      {workspaceStatus(
                              currentWorkspace
                            )}
                    </span>
                  </div>

                  <code>
                    {selectedWorkspace.slug}
                  </code>
                </div>
              </div>

              <nav
                class="workspace-tabs"
                aria-label="Workspace sections"
              >
                <button
                  class:active={
                    activeView === 'overview'
                  }
                  type="button"
                  role="tab"
                  aria-selected={
                    activeView === 'overview'
                  }
                  onclick={() => {
                    activeView = 'overview';
                  }}
                >
                  Overview
                </button>

                {#if canManage}
                  <button
                    class:active={
                      activeView === 'settings'
                    }
                    type="button"
                    role="tab"
                    aria-selected={
                      activeView === 'settings'
                    }
                    onclick={() => {
                      activeView = 'settings';
                    }}
                  >
                    Settings
                  </button>
                {/if}
              </nav>
            </header>

            {#if loadingDetail}
              <span
                class="detail-progress"
                aria-hidden="true"
              ></span>
            {/if}

            <!-- =========================================
                 OVERVIEW
                 ========================================= -->
            {#if activeView === 'overview'}
              <div
                class="workspace-overview"
                role="tabpanel"
              >
                <section class="workspace-metrics">
                  <article>
                    <span>Projects</span>

                    <strong>
                      {projects.length}
                    </strong>

                    <small>
                      Connected to this workspace
                    </small>
                  </article>

                  <article>
                    <span>Active members</span>

                    <strong>
                      {activeMembers}
                    </strong>

                    <small>
                      People with current access
                    </small>
                  </article>

                  {#if canTokens}
                    <article>
                      <span>Active CLI tokens</span>

                      <strong>
                        {activeTokens}
                      </strong>

                      <small>
                        Credentials still usable
                      </small>
                    </article>
                  {:else}
                    <article>
                      <span>Your role</span>

                      <strong>
                        {roleLabel(currentRole)}
                      </strong>

                      <small>
                        {accessLabel(
                          selectedWorkspace
                            .access_scope
                        )}
                      </small>
                    </article>
                  {/if}
                </section>

                <nav
                  class="resource-navigation"
                  aria-label="Workspace resources"
                >
                  <a
                    href={`/projects?workspace_id=${selectedWorkspace.id}`}
                  >
                    <span class="resource-icon">
                      <svg
                        viewBox="0 0 24 24"
                        fill="none"
                        stroke="currentColor"
                        stroke-width="1.8"
                        aria-hidden="true"
                      >
                        <path d="M3 6h6l2 2h10v11H3z"></path>
                      </svg>
                    </span>

                    <span>
                      <strong>Projects</strong>
                      <small>
                        Connected Vix projects
                      </small>
                    </span>

                    <svg
                      class="resource-arrow"
                      viewBox="0 0 24 24"
                      aria-hidden="true"
                    >
                      <path d="m9 18 6-6-6-6"></path>
                    </svg>
                  </a>

                  <a
                    href={`/packages?workspace_id=${selectedWorkspace.id}`}
                  >
                    <span class="resource-icon">
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
                    </span>

                    <span>
                      <strong>Packages</strong>
                      <small>
                        Private package versions
                      </small>
                    </span>

                    <svg
                      class="resource-arrow"
                      viewBox="0 0 24 24"
                      aria-hidden="true"
                    >
                      <path d="m9 18 6-6-6-6"></path>
                    </svg>
                  </a>

                  {#if canMembers}
                    <a
                      href={`/members?workspace_id=${selectedWorkspace.id}`}
                    >
                      <span class="resource-icon">
                        <svg
                          viewBox="0 0 24 24"
                          fill="none"
                          stroke="currentColor"
                          stroke-width="1.8"
                          aria-hidden="true"
                        >
                          <circle
                            cx="8"
                            cy="8"
                            r="3"
                          ></circle>

                          <circle
                            cx="17"
                            cy="9"
                            r="2.5"
                          ></circle>

                          <path d="M2.5 20a5.5 5.5 0 0 1 11 0"></path>
                          <path d="M13 20a4 4 0 0 1 8 0"></path>
                        </svg>
                      </span>

                      <span>
                        <strong>Members</strong>
                        <small>
                          Roles and workspace access
                        </small>
                      </span>

                      <svg
                        class="resource-arrow"
                        viewBox="0 0 24 24"
                        aria-hidden="true"
                      >
                        <path d="m9 18 6-6-6-6"></path>
                      </svg>
                    </a>
                  {/if}

                  {#if canTokens}
                    <a
                      href={`/tokens?workspace_id=${selectedWorkspace.id}`}
                    >
                      <span class="resource-icon">
                        <svg
                          viewBox="0 0 24 24"
                          fill="none"
                          stroke="currentColor"
                          stroke-width="1.8"
                          aria-hidden="true"
                        >
                          <circle
                            cx="8"
                            cy="15"
                            r="3"
                          ></circle>

                          <path d="m10.5 13.5 8-8"></path>
                          <path d="m15 5 4 4"></path>
                        </svg>
                      </span>

                      <span>
                        <strong>CLI tokens</strong>
                        <small>
                          Connect local tooling
                        </small>
                      </span>

                      <svg
                        class="resource-arrow"
                        viewBox="0 0 24 24"
                        aria-hidden="true"
                      >
                        <path d="m9 18 6-6-6-6"></path>
                      </svg>
                    </a>
                  {/if}
                </nav>

                {#if projects.length === 0}
                  <section class="next-step">
                    <span class="next-step__icon">
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
                      <p class="panel-kicker">
                        Next step
                      </p>

                      <h3>
                        Connect the first Vix project
                      </h3>

                      <p>
                        Run the command from the local
                        project that should belong to
                        this workspace.
                      </p>
                    </div>

                    <code>
                      <span>$</span>
                      vix cloud init
                    </code>

                    {#if canProjects}
                      <a
                        href={`/projects?workspace_id=${selectedWorkspace.id}`}
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
                  </section>
                {:else}
                  <section class="workspace-ready">
                    <span aria-hidden="true">
                      <svg
                        viewBox="0 0 24 24"
                        fill="none"
                        stroke="currentColor"
                        stroke-width="2"
                        stroke-linecap="round"
                        stroke-linejoin="round"
                      >
                        <path d="m5 12 4 4L19 6"></path>
                      </svg>
                    </span>

                    <div>
                      <strong>
                        Workspace connected
                      </strong>

                      <p>
                        This workspace already contains
                        {projects.length}
                        {projects.length === 1
                          ? ' project'
                          : ' projects'}.
                      </p>
                    </div>

                    <a
                      href={`/projects?workspace_id=${selectedWorkspace.id}`}
                    >
                      Open projects
                    </a>
                  </section>
                {/if}

                <section class="workspace-metadata">
                  <div>
                    <span>Your role</span>
                    <strong>
                      {roleLabel(currentRole)}
                    </strong>
                  </div>

                  <div>
                    <span>Access scope</span>
                    <strong>
                      {accessLabel(
                        selectedWorkspace
                          .access_scope
                      )}
                    </strong>
                  </div>

                  <div class="workspace-id">
                    <span>Workspace ID</span>

                    <code>
                      {selectedWorkspace.id}
                    </code>

                    <button
                      class:copied={
                        copiedWorkspaceId
                      }
                      type="button"
                      aria-label={
                        copiedWorkspaceId
                          ? 'Workspace ID copied'
                          : 'Copy workspace ID'
                      }
                      onclick={copyWorkspaceId}
                    >
                      {#if copiedWorkspaceId}
                        <svg
                          viewBox="0 0 24 24"
                          aria-hidden="true"
                        >
                          <path d="m5 12 4 4L19 6"></path>
                        </svg>
                      {:else}
                        <svg
                          viewBox="0 0 24 24"
                          aria-hidden="true"
                        >
                          <rect
                            x="8"
                            y="8"
                            width="11"
                            height="11"
                            rx="2"
                          ></rect>

                          <path d="M16 8V5a2 2 0 0 0-2-2H5a2 2 0 0 0-2 2v9a2 2 0 0 0 2 2h3"></path>
                        </svg>
                      {/if}
                    </button>
                  </div>
                </section>
              </div>

            <!-- =========================================
                 SETTINGS
                 ========================================= -->
            {:else if canManage}
              <div
                class="workspace-settings"
                role="tabpanel"
              >
                <section class="identity-settings">
                  <header>
                    <div>
                      <p class="panel-kicker">
                        Identity
                      </p>

                      <h3>
                        Workspace name and logo
                      </h3>

                      <p>
                        These details identify the
                        workspace throughout Cloud.
                      </p>
                    </div>
                  </header>

                  <div class="identity-settings__content">
                    <div class="avatar-settings">
                      <WorkspaceAvatar
                        workspace={selectedWorkspace}
                        size="lg"
                        previewUrl={avatarPreviewUrl}
                      />

                      <div>
                        <strong>
                          Workspace logo
                        </strong>

                        <p>
                          JPG, PNG or WebP. Maximum 2MB.
                        </p>

                        <div>
                          <label class="upload-button">
                            <span>
                              {uploadingAvatar
                                ? 'Uploading…'
                                : 'Upload logo'}
                            </span>

                            <input
                              type="file"
                              accept="image/jpeg,image/png,image/webp"
                              disabled={uploadingAvatar}
                              onchange={handleAvatarUpload}
                            />
                          </label>

                          {#if selectedWorkspace.avatar_url || avatarPreviewUrl}
                            <button
                              class="button-secondary"
                              type="button"
                              disabled={uploadingAvatar}
                              onclick={removeAvatar}
                            >
                              Remove
                            </button>
                          {/if}
                        </div>
                      </div>
                    </div>

                    <form
                      class="identity-form"
                      onsubmit={(event) => {
                        event.preventDefault();
                        void saveWorkspaceSettings();
                      }}
                    >
                      <label>
                        <span>Name</span>

                        <input
                          bind:value={editName}
                          required
                          autocomplete="off"
                        />
                      </label>

                      <label>
                        <span>Slug</span>

                        <input
                          bind:value={editSlug}
                          required
                          autocomplete="off"
                        />
                      </label>

                      <button
                        type="submit"
                        disabled={
                          savingSettings ||
                          !workspaceSettingsChanged ||
                          !editName.trim() ||
                          !editSlug.trim()
                        }
                      >
                        {savingSettings
                          ? 'Saving…'
                          : 'Save changes'}
                      </button>
                    </form>
                  </div>
                </section>

                {#if canOwn}
                  <details class="danger-zone">
                    <summary>
                      <span>
                        <strong>
                          Danger zone
                        </strong>

                        <small>
                          Suspend or delete this
                          workspace.
                        </small>
                      </span>

                      <svg
                        viewBox="0 0 24 24"
                        aria-hidden="true"
                      >
                        <path d="m6 9 6 6 6-6"></path>
                      </svg>
                    </summary>

                    <div class="danger-zone__content">
                      <div class="danger-action">
                        <div>
                          <strong>
                            {workspaceStatus(
                              currentWorkspace
                            ) === 'suspended'
                              ? 'Reactivate workspace'
                              : 'Suspend workspace'}
                          </strong>

                          <p>
                            {workspaceStatus(
                              currentWorkspace
                            ) === 'suspended'
                              ? 'Restore write actions for this workspace.'
                              : 'Temporarily stop write actions without deleting records.'}
                          </p>
                        </div>

                        <button
                          class="button-secondary"
                          type="button"
                          disabled={savingSettings}
                          onclick={() =>
                            requestDangerAction(
                              workspaceStatus(
                              currentWorkspace
                            ) === 'suspended'
                                ? 'reactivate'
                                : 'suspend'
                            )}
                        >
                          {workspaceStatus(
                              currentWorkspace
                            ) === 'suspended'
                            ? 'Reactivate'
                            : 'Suspend'}
                        </button>
                      </div>

                      <div class="danger-action danger-action--delete">
                        <div>
                          <strong>
                            Delete workspace
                          </strong>

                          <p>
                            Remove the workspace from
                            normal use while retaining
                            records for safety.
                          </p>
                        </div>

                        <button
                          class="danger-button"
                          type="button"
                          disabled={savingSettings}
                          onclick={() =>
                            requestDangerAction(
                              'delete'
                            )}
                        >
                          Delete
                        </button>
                      </div>

                      {#if pendingDangerAction}
                        <div
                          class="danger-confirmation"
                          role="alert"
                        >
                          <div>
                            <strong>
                              Confirm
                              {pendingDangerAction}
                            </strong>

                            <p>
                              {#if pendingDangerAction === 'delete'}
                                Enter
                                <code>
                                  {selectedWorkspace.slug}
                                </code>
                                to confirm permanent
                                removal from normal use.
                              {:else if pendingDangerAction === 'suspend'}
                                Write actions will stop
                                until the workspace is
                                reactivated.
                              {:else}
                                Write actions will become
                                available again.
                              {/if}
                            </p>
                          </div>

                          {#if dangerNeedsSlug}
                            <input
                              bind:value={dangerConfirmation}
                              placeholder={selectedWorkspace.slug}
                              autocomplete="off"
                            />
                          {/if}

                          <div class="danger-confirmation__actions">
                            <button
                              class="button-secondary"
                              type="button"
                              disabled={savingSettings}
                              onclick={cancelDangerAction}
                            >
                              Cancel
                            </button>

                            <button
                              class:danger-button={
                                pendingDangerAction ===
                                'delete'
                              }
                              type="button"
                              disabled={
                                savingSettings ||
                                !dangerCanContinue
                              }
                              onclick={() =>
                                void confirmDangerAction()}
                            >
                              {savingSettings
                                ? 'Working…'
                                : `Confirm ${pendingDangerAction}`}
                            </button>
                          </div>
                        </div>
                      {/if}
                    </div>
                  </details>
                {/if}
              </div>
            {/if}
          </section>
        {/if}
      </main>
    </div>
  {/if}
</div>

<style>
  /* =====================================================
     PAGE
     ===================================================== */

  .workspace-page {
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

  .page-header > div {
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

  .page-header h1 {
    color: var(--text);
    font-size: clamp(28px, 4vw, 42px);
    font-weight: 650;
    line-height: 1;
    letter-spacing: -0.045em;
  }

  .page-header > div > p:last-child {
    color: var(--text-muted);
    font-size: 12.5px;
    line-height: 1.55;
  }

  .page-header strong {
    color: var(--text-soft);
    font-weight: 600;
  }

  .header-action {
    flex: 0 0 auto;
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
     BUTTONS AND SHARED UI
     ===================================================== */

  button {
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
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease),
      color var(--speed) var(--ease),
      transform var(--speed) var(--ease);
  }

  button:hover:not(:disabled) {
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

  .panel-kicker {
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
     CREATE WORKSPACE
     ===================================================== */

  .create-workspace {
    display: grid;
    grid-template-columns:
      minmax(240px, 0.72fr)
      minmax(0, 1.28fr);
    gap: 28px;
    padding: 24px;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    box-shadow: inset 3px 0 0 var(--brand);
  }

  .create-workspace.first-workspace {
    min-height: 340px;
    align-items: center;
    padding: clamp(28px, 5vw, 52px);
    border-radius: var(--radius-lg);
    box-shadow:
      inset 3px 0 0 var(--brand),
      var(--shadow-panel);
  }

  .create-workspace__intro {
    display: flex;
    align-items: flex-start;
    gap: 12px;
  }

  .create-workspace__icon {
    display: grid;
    width: 44px;
    height: 44px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 10px;
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .create-workspace__icon svg {
    width: 21px;
    height: 21px;
  }

  .create-workspace__intro > div {
    display: grid;
    gap: 6px;
  }

  .create-workspace__intro h2 {
    color: var(--text);
    font-size: clamp(19px, 3vw, 28px);
    font-weight: 620;
    letter-spacing: -0.035em;
  }

  .create-workspace__intro p:not(.panel-kicker) {
    max-width: 340px;
    color: var(--text-muted);
    font-size: 11px;
    line-height: 1.6;
  }

  .create-workspace__form {
    display: grid;
    grid-template-columns:
      repeat(2, minmax(0, 1fr));
    gap: 13px;
  }

  .create-workspace__form label,
  .identity-form label {
    display: grid;
    gap: 7px;
    color: var(--text-soft);
    font-size: 10.5px;
    font-weight: 550;
  }

  .create-workspace__form input,
  .identity-form input,
  .danger-confirmation input {
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

  .create-workspace__form input:focus,
  .identity-form input:focus,
  .danger-confirmation input:focus {
    border-color: var(--brand);
    box-shadow: 0 0 0 3px var(--brand-faint);
  }

  .slug-preview {
    grid-column: 1 / -1;
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 9px;
    padding: 9px 10px;
    border: 1px solid var(--line-soft);
    border-radius: 6px;
    background: var(--bg-ink-soft);
  }

  .slug-preview > span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 7px;
    text-transform: uppercase;
  }

  .slug-preview code {
    color: var(--brand-bright);
    font-size: 10.5px;
  }

  .create-workspace__actions {
    grid-column: 1 / -1;
    display: flex;
    justify-content: flex-end;
    gap: 8px;
  }

  .create-workspace__path {
    grid-column: 1 / -1;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 15px;
    padding-top: 22px;
    border-top: 1px solid var(--line);
  }

  .create-workspace__path span {
    display: inline-flex;
    align-items: center;
    gap: 7px;
    color: var(--text-muted);
    font-size: 9px;
  }

  .create-workspace__path strong {
    color: var(--brand-soft);
    font-family: var(--font-mono);
    font-size: 7px;
  }

  .create-workspace__path i {
    width: 28px;
    height: 1px;
    background: var(--line-strong);
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
     LAYOUT
     ===================================================== */

  .workspace-layout {
    display: grid;
    grid-template-columns:
      268px minmax(0, 1fr);
    gap: 16px;
    align-items: start;
  }

  /* =====================================================
     DIRECTORY
     ===================================================== */

  .workspace-directory {
    position: sticky;
    top: 24px;
    overflow: hidden;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .directory-header {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 14px;
    padding: 14px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .directory-header > div {
    display: grid;
    gap: 3px;
  }

  .directory-header h2 {
    color: var(--text);
    font-size: 13px;
    font-weight: 600;
  }

  .directory-count {
    display: grid;
    min-width: 24px;
    height: 22px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    background: var(--bg-elevated);
    color: var(--text-muted);
    padding: 0 7px;
    font-family: var(--font-mono);
    font-size: 10px;
  }

  .workspace-search {
    position: relative;
    display: block;
    padding: 10px;
    border-bottom: 1px solid var(--line-soft);
  }

  .workspace-search svg {
    position: absolute;
    top: 50%;
    left: 21px;
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

  .workspace-search input {
    min-height: 34px;
    padding-left: 33px;
    font-size: 11px;
  }

  .workspace-options {
    display: grid;
    max-height: calc(100vh - 220px);
    overflow-y: auto;
  }

  .workspace-option {
    display: grid;
    width: 100%;
    min-height: 59px;
    grid-template-columns:
      30px minmax(0, 1fr) auto;
    align-items: center;
    gap: 9px;
    padding: 9px 11px;
    border: 0;
    border-bottom: 1px solid var(--line-soft);
    border-radius: 0;
    background: transparent;
    color: var(--text);
    text-align: left;
    transition: background var(--speed) var(--ease);
  }

  .workspace-option:last-child {
    border-bottom: 0;
  }

  .workspace-option:hover:not(:disabled) {
    background: var(--bg-elevated);
    transform: none;
  }

  .workspace-option.selected {
    background: var(--brand-faint);
    box-shadow: inset 2px 0 0 var(--brand);
  }

  .workspace-option__copy {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .workspace-option__copy strong {
    overflow: hidden;
    color: var(--text);
    font-size: 11.5px;
    font-weight: 600;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .workspace-option.selected
    .workspace-option__copy strong {
    color: var(--brand-bright);
  }

  .workspace-option__copy code {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 9px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .workspace-option__role {
    color: var(--text-muted);
    font-size: 9px;
  }

  .directory-message {
    padding: 18px 14px;
    color: var(--text-muted);
    font-size: 11px;
  }

  /* =====================================================
     WORKSPACE CONTROL
     ===================================================== */

  .workspace-control {
    min-width: 0;
  }

  .workspace-card {
    position: relative;
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    box-shadow: var(--shadow-soft);
  }

  .workspace-card__header {
    display: flex;
    min-height: 92px;
    align-items: center;
    justify-content: space-between;
    gap: 22px;
    padding: 17px 19px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .workspace-identity {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 12px;
  }

  .workspace-identity > div {
    display: grid;
    min-width: 0;
    gap: 5px;
  }

  .workspace-title {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 9px;
  }

  .workspace-title h2 {
    color: var(--text);
    font-size: 20px;
    font-weight: 620;
    letter-spacing: -0.035em;
    overflow-wrap: anywhere;
  }

  .workspace-identity > div > code {
    color: var(--text-muted);
    font-size: 9px;
  }

  .workspace-state {
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

  .workspace-state i {
    width: 4px;
    height: 4px;
    border-radius: 50%;
    background: currentColor;
  }

  .workspace-state.active {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .workspace-tabs {
    display: flex;
    flex: 0 0 auto;
    gap: 4px;
    padding: 4px;
    border: 1px solid var(--line);
    border-radius: 8px;
    background: var(--bg-panel);
  }

  .workspace-tabs button {
    min-height: 29px;
    border: 0;
    background: transparent;
    color: var(--text-muted);
    padding: 0 10px;
    font-size: 9.5px;
  }

  .workspace-tabs button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
    transform: none;
  }

  .workspace-tabs button.active {
    background: var(--brand-faint);
    color: var(--brand-bright);
  }

  .detail-progress {
    position: absolute;
    z-index: 3;
    top: 91px;
    right: 0;
    left: 0;
    height: 2px;
    background: var(--brand);
    animation: detail-loading 1.25s ease-in-out infinite;
    transform-origin: left;
  }

  @keyframes detail-loading {
    0% {
      transform: scaleX(0);
    }

    70% {
      transform: scaleX(1);
    }

    100% {
      opacity: 0;
    }
  }

  /* =====================================================
     OVERVIEW
     ===================================================== */

  .workspace-overview {
    display: grid;
  }

  .workspace-metrics {
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    border-bottom: 1px solid var(--line-soft);
  }

  .workspace-metrics article {
    display: grid;
    gap: 4px;
    padding: 16px 19px;
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
    font-size: 18px;
    font-weight: 620;
    line-height: 1.2;
  }

  .workspace-metrics small {
    color: var(--text-faint);
    font-size: 8.5px;
  }

  .resource-navigation {
    display: grid;
    grid-template-columns:
      repeat(2, minmax(0, 1fr));
    gap: 9px;
    padding: 16px;
    border-bottom: 1px solid var(--line-soft);
  }

  .resource-navigation > a {
    display: grid;
    min-height: 68px;
    grid-template-columns:
      auto minmax(0, 1fr) auto;
    align-items: center;
    gap: 10px;
    padding: 10px;
    border: 1px solid var(--line);
    border-radius: 8px;
    background: var(--bg-ink-soft);
    color: inherit;
    text-decoration: none;
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease),
      transform var(--speed) var(--ease);
  }

  .resource-navigation > a:hover {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    transform: translateY(-1px);
  }

  .resource-icon {
    display: grid;
    width: 35px;
    height: 35px;
    place-items: center;
    border: 1px solid var(--line);
    border-radius: 8px;
    background: var(--bg-panel);
    color: var(--brand-soft);
  }

  .resource-icon svg {
    width: 16px;
    height: 16px;
  }

  .resource-navigation > a > span:nth-child(2) {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .resource-navigation strong {
    color: var(--text);
    font-size: 10.5px;
    font-weight: 600;
  }

  .resource-navigation small {
    color: var(--text-muted);
    font-size: 8.5px;
  }

  .resource-arrow {
    width: 13px;
    height: 13px;
    fill: none;
    stroke: var(--text-faint);
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
    transition:
      stroke var(--speed) var(--ease),
      transform var(--speed) var(--ease);
  }

  .resource-navigation > a:hover
    .resource-arrow {
    stroke: var(--brand);
    transform: translateX(2px);
  }

  /* =====================================================
     NEXT STEP
     ===================================================== */

  .next-step {
    display: grid;
    grid-template-columns:
      auto minmax(0, 1fr);
    gap: 12px;
    margin: 16px;
    padding: 17px;
    border: 1px solid var(--brand-line);
    border-radius: 9px;
    background: var(--brand-faint);
  }

  .next-step__icon {
    display: grid;
    width: 41px;
    height: 41px;
    grid-row: 1 / 4;
    place-items: center;
    border-radius: 9px;
    background: var(--bg-panel-strong);
    color: var(--brand-soft);
  }

  .next-step__icon svg {
    width: 19px;
    height: 19px;
  }

  .next-step > div {
    display: grid;
    gap: 4px;
  }

  .next-step h3 {
    color: var(--text);
    font-size: 14px;
    font-weight: 610;
    letter-spacing: -0.02em;
  }

  .next-step div > p:not(.panel-kicker) {
    color: var(--text-muted);
    font-size: 9.5px;
    line-height: 1.5;
  }

  .next-step > code {
    grid-column: 2;
    display: flex;
    min-height: 39px;
    align-items: center;
    gap: 8px;
    padding: 0 11px;
    border: 1px solid var(--line-ink);
    border-radius: 6px;
    background: var(--bg-ink);
    color: var(--text-soft);
    font-size: 9.5px;
  }

  .next-step > code span {
    color: var(--brand);
    font-weight: 800;
  }

  .next-step > a {
    grid-column: 2;
    display: inline-flex;
    width: fit-content;
    align-items: center;
    gap: 6px;
    color: var(--brand-soft);
    font-size: 9.5px;
    font-weight: 600;
  }

  .next-step > a:hover {
    color: var(--brand-bright);
  }

  .next-step > a svg {
    width: 12px;
    height: 12px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
  }

  .workspace-ready {
    display: grid;
    grid-template-columns:
      auto minmax(0, 1fr) auto;
    align-items: center;
    gap: 11px;
    margin: 16px;
    padding: 14px;
    border: 1px solid var(--green-line);
    border-radius: 8px;
    background: var(--green-faint);
  }

  .workspace-ready > span {
    display: grid;
    width: 33px;
    height: 33px;
    place-items: center;
    border-radius: 7px;
    background: var(--bg-panel-strong);
    color: var(--green-soft);
  }

  .workspace-ready svg {
    width: 15px;
    height: 15px;
  }

  .workspace-ready > div {
    display: grid;
    gap: 3px;
  }

  .workspace-ready strong {
    color: var(--text);
    font-size: 10.5px;
  }

  .workspace-ready p {
    color: var(--text-muted);
    font-size: 9px;
  }

  .workspace-ready a {
    color: var(--green-soft);
    font-size: 9.5px;
    font-weight: 600;
  }

  /* =====================================================
     METADATA
     ===================================================== */

  .workspace-metadata {
    display: grid;
    grid-template-columns:
      minmax(120px, 0.7fr)
      minmax(150px, 0.9fr)
      minmax(0, 1.6fr);
    border-top: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .workspace-metadata > div {
    display: grid;
    align-content: center;
    gap: 4px;
    min-height: 62px;
    padding: 11px 15px;
    border-right: 1px solid var(--line-soft);
  }

  .workspace-metadata > div:last-child {
    border-right: 0;
  }

  .workspace-metadata span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6px;
    text-transform: uppercase;
  }

  .workspace-metadata strong {
    color: var(--text-soft);
    font-size: 9.5px;
    font-weight: 550;
  }

  .workspace-id {
    grid-template-columns:
      minmax(0, 1fr) 29px;
  }

  .workspace-id > span {
    grid-column: 1 / -1;
  }

  .workspace-id code {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 8.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .workspace-id button {
    display: grid;
    width: 27px;
    height: 27px;
    min-height: 27px;
    place-items: center;
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-muted);
    padding: 0;
  }

  .workspace-id button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
    transform: none;
  }

  .workspace-id button.copied {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .workspace-id button svg {
    width: 13px;
    height: 13px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  /* =====================================================
     SETTINGS
     ===================================================== */

  .workspace-settings {
    display: grid;
    gap: 15px;
    padding: 16px;
  }

  .identity-settings {
    overflow: hidden;
    border: 1px solid var(--line);
    border-radius: 9px;
    background: var(--bg-ink-soft);
  }

  .identity-settings > header {
    padding: 14px 16px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .identity-settings > header > div {
    display: grid;
    gap: 4px;
  }

  .identity-settings h3 {
    color: var(--text);
    font-size: 13px;
    font-weight: 600;
  }

  .identity-settings header p:not(.panel-kicker) {
    color: var(--text-muted);
    font-size: 9.5px;
  }

  .identity-settings__content {
    display: grid;
    grid-template-columns:
      minmax(260px, 0.85fr)
      minmax(0, 1.15fr);
    gap: 18px;
    padding: 16px;
  }

  .avatar-settings {
    display: grid;
    grid-template-columns:
      88px minmax(0, 1fr);
    align-items: center;
    gap: 13px;
    padding: 13px;
    border: 1px solid var(--line-soft);
    border-radius: 8px;
    background: var(--bg-panel);
  }

  .avatar-settings > div {
    display: grid;
    gap: 5px;
  }

  .avatar-settings strong {
    color: var(--text);
    font-size: 10.5px;
  }

  .avatar-settings p {
    color: var(--text-muted);
    font-size: 8.5px;
  }

  .avatar-settings > div > div {
    display: flex;
    flex-wrap: wrap;
    gap: 7px;
    margin-top: 4px;
  }

  .upload-button {
    position: relative;
    display: inline-flex;
    min-height: 34px;
    align-items: center;
    justify-content: center;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-sm);
    background: var(--brand-faint);
    color: var(--brand-bright);
    padding: 0 11px;
    font-size: 10px;
    font-weight: 600;
    cursor: pointer;
  }

  .upload-button input {
    position: absolute;
    inset: 0;
    opacity: 0;
    cursor: pointer;
  }

  .identity-form {
    display: grid;
    grid-template-columns:
      repeat(2, minmax(0, 1fr));
    align-items: end;
    gap: 12px;
  }

  .identity-form button {
    width: fit-content;
  }

  /* =====================================================
     DANGER ZONE
     ===================================================== */

  .danger-zone {
    overflow: hidden;
    border: 1px solid var(--danger-line);
    border-radius: 9px;
    background: var(--bg-ink-soft);
  }

  .danger-zone summary {
    display: flex;
    min-height: 62px;
    align-items: center;
    justify-content: space-between;
    gap: 15px;
    padding: 12px 15px;
    cursor: pointer;
    list-style: none;
  }

  .danger-zone summary::-webkit-details-marker {
    display: none;
  }

  .danger-zone summary > span {
    display: grid;
    gap: 3px;
  }

  .danger-zone summary strong {
    color: var(--danger);
    font-size: 10.5px;
  }

  .danger-zone summary small {
    color: var(--text-muted);
    font-size: 8.5px;
  }

  .danger-zone summary > svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: var(--text-muted);
    stroke-width: 1.8;
    transition: transform var(--speed) var(--ease);
  }

  .danger-zone[open] summary > svg {
    transform: rotate(180deg);
  }

  .danger-zone__content {
    display: grid;
    border-top: 1px solid var(--danger-line);
  }

  .danger-action {
    display: flex;
    min-height: 70px;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    padding: 13px 15px;
    border-bottom: 1px solid var(--line-soft);
  }

  .danger-action > div {
    display: grid;
    gap: 4px;
  }

  .danger-action strong {
    color: var(--text);
    font-size: 10px;
  }

  .danger-action p {
    max-width: 540px;
    color: var(--text-muted);
    font-size: 8.5px;
    line-height: 1.5;
  }

  .danger-action--delete strong {
    color: var(--danger);
  }

  .danger-confirmation {
    display: grid;
    gap: 12px;
    padding: 15px;
    background: var(--danger-faint);
  }

  .danger-confirmation > div:first-child {
    display: grid;
    gap: 4px;
  }

  .danger-confirmation strong {
    color: var(--danger);
    font-size: 10.5px;
    text-transform: capitalize;
  }

  .danger-confirmation p {
    color: var(--text-soft);
    font-size: 9px;
    line-height: 1.55;
  }

  .danger-confirmation code {
    color: var(--danger);
  }

  .danger-confirmation input {
    max-width: 360px;
    border-color: var(--danger-line);
  }

  .danger-confirmation__actions {
    display: flex;
    gap: 8px;
  }

  /* =====================================================
     REDUCED MOTION
     ===================================================== */

  @media (prefers-reduced-motion: reduce) {
    .workspace-page,
    .page-loading__bar,
    .detail-progress {
      animation: none;
    }
  }

  /* =====================================================
     RESPONSIVE
     ===================================================== */

  @media (max-width: 980px) {
    .create-workspace,
    .identity-settings__content {
      grid-template-columns: 1fr;
    }

    .workspace-layout {
      grid-template-columns: 1fr;
    }

    .workspace-directory {
      position: static;
    }

    .workspace-options {
      max-height: 260px;
    }
  }

  @media (max-width: 760px) {
    .page-header {
      align-items: stretch;
      flex-direction: column;
    }

    .header-action {
      width: 100%;
    }

    .workspace-card__header {
      align-items: flex-start;
      flex-direction: column;
    }

    .workspace-tabs {
      width: 100%;
    }

    .workspace-tabs button {
      flex: 1 1 0;
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

    .workspace-metadata {
      grid-template-columns: 1fr;
    }

    .workspace-metadata > div {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .workspace-metadata > div:last-child {
      border-bottom: 0;
    }
  }

  @media (max-width: 620px) {
    .create-workspace__form,
    .identity-form {
      grid-template-columns: 1fr;
    }

    .slug-preview,
    .create-workspace__actions {
      grid-column: auto;
    }

    .create-workspace__actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .create-workspace__actions button {
      width: 100%;
    }

    .create-workspace__path {
      display: grid;
      justify-content: start;
    }

    .create-workspace__path i {
      display: none;
    }

    .resource-navigation {
      grid-template-columns: 1fr;
    }

    .next-step {
      grid-template-columns: 1fr;
    }

    .next-step__icon {
      grid-row: auto;
    }

    .next-step > code,
    .next-step > a {
      grid-column: auto;
    }

    .workspace-ready {
      grid-template-columns:
        auto minmax(0, 1fr);
    }

    .workspace-ready > a {
      grid-column: 2;
    }

    .avatar-settings {
      grid-template-columns: 1fr;
    }

    .danger-action {
      align-items: stretch;
      flex-direction: column;
    }

    .danger-action button {
      width: 100%;
    }
  }

  @media (max-width: 460px) {
    .create-workspace {
      padding: 18px;
    }

    .workspace-option {
      grid-template-columns:
        30px minmax(0, 1fr);
    }

    .workspace-option__role {
      display: none;
    }

    .danger-confirmation__actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .danger-confirmation__actions button {
      width: 100%;
    }
  }
</style>
