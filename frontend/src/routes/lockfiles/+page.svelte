<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import {
    deleteLockfile,
    listLockfiles,
    uploadLockfile
  } from '$lib/api/lockfiles';
  import { listProjects } from '$lib/api/projects';
  import { listWorkspaces } from '$lib/api/workspaces';
  import {
    ApiError,
    type Lockfile,
    type Project,
    type Workspace
  } from '$lib/api/types';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import { canUploadLockfile } from '$lib/permissions';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  let workspaces: Workspace[] = [];
  let projects: Project[] = [];
  let lockfiles: Lockfile[] = [];

  let selectedWorkspaceId = '';
  let selectedProjectId = '';

  let loading = true;
  let loadingLockfiles = false;
  let initialized = false;
  let projectListRequestId = 0;
  let saving = false;
  let busyLockfileId = '';

  let error = '';
  let success = '';

  let projectQuery = '';
  let lockfileQuery = '';
  let sourceFilter = 'all';

  let showUploadForm = false;
  let openJsonId = '';
  let copiedKey = '';
  let copyResetTimer: ReturnType<typeof setTimeout> | undefined;

  let checksumSha256 = '';
  let lockfileJson = '{}';
  let source = 'dashboard';

  let lockfileRequestId = 0;

  $: selectedWorkspace =
    workspaces.find(
      (workspace) => workspace.id === selectedWorkspaceId
    ) ?? null;

  $: selectedProject =
    projects.find(
      (project) => project.id === selectedProjectId
    ) ?? null;

  $: currentRole =
    selectedWorkspace?.current_user_role ?? 'viewer';

  $: canUpload = canUploadLockfile(currentRole);

  $: globalWorkspaceId =
    $workspaceContext.selectedWorkspace?.id ?? '';

  $: if (
    initialized &&
    globalWorkspaceId &&
    globalWorkspaceId !== selectedWorkspaceId &&
    workspaces.some((workspace) => workspace.id === globalWorkspaceId)
  ) {
    void switchWorkspace(globalWorkspaceId);
  }

  $: visibleProjects = projects.filter((project) => {
    const query = projectQuery.trim().toLowerCase();

    if (!query) {
      return true;
    }

    return (
      project.name.toLowerCase().includes(query) ||
      project.slug.toLowerCase().includes(query)
    );
  });

  $: sourceOptions = Array.from(
    new Set(
      lockfiles
        .map((lockfile) => lockfile.source)
        .filter((value): value is string => Boolean(value))
    )
  );

  $: visibleLockfiles = lockfiles.filter((lockfile) => {
    const query = lockfileQuery.trim().toLowerCase();

    const matchesQuery =
      !query ||
      lockfile.checksum_sha256.toLowerCase().includes(query) ||
      lockfile.source?.toLowerCase().includes(query) ||
      lockfile.lockfile_json?.toLowerCase().includes(query);

    const matchesSource =
      sourceFilter === 'all' ||
      lockfile.source === sourceFilter;

    return matchesQuery && matchesSource;
  });

  $: latestLockfile = lockfiles[0] ?? null;

  function projectInitial(project: Project) {
    return project.name
      .slice(0, 1)
      .toUpperCase();
  }

  function shortChecksum(value: string) {
    if (value.length <= 24) {
      return value;
    }

    return `${value.slice(0, 14)}…${value.slice(-8)}`;
  }

  function prettyJson(value?: string) {
    if (!value) {
      return '{}';
    }

    try {
      return JSON.stringify(
        JSON.parse(value),
        null,
        2
      );
    } catch {
      return value;
    }
  }

  function formatDate(value?: string | number) {
    if (value === undefined || value === null || value === '') {
      return 'Unknown date';
    }

    let date: Date;

    if (typeof value === 'number') {
      date = new Date(
        value < 1_000_000_000_000
          ? value * 1000
          : value
      );
    } else {
      const numericValue = Number(value);

      if (
        Number.isFinite(numericValue) &&
        value.trim() !== ''
      ) {
        date = new Date(
          numericValue < 1_000_000_000_000
            ? numericValue * 1000
            : numericValue
        );
      } else {
        date = new Date(value);
      }
    }

    if (Number.isNaN(date.getTime())) {
      return 'Unknown date';
    }

    return new Intl.DateTimeFormat(undefined, {
      dateStyle: 'medium',
      timeStyle: 'short'
    }).format(date);
  }

  function resetUploadForm() {
    checksumSha256 = '';
    lockfileJson = '{}';
    source = 'dashboard';
  }

  function toggleUploadForm() {
    showUploadForm = !showUploadForm;
    error = '';
    success = '';

    if (!showUploadForm) {
      resetUploadForm();
    }
  }

  function closeUploadForm() {
    if (saving) {
      return;
    }

    showUploadForm = false;
    resetUploadForm();
    error = '';
  }

  function handleUploadSubmit(event: SubmitEvent) {
    event.preventDefault();
    void submitLockfile();
  }

  async function deleteLockfileRecord(lockfile: Lockfile) {
    if (!selectedWorkspaceId || !selectedProjectId || !canUpload) {
      return;
    }

    if (!window.confirm('Hide this lockfile record? The content and checksum are kept for history.')) {
      return;
    }

    busyLockfileId = lockfile.id;
    error = '';
    success = '';

    try {
      await deleteLockfile(selectedWorkspaceId, selectedProjectId, lockfile.id);
      lockfiles = lockfiles.filter((item) => item.id !== lockfile.id);
      success = 'Lockfile hidden.';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to hide lockfile.';
    } finally {
      busyLockfileId = '';
    }
  }

  function toggleJson(lockfileId: string) {
    openJsonId =
      openJsonId === lockfileId
        ? ''
        : lockfileId;
  }

  function updatePageUrl(projectId?: string) {
    const url = new URL(window.location.href);

    if (selectedWorkspaceId) {
      url.searchParams.set(
        'workspace_id',
        selectedWorkspaceId
      );
    } else {
      url.searchParams.delete('workspace_id');
    }

    if (projectId) {
      url.searchParams.set(
        'project_id',
        projectId
      );
    } else {
      url.searchParams.delete('project_id');
    }

    history.replaceState(
      null,
      '',
      `${url.pathname}${url.search}`
    );
  }

  async function copyValue(
    key: string,
    value: string
  ) {
    try {
      await navigator.clipboard.writeText(value);

      copiedKey = key;

      if (copyResetTimer) {
        clearTimeout(copyResetTimer);
      }

      copyResetTimer = setTimeout(() => {
        copiedKey = '';
      }, 1800);
    } catch {
      error = 'Unable to copy this value.';
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
      const params = new URLSearchParams(
        window.location.search
      );

      const workspaceData = await listWorkspaces(user.id);

      workspaces = workspaceData.workspaces;

      const requestedWorkspaceId =
        params.get('workspace_id');

      const storedWorkspaceId =
        $workspaceContext.selectedWorkspace?.id;

      selectedWorkspaceId =
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
            : workspaces[0]?.id ?? '';

      workspaceContext.setWorkspaces(
        workspaces,
        selectedWorkspaceId
      );

      await loadProjects(
        params.get('project_id')
      );
      initialized = true;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load lockfiles.';
    } finally {
      loading = false;
    }
  }

  async function handleWorkspaceChange(
    event: Event
  ) {
    const select =
      event.currentTarget as HTMLSelectElement;

    await switchWorkspace(select.value);
  }

  async function loadProjects(
    requestedProjectId?: string | null
  ) {
    const requestId = ++projectListRequestId;

    if (!selectedWorkspaceId) {
      projects = [];
      lockfiles = [];
      selectedProjectId = '';
      return;
    }

    workspaceContext.setSelectedWorkspace(
      selectedWorkspaceId
    );

    const data = await listProjects(
      selectedWorkspaceId
    );

    if (requestId !== projectListRequestId) {
      return;
    }

    projects = data.projects;

    selectedProjectId =
      (
        requestedProjectId &&
        projects.some(
          (project) =>
            project.id === requestedProjectId
        )
      )
        ? requestedProjectId
        : projects[0]?.id ?? '';

    if (selectedProjectId) {
      updatePageUrl(selectedProjectId);
    }

    await loadLockfiles(selectedProjectId);
  }

  async function switchWorkspace(workspaceId: string) {
    selectedWorkspaceId = workspaceId;
    selectedProjectId = '';
    projects = [];
    lockfiles = [];
    projectQuery = '';
    lockfileQuery = '';
    sourceFilter = 'all';
    openJsonId = '';
    showUploadForm = false;
    resetUploadForm();
    error = '';
    success = '';
    loading = true;

    const url = new URL(window.location.href);
    url.searchParams.set('workspace_id', workspaceId);
    url.searchParams.delete('project_id');
    history.replaceState(null, '', `${url.pathname}${url.search}`);

    try {
      await loadProjects();
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load lockfiles.';
    } finally {
      loading = false;
    }
  }

  async function selectProject(project: Project) {
    if (
      project.id === selectedProjectId ||
      loadingLockfiles
    ) {
      return;
    }

    selectedProjectId = project.id;

    lockfileQuery = '';
    sourceFilter = 'all';
    openJsonId = '';
    copiedKey = '';
    error = '';
    success = '';

    showUploadForm = false;
    resetUploadForm();

    updatePageUrl(project.id);

    await loadLockfiles(project.id);
  }

  async function loadLockfiles(projectId: string) {
    if (!selectedWorkspaceId || !projectId) {
      lockfiles = [];
      return;
    }

    const requestId = ++lockfileRequestId;
    const workspaceId = selectedWorkspaceId;

    loadingLockfiles = true;
    error = '';
    lockfiles = [];

    try {
      const data = await listLockfiles(
        workspaceId,
        projectId
      );

      if (
        requestId !== lockfileRequestId ||
        selectedWorkspaceId !== workspaceId ||
        selectedProjectId !== projectId
      ) {
        return;
      }

      lockfiles = data.lockfiles;
    } catch (err) {
      if (requestId !== lockfileRequestId) {
        return;
      }

      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load lockfile history.';
    } finally {
      if (requestId === lockfileRequestId) {
        loadingLockfiles = false;
      }
    }
  }

  async function submitLockfile() {
    const user = $auth.user;

    const cleanChecksum =
      checksumSha256.trim();

    const cleanSource =
      source.trim() || 'dashboard';

    if (
      !user ||
      !selectedWorkspaceId ||
      !selectedProjectId ||
      !cleanChecksum ||
      !canUpload
    ) {
      return;
    }

    if (!/^[a-fA-F0-9]{64}$/.test(cleanChecksum)) {
      error =
        'Checksum SHA256 must contain exactly 64 hexadecimal characters.';
      return;
    }

    let normalizedJson = '';

    try {
      normalizedJson = JSON.stringify(
        JSON.parse(lockfileJson || '{}')
      );
    } catch {
      error = 'Lockfile JSON is not valid.';
      return;
    }

    saving = true;
    error = '';
    success = '';

    try {
      const uploaded = await uploadLockfile({
        workspaceId: selectedWorkspaceId,
        projectId: selectedProjectId,
        uploadedByUserId: user.id,
        checksumSha256: cleanChecksum,
        lockfileJson: normalizedJson,
        source: cleanSource
      });

      lockfiles = [
        uploaded.lockfile,
        ...lockfiles
      ];

      success = 'Lockfile uploaded.';

      resetUploadForm();
      showUploadForm = false;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to upload lockfile.';
    } finally {
      saving = false;
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
  <title>Lockfiles | Softadastra Cloud</title>
</svelte:head>

<PageHeader
  eyebrow="Dependencies"
  title="Lockfiles"
  workspaceName={selectedWorkspace?.name ?? ''}
  role={currentRole}
>
  {#if canUpload && selectedProject && !loading}
    <button
      class:secondary-button={showUploadForm}
      class="upload-lockfile-button"
      type="button"
      onclick={toggleUploadForm}
    >
      {showUploadForm ? 'Close' : 'Upload lockfile'}
    </button>
  {/if}
</PageHeader>

<p class="page-description">
  Review dependency states uploaded for each project and inspect
  their exact JSON content.
</p>

<InlineError message={error} />

{#if success}
  <p class="success-message" role="status">
    {success}
  </p>
{/if}

{#if showUploadForm && canUpload && selectedProject}
  <section
    class="upload-section"
    aria-labelledby="upload-lockfile-title"
  >
    <div class="upload-heading">
      <div>
        <h2 id="upload-lockfile-title">
          Upload a lockfile
        </h2>

        <p>
          Uploading to
          <strong>{selectedProject.name}</strong>.
          The checksum must match the submitted JSON.
        </p>
      </div>
    </div>

    <form
      class="upload-form"
      onsubmit={handleUploadSubmit}
    >
      <label class="checksum-field">
        Checksum SHA256

        <input
          bind:value={checksumSha256}
          maxlength="64"
          minlength="64"
          placeholder="64 hexadecimal characters"
          required
          autocomplete="off"
          spellcheck="false"
        />
      </label>

      <label>
        Source

        <input
          bind:value={source}
          list="lockfile-sources"
          placeholder="dashboard"
          required
          autocomplete="off"
        />

        <datalist id="lockfile-sources">
          <option value="dashboard"></option>
          <option value="cli"></option>
          <option value="ci"></option>
          <option value="vix cloud"></option>
        </datalist>
      </label>

      <label class="json-field">
        Lockfile JSON

        <textarea
          bind:value={lockfileJson}
          rows="12"
          spellcheck="false"
        ></textarea>
      </label>

      <div class="upload-actions">
        <button
          class="secondary-button"
          type="button"
          disabled={saving}
          onclick={closeUploadForm}
        >
          Cancel
        </button>

        <button
          type="submit"
          disabled={
            saving ||
            !selectedProjectId ||
            checksumSha256.trim().length !== 64
          }
        >
          {saving
            ? 'Uploading…'
            : 'Upload lockfile'}
        </button>
      </div>
    </form>
  </section>
{/if}

<div class="lockfiles-layout">
  <aside
    class="project-directory"
    aria-label="Project directory"
  >
    <div class="directory-header">
      <div>
        <h2>Projects</h2>
        <p>Choose the project history to inspect.</p>
      </div>

      <span class="section-count">
        {projects.length}
      </span>
    </div>

    {#if workspaces.length > 1}
      <div class="workspace-selector">
        <label>
          Workspace

          <select
            value={selectedWorkspaceId}
            onchange={handleWorkspaceChange}
          >
            {#each workspaces as workspace (workspace.id)}
              <option value={workspace.id}>
                {workspace.name}
              </option>
            {/each}
          </select>
        </label>
      </div>
    {/if}

    {#if projects.length > 1}
      <div class="project-search">
        <svg viewBox="0 0 24 24" aria-hidden="true">
          <circle cx="11" cy="11" r="7"></circle>
          <path d="m20 20-4-4"></path>
        </svg>

        <input
          bind:value={projectQuery}
          type="search"
          placeholder="Find a project"
          aria-label="Find a project"
        />
      </div>
    {/if}

    {#if loading}
      <p class="directory-loading">
        Loading projects…
      </p>
    {:else if projects.length === 0}
      <div class="directory-empty">
        <EmptyState
          title="No projects"
          body="Create a project before uploading lockfiles."
        />
      </div>
    {:else if visibleProjects.length === 0}
      <p class="directory-loading">
        No project matches your search.
      </p>
    {:else}
      <div class="project-options">
        {#each visibleProjects as project (project.id)}
          <button
            class:selected={
              project.id === selectedProjectId
            }
            class="project-option"
            type="button"
            aria-pressed={
              project.id === selectedProjectId
            }
            onclick={() => selectProject(project)}
          >
            <span class="project-option__mark">
              {projectInitial(project)}
            </span>

            <span class="project-option__content">
              <strong>{project.name}</strong>
              <code>{project.slug}</code>
            </span>
          </button>
        {/each}
      </div>
    {/if}
  </aside>

  <main class="lockfile-detail">
    {#if loading}
      <section class="detail-section">
        <p class="loading-state">
          Loading lockfiles…
        </p>
      </section>
    {:else if !selectedProject}
      <section class="detail-section empty-detail">
        <EmptyState
          title="Select a project"
          body="Its uploaded lockfile history will appear here."
        />
      </section>
    {:else}
      <section
        class="project-overview"
        aria-labelledby="selected-project-name"
      >
        <div class="project-overview__header">
          <div class="project-identity">
            <span class="project-mark">
              {projectInitial(selectedProject)}
            </span>

            <div>
              <h2 id="selected-project-name">
                {selectedProject.name}
              </h2>

              <code>{selectedProject.slug}</code>
            </div>
          </div>

          <a
            class="project-link"
            href={`/projects?workspace_id=${selectedWorkspaceId}&project_id=${selectedProject.id}`}
          >
            Open project

            <svg viewBox="0 0 24 24" aria-hidden="true">
              <path d="m9 18 6-6-6-6"></path>
            </svg>
          </a>
        </div>

        <dl class="lockfile-summary">
          <div>
            <dt>Lockfiles</dt>
            <dd>{lockfiles.length}</dd>
          </div>

          <div>
            <dt>Sources</dt>
            <dd>{sourceOptions.length}</dd>
          </div>

          <div>
            <dt>Latest source</dt>
            <dd>
              {latestLockfile?.source || '—'}
            </dd>
          </div>

          <div>
            <dt>Latest upload</dt>
            <dd>
              {latestLockfile
                ? formatDate(latestLockfile.created_at)
                : '—'}
            </dd>
          </div>
        </dl>

        <nav
          class="project-links"
          aria-label="Project resources"
        >
          <a
            href={`/projects?workspace_id=${selectedWorkspaceId}&project_id=${selectedProject.id}`}
          >
            Project overview
          </a>

          <a
            href={`/build-reports?workspace_id=${selectedWorkspaceId}&project_id=${selectedProject.id}`}
          >
            Build reports
          </a>

          <a
            href={`/packages?workspace_id=${selectedWorkspaceId}`}
          >
            Workspace packages
          </a>
        </nav>
      </section>

      {#if !canUpload}
        <ReadOnlyNotice
          message="You can review lockfile history, but your role cannot upload new lockfiles."
        />
      {/if}

      <section
        class="detail-section history-section"
        aria-labelledby="lockfile-history-title"
      >
        <div class="section-header">
          <div>
            <h2 id="lockfile-history-title">
              Lockfile history
            </h2>

            <p>
              Uploaded dependency states for this project.
            </p>
          </div>

          <span class="section-count">
            {visibleLockfiles.length}
          </span>
        </div>

        {#if lockfiles.length > 0}
          <div class="history-toolbar">
            <div class="lockfile-search">
              <svg viewBox="0 0 24 24" aria-hidden="true">
                <circle cx="11" cy="11" r="7"></circle>
                <path d="m20 20-4-4"></path>
              </svg>

              <input
                bind:value={lockfileQuery}
                type="search"
                placeholder="Find a checksum or source"
                aria-label="Find a lockfile"
              />
            </div>

            <select
              bind:value={sourceFilter}
              aria-label="Filter lockfiles by source"
            >
              <option value="all">
                All sources
              </option>

              {#each sourceOptions as sourceOption}
                <option value={sourceOption}>
                  {sourceOption}
                </option>
              {/each}
            </select>
          </div>
        {/if}

        {#if loadingLockfiles}
          <p class="loading-state">
            Loading lockfile history…
          </p>
        {:else if lockfiles.length === 0}
          <div class="empty-wrapper">
            <EmptyState
              title="No lockfiles uploaded"
              body={canUpload
                ? 'Upload the first lockfile for this project.'
                : 'No lockfile has been uploaded for this project.'}
            />
          </div>
        {:else if visibleLockfiles.length === 0}
          <div class="no-results">
            <strong>No matching lockfiles</strong>

            <p>
              Change the search text or selected source.
            </p>
          </div>
        {:else}
          <div class="lockfile-table">
            <div class="table-heading">
              <span>Checksum</span>
              <span>Source</span>
              <span>Uploaded</span>
              <span class="align-right">Actions</span>
            </div>

            {#each visibleLockfiles as lockfile (lockfile.id)}
              <div class="lockfile-entry">
                <div class="lockfile-row">
                  <div
                    class="checksum-cell"
                    data-label="Checksum"
                  >
                    <code title={lockfile.checksum_sha256}>
                      {shortChecksum(lockfile.checksum_sha256)}
                    </code>

                    <button
                      class:copied={
                        copiedKey === `checksum:${lockfile.id}`
                      }
                      class="copy-button"
                      type="button"
                      onclick={() =>
                        copyValue(
                          `checksum:${lockfile.id}`,
                          lockfile.checksum_sha256
                        )}
                    >
                      {copiedKey === `checksum:${lockfile.id}`
                        ? 'Copied'
                        : 'Copy'}
                    </button>

                    {#if canUpload}
                      <button
                        class="danger-link"
                        type="button"
                        disabled={busyLockfileId === lockfile.id}
                        onclick={() => deleteLockfileRecord(lockfile)}
                      >
                        Delete
                      </button>
                    {/if}
                  </div>

                  <div
                    class="source-cell"
                    data-label="Source"
                  >
                    <span>
                      {lockfile.source || 'Unknown'}
                    </span>
                  </div>

                  <div
                    class="date-cell"
                    data-label="Uploaded"
                  >
                    {formatDate(lockfile.created_at)}
                  </div>

                  <div
                    class="row-actions"
                    data-label="Actions"
                  >
                    <button
                      class="json-button"
                      type="button"
                      aria-expanded={
                        openJsonId === lockfile.id
                      }
                      onclick={() =>
                        toggleJson(lockfile.id)}
                    >
                      {openJsonId === lockfile.id
                        ? 'Hide JSON'
                        : 'View JSON'}
                    </button>
                  </div>
                </div>

                {#if openJsonId === lockfile.id}
                  <div class="json-panel">
                    <div class="json-panel__header">
                      <div>
                        <strong>Lockfile JSON</strong>

                        <span>
                          {lockfile.source || 'Unknown source'}
                        </span>
                      </div>

                      <button
                        class:copied={
                          copiedKey === `json:${lockfile.id}`
                        }
                        class="copy-json-button"
                        type="button"
                        onclick={() =>
                          copyValue(
                            `json:${lockfile.id}`,
                            prettyJson(lockfile.lockfile_json)
                          )}
                      >
                        {copiedKey === `json:${lockfile.id}`
                          ? 'Copied'
                          : 'Copy JSON'}
                      </button>
                    </div>

                    <pre>{prettyJson(lockfile.lockfile_json)}</pre>
                  </div>
                {/if}
              </div>
            {/each}
          </div>
        {/if}
      </section>
    {/if}
  </main>
</div>

<style>
  .page-description {
    max-width: 740px;
    margin: -8px 0 18px;
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.6;
  }

  .upload-lockfile-button {
    border-color: var(--brand);
    background: var(--brand);
    color: var(--brand-ink);
  }

  .upload-lockfile-button:hover:not(:disabled) {
    border-color: var(--brand-soft);
    background: var(--brand-soft);
  }

  .upload-lockfile-button.secondary-button {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-soft);
  }

  .upload-lockfile-button.secondary-button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
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

  /* Upload form */

  .upload-section {
    display: grid;
    gap: 17px;
    margin-bottom: 18px;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    padding: 18px;
  }

  .upload-heading {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 16px;
  }

  .upload-heading > div {
    display: grid;
    gap: 4px;
  }

  .upload-heading h2 {
    font-size: 14px;
  }

  .upload-heading p {
    color: var(--text-muted);
    font-size: 12px;
    line-height: 1.5;
  }

  .upload-heading strong {
    color: var(--text-soft);
    font-weight: 600;
  }

  .upload-form {
    display: grid;
    grid-template-columns:
      minmax(0, 1fr)
      minmax(180px, 0.4fr);
    gap: 14px;
  }

  .json-field,
  .upload-actions {
    grid-column: 1 / -1;
  }

  .json-field textarea {
    min-height: 230px;
    resize: vertical;
    font-family: var(--font-mono);
    font-size: 11px;
    line-height: 1.55;
    tab-size: 2;
  }

  .upload-actions {
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    border-top: 1px solid var(--line-soft);
    padding-top: 14px;
  }

  /* Main layout */

  .lockfiles-layout {
    display: grid;
    grid-template-columns: 270px minmax(0, 1fr);
    gap: 16px;
    align-items: start;
  }

  /* Project directory */

  .project-directory {
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
  .section-header > div {
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

  .workspace-selector {
    padding: 10px;
    border-bottom: 1px solid var(--line-soft);
  }

  .workspace-selector label {
    gap: 5px;
    color: var(--text-muted);
    font-size: 10px;
  }

  .workspace-selector select {
    min-height: 34px;
    font-size: 12px;
  }

  .project-search {
    position: relative;
    padding: 10px;
    border-bottom: 1px solid var(--line-soft);
  }

  .project-search svg,
  .lockfile-search svg {
    position: absolute;
    top: 50%;
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

  .project-search svg {
    left: 21px;
  }

  .project-search input {
    min-height: 34px;
    padding-left: 33px;
    font-size: 12px;
  }

  .project-options {
    display: grid;
    max-height: calc(100vh - 280px);
    overflow-y: auto;
  }

  .project-option {
    display: grid;
    grid-template-columns: 30px minmax(0, 1fr);
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

  .project-option:last-child {
    border-bottom: 0;
  }

  .project-option:hover:not(:disabled) {
    background: var(--bg-elevated);
  }

  .project-option.selected {
    background: var(--brand-faint);
    box-shadow: inset 2px 0 0 var(--brand);
  }

  .project-option__mark {
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

  .project-option.selected .project-option__mark {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-bright);
  }

  .project-option__content {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .project-option__content strong {
    overflow: hidden;
    color: var(--text);
    font-size: 12px;
    font-weight: 600;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .project-option.selected .project-option__content strong {
    color: var(--brand-bright);
  }

  .project-option__content code {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 9.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .directory-loading {
    padding: 18px 14px;
    color: var(--text-muted);
    font-size: 12px;
  }

  .directory-empty {
    padding: 10px;
  }

  /* Project overview */

  .lockfile-detail {
    display: grid;
    min-width: 0;
    gap: 16px;
  }

  .project-overview,
  .detail-section {
    min-width: 0;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    overflow: hidden;
  }

  .project-overview__header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    padding: 16px;
    border-bottom: 1px solid var(--line-soft);
  }

  .project-identity {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 11px;
  }

  .project-mark {
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

  .project-identity > div {
    display: grid;
    min-width: 0;
    gap: 4px;
  }

  .project-identity h2 {
    color: var(--text);
    font-size: 16px;
    font-weight: 600;
    overflow-wrap: anywhere;
  }

  .project-identity code {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .project-link,
  .project-links a {
    color: var(--link);
    font-weight: 550;
  }

  .project-link:hover,
  .project-links a:hover {
    color: var(--link-hover);
  }

  .project-link {
    display: inline-flex;
    flex: 0 0 auto;
    align-items: center;
    gap: 5px;
    font-size: 11.5px;
  }

  .project-link svg {
    width: 15px;
    height: 15px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .lockfile-summary {
    display: grid;
    grid-template-columns: repeat(4, minmax(0, 1fr));
    margin: 0;
    border-bottom: 1px solid var(--line-soft);
  }

  .lockfile-summary > div {
    display: grid;
    min-width: 0;
    gap: 4px;
    padding: 12px 14px;
    border-right: 1px solid var(--line-soft);
  }

  .lockfile-summary > div:last-child {
    border-right: 0;
  }

  .lockfile-summary dt {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .lockfile-summary dd {
    margin: 0;
    color: var(--text-soft);
    font-size: 12px;
    font-weight: 550;
    overflow-wrap: anywhere;
  }

  .project-links {
    display: flex;
    align-items: center;
    gap: 18px;
    flex-wrap: wrap;
    padding: 11px 14px;
  }

  .project-links a {
    font-size: 11.5px;
  }

  /* History toolbar */

  .history-toolbar {
    display: grid;
    grid-template-columns:
      minmax(220px, 1fr)
      minmax(150px, 210px);
    gap: 10px;
    padding: 10px 12px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .lockfile-search {
    position: relative;
  }

  .lockfile-search svg {
    left: 11px;
  }

  .lockfile-search input {
    min-height: 34px;
    padding-left: 33px;
    font-size: 12px;
  }

  .history-toolbar select {
    min-height: 34px;
    font-size: 12px;
  }

  /* Lockfile table */
  .lockfile-table {
    width: 100%;
    overflow-x: auto;
    overflow-y: hidden;
    scrollbar-width: thin;
    scrollbar-color: var(--line-strong) transparent;
  }

  .lockfile-table::-webkit-scrollbar {
    height: 8px;
  }

  .lockfile-table::-webkit-scrollbar-track {
    background: transparent;
  }

  .lockfile-table::-webkit-scrollbar-thumb {
    border: 2px solid transparent;
    border-radius: 999px;
    background: var(--line-strong);
    background-clip: padding-box;
  }

  .lockfile-table::-webkit-scrollbar-thumb:hover {
    background: var(--text-faint);
    background-clip: padding-box;
  }

  .table-heading,
  .lockfile-row {
    display: grid;
    grid-template-columns:
      minmax(270px, 1.2fr)
      minmax(120px, 0.45fr)
      minmax(190px, 0.7fr)
      minmax(120px, auto);
    gap: 16px;
    align-items: center;
    min-width: 790px;
  }

  .table-heading {
    min-height: 35px;
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

  .align-right {
    text-align: right;
  }

  .lockfile-entry {
    border-bottom: 1px solid var(--line-soft);
  }

  .lockfile-entry:last-child {
    border-bottom: 0;
  }

  .lockfile-row {
    min-height: 62px;
    padding: 10px 16px;
  }

  .lockfile-row:hover {
    background: rgba(255, 255, 255, 0.018);
  }

  .checksum-cell {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 8px;
  }

  .checksum-cell code {
    min-width: 0;
    overflow: hidden;
    color: var(--text-soft);
    font-size: 10px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .copy-button,
  .copy-json-button {
    min-height: 28px;
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-muted);
    padding: 0 9px;
    font-size: 10.5px;
    white-space: nowrap;
  }

  .danger-link { min-height: 32px; border: 1px solid color-mix(in srgb, var(--danger) 45%, var(--line)); border-radius: var(--radius-sm); background: transparent; padding: 0 10px; color: var(--danger); font-size: 11px; font-weight: 700; }

  .json-button {
    min-height: 30px;
    border: 1px solid var(--info-line);
    border-radius: var(--radius-sm);
    background: var(--info-faint);
    color: var(--link);
    padding: 0 11px;
    font-size: 11px;
    font-weight: 600;
    white-space: nowrap;
  }
  .copy-button:hover:not(:disabled),
  .copy-json-button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
  }

  .json-button:hover:not(:disabled) {
    border-color: var(--link);
    background: rgba(107, 184, 255, 0.14);
    color: var(--link-hover);
  }

  .copy-button.copied,
  .copy-json-button.copied {
    border-color: var(--brand-line);
    color: var(--brand);
  }

  .source-cell span {
    display: inline-flex;
    width: fit-content;
    min-height: 21px;
    align-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    background: var(--bg-elevated);
    color: var(--text-muted);
    padding: 0 8px;
    font-size: 10.5px;
  }

  .date-cell {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .row-actions {
    display: flex;
    min-width: 120px;
    align-items: center;
    justify-content: flex-end;
  }

  /* JSON preview */

  .json-panel {
    display: grid;
    gap: 0;
    border-top: 1px solid var(--line-soft);
    background: var(--bg-ink);
  }

  .json-panel__header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 14px;
    padding: 10px 14px;
    border-bottom: 1px solid var(--line-soft);
  }

  .json-panel__header > div {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .json-panel__header strong {
    color: var(--text-soft);
    font-size: 11.5px;
    font-weight: 600;
  }

  .json-panel__header span {
    color: var(--text-muted);
    font-size: 10px;
  }

  .json-panel pre {
    max-height: 440px;
    margin: 0;
    overflow: auto;
    padding: 16px;
    color: var(--text-soft);
    font-family: var(--font-mono);
    font-size: 10.5px;
    line-height: 1.65;
    tab-size: 2;
    white-space: pre;
  }

  .loading-state {
    min-height: 90px;
    padding: 22px 16px;
    color: var(--text-muted);
    font-size: 12px;
  }

  .empty-wrapper,
  .empty-detail {
    padding: 14px;
  }

  .no-results {
    display: grid;
    min-height: 120px;
    place-content: center;
    gap: 5px;
    padding: 24px;
    text-align: center;
  }

  .no-results strong {
    color: var(--text-soft);
    font-size: 13px;
  }

  .no-results p {
    color: var(--text-muted);
    font-size: 11.5px;
  }

  @media (max-width: 980px) {
    .table-heading,
    .lockfile-row {
      grid-template-columns:
        minmax(210px, 1fr)
        minmax(100px, 0.4fr)
        minmax(150px, 0.65fr)
        minmax(95px, auto);
      gap: 11px;
    }
  }

  @media (max-width: 880px) {
    .lockfiles-layout {
      grid-template-columns: 1fr;
    }

    .project-directory {
      position: static;
    }

    .project-options {
      max-height: 280px;
    }
  }

  @media (max-width: 760px) {
    .upload-form {
      grid-template-columns: 1fr;
    }

    .json-field,
    .upload-actions {
      grid-column: auto;
    }

    .project-overview__header {
      align-items: flex-start;
      flex-direction: column;
    }

    .lockfile-summary {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }

    .lockfile-summary > div:nth-child(2) {
      border-right: 0;
    }

    .lockfile-summary > div:nth-child(-n + 2) {
      border-bottom: 1px solid var(--line-soft);
    }

    .table-heading {
      display: none;
    }

    .lockfile-row {
      grid-template-columns: 1fr;
      gap: 10px;
      padding: 14px;
    }

    .checksum-cell[data-label],
    .source-cell[data-label],
    .date-cell[data-label],
    .row-actions[data-label] {
      display: grid;
      grid-template-columns: 92px minmax(0, 1fr);
      align-items: center;
      gap: 10px;
    }

    .checksum-cell[data-label]::before,
    .source-cell[data-label]::before,
    .date-cell[data-label]::before,
    .row-actions[data-label]::before {
      content: attr(data-label);
      color: var(--text-muted);
      font-size: 10px;
    }

    .checksum-cell {
      grid-template-columns:
        92px minmax(0, 1fr) auto !important;
    }

    .checksum-cell::before {
      grid-column: 1;
    }

    .checksum-cell code {
      grid-column: 2;
    }

    .checksum-cell .copy-button {
      grid-column: 3;
    }

    .row-actions {
      justify-content: flex-start;
    }
  }

  @media (max-width: 600px) {
    .history-toolbar {
      grid-template-columns: 1fr;
    }

    .upload-actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .upload-actions button {
      width: 100%;
    }

    .project-links {
      align-items: flex-start;
      flex-direction: column;
      gap: 9px;
    }
  }

  @media (max-width: 460px) {
    .upload-section {
      padding: 14px;
    }

    .lockfile-summary {
      grid-template-columns: 1fr;
    }

    .lockfile-summary > div,
    .lockfile-summary > div:nth-child(2) {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .lockfile-summary > div:last-child {
      border-bottom: 0;
    }

    .checksum-cell[data-label],
    .source-cell[data-label],
    .date-cell[data-label],
    .row-actions[data-label] {
      grid-template-columns: 1fr !important;
      gap: 4px;
    }

    .checksum-cell code,
    .checksum-cell .copy-button {
      grid-column: auto;
    }

    .json-panel__header {
      align-items: stretch;
      flex-direction: column;
    }

    .copy-json-button {
      width: 100%;
    }
  }
</style>
