<script lang="ts">
  import { goto } from '$app/navigation';
  import { browser } from '$app/environment';
  import { onMount } from 'svelte';
  import { listMembers } from '$lib/api/members';
  import { listProjects } from '$lib/api/projects';
  import { listTokens } from '$lib/api/tokens';
  import {
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
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import {
    canCreateProject,
    canManageMembers,
    canManageTokens
  } from '$lib/permissions';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  const API_BASE_URL =
    import.meta.env.VITE_API_BASE_URL ??
    (browser ? `${window.location.protocol}//${window.location.hostname}:8080` : '');

  let workspaces: Workspace[] = [];
  let selectedWorkspace: Workspace | null = null;

  let projects: Project[] = [];
  let members: Member[] = [];
  let tokens: CliToken[] = [];

  let workspaceQuery = '';
  let copiedWorkspaceId = false;
  let copyResetTimer: ReturnType<typeof setTimeout> | undefined;

  let loading = true;
  let loadingDetail = false;
  let savingSettings = false;
  let uploadingAvatar = false;
  let error = '';
  let success = '';

  let editName = '';
  let editSlug = '';

  $: currentRole =
    selectedWorkspace?.current_user_role ?? 'viewer';

  $: canProjects = canCreateProject(currentRole);
  $: canMembers = canManageMembers(currentRole);
  $: canTokens = canManageTokens(currentRole);

  $: visibleWorkspaces = workspaces.filter((workspace) => {
    const query = workspaceQuery.trim().toLowerCase();

    if (!query) {
      return true;
    }

    return (
      workspace.name.toLowerCase().includes(query) ||
      workspace.slug.toLowerCase().includes(query)
    );
  });

  $: activeMembers = members.filter(
    (member) => member.status === 'active'
  ).length;

  $: inactiveMembers = members.filter(
    (member) => member.status !== 'active'
  ).length;

  $: activeTokens = tokens.filter(
    (token) => !token.revoked
  ).length;

  function roleLabel(value?: string) {
    if (!value) {
      return 'Viewer';
    }

    return value.charAt(0).toUpperCase() + value.slice(1);
  }

  function statusLabel(value?: string) {
    if (!value) {
      return 'Unknown';
    }

    return value.charAt(0).toUpperCase() + value.slice(1);
  }

  function accessLabel(value?: string) {
    return value === 'selected_projects'
      ? 'Selected projects'
      : 'Entire workspace';
  }

  function workspaceStatus(workspace: Workspace) {
    return workspace.status || (workspace.active === false ? 'inactive' : 'active');
  }

  function tokenStatus(token: CliToken) {
    return token.revoked ? 'Revoked' : 'Active';
  }

  function workspaceInitial(workspace: Workspace) {
    return workspace.name.slice(0, 1).toUpperCase();
  }

  function workspaceAvatarUrl(workspace: Workspace) {
    if (!workspace.avatar_url) {
      return '';
    }

    return workspace.avatar_url.startsWith('http')
      ? workspace.avatar_url
      : `${API_BASE_URL}${workspace.avatar_url}`;
  }

  function canManageWorkspace() {
    return currentRole === 'owner' || currentRole === 'admin';
  }

  function canOwnWorkspace() {
    return currentRole === 'owner';
  }

  function syncWorkspace(updated: Workspace | null) {
    if (!updated) {
      return;
    }

    workspaces = workspaces.map((workspace) =>
      workspace.id === updated.id ? updated : workspace
    );
    selectedWorkspace = updated;
    workspaceContext.setWorkspaces(workspaces, updated.id);
  }

  function updateWorkspaceUrl(workspaceId: string) {
    const url = new URL(window.location.href);

    url.searchParams.set('workspace_id', workspaceId);

    history.replaceState(
      null,
      '',
      `${url.pathname}${url.search}`
    );
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
      error = 'Unable to copy the workspace ID.';
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
      const workspaceData = await listWorkspaces(user.id);

      workspaces = workspaceData.workspaces;

      const requestedWorkspaceId =
        new URLSearchParams(
          window.location.search
        ).get('workspace_id');

      const storedWorkspaceId =
        $workspaceContext.selectedWorkspace?.id;

      selectedWorkspace =
        (
          requestedWorkspaceId
            ? workspaces.find(
                (workspace) =>
                  workspace.id === requestedWorkspaceId
              )
            : null
        ) ??
        (
          storedWorkspaceId
            ? workspaces.find(
                (workspace) =>
                  workspace.id === storedWorkspaceId
              )
            : null
        ) ??
        workspaces[0] ??
        null;

      workspaceContext.setWorkspaces(
        workspaces,
        selectedWorkspace?.id ?? ''
      );

      if (selectedWorkspace) {
        editName = selectedWorkspace.name;
        editSlug = selectedWorkspace.slug;
      }

      await loadDetail(selectedWorkspace);
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load workspaces.';
    } finally {
      loading = false;
    }
  }

  async function selectWorkspace(workspace: Workspace) {
    if (
      workspace.id === selectedWorkspace?.id ||
      loadingDetail
    ) {
      return;
    }

    selectedWorkspace = workspace;
    editName = workspace.name;
    editSlug = workspace.slug;
    copiedWorkspaceId = false;
    error = '';
    success = '';

    workspaceContext.setSelectedWorkspace(workspace.id);
    updateWorkspaceUrl(workspace.id);

    await loadDetail(workspace);
  }

  async function saveWorkspaceSettings() {
    if (!selectedWorkspace || !canManageWorkspace()) {
      return;
    }

    savingSettings = true;
    error = '';
    success = '';

    try {
      const updated = await updateWorkspace({
        id: selectedWorkspace.id,
        name: editName.trim(),
        slug: editSlug.trim()
      });

      syncWorkspace(updated.workspace);
      success = 'Workspace updated.';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to update workspace.';
    } finally {
      savingSettings = false;
    }
  }

  async function handleAvatarUpload(event: Event) {
    const input = event.currentTarget as HTMLInputElement;
    const file = input.files?.[0];

    if (!selectedWorkspace || !file || !canManageWorkspace()) {
      return;
    }

    uploadingAvatar = true;
    error = '';
    success = '';

    try {
      const updated = await uploadWorkspaceAvatar(selectedWorkspace.id, file);
      syncWorkspace(updated.workspace);
      success = 'Workspace avatar updated.';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to upload workspace avatar.';
    } finally {
      uploadingAvatar = false;
      input.value = '';
    }
  }

  async function removeAvatar() {
    if (!selectedWorkspace || !canManageWorkspace()) {
      return;
    }

    uploadingAvatar = true;
    error = '';
    success = '';

    try {
      await deleteWorkspaceAvatar(selectedWorkspace.id);
      syncWorkspace({
        ...selectedWorkspace,
        avatar_url: '',
        avatar_storage_path: '',
        avatar_updated_at: 0
      });
      success = 'Workspace avatar removed.';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to remove workspace avatar.';
    } finally {
      uploadingAvatar = false;
    }
  }

  async function changeWorkspaceState(action: 'suspend' | 'reactivate' | 'delete') {
    if (!selectedWorkspace || !canOwnWorkspace()) {
      return;
    }

    const message =
      action === 'delete'
        ? `Delete workspace ${selectedWorkspace.name}? Existing records will be kept for safety.`
        : action === 'suspend'
          ? `Suspend workspace ${selectedWorkspace.name}? Write actions should stop until it is reactivated.`
          : `Reactivate workspace ${selectedWorkspace.name}?`;

    if (!window.confirm(message)) {
      return;
    }

    savingSettings = true;
    error = '';
    success = '';

    try {
      const updated =
        action === 'delete'
          ? await deleteWorkspace(selectedWorkspace.id)
          : action === 'suspend'
            ? await suspendWorkspace(selectedWorkspace.id)
            : await reactivateWorkspace(selectedWorkspace.id);

      if (action === 'delete') {
        workspaces = workspaces.filter((workspace) => workspace.id !== selectedWorkspace?.id);
        selectedWorkspace = workspaces[0] ?? null;
        workspaceContext.setWorkspaces(workspaces, selectedWorkspace?.id ?? '');
        if (selectedWorkspace) {
          await loadDetail(selectedWorkspace);
        }
      } else {
        syncWorkspace(updated.workspace);
      }

      success =
        action === 'delete'
          ? 'Workspace deleted.'
          : action === 'suspend'
            ? 'Workspace suspended.'
            : 'Workspace reactivated.';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : `Unable to ${action} workspace.`;
    } finally {
      savingSettings = false;
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

    loadingDetail = true;
    error = '';

    projects = [];
    members = [];
    tokens = [];

    try {
      const role =
        workspace.current_user_role ?? 'viewer';

      const tokenRequest = canManageTokens(role)
        ? listTokens(workspace.id, user.id)
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

      if (selectedWorkspace?.id !== workspace.id) {
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

  onMount(() => {
    void load();

    return () => {
      if (copyResetTimer) {
        clearTimeout(copyResetTimer);
      }
    };
  });
</script>

<svelte:head>
  <title>Workspaces | Softadastra Cloud</title>
</svelte:head>

<PageHeader
  eyebrow="Cloud"
  title="Workspaces"
  workspaceName={selectedWorkspace?.name ?? ''}
  role={currentRole}
/>

<p class="page-description">
  Select a workspace to review its access, projects, members and CLI
  credentials.
</p>

<InlineError message={error} />

{#if success}
  <p class="success-message" role="status">
    {success}
  </p>
{/if}

<div class="workspace-layout">
  <aside
    class="workspace-directory"
    aria-label="Workspace directory"
  >
    <div class="directory-header">
      <div>
        <h2>Workspaces</h2>
        <p>Your available cloud environments.</p>
      </div>

      <span class="section-count">
        {workspaces.length}
      </span>
    </div>

    {#if workspaces.length > 1}
      <div class="workspace-search">
        <svg viewBox="0 0 24 24" aria-hidden="true">
          <circle cx="11" cy="11" r="7"></circle>
          <path d="m20 20-4-4"></path>
        </svg>

        <input
          bind:value={workspaceQuery}
          type="search"
          placeholder="Find a workspace"
          aria-label="Find a workspace"
        />
      </div>
    {/if}

    {#if loading}
      <p class="directory-loading">
        Loading workspaces…
      </p>
    {:else if workspaces.length === 0}
      <div class="directory-empty">
        <EmptyState
          title="No workspaces"
          body="Create your first workspace from the Dashboard."
        />
      </div>
    {:else if visibleWorkspaces.length === 0}
      <p class="directory-loading">
        No workspace matches your search.
      </p>
    {:else}
      <div class="workspace-options">
        {#each visibleWorkspaces as workspace (workspace.id)}
          <button
            class:selected={
              workspace.id === selectedWorkspace?.id
            }
            class="workspace-option"
            type="button"
            aria-pressed={
              workspace.id === selectedWorkspace?.id
            }
            onclick={() => selectWorkspace(workspace)}
          >
            <span class="workspace-option__mark">
              {#if workspaceAvatarUrl(workspace)}
                <img src={workspaceAvatarUrl(workspace)} alt="" aria-hidden="true" />
              {:else}
                {workspaceInitial(workspace)}
              {/if}
            </span>

            <span class="workspace-option__content">
              <strong>{workspace.name}</strong>
              <code>{workspace.slug}</code>
            </span>

            <span class="workspace-option__role">
              {roleLabel(workspace.current_user_role)}
            </span>
          </button>
        {/each}
      </div>
    {/if}
  </aside>

  <main class="workspace-detail">
    {#if loading}
      <section class="detail-section">
        <p class="detail-loading">
          Loading workspace…
        </p>
      </section>
    {:else if !selectedWorkspace}
      <section class="detail-section empty-detail">
        <EmptyState
          title="Select a workspace"
          body="Its projects, members and CLI credentials will appear here."
        />
      </section>
    {:else}
      <section
        class="workspace-overview"
        aria-labelledby="workspace-name"
      >
        <div class="workspace-overview__header">
          <div class="workspace-identity">
            <span class="workspace-mark">
              {#if workspaceAvatarUrl(selectedWorkspace)}
                <img src={workspaceAvatarUrl(selectedWorkspace)} alt="" aria-hidden="true" />
              {:else}
                {workspaceInitial(selectedWorkspace)}
              {/if}
            </span>

            <div>
              <div class="workspace-name-line">
                <h2 id="workspace-name">
                  {selectedWorkspace.name}
                </h2>

                <span
                  class:inactive={
                    selectedWorkspace.active === false
                  }
                  class="workspace-state"
                >
                  {workspaceStatus(selectedWorkspace)}
                </span>
              </div>

              <code>{selectedWorkspace.slug}</code>
            </div>
          </div>

          <a
            class="open-workspace-link"
            href={`/projects?workspace_id=${selectedWorkspace.id}`}
          >
            Open projects

            <svg viewBox="0 0 24 24" aria-hidden="true">
              <path d="m9 18 6-6-6-6"></path>
            </svg>
          </a>
        </div>

        <dl class="workspace-summary">
          <div>
            <dt>Your role</dt>
            <dd>{roleLabel(currentRole)}</dd>
          </div>

          <div>
            <dt>Access</dt>
            <dd>
              {accessLabel(selectedWorkspace.access_scope)}
            </dd>
          </div>

          <div>
            <dt>Projects</dt>
            <dd>{projects.length}</dd>
          </div>

          <div>
            <dt>Active members</dt>
            <dd>{activeMembers}</dd>
          </div>

          {#if canTokens}
            <div>
              <dt>Active tokens</dt>
              <dd>{activeTokens}</dd>
            </div>
          {/if}
        </dl>

        <div class="workspace-id-row">
          <span>Workspace ID</span>

          <code>{selectedWorkspace.id}</code>

          <button
            class:copied={copiedWorkspaceId}
            class="copy-id-button"
            type="button"
            aria-label={
              copiedWorkspaceId
                ? 'Workspace ID copied'
                : 'Copy workspace ID'
            }
            title={
              copiedWorkspaceId
                ? 'Copied'
                : 'Copy workspace ID'
            }
            onclick={copyWorkspaceId}
          >
            {#if copiedWorkspaceId}
              <svg viewBox="0 0 24 24" aria-hidden="true">
                <path d="m5 12 4 4L19 6"></path>
              </svg>
            {:else}
              <svg viewBox="0 0 24 24" aria-hidden="true">
                <rect
                  x="8"
                  y="8"
                  width="11"
                  height="11"
                  rx="2"
                ></rect>

                <path
                  d="M16 8V5a2 2 0 0 0-2-2H5a2 2 0 0 0-2 2v9a2 2 0 0 0 2 2h3"
                ></path>
              </svg>
            {/if}
          </button>
        </div>

        <nav
          class="workspace-actions"
          aria-label="Workspace actions"
        >
          <a
            href={`/projects?workspace_id=${selectedWorkspace.id}`}
          >
            Projects
          </a>

          <a
            href={`/packages?workspace_id=${selectedWorkspace.id}`}
          >
            Packages
          </a>

          {#if canMembers}
            <a
              href={`/members?workspace_id=${selectedWorkspace.id}`}
            >
              Manage members
            </a>
          {/if}

          {#if canTokens}
            <a
              href={`/tokens?workspace_id=${selectedWorkspace.id}`}
            >
              Manage tokens
            </a>
          {/if}
        </nav>
      </section>

      {#if !canProjects && !canMembers && !canTokens}
        <ReadOnlyNotice
          message="Your role can review this workspace, but cannot make administrative changes."
        />
      {/if}

      {#if canManageWorkspace()}
        <section
          class="detail-section workspace-settings-section"
          aria-labelledby="workspace-settings-title"
        >
          <div class="section-header">
            <div>
              <h2 id="workspace-settings-title">Workspace settings</h2>
              <p>Update the workspace identity and logo.</p>
            </div>
          </div>

          <div class="workspace-settings-grid">
            <div class="avatar-control">
              <span class="workspace-mark workspace-mark--large">
                {#if workspaceAvatarUrl(selectedWorkspace)}
                  <img src={workspaceAvatarUrl(selectedWorkspace)} alt="" aria-hidden="true" />
                {:else}
                  {workspaceInitial(selectedWorkspace)}
                {/if}
              </span>

              <div>
                <label class="avatar-upload-button">
                  <span>{uploadingAvatar ? 'Uploading...' : 'Upload logo'}</span>
                  <input
                    type="file"
                    accept="image/jpeg,image/png,image/webp"
                    disabled={uploadingAvatar}
                    onchange={handleAvatarUpload}
                  />
                </label>

                {#if selectedWorkspace.avatar_url}
                  <button
                    class="secondary-button"
                    type="button"
                    disabled={uploadingAvatar}
                    onclick={removeAvatar}
                  >
                    Remove logo
                  </button>
                {/if}

                <p>JPG, PNG or WebP. Max 2MB.</p>
              </div>
            </div>

            <form
              class="workspace-settings-form"
              onsubmit={(event) => {
                event.preventDefault();
                void saveWorkspaceSettings();
              }}
            >
              <label>
                Name
                <input bind:value={editName} required autocomplete="off" />
              </label>

              <label>
                Slug
                <input bind:value={editSlug} required autocomplete="off" />
              </label>

              <button
                type="submit"
                disabled={savingSettings || !editName.trim() || !editSlug.trim()}
              >
                {savingSettings ? 'Saving...' : 'Save changes'}
              </button>
            </form>
          </div>
        </section>
      {/if}

      {#if canOwnWorkspace()}
        <section
          class="detail-section danger-zone"
          aria-labelledby="workspace-danger-title"
        >
          <div class="section-header">
            <div>
              <h2 id="workspace-danger-title">Danger zone</h2>
              <p>Suspending stops write actions. Deleting hides the workspace from normal use while keeping records for safety.</p>
            </div>
          </div>

          <div class="danger-actions">
            {#if workspaceStatus(selectedWorkspace) === 'suspended'}
              <button
                class="secondary-button"
                type="button"
                disabled={savingSettings}
                onclick={() => changeWorkspaceState('reactivate')}
              >
                Reactivate workspace
              </button>
            {:else}
              <button
                class="secondary-button"
                type="button"
                disabled={savingSettings}
                onclick={() => changeWorkspaceState('suspend')}
              >
                Suspend workspace
              </button>
            {/if}

            <button
              class="danger-button"
              type="button"
              disabled={savingSettings}
              onclick={() => changeWorkspaceState('delete')}
            >
              Delete workspace
            </button>
          </div>
        </section>
      {/if}


      <div class="workspace-content-grid">
        <section
          class="detail-section projects-section"
          aria-labelledby="projects-title"
        >
          <div class="section-header">
            <div>
              <h2 id="projects-title">Projects</h2>
              <p>Projects connected to this workspace.</p>
            </div>

            <div class="section-header__actions">
              <span class="section-count">
                {projects.length}
              </span>

              <a
                class="section-link"
                href={`/projects?workspace_id=${selectedWorkspace.id}`}
              >
                View all
              </a>
            </div>
          </div>

          {#if loadingDetail}
            <p class="detail-loading">
              Loading projects…
            </p>
          {:else if projects.length === 0}
            <div class="section-empty">
              <EmptyState
                title="No projects"
                body={canProjects
                  ? 'Run vix cloud init in a local project.'
                  : 'No projects are currently visible.'}
              />
            </div>
          {:else}
            <div class="resource-list">
              {#each projects as project (project.id)}
                <a
                  class="resource-row"
                  href={`/projects?workspace_id=${selectedWorkspace.id}&project_id=${project.id}`}
                >
                  <div>
                    <strong>{project.name}</strong>
                    <code>{project.slug}</code>
                  </div>

                  <svg viewBox="0 0 24 24" aria-hidden="true">
                    <path d="m9 18 6-6-6-6"></path>
                  </svg>
                </a>
              {/each}
            </div>
          {/if}
        </section>

        <div class="secondary-column">
          <section
            class="detail-section"
            aria-labelledby="members-title"
          >
            <div class="section-header">
              <div>
                <h2 id="members-title">Members</h2>
                <p>People with workspace access.</p>
              </div>

              <div class="section-header__actions">
                <span class="section-count">
                  {members.length}
                </span>

                <a
                  class="section-link"
                  href={`/members?workspace_id=${selectedWorkspace.id}`}
                >
                  View all
                </a>
              </div>
            </div>

            {#if loadingDetail}
              <p class="detail-loading">
                Loading members…
              </p>
            {:else if members.length === 0}
              <div class="section-empty">
                <EmptyState
                  title="No members"
                  body={canMembers
                    ? 'Invite the first member from the Members page.'
                    : 'No members are currently visible.'}
                />
              </div>
            {:else}
              <div class="member-list">
                {#each members as member (member.id)}
                  <div class="member-row">
                    <div>
                      <strong>
                        {member.email || 'Member'}
                      </strong>

                      <span>
                        {roleLabel(member.role)}
                      </span>
                    </div>

                    <span
                      class:inactive={
                        member.status !== 'active'
                      }
                      class="item-state"
                    >
                      {statusLabel(member.status)}
                    </span>
                  </div>
                {/each}
              </div>

              {#if inactiveMembers > 0}
                <p class="section-footnote">
                  {inactiveMembers}
                  {inactiveMembers === 1
                    ? ' inactive member'
                    : ' inactive members'}
                </p>
              {/if}
            {/if}
          </section>

          <section
            class="detail-section"
            aria-labelledby="tokens-title"
          >
            <div class="section-header">
              <div>
                <h2 id="tokens-title">CLI tokens</h2>
                <p>Credentials connected to this workspace.</p>
              </div>

              {#if canTokens}
                <div class="section-header__actions">
                  <span class="section-count">
                    {tokens.length}
                  </span>

                  <a
                    class="section-link"
                    href={`/tokens?workspace_id=${selectedWorkspace.id}`}
                  >
                    View all
                  </a>
                </div>
              {/if}
            </div>

            {#if !canTokens}
              <div class="section-notice">
                <ReadOnlyNotice
                  message="Only workspace owners and admins can view CLI tokens."
                />
              </div>
            {:else if loadingDetail}
              <p class="detail-loading">
                Loading tokens…
              </p>
            {:else if tokens.length === 0}
              <div class="section-empty">
                <EmptyState
                  title="No tokens"
                  body="Create a token to connect the Vix CLI."
                />
              </div>
            {:else}
              <div class="token-list">
                {#each tokens as token (token.id)}
                  <a
                    class="token-row"
                    href={`/tokens?workspace_id=${selectedWorkspace.id}`}
                  >
                    <strong>{token.name}</strong>

                    <span
                      class:inactive={token.revoked}
                      class="item-state"
                    >
                      {tokenStatus(token)}
                    </span>
                  </a>
                {/each}
              </div>
            {/if}
          </section>
        </div>
      </div>
    {/if}
  </main>
</div>

<style>
  .page-description {
    max-width: 720px;
    margin: -8px 0 18px;
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.6;
  }

  .workspace-layout {
    display: grid;
    grid-template-columns: 270px minmax(0, 1fr);
    gap: 16px;
    align-items: start;
  }

  /* Workspace directory */

  .workspace-directory {
    position: sticky;
    top: 24px;
    min-width: 0;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    overflow: hidden;
  }

  .directory-header,
  .section-header {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 14px;
    padding: 14px 15px;
    border-bottom: 1px solid var(--line-soft);
  }

  .directory-header > div,
  .section-header > div:first-child {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .directory-header h2,
  .section-header h2 {
    font-size: 13px;
  }

  .directory-header p,
  .section-header p {
    color: var(--text-muted);
    font-size: 11px;
    line-height: 1.45;
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

  .workspace-search {
    position: relative;
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
    font-size: 12px;
  }

  .workspace-options {
    display: grid;
    max-height: calc(100vh - 220px);
    overflow-y: auto;
  }

  .workspace-option {
    display: grid;
    grid-template-columns: 30px minmax(0, 1fr) auto;
    gap: 9px;
    align-items: center;
    width: 100%;
    min-height: 56px;
    padding: 9px 11px;
    border: 0;
    border-bottom: 1px solid var(--line-soft);
    border-radius: 0;
    background: transparent;
    color: var(--text);
    text-align: left;
  }

  .workspace-option:last-child {
    border-bottom: 0;
  }

  .workspace-option:hover:not(:disabled) {
    background: var(--bg-elevated);
  }

  .workspace-option.selected {
    background: var(--brand-faint);
    box-shadow: inset 2px 0 0 var(--brand);
  }

  .workspace-option__mark {
    display: grid;
    width: 30px;
    height: 30px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    background: var(--bg-elevated);
    color: var(--text-soft);
    font-size: 11px;
    font-weight: 650;
  }

  .workspace-option.selected .workspace-option__mark {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-bright);
  }

  .workspace-option__content {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .workspace-option__content strong {
    overflow: hidden;
    color: var(--text);
    font-size: 12px;
    font-weight: 600;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .workspace-option.selected .workspace-option__content strong {
    color: var(--brand-bright);
  }

  .workspace-option__content code {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 9.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .workspace-option__role {
    color: var(--text-muted);
    font-size: 10px;
  }

  .directory-loading {
    padding: 18px 14px;
    color: var(--text-muted);
    font-size: 12px;
  }

  .directory-empty {
    padding: 10px;
  }

  /* Workspace detail */

  .workspace-detail {
    display: grid;
    min-width: 0;
    gap: 16px;
  }

  .workspace-overview,
  .detail-section {
    min-width: 0;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    overflow: hidden;
  }

  .workspace-overview {
    display: grid;
  }

  .workspace-overview__header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    padding: 16px;
    border-bottom: 1px solid var(--line-soft);
  }

  .workspace-identity {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 11px;
  }

  .workspace-mark {
    display: grid;
    width: 40px;
    height: 40px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    background: var(--bg-elevated);
    color: var(--text-soft);
    font-size: 14px;
    font-weight: 650;
  }

  .workspace-identity > div {
    display: grid;
    min-width: 0;
    gap: 4px;
  }

  .workspace-name-line {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 8px;
    flex-wrap: wrap;
  }

  .workspace-name-line h2 {
    color: var(--text);
    font-size: 16px;
    font-weight: 600;
    overflow-wrap: anywhere;
  }

  .workspace-identity code {
    color: var(--text-muted);
    font-size: 10.5px;
    overflow-wrap: anywhere;
  }

  .workspace-state,
  .item-state {
    display: inline-flex;
    width: fit-content;
    min-height: 20px;
    align-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    background: var(--bg-elevated);
    color: var(--text-soft);
    padding: 0 7px;
    font-size: 10px;
    white-space: nowrap;
  }

  .workspace-state.inactive,
  .item-state.inactive {
    border-color: var(--danger-line);
    background: var(--danger-faint);
    color: var(--danger);
  }

  .open-workspace-link,
  .section-link,
  .workspace-actions a {
    color: var(--link);
    font-weight: 550;
  }

  .open-workspace-link:hover,
  .section-link:hover,
  .workspace-actions a:hover {
    color: var(--link-hover);
  }

  .open-workspace-link {
    display: inline-flex;
    flex: 0 0 auto;
    align-items: center;
    gap: 5px;
    font-size: 11.5px;
  }

  .open-workspace-link svg,
  .resource-row > svg {
    width: 15px;
    height: 15px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .workspace-summary {
    display: grid;
    grid-template-columns: repeat(5, minmax(0, 1fr));
    margin: 0;
    border-bottom: 1px solid var(--line-soft);
  }

  .workspace-summary > div {
    display: grid;
    min-width: 0;
    gap: 4px;
    padding: 12px 14px;
    border-right: 1px solid var(--line-soft);
  }

  .workspace-summary > div:last-child {
    border-right: 0;
  }

  .workspace-summary dt {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .workspace-summary dd {
    margin: 0;
    color: var(--text-soft);
    font-size: 12px;
    font-weight: 550;
    overflow-wrap: anywhere;
  }

  .workspace-id-row {
    display: grid;
    grid-template-columns: auto minmax(0, 1fr) 30px;
    gap: 10px;
    align-items: center;
    padding: 10px 14px;
    border-bottom: 1px solid var(--line-soft);
  }

  .workspace-id-row > span {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .workspace-id-row > code {
    min-width: 0;
    overflow: hidden;
    color: var(--text-soft);
    font-size: 10.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .copy-id-button {
    display: grid;
    width: 28px;
    height: 28px;
    min-height: 28px;
    place-items: center;
    border: 1px solid var(--line-strong);
    background: transparent;
    color: var(--text-muted);
    padding: 0;
  }

  .copy-id-button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
  }

  .copy-id-button.copied {
    border-color: var(--brand-line);
    color: var(--brand);
  }

  .copy-id-button svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .workspace-actions {
    display: flex;
    align-items: center;
    gap: 18px;
    flex-wrap: wrap;
    padding: 11px 14px;
  }

  .workspace-actions a {
    font-size: 11.5px;
  }

  /* Workspace content */

  .workspace-content-grid {
    display: grid;
    grid-template-columns: minmax(0, 1.35fr) minmax(300px, 0.85fr);
    gap: 16px;
    align-items: start;
  }

  .secondary-column {
    display: grid;
    min-width: 0;
    gap: 16px;
  }

  .section-header__actions {
    display: flex;
    align-items: center;
    gap: 10px;
    flex: 0 0 auto;
  }

  .section-link {
    font-size: 11px;
  }

  .resource-list,
  .member-list,
  .token-list {
    display: grid;
  }

  .resource-row,
  .member-row,
  .token-row {
    min-width: 0;
    border-bottom: 1px solid var(--line-soft);
  }

  .resource-row:last-child,
  .member-row:last-child,
  .token-row:last-child {
    border-bottom: 0;
  }

  .resource-row {
    display: flex;
    min-height: 55px;
    align-items: center;
    justify-content: space-between;
    gap: 14px;
    padding: 10px 15px;
    color: var(--link);
  }

  .resource-row:hover,
  .token-row:hover {
    background: var(--info-faint);
  }

  .resource-row > div {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .resource-row strong {
    color: var(--link);
    font-size: 12px;
    font-weight: 600;
    overflow-wrap: anywhere;
  }

  .resource-row:hover strong {
    color: var(--link-hover);
  }

  .resource-row code {
    color: var(--text-muted);
    font-size: 10px;
    overflow-wrap: anywhere;
  }

  .member-row,
  .token-row {
    display: flex;
    min-height: 51px;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    padding: 9px 14px;
  }

  .member-row > div {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .member-row strong {
    color: var(--text);
    font-size: 11.5px;
    font-weight: 550;
    overflow-wrap: anywhere;
  }

  .member-row > div span {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .token-row strong {
    color: var(--link);
    font-size: 11.5px;
    font-weight: 550;
    overflow-wrap: anywhere;
  }

  .token-row:hover strong {
    color: var(--link-hover);
  }

  .section-footnote {
    border-top: 1px solid var(--line-soft);
    padding: 9px 14px;
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .section-empty,
  .section-notice {
    padding: 11px;
  }

  .detail-loading {
    min-height: 72px;
    padding: 20px 15px;
    color: var(--text-muted);
    font-size: 12px;
  }

  .empty-detail {
    padding: 14px;
  }

  @media (max-width: 1080px) {
    .workspace-summary {
      grid-template-columns: repeat(3, minmax(0, 1fr));
    }

    .workspace-summary > div:nth-child(3) {
      border-right: 0;
    }

    .workspace-summary > div:nth-child(-n + 3) {
      border-bottom: 1px solid var(--line-soft);
    }

    .workspace-content-grid {
      grid-template-columns: 1fr;
    }

    .secondary-column {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }
  }

  @media (max-width: 880px) {
    .workspace-layout {
      grid-template-columns: 1fr;
    }

    .workspace-directory {
      position: static;
    }

    .workspace-options {
      max-height: 280px;
    }
  }

  @media (max-width: 680px) {
    .workspace-overview__header {
      align-items: flex-start;
      flex-direction: column;
    }

    .workspace-summary {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }

    .workspace-summary > div,
    .workspace-summary > div:nth-child(3) {
      border-right: 1px solid var(--line-soft);
      border-bottom: 1px solid var(--line-soft);
    }

    .workspace-summary > div:nth-child(even) {
      border-right: 0;
    }

    .workspace-summary > div:last-child {
      border-bottom: 0;
    }

    .secondary-column {
      grid-template-columns: 1fr;
    }

    .workspace-id-row {
      grid-template-columns: 1fr 30px;
    }

    .workspace-id-row > span {
      grid-column: 1 / -1;
    }
  }

  @media (max-width: 480px) {
    .workspace-summary {
      grid-template-columns: 1fr;
    }

    .workspace-summary > div,
    .workspace-summary > div:nth-child(even) {
      border-right: 0;
    }

    .workspace-option {
      grid-template-columns: 30px minmax(0, 1fr);
    }

    .workspace-option__role {
      display: none;
    }

    .workspace-actions {
      align-items: flex-start;
      flex-direction: column;
      gap: 9px;
    }
  }
</style>
