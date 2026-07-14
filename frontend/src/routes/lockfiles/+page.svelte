<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount, tick } from 'svelte';

  import { usePageRefresh } from '$lib/stores/pageRefresh';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

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

  import InlineError from '$lib/components/InlineError.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import WorkspaceAvatar from '$lib/components/WorkspaceAvatar.svelte';

  import { canUploadLockfile } from '$lib/permissions';

  type Confirmation = {
    lockfile: Lockfile;
  };

  let workspaces: Workspace[] = [];
  let projects: Project[] = [];
  let lockfiles: Lockfile[] = [];

  let selectedWorkspaceId = '';
  let selectedProjectId = '';
  let selectedLockfileId = '';

  let loading = true;
  let loadingLockfiles = false;
  let initialized = false;

  let projectListRequestId = 0;
  let lockfileRequestId = 0;

  let saving = false;
  let generatingChecksum = false;
  let busyLockfileId = '';

  let error = '';
  let success = '';

  let projectQuery = '';
  let lockfileQuery = '';
  let sourceFilter = 'all';

  let showUploadForm = false;

  let checksumSha256 = '';
  let lockfileJson = '{}';
  let source = 'dashboard';

  let copiedKey = '';

  let copyResetTimer:
    | ReturnType<typeof setTimeout>
    | undefined;

  let confirmation: Confirmation | null = null;
  let confirmationDialog: HTMLDialogElement | null = null;
  let confirmationValue = '';

  $: selectedWorkspace =
    workspaces.find(
      (workspace) =>
        workspace.id === selectedWorkspaceId
    ) ?? null;

  $: selectedProject =
    projects.find(
      (project) =>
        project.id === selectedProjectId
    ) ?? null;

  $: selectedLockfile =
    lockfiles.find(
      (lockfile) =>
        lockfile.id === selectedLockfileId
    ) ?? null;

  $: currentRole =
    selectedWorkspace?.current_user_role ??
    'viewer';

  $: canUpload =
    canUploadLockfile(currentRole);

  $: canUploadCurrent =
    canUpload &&
    selectedProject?.active !== false;

  $: globalWorkspaceId =
    $workspaceContext.selectedWorkspace?.id ??
    '';

  $: if (
    initialized &&
    globalWorkspaceId &&
    globalWorkspaceId !== selectedWorkspaceId &&
    workspaces.some(
      (workspace) =>
        workspace.id === globalWorkspaceId
    )
  ) {
    void switchWorkspace(globalWorkspaceId);
  }

  $: visibleProjects = projects.filter(
    (project) => {
      const query =
        projectQuery.trim().toLowerCase();

      if (!query) {
        return true;
      }

      return (
        project.name
          .toLowerCase()
          .includes(query) ||
        project.slug
          .toLowerCase()
          .includes(query)
      );
    }
  );

  $: sourceOptions = Array.from(
    new Set(
      lockfiles
        .map((lockfile) =>
          lockfile.source?.trim()
        )
        .filter(
          (value): value is string =>
            Boolean(value)
        )
    )
  );

  $: visibleLockfiles = lockfiles.filter(
    (lockfile) => {
      const query =
        lockfileQuery.trim().toLowerCase();

      const matchesQuery =
        !query ||
        lockfile.checksum_sha256
          .toLowerCase()
          .includes(query) ||
        lockfile.source
          ?.toLowerCase()
          .includes(query);

      const matchesSource =
        sourceFilter === 'all' ||
        lockfile.source === sourceFilter;

      return matchesQuery && matchesSource;
    }
  );

  $: latestLockfile =
    lockfiles[0] ?? null;

  $: confirmationToken =
    confirmation?.lockfile.checksum_sha256
      .slice(0, 12) ?? '';

  $: confirmationCanContinue =
    Boolean(confirmation) &&
    confirmationValue === confirmationToken;

  $: if (
    selectedLockfileId &&
    !lockfiles.some(
      (lockfile) =>
        lockfile.id === selectedLockfileId
    )
  ) {
    selectedLockfileId = '';
  }

  function projectInitial(
    project: Project
  ) {
    return project.name
      .slice(0, 1)
      .toUpperCase();
  }

  function shortChecksum(
    value: string
  ) {
    if (value.length <= 25) {
      return value;
    }

    return (
      `${value.slice(0, 14)}…` +
      value.slice(-8)
    );
  }

  function prettyJson(
    value?: string
  ) {
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

  function normalizeJson(
    value: string
  ) {
    return JSON.stringify(
      JSON.parse(value || '{}')
    );
  }

  async function sha256Hex(
    value: string
  ) {
    const bytes =
      new TextEncoder().encode(value);

    const digest =
      await crypto.subtle.digest(
        'SHA-256',
        bytes
      );

    return Array.from(
      new Uint8Array(digest)
    )
      .map((byte) =>
        byte.toString(16).padStart(2, '0')
      )
      .join('');
  }

  function formatDate(
    value?: string | number
  ) {
    if (
      value === undefined ||
      value === null ||
      value === ''
    ) {
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
      const numericValue =
        Number(value);

      if (
        Number.isFinite(numericValue) &&
        value.trim() !== ''
      ) {
        date = new Date(
          numericValue <
          1_000_000_000_000
            ? numericValue * 1000
            : numericValue
        );
      } else {
        date = new Date(value);
      }
    }

    if (
      Number.isNaN(date.getTime())
    ) {
      return 'Unknown date';
    }

    return new Intl.DateTimeFormat(
      undefined,
      {
        dateStyle: 'medium',
        timeStyle: 'short'
      }
    ).format(date);
  }

  function sourceLabel(
    value?: string
  ) {
    if (!value) {
      return 'Unknown';
    }

    if (value === 'ci') {
      return 'CI';
    }

    if (value === 'cli') {
      return 'CLI';
    }

    return (
      value.charAt(0).toUpperCase() +
      value.slice(1)
    );
  }

  function resetUploadForm() {
    checksumSha256 = '';
    lockfileJson = '{}';
    source = 'dashboard';
  }

  function openUploadForm() {
    if (!canUploadCurrent) {
      return;
    }

    showUploadForm = true;
    error = '';
    success = '';
  }

  function closeUploadForm() {
    if (saving) {
      return;
    }

    showUploadForm = false;
    resetUploadForm();
    error = '';
  }

  function toggleUploadForm() {
    if (showUploadForm) {
      closeUploadForm();
      return;
    }

    openUploadForm();
  }

  function updatePageUrl(
    projectId = ''
  ) {
    const url =
      new URL(window.location.href);

    if (selectedWorkspaceId) {
      url.searchParams.set(
        'workspace_id',
        selectedWorkspaceId
      );
    } else {
      url.searchParams.delete(
        'workspace_id'
      );
    }

    if (projectId) {
      url.searchParams.set(
        'project_id',
        projectId
      );
    } else {
      url.searchParams.delete(
        'project_id'
      );
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
      await navigator.clipboard.writeText(
        value
      );

      copiedKey = key;

      if (copyResetTimer) {
        clearTimeout(copyResetTimer);
      }

      copyResetTimer = setTimeout(
        () => {
          copiedKey = '';
        },
        1800
      );
    } catch {
      error =
        'Unable to copy this value.';
    }
  }

  async function generateChecksum() {
    generatingChecksum = true;
    error = '';

    try {
      const normalized =
        normalizeJson(lockfileJson);

      checksumSha256 =
        await sha256Hex(normalized);
    } catch {
      error =
        'Lockfile JSON must be valid before generating its checksum.';
    } finally {
      generatingChecksum = false;
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
      const params =
        new URLSearchParams(
          window.location.search
        );

      const workspaceData =
        await listWorkspaces(user.id);

      workspaces =
        workspaceData.workspaces;

      const requestedWorkspaceId =
        params.get('workspace_id');

      const storedWorkspaceId =
        $workspaceContext
          .selectedWorkspace?.id;

      selectedWorkspaceId =
        requestedWorkspaceId &&
        workspaces.some(
          (workspace) =>
            workspace.id ===
            requestedWorkspaceId
        )
          ? requestedWorkspaceId
          : storedWorkspaceId &&
              workspaces.some(
                (workspace) =>
                  workspace.id ===
                  storedWorkspaceId
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

  async function loadProjects(
    requestedProjectId?: string | null
  ) {
    const requestId =
      ++projectListRequestId;

    const workspaceId =
      selectedWorkspaceId;

    if (!workspaceId) {
      projects = [];
      lockfiles = [];

      selectedProjectId = '';
      selectedLockfileId = '';

      updatePageUrl();
      return;
    }

    workspaceContext.setSelectedWorkspace(
      workspaceId
    );

    const data =
      await listProjects(workspaceId);

    if (
      requestId !== projectListRequestId ||
      selectedWorkspaceId !== workspaceId
    ) {
      return;
    }

    projects = data.projects;

    selectedProjectId =
      requestedProjectId &&
      projects.some(
        (project) =>
          project.id === requestedProjectId
      )
        ? requestedProjectId
        : projects[0]?.id ?? '';

    updatePageUrl(
      selectedProjectId
    );

    await loadLockfiles(
      selectedProjectId
    );
  }

  async function switchWorkspace(
    workspaceId: string
  ) {
    if (
      workspaceId ===
      selectedWorkspaceId
    ) {
      return;
    }

    selectedWorkspaceId =
      workspaceId;

    selectedProjectId = '';
    selectedLockfileId = '';

    projects = [];
    lockfiles = [];

    projectQuery = '';
    lockfileQuery = '';
    sourceFilter = 'all';

    showUploadForm = false;
    resetUploadForm();

    confirmation = null;

    error = '';
    success = '';
    loading = true;

    workspaceContext.setSelectedWorkspace(
      workspaceId
    );

    updatePageUrl();

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

  async function selectProject(
    project: Project
  ) {
    if (
      project.id ===
      selectedProjectId
    ) {
      return;
    }

    selectedProjectId =
      project.id;

    selectedLockfileId = '';

    lockfileQuery = '';
    sourceFilter = 'all';
    copiedKey = '';

    showUploadForm = false;
    resetUploadForm();

    confirmation = null;

    error = '';
    success = '';

    updatePageUrl(project.id);

    await loadLockfiles(project.id);
  }

  async function loadLockfiles(
    projectId: string
  ) {
    if (
      !selectedWorkspaceId ||
      !projectId
    ) {
      lockfiles = [];
      selectedLockfileId = '';
      return;
    }

    const requestId =
      ++lockfileRequestId;

    const workspaceId =
      selectedWorkspaceId;

    loadingLockfiles = true;
    error = '';

    lockfiles = [];
    selectedLockfileId = '';

    try {
      const data =
        await listLockfiles(
          workspaceId,
          projectId
        );

      if (
        requestId !== lockfileRequestId ||
        selectedWorkspaceId !==
          workspaceId ||
        selectedProjectId !== projectId
      ) {
        return;
      }

      lockfiles = data.lockfiles;
    } catch (err) {
      if (
        requestId !== lockfileRequestId
      ) {
        return;
      }

      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load lockfile history.';
    } finally {
      if (
        requestId === lockfileRequestId
      ) {
        loadingLockfiles = false;
      }
    }
  }

  async function submitLockfile() {
    const user = $auth.user;

    const cleanSource =
      source.trim() || 'dashboard';

    if (
      !user ||
      !selectedWorkspaceId ||
      !selectedProjectId ||
      !canUploadCurrent
    ) {
      return;
    }

    let normalizedJson = '';

    try {
      normalizedJson =
        normalizeJson(lockfileJson);
    } catch {
      error =
        'Lockfile JSON is not valid.';

      return;
    }

    saving = true;
    error = '';
    success = '';

    try {
      const expectedChecksum =
        await sha256Hex(normalizedJson);

      const suppliedChecksum =
        checksumSha256
          .trim()
          .toLowerCase();

      if (
        suppliedChecksum &&
        suppliedChecksum !==
          expectedChecksum
      ) {
        error =
          'The supplied checksum does not match the submitted JSON.';

        return;
      }

      const uploaded =
        await uploadLockfile({
          workspaceId:
            selectedWorkspaceId,

          projectId:
            selectedProjectId,

          uploadedByUserId:
            user.id,

          checksumSha256:
            expectedChecksum,

          lockfileJson:
            normalizedJson,

          source:
            cleanSource
        });

      lockfiles = [
        uploaded.lockfile,
        ...lockfiles
      ];

      selectedLockfileId =
        uploaded.lockfile.id;

      success =
        'Dependency state uploaded and checksum verified.';

      resetUploadForm();
      showUploadForm = false;
      sourceFilter = 'all';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to upload lockfile.';
    } finally {
      saving = false;
    }
  }

  function inspectLockfile(
    lockfile: Lockfile
  ) {
    selectedLockfileId =
      selectedLockfileId === lockfile.id
        ? ''
        : lockfile.id;
  }

  async function openConfirmation(
    lockfile: Lockfile
  ) {
    confirmation = {
      lockfile
    };

    confirmationValue = '';

    await tick();

    confirmationDialog?.focus();
  }

  function closeConfirmation() {
    if (busyLockfileId) {
      return;
    }

    confirmation = null;
    confirmationValue = '';
  }

  async function confirmHideLockfile() {
    if (
      !confirmation ||
      !selectedWorkspaceId ||
      !selectedProjectId ||
      !canUploadCurrent ||
      !confirmationCanContinue
    ) {
      return;
    }

    const lockfile =
      confirmation.lockfile;

    busyLockfileId =
      lockfile.id;

    error = '';
    success = '';

    try {
      await deleteLockfile(
        selectedWorkspaceId,
        selectedProjectId,
        lockfile.id
      );

      lockfiles =
        lockfiles.filter(
          (item) =>
            item.id !== lockfile.id
        );

      if (
        selectedLockfileId ===
        lockfile.id
      ) {
        selectedLockfileId = '';
      }

      success =
        'Lockfile record hidden from normal history.';

      closeConfirmation();
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to hide lockfile.';
    } finally {
      busyLockfileId = '';
    }
  }

  function handleModalKeydown(
    event: KeyboardEvent
  ) {
    if (
      event.key === 'Escape' &&
      confirmation &&
      !busyLockfileId
    ) {
      event.preventDefault();
      closeConfirmation();
    }
  }

  usePageRefresh(load);

  onMount(() => {
    return () => {
      if (copyResetTimer) {
        clearTimeout(copyResetTimer);
      }
    };
  });
</script>

<svelte:head>
  <title>
    Lockfiles | Softadastra Cloud
  </title>

  <meta
    name="description"
    content="Review verified Vix dependency states, checksums, sources and exact lockfile JSON history in Softadastra Cloud."
  />
</svelte:head>

<svelte:window
  onkeydown={handleModalKeydown}
/>

<div class="lockfiles-page">
  <!-- ===================================================
       HEADER
       =================================================== -->
  <header class="page-header">
    <div class="page-heading">
      <p class="page-eyebrow">
        <span aria-hidden="true"></span>
        Dependency history
      </p>

      <h1>Lockfiles</h1>

      <p>
        {#if loading}
          Loading dependency history…
        {:else if selectedProject}
          Verified dependency states for
          <strong>
            {selectedProject.name}
          </strong>.
        {:else if selectedWorkspace}
          Connect a project before uploading
          lockfiles.
        {:else}
          Create a workspace before recording
          dependency states.
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
              {currentRole}
            </small>
          </span>
        </a>
      {/if}

      {#if
        !loading &&
        selectedProject &&
        canUploadCurrent
      }
        <button
          class:button-secondary={
            showUploadForm
          }
          class="header-action"
          type="button"
          onclick={toggleUploadForm}
        >
          {#if !showUploadForm}
            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path d="M12 5v14"></path>
              <path d="M5 12h14"></path>
            </svg>
          {/if}

          {showUploadForm
            ? 'Close'
            : lockfiles.length === 0
              ? 'Upload first lockfile'
              : 'Upload manually'}
        </button>
      {/if}
    </div>
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
       LOADING
       =================================================== -->
  {#if loading}
    <section class="page-loading">
      <span class="page-loading__mark">
        L
      </span>

      <div>
        <strong>
          Loading dependency history
        </strong>

        <p>
          Preparing projects, checksums and
          lockfile snapshots.
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
    <section class="required-state">
      <span
        class="required-state__icon"
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
          Dependency history begins inside a
          workspace.
        </h2>

        <p>
          The workspace owns projects, dependency
          snapshots and the access rules used to
          upload them.
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

  <!-- ===================================================
       NO PROJECT
       =================================================== -->
  {:else if projects.length === 0}
    <section class="required-state">
      <span
        class="required-state__icon"
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
          <path d="M3 6h6l2 2h10v11H3z"></path>
          <path d="M15 12v5"></path>
          <path d="M12.5 14.5h5"></path>
        </svg>
      </span>

      <div>
        <p class="panel-kicker">
          Project required
        </p>

        <h2>
          Connect the project before recording
          its dependencies.
        </h2>

        <p>
          Lockfiles belong to a specific Vix
          project so every dependency state remains
          connected to the correct project history.
        </p>
      </div>

      <a
        class="primary-link"
        href={`/projects?workspace_id=${encodeURIComponent(
          selectedWorkspace.id
        )}`}
      >
        Open projects

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
         MANUAL UPLOAD
         ================================================= -->
    {#if
      showUploadForm &&
      selectedProject &&
      canUploadCurrent
    }
      <section
        class="upload-panel"
        aria-labelledby="upload-title"
      >
        <header class="upload-panel__header">
          <span
            class="upload-panel__icon"
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
              <path d="M12 3v12"></path>
              <path d="m7 8 5-5 5 5"></path>
              <path d="M5 14v6h14v-6"></path>
            </svg>
          </span>

          <div>
            <p class="panel-kicker">
              Manual upload
            </p>

            <h2 id="upload-title">
              Upload a dependency snapshot
            </h2>

            <p>
              The JSON is normalized and its SHA256
              checksum is verified before the
              snapshot is stored.
            </p>
          </div>
        </header>

        <form
          class="upload-form"
          onsubmit={(event) => {
            event.preventDefault();
            void submitLockfile();
          }}
        >
          <div class="upload-form__fields">
            <label>
              <span>Source</span>

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

              <small>
                Identify where this dependency
                snapshot originated.
              </small>
            </label>

            <label class="checksum-field">
              <span>Checksum SHA256</span>

              <div class="checksum-input">
                <input
                  bind:value={checksumSha256}
                  maxlength="64"
                  placeholder="Generated automatically from the JSON"
                  autocomplete="off"
                  spellcheck="false"
                />

                <button
                  class="button-secondary checksum-generate"
                  type="button"
                  disabled={generatingChecksum}
                  onclick={() =>
                    void generateChecksum()}
                >
                  {generatingChecksum
                    ? 'Generating…'
                    : 'Generate'}
                </button>
              </div>

              <small>
                Leave empty to generate it during
                upload. A supplied value must match
                the submitted JSON.
              </small>
            </label>

            <label class="json-field">
              <span>Lockfile JSON</span>

              <textarea
                bind:value={lockfileJson}
                rows="14"
                spellcheck="false"
              ></textarea>
            </label>
          </div>

          <aside class="upload-preview">
            <p class="panel-kicker">
              Snapshot preview
            </p>

            <span class="upload-preview__mark">
              {projectInitial(
                selectedProject
              )}
            </span>

            <div class="upload-preview__identity">
              <strong>
                {selectedProject.name}
              </strong>

              <span>
                Dependency state
              </span>
            </div>

            <dl>
              <div>
                <dt>Project</dt>

                <dd>
                  {selectedProject.slug}
                </dd>
              </div>

              <div>
                <dt>Source</dt>

                <dd>
                  {sourceLabel(source)}
                </dd>
              </div>

              <div>
                <dt>Checksum</dt>

                <dd>
                  {checksumSha256.trim()
                    ? shortChecksum(
                        checksumSha256.trim()
                      )
                    : 'Generated on upload'}
                </dd>
              </div>
            </dl>

            <div class="upload-preview__workflow">
              <span>Preferred workflow</span>

              <code>
                <i>$</i>
                vix cloud lockfile upload
              </code>
            </div>
          </aside>

          <div class="upload-form__actions">
            <button
              class="button-secondary"
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
                !lockfileJson.trim() ||
                !source.trim()
              }
            >
              {saving
                ? 'Uploading snapshot…'
                : 'Upload snapshot'}
            </button>
          </div>
        </form>
      </section>
    {/if}

    <div class="lockfiles-layout">
      <!-- ===============================================
           PROJECT DIRECTORY
           =============================================== -->
      <aside
        class="project-directory"
        aria-label="Project directory"
      >
        <header class="directory-header">
          <div>
            <p class="panel-kicker">
              Workspace projects
            </p>

            <h2>Projects</h2>
          </div>

          <span class="directory-count">
            {projects.length}
          </span>
        </header>

        {#if projects.length > 4}
          <label class="project-search">
            <span class="visually-hidden">
              Find a project
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
              bind:value={projectQuery}
              type="search"
              placeholder="Find a project"
            />
          </label>
        {/if}

        {#if visibleProjects.length === 0}
          <p class="directory-message">
            No project matches your search.
          </p>
        {:else}
          <div class="project-options">
            {#each visibleProjects as project (project.id)}
              <button
                class:selected={
                  project.id ===
                  selectedProjectId
                }
                class="project-option"
                type="button"
                aria-pressed={
                  project.id ===
                  selectedProjectId
                }
                onclick={() =>
                  void selectProject(project)}
              >
                <span class="project-option__mark">
                  {projectInitial(project)}
                </span>

                <span class="project-option__content">
                  <strong>
                    {project.name}
                  </strong>

                  <code>
                    {project.slug}
                  </code>
                </span>
              </button>
            {/each}
          </div>
        {/if}

        <a
          class="directory-footer"
          href={`/projects?workspace_id=${encodeURIComponent(
            selectedWorkspaceId
          )}`}
        >
          Manage projects

          <svg
            viewBox="0 0 24 24"
            aria-hidden="true"
          >
            <path d="m9 18 6-6-6-6"></path>
          </svg>
        </a>
      </aside>

      <!-- ===============================================
           PROJECT DETAIL
           =============================================== -->
      <main class="lockfile-detail">
        {#if selectedProject}
          <section class="project-overview">
            <header class="project-overview__header">
              <div class="project-identity">
                <span class="project-mark">
                  {projectInitial(
                    selectedProject
                  )}
                </span>

                <div>
                  <p class="panel-kicker">
                    Current project
                  </p>

                  <h2>
                    {selectedProject.name}
                  </h2>

                  <code>
                    {selectedProject.slug}
                  </code>
                </div>
              </div>

              <a
                class="project-link"
                href={`/projects?workspace_id=${encodeURIComponent(
                  selectedWorkspaceId
                )}&project_id=${encodeURIComponent(
                  selectedProject.id
                )}`}
              >
                Open project

                <svg
                  viewBox="0 0 24 24"
                  aria-hidden="true"
                >
                  <path d="m9 18 6-6-6-6"></path>
                </svg>
              </a>
            </header>

            {#if loadingLockfiles}
              <span
                class="detail-progress"
                aria-hidden="true"
              ></span>
            {/if}

            <div class="lockfile-metrics">
              <article>
                <span>Snapshots</span>

                <strong>
                  {lockfiles.length}
                </strong>

                <small>
                  Recorded dependency states
                </small>
              </article>

              <article>
                <span>Sources</span>

                <strong>
                  {sourceOptions.length}
                </strong>

                <small>
                  CLI, CI or manual origins
                </small>
              </article>

              <article>
                <span>Latest upload</span>

                <strong>
                  {latestLockfile
                    ? sourceLabel(
                        latestLockfile.source
                      )
                    : 'No snapshot'}
                </strong>

                <small>
                  {latestLockfile
                    ? formatDate(
                        latestLockfile.created_at
                      )
                    : 'Upload the first lockfile'}
                </small>
              </article>
            </div>

            {#if latestLockfile}
              <div class="latest-snapshot">
                <span
                  class="latest-snapshot__icon"
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
                      x="4"
                      y="3"
                      width="16"
                      height="18"
                      rx="2"
                    ></rect>

                    <path d="M8 8h8"></path>
                    <path d="M8 12h8"></path>
                    <path d="M8 16h5"></path>
                  </svg>
                </span>

                <div>
                  <span>Latest checksum</span>

                  <code>
                    {shortChecksum(
                      latestLockfile
                        .checksum_sha256
                    )}
                  </code>
                </div>

                <button
                  class:copied={
                    copiedKey ===
                    `latest:${latestLockfile.id}`
                  }
                  class="copy-icon-button"
                  type="button"
                  aria-label="Copy latest checksum"
                  onclick={() =>
                    copyValue(
                      `latest:${latestLockfile.id}`,
                      latestLockfile
                        .checksum_sha256
                    )}
                >
                  {#if
                    copiedKey ===
                    `latest:${latestLockfile.id}`
                  }
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

                      <path
                        d="M16 8V5a2 2 0 0 0-2-2H5a2 2 0 0 0-2 2v9a2 2 0 0 0 2 2h3"
                      ></path>
                    </svg>
                  {/if}
                </button>
              </div>
            {/if}
          </section>

          {#if selectedProject.active === false}
            <div class="archived-notice">
              <span aria-hidden="true">
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="1.8"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                >
                  <path d="M4 7h16"></path>
                  <path d="M6 7v13h12V7"></path>
                  <path d="M9 11h6"></path>
                  <path d="M3 3h18v4H3z"></path>
                </svg>
              </span>

              <div>
                <strong>
                  This project is archived
                </strong>

                <p>
                  Existing dependency history remains
                  visible, but new snapshots are
                  disabled.
                </p>
              </div>

              <a
                href={`/projects?workspace_id=${encodeURIComponent(
                  selectedWorkspaceId
                )}&project_id=${encodeURIComponent(
                  selectedProject.id
                )}`}
              >
                Manage project
              </a>
            </div>
          {:else if !canUpload}
            <ReadOnlyNotice
              message="Your role can inspect lockfile history and exact JSON content, but cannot upload or hide dependency snapshots."
            />
          {/if}

          <!-- ===========================================
               FIRST SNAPSHOT
               =========================================== -->
          {#if
            !loadingLockfiles &&
            lockfiles.length === 0 &&
            !showUploadForm
          }
            <section class="lockfile-onboarding">
              <div class="lockfile-onboarding__copy">
                <p class="panel-kicker">
                  First dependency snapshot
                </p>

                <h2>
                  Record the exact dependencies
                  <span>
                    used by this project.
                  </span>
                </h2>

                <p>
                  The lockfile snapshot preserves the
                  dependency state used locally. Its
                  checksum makes later verification
                  possible without moving the build
                  workflow into Cloud.
                </p>

                {#if canUploadCurrent}
                  <button
                    class="manual-upload-action"
                    type="button"
                    onclick={openUploadForm}
                  >
                    Upload manually
                  </button>
                {/if}
              </div>

              <div class="lockfile-terminal">
                <header>
                  <div aria-hidden="true">
                    <span></span>
                    <span></span>
                    <span></span>
                  </div>

                  <strong>
                    project terminal
                  </strong>

                  <small>local</small>
                </header>

                <div class="lockfile-terminal__body">
                  <div class="terminal-command">
                    <span>$</span>

                    <code>
                      cd {selectedProject.slug}
                    </code>
                  </div>

                  <div class="terminal-output">
                    <span>→</span>

                    <code>
                      Project lockfile detected
                    </code>
                  </div>

                  <div class="terminal-command terminal-command--primary">
                    <span>$</span>

                    <code>
                      vix cloud lockfile upload
                    </code>
                  </div>

                  <div class="terminal-output terminal-output--success">
                    <span>✓</span>

                    <code>
                      Dependency state and checksum
                      recorded
                    </code>
                  </div>
                </div>

                <footer>
                  <span>
                    Dependencies stay reproducible
                  </span>

                  <span>
                    History becomes inspectable
                  </span>
                </footer>
              </div>

              <div class="lockfile-onboarding__steps">
                <article class="active">
                  <span>01</span>

                  <div>
                    <strong>
                      Resolve dependencies
                    </strong>

                    <p>
                      Let Vix produce the project
                      lockfile locally.
                    </p>
                  </div>
                </article>

                <article>
                  <span>02</span>

                  <div>
                    <strong>
                      Upload the snapshot
                    </strong>

                    <p>
                      Send the JSON and its verified
                      checksum.
                    </p>
                  </div>
                </article>

                <article>
                  <span>03</span>

                  <div>
                    <strong>
                      Compare changes later
                    </strong>

                    <p>
                      Keep dependency history connected
                      to the project.
                    </p>
                  </div>
                </article>
              </div>
            </section>

          <!-- ===========================================
               HISTORY
               =========================================== -->
          {:else if lockfiles.length > 0}
            <section class="history-panel">
              <header class="history-panel__header">
                <div>
                  <p class="panel-kicker">
                    Dependency snapshots
                  </p>

                  <h2>Lockfile history</h2>

                  <p>
                    Verified JSON states uploaded for
                    this project.
                  </p>
                </div>

                <span class="history-count">
                  {visibleLockfiles.length}
                </span>
              </header>

              <div class="history-toolbar">
                <label class="lockfile-search">
                  <span class="visually-hidden">
                    Find a lockfile
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
                    bind:value={lockfileQuery}
                    type="search"
                    placeholder="Find a checksum or source"
                  />
                </label>

                <select
                  bind:value={sourceFilter}
                  aria-label="Filter lockfiles by source"
                >
                  <option value="all">
                    All sources
                  </option>

                  {#each sourceOptions as sourceOption}
                    <option value={sourceOption}>
                      {sourceLabel(
                        sourceOption
                      )}
                    </option>
                  {/each}
                </select>
              </div>

              {#if visibleLockfiles.length === 0}
                <div class="history-empty">
                  <span
                    class="history-empty__icon"
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
                        cx="11"
                        cy="11"
                        r="7"
                      ></circle>

                      <path d="m20 20-4-4"></path>
                    </svg>
                  </span>

                  <strong>
                    No matching snapshots
                  </strong>

                  <p>
                    Change the checksum search or
                    selected source.
                  </p>
                </div>
              {:else}
                <div class="lockfile-list">
                  <div class="lockfile-list__heading">
                    <span>Checksum</span>
                    <span>Source</span>
                    <span>Uploaded</span>
                    <span></span>
                  </div>

                  {#each visibleLockfiles as lockfile (lockfile.id)}
                    <article
                      class:selected={
                        selectedLockfileId ===
                        lockfile.id
                      }
                      class="lockfile-row"
                    >
                      <div class="lockfile-checksum">
                        <span
                          class="lockfile-icon"
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
                              x="4"
                              y="3"
                              width="16"
                              height="18"
                              rx="2"
                            ></rect>

                            <path d="M8 8h8"></path>
                            <path d="M8 12h8"></path>
                            <path d="M8 16h5"></path>
                          </svg>
                        </span>

                        <div>
                          <code
                            title={lockfile.checksum_sha256}
                          >
                            {shortChecksum(
                              lockfile
                                .checksum_sha256
                            )}
                          </code>

                          <small>
                            SHA256 verified snapshot
                          </small>
                        </div>

                        <button
                          class:copied={
                            copiedKey ===
                            `checksum:${lockfile.id}`
                          }
                          class="copy-icon-button"
                          type="button"
                          aria-label="Copy lockfile checksum"
                          onclick={() =>
                            copyValue(
                              `checksum:${lockfile.id}`,
                              lockfile
                                .checksum_sha256
                            )}
                        >
                          {#if
                            copiedKey ===
                            `checksum:${lockfile.id}`
                          }
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

                              <path
                                d="M16 8V5a2 2 0 0 0-2-2H5a2 2 0 0 0-2 2v9a2 2 0 0 0 2 2h3"
                              ></path>
                            </svg>
                          {/if}
                        </button>
                      </div>

                      <div class="lockfile-source">
                        <span>Source</span>

                        <strong>
                          {sourceLabel(
                            lockfile.source
                          )}
                        </strong>
                      </div>

                      <div class="lockfile-date">
                        <span>Uploaded</span>

                        <strong>
                          {formatDate(
                            lockfile.created_at
                          )}
                        </strong>
                      </div>

                      <div class="lockfile-controls">
                        <button
                          class="inspect-button"
                          type="button"
                          aria-expanded={
                            selectedLockfileId ===
                            lockfile.id
                          }
                          onclick={() =>
                            inspectLockfile(
                              lockfile
                            )}
                        >
                          {selectedLockfileId ===
                          lockfile.id
                            ? 'Close JSON'
                            : 'Inspect JSON'}
                        </button>

                        {#if canUploadCurrent}
                          <details class="lockfile-actions">
                            <summary
                              aria-label="Lockfile actions"
                            >
                              <svg
                                viewBox="0 0 24 24"
                                aria-hidden="true"
                              >
                                <circle
                                  cx="5"
                                  cy="12"
                                  r="1"
                                ></circle>

                                <circle
                                  cx="12"
                                  cy="12"
                                  r="1"
                                ></circle>

                                <circle
                                  cx="19"
                                  cy="12"
                                  r="1"
                                ></circle>
                              </svg>
                            </summary>

                            <div>
                              <button
                                class="action-danger"
                                type="button"
                                disabled={
                                  busyLockfileId ===
                                  lockfile.id
                                }
                                onclick={() =>
                                  openConfirmation(
                                    lockfile
                                  )}
                              >
                                Hide record
                              </button>
                            </div>
                          </details>
                        {/if}
                      </div>
                    </article>
                  {/each}
                </div>
              {/if}
            </section>

            <!-- =========================================
                 SINGLE JSON INSPECTOR
                 ========================================= -->
            {#if selectedLockfile}
              <section
                class="json-inspector"
                aria-labelledby="json-inspector-title"
              >
                <header class="json-inspector__header">
                  <div>
                    <p class="panel-kicker">
                      Exact dependency state
                    </p>

                    <h2 id="json-inspector-title">
                      Lockfile JSON
                    </h2>

                    <p>
                      Uploaded from
                      {sourceLabel(
                        selectedLockfile.source
                      )}
                      on
                      {formatDate(
                        selectedLockfile.created_at
                      )}.
                    </p>
                  </div>

                  <div class="json-inspector__actions">
                    <button
                      class:copied={
                        copiedKey ===
                        `json:${selectedLockfile.id}`
                      }
                      class="button-secondary"
                      type="button"
                      onclick={() =>
                        copyValue(
                          `json:${selectedLockfile.id}`,
                          prettyJson(
                            selectedLockfile
                              .lockfile_json
                          )
                        )}
                    >
                      {copiedKey ===
                      `json:${selectedLockfile.id}`
                        ? 'JSON copied'
                        : 'Copy JSON'}
                    </button>

                    <button
                      class="icon-close-button"
                      type="button"
                      aria-label="Close JSON inspector"
                      onclick={() => {
                        selectedLockfileId = '';
                      }}
                    >
                      <svg
                        viewBox="0 0 24 24"
                        aria-hidden="true"
                      >
                        <path d="M6 6l12 12"></path>
                        <path d="M18 6 6 18"></path>
                      </svg>
                    </button>
                  </div>
                </header>

                <div class="json-inspector__metadata">
                  <div>
                    <span>Checksum</span>

                    <code>
                      {selectedLockfile
                        .checksum_sha256}
                    </code>
                  </div>

                  <div>
                    <span>Source</span>

                    <strong>
                      {sourceLabel(
                        selectedLockfile.source
                      )}
                    </strong>
                  </div>
                </div>

                <pre><code>{prettyJson(
                  selectedLockfile.lockfile_json
                )}</code></pre>
              </section>
            {/if}
          {/if}
        {/if}
      </main>
    </div>
  {/if}
</div>

<!-- =====================================================
     HIDE CONFIRMATION
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
        class="confirm-modal__icon"
        aria-hidden="true"
      >
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

          <path
            d="m10.3 3.7-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.7-3.3l-8-14a2 2 0 0 0-3.4 0Z"
          ></path>
        </svg>
      </span>

      <div class="confirm-modal__content">
        <h2 id="confirmation-title">
          Hide this lockfile record?
        </h2>

        <p id="confirmation-description">
          The snapshot will disappear from normal
          project history. Its stored content and
          checksum may still be retained for safety.
        </p>
      </div>

      <label class="confirmation-field">
        <span>
          Enter the checksum prefix
          <code>
            {confirmationToken}
          </code>
          to confirm
        </span>

        <input
          bind:value={confirmationValue}
          placeholder={confirmationToken}
          autocomplete="off"
        />
      </label>

      <div class="confirm-modal__actions">
        <button
          class="button-secondary"
          type="button"
          disabled={Boolean(
            busyLockfileId
          )}
          onclick={closeConfirmation}
        >
          Cancel
        </button>

        <button
          class="danger-button"
          type="button"
          disabled={
            Boolean(busyLockfileId) ||
            !confirmationCanContinue
          }
          onclick={confirmHideLockfile}
        >
          {busyLockfileId
            ? 'Hiding record…'
            : 'Hide record'}
        </button>
      </div>
    </dialog>
  </div>
{/if}

<style>
  .lockfiles-page {
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

  /* Header */

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
    text-transform: capitalize;
  }

  .header-action svg {
    width: 13px;
    height: 13px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
  }

  /* Shared controls */

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

  .button-secondary,
  .manual-upload-action {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-soft);
  }

  .button-secondary:hover:not(:disabled),
  .manual-upload-action:hover:not(:disabled) {
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

  .primary-link svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
  }

  .panel-kicker {
    color: var(--brand-soft);
    font-family: var(--font-mono);
    font-size: 8px;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }

  /* Feedback */

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

  /* Loading */

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

  /* Required states */

  .required-state {
    display: grid;
    max-width: 780px;
    min-height: 320px;
    align-content: center;
    justify-items: start;
    gap: 15px;
    padding: clamp(28px, 6vw, 56px);
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-lg);
    background: var(--bg-panel);
    box-shadow: var(--shadow-panel);
  }

  .required-state__icon {
    display: grid;
    width: 49px;
    height: 49px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 11px;
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .required-state__icon svg {
    width: 22px;
    height: 22px;
  }

  .required-state > div {
    display: grid;
    gap: 7px;
  }

  .required-state h2 {
    max-width: 650px;
    color: var(--text);
    font-size: clamp(27px, 4vw, 42px);
    font-weight: 630;
    line-height: 1.04;
    letter-spacing: -0.045em;
  }

  .required-state div > p:not(.panel-kicker) {
    max-width: 590px;
    color: var(--text-muted);
    font-size: 12px;
    line-height: 1.65;
  }

  /* Upload panel */

  .upload-panel {
    overflow: hidden;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    box-shadow: inset 3px 0 0 var(--brand);
    animation: form-enter 300ms var(--ease) both;
  }

  @keyframes form-enter {
    from {
      opacity: 0;
      transform: translateY(-6px);
    }
  }

  .upload-panel__header {
    display: flex;
    align-items: flex-start;
    gap: 12px;
    padding: 17px 19px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .upload-panel__icon {
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

  .upload-panel__icon svg {
    width: 20px;
    height: 20px;
  }

  .upload-panel__header > div {
    display: grid;
    gap: 4px;
  }

  .upload-panel h2 {
    color: var(--text);
    font-size: 16px;
    font-weight: 620;
  }

  .upload-panel__header p:not(.panel-kicker) {
    max-width: 650px;
    color: var(--text-muted);
    font-size: 10px;
    line-height: 1.5;
  }

  .upload-form {
    display: grid;
    grid-template-columns:
      minmax(0, 1.35fr)
      minmax(280px, 0.65fr);
  }

  .upload-form__fields {
    display: grid;
    gap: 16px;
    padding: 20px;
    border-right: 1px solid var(--line-soft);
  }

  .upload-form label {
    display: grid;
    gap: 7px;
  }

  .upload-form label > span {
    color: var(--text-soft);
    font-size: 10.5px;
    font-weight: 550;
  }

  .upload-form label > small {
    color: var(--text-muted);
    font-size: 8.5px;
    line-height: 1.45;
  }

  .upload-form input,
  .upload-form textarea,
  .confirmation-field input {
    width: 100%;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    outline: 0;
    background: var(--bg-ink-soft);
    color: var(--text);
    padding: 0 11px;
    font: inherit;
    font-size: 12px;
  }

  .upload-form input,
  .confirmation-field input {
    min-height: 41px;
  }

  .upload-form textarea {
    min-height: 250px;
    resize: vertical;
    padding-block: 11px;
    font-family: var(--font-mono);
    font-size: 10.5px;
    line-height: 1.6;
    tab-size: 2;
  }

  .upload-form input:focus,
  .upload-form textarea:focus,
  .confirmation-field input:focus {
    border-color: var(--brand);
    box-shadow: 0 0 0 3px var(--brand-faint);
  }

  .checksum-input {
    display: grid;
    grid-template-columns:
      minmax(0, 1fr) auto;
    gap: 8px;
  }

  .checksum-generate {
    min-height: 41px;
  }

  .upload-preview {
    display: grid;
    align-content: start;
    justify-items: start;
    gap: 12px;
    padding: 20px;
    background: var(--bg-ink-soft);
  }

  .upload-preview__mark {
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

  .upload-preview__identity {
    display: grid;
    gap: 3px;
  }

  .upload-preview__identity strong {
    color: var(--text);
    font-size: 11px;
  }

  .upload-preview__identity span {
    color: var(--text-muted);
    font-size: 8.5px;
  }

  .upload-preview dl {
    display: grid;
    width: 100%;
    margin: 3px 0 0;
    border: 1px solid var(--line-soft);
    border-radius: 8px;
  }

  .upload-preview dl > div {
    display: grid;
    gap: 4px;
    padding: 10px;
    border-bottom: 1px solid var(--line-soft);
  }

  .upload-preview dl > div:last-child {
    border-bottom: 0;
  }

  .upload-preview dt {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 7px;
    text-transform: uppercase;
  }

  .upload-preview dd {
    overflow-wrap: anywhere;
    margin: 0;
    color: var(--text-soft);
    font-size: 8.5px;
  }

  .upload-preview__workflow {
    display: grid;
    width: 100%;
    gap: 7px;
  }

  .upload-preview__workflow > span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 7px;
    text-transform: uppercase;
  }

  .upload-preview__workflow code {
    display: flex;
    min-height: 39px;
    align-items: center;
    gap: 8px;
    padding: 0 10px;
    border: 1px solid var(--line-ink);
    border-radius: 6px;
    background: var(--bg-ink);
    color: var(--text-soft);
    font-size: 8.5px;
  }

  .upload-preview__workflow i {
    color: var(--brand);
    font-style: normal;
    font-weight: 800;
  }

  .upload-form__actions {
    grid-column: 1 / -1;
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    padding: 13px 20px;
    border-top: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  /* Main layout */

  .lockfiles-layout {
    display: grid;
    grid-template-columns:
      260px minmax(0, 1fr);
    gap: 16px;
    align-items: start;
  }

  /* Directory */

  .project-directory {
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
    gap: 12px;
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
    font-size: 12.5px;
    font-weight: 600;
  }

  .directory-count,
  .history-count {
    display: grid;
    min-width: 24px;
    height: 22px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    background: var(--bg-elevated);
    color: var(--text-muted);
    padding-inline: 7px;
    font-family: var(--font-mono);
    font-size: 9px;
  }

  .project-search {
    position: relative;
    display: block;
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

  .project-search input,
  .lockfile-search input {
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

  .project-search input:focus,
  .lockfile-search input:focus {
    border-color: var(--brand);
  }

  .project-options {
    display: grid;
    max-height: calc(100vh - 270px);
    overflow-y: auto;
  }

  .project-option {
    display: grid;
    width: 100%;
    min-height: 59px;
    grid-template-columns:
      31px minmax(0, 1fr);
    align-items: center;
    gap: 9px;
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
    transform: none;
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
    border: 1px solid var(--line);
    border-radius: 7px;
    background: var(--bg-ink-soft);
    color: var(--text-muted);
    font-size: 9px;
    font-weight: 700;
  }

  .project-option.selected
    .project-option__mark {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .project-option__content {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .project-option__content strong {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 10.5px;
    font-weight: 600;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .project-option.selected
    .project-option__content strong {
    color: var(--brand-bright);
  }

  .project-option__content code {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 8px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .directory-message {
    padding: 18px 14px;
    color: var(--text-muted);
    font-size: 11px;
  }

  .directory-footer {
    display: flex;
    min-height: 43px;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
    padding: 0 13px;
    border-top: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
    color: var(--text-muted);
    font-size: 9px;
    text-decoration: none;
  }

  .directory-footer:hover {
    color: var(--brand-soft);
  }

  .directory-footer svg {
    width: 12px;
    height: 12px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
  }

  /* Project overview */

  .lockfile-detail {
    display: grid;
    min-width: 0;
    gap: 16px;
  }

  .project-overview {
    position: relative;
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .project-overview__header {
    display: flex;
    min-height: 94px;
    align-items: center;
    justify-content: space-between;
    gap: 20px;
    padding: 17px 19px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .project-identity {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 12px;
  }

  .project-mark {
    display: grid;
    width: 45px;
    height: 45px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 10px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 14px;
    font-weight: 750;
  }

  .project-identity > div {
    display: grid;
    min-width: 0;
    gap: 5px;
  }

  .project-identity h2 {
    overflow-wrap: anywhere;
    color: var(--text);
    font-size: 18px;
    font-weight: 620;
    letter-spacing: -0.03em;
  }

  .project-identity code {
    color: var(--text-muted);
    font-size: 9px;
  }

  .project-link {
    display: inline-flex;
    flex: 0 0 auto;
    align-items: center;
    gap: 5px;
    color: var(--brand-soft);
    font-size: 9.5px;
    font-weight: 600;
    text-decoration: none;
  }

  .project-link:hover {
    color: var(--brand-bright);
  }

  .project-link svg {
    width: 12px;
    height: 12px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
  }

  .detail-progress {
    position: absolute;
    z-index: 3;
    top: 93px;
    right: 0;
    left: 0;
    height: 2px;
    background: var(--brand);
    transform-origin: left;
    animation: detail-loading 1.25s ease-in-out infinite;
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

  .lockfile-metrics {
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    border-bottom: 1px solid var(--line-soft);
  }

  .lockfile-metrics article {
    display: grid;
    min-width: 0;
    gap: 4px;
    padding: 15px 18px;
    border-right: 1px solid var(--line-soft);
  }

  .lockfile-metrics article:last-child {
    border-right: 0;
  }

  .lockfile-metrics span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 7px;
    letter-spacing: 0.06em;
    text-transform: uppercase;
  }

  .lockfile-metrics strong {
    overflow-wrap: anywhere;
    color: var(--text);
    font-size: 16px;
    font-weight: 620;
  }

  .lockfile-metrics small {
    overflow: hidden;
    color: var(--text-faint);
    font-size: 8px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .latest-snapshot {
    display: grid;
    grid-template-columns:
      auto minmax(0, 1fr) auto;
    align-items: center;
    gap: 10px;
    padding: 11px 15px;
    background: var(--bg-panel-strong);
  }

  .latest-snapshot__icon {
    display: grid;
    width: 34px;
    height: 34px;
    place-items: center;
    border: 1px solid var(--green-line);
    border-radius: 8px;
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .latest-snapshot__icon svg {
    width: 16px;
    height: 16px;
  }

  .latest-snapshot > div {
    display: grid;
    min-width: 0;
    gap: 4px;
  }

  .latest-snapshot div > span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 7px;
    text-transform: uppercase;
  }

  .latest-snapshot code {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 8.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .copy-icon-button,
  .icon-close-button {
    display: grid;
    width: 29px;
    height: 29px;
    min-height: 29px;
    place-items: center;
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-muted);
    padding: 0;
  }

  .copy-icon-button:hover:not(:disabled),
  .icon-close-button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
    transform: none;
  }

  .copy-icon-button.copied {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .copy-icon-button svg,
  .icon-close-button svg {
    width: 13px;
    height: 13px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
  }

  /* Archived */

  .archived-notice {
    display: grid;
    grid-template-columns:
      auto minmax(0, 1fr) auto;
    align-items: center;
    gap: 11px;
    padding: 13px;
    border: 1px solid var(--warning-line);
    border-radius: var(--radius-sm);
    background: var(--warning-faint);
  }

  .archived-notice > span {
    display: grid;
    width: 34px;
    height: 34px;
    place-items: center;
    border-radius: 8px;
    background: var(--bg-panel);
    color: var(--warning);
  }

  .archived-notice svg {
    width: 16px;
    height: 16px;
  }

  .archived-notice > div {
    display: grid;
    gap: 3px;
  }

  .archived-notice strong {
    color: var(--text);
    font-size: 10.5px;
  }

  .archived-notice p {
    color: var(--text-muted);
    font-size: 8.5px;
  }

  .archived-notice a {
    color: var(--warning);
    font-size: 9px;
    font-weight: 600;
  }

  /* Onboarding */

  .lockfile-onboarding {
    display: grid;
    grid-template-columns:
      minmax(0, 0.82fr)
      minmax(400px, 1.18fr);
    gap: clamp(38px, 7vw, 76px);
    align-items: center;
    padding: clamp(30px, 5vw, 50px);
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-lg);
    background: var(--bg-panel);
    box-shadow: var(--shadow-panel);
  }

  .lockfile-onboarding__copy {
    display: grid;
    align-content: center;
  }

  .lockfile-onboarding__copy h2 {
    max-width: 550px;
    margin-top: 10px;
    color: var(--text);
    font-size: clamp(31px, 4.6vw, 50px);
    font-weight: 640;
    line-height: 1;
    letter-spacing: -0.052em;
  }

  .lockfile-onboarding__copy h2 span {
    display: block;
    color: var(--brand-soft);
  }

  .lockfile-onboarding__copy > p:not(.panel-kicker) {
    max-width: 540px;
    margin-top: 17px;
    color: var(--text-muted);
    font-size: 12px;
    line-height: 1.7;
  }

  .manual-upload-action {
    width: fit-content;
    margin-top: 24px;
  }

  .lockfile-terminal {
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: 11px;
    background: var(--bg-ink);
    box-shadow: var(--shadow-panel);
  }

  .lockfile-terminal > header {
    display: grid;
    min-height: 43px;
    grid-template-columns:
      auto minmax(0, 1fr) auto;
    align-items: center;
    gap: 10px;
    padding: 0 12px;
    border-bottom: 1px solid var(--line-ink);
    background: var(--bg-ink-soft);
  }

  .lockfile-terminal header > div {
    display: flex;
    gap: 5px;
  }

  .lockfile-terminal header > div span {
    width: 7px;
    height: 7px;
    border-radius: 50%;
    background: var(--text-faint);
  }

  .lockfile-terminal header > div span:first-child {
    background: var(--brand);
  }

  .lockfile-terminal header strong,
  .lockfile-terminal header small {
    font-family: var(--font-mono);
    font-size: 7px;
  }

  .lockfile-terminal header strong {
    overflow: hidden;
    color: var(--text-muted);
    text-align: center;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .lockfile-terminal header small {
    color: var(--text-faint);
  }

  .lockfile-terminal__body {
    display: grid;
    align-content: center;
    min-height: 225px;
    padding: 24px 20px;
  }

  .terminal-command,
  .terminal-output {
    display: grid;
    min-height: 35px;
    grid-template-columns:
      23px minmax(0, 1fr);
    align-items: center;
    font-family: var(--font-mono);
    font-size: 9px;
  }

  .terminal-command span {
    color: var(--brand);
    font-weight: 800;
  }

  .terminal-command code {
    color: var(--text);
  }

  .terminal-output span,
  .terminal-output code {
    color: var(--text-muted);
  }

  .terminal-command--primary {
    margin-top: 17px;
  }

  .terminal-output--success span,
  .terminal-output--success code {
    color: var(--green-soft);
  }

  .lockfile-terminal > footer {
    display: flex;
    min-height: 39px;
    align-items: center;
    justify-content: space-between;
    gap: 15px;
    padding: 0 12px;
    border-top: 1px solid var(--line-ink);
    background: var(--bg-ink-soft);
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 6.5px;
  }

  .lockfile-onboarding__steps {
    grid-column: 1 / -1;
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    padding-top: 25px;
    border-top: 1px solid var(--line);
  }

  .lockfile-onboarding__steps article {
    display: grid;
    grid-template-columns:
      auto minmax(0, 1fr);
    gap: 10px;
    padding: 0 18px;
    border-right: 1px solid var(--line);
  }

  .lockfile-onboarding__steps article:first-child {
    padding-left: 0;
  }

  .lockfile-onboarding__steps article:last-child {
    border-right: 0;
  }

  .lockfile-onboarding__steps article > span {
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 8px;
    font-weight: 700;
  }

  .lockfile-onboarding__steps article.active > span {
    color: var(--brand-soft);
  }

  .lockfile-onboarding__steps article > div {
    display: grid;
    gap: 4px;
  }

  .lockfile-onboarding__steps strong {
    color: var(--text-soft);
    font-size: 10px;
  }

  .lockfile-onboarding__steps p {
    color: var(--text-muted);
    font-size: 8.5px;
    line-height: 1.45;
  }

  /* History */

  .history-panel {
    overflow: visible;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .history-panel__header {
    display: flex;
    min-height: 79px;
    align-items: center;
    justify-content: space-between;
    gap: 20px;
    padding: 14px 17px;
    border-bottom: 1px solid var(--line-soft);
    border-radius:
      var(--radius-md)
      var(--radius-md)
      0 0;
    background: var(--bg-panel-strong);
  }

  .history-panel__header > div {
    display: grid;
    gap: 3px;
  }

  .history-panel__header h2 {
    color: var(--text);
    font-size: 14px;
    font-weight: 610;
  }

  .history-panel__header p:not(.panel-kicker) {
    color: var(--text-muted);
    font-size: 9.5px;
  }

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

  .history-toolbar select {
    min-height: 35px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    background: var(--bg-ink-soft);
    color: var(--text-soft);
    padding: 0 10px;
    font: inherit;
    font-size: 10.5px;
  }

  .history-empty {
    display: grid;
    min-height: 190px;
    place-content: center;
    justify-items: center;
    gap: 6px;
    padding: 25px;
    text-align: center;
  }

  .history-empty__icon {
    display: grid;
    width: 42px;
    height: 42px;
    place-items: center;
    border: 1px solid var(--line);
    border-radius: 10px;
    background: var(--bg-ink-soft);
    color: var(--text-muted);
  }

  .history-empty__icon svg {
    width: 18px;
    height: 18px;
  }

  .history-empty strong {
    color: var(--text-soft);
    font-size: 11px;
  }

  .history-empty p {
    color: var(--text-muted);
    font-size: 9px;
  }

  /* Lockfile rows */

  .lockfile-list {
    display: grid;
  }

  .lockfile-list__heading,
  .lockfile-row {
    display: grid;
    grid-template-columns:
      minmax(260px, 1.25fr)
      minmax(100px, 0.4fr)
      minmax(170px, 0.7fr)
      minmax(130px, auto);
    align-items: center;
    gap: 14px;
  }

  .lockfile-list__heading {
    min-height: 34px;
    padding: 7px 14px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 7px;
    font-weight: 600;
    letter-spacing: 0.07em;
    text-transform: uppercase;
  }

  .lockfile-row {
    min-height: 74px;
    padding: 10px 14px;
    border-bottom: 1px solid var(--line-soft);
    transition:
      background var(--speed) var(--ease),
      box-shadow var(--speed) var(--ease);
  }

  .lockfile-row:last-child {
    border-bottom: 0;
  }

  .lockfile-row:hover {
    background: rgba(255, 255, 255, 0.018);
  }

  .lockfile-row.selected {
    background: var(--brand-faint);
    box-shadow: inset 2px 0 0 var(--brand);
  }

  .lockfile-checksum {
    display: grid;
    min-width: 0;
    grid-template-columns:
      auto minmax(0, 1fr) auto;
    align-items: center;
    gap: 9px;
  }

  .lockfile-icon {
    display: grid;
    width: 33px;
    height: 33px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 8px;
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .lockfile-icon svg {
    width: 16px;
    height: 16px;
  }

  .lockfile-checksum > div {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .lockfile-checksum code {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 8.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .lockfile-checksum small {
    color: var(--text-muted);
    font-size: 7.5px;
  }

  .lockfile-source,
  .lockfile-date {
    display: grid;
    min-width: 0;
    gap: 4px;
  }

  .lockfile-source > span,
  .lockfile-date > span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6.5px;
    text-transform: uppercase;
  }

  .lockfile-source strong {
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
    font-weight: 550;
  }

  .lockfile-date strong {
    overflow-wrap: anywhere;
    color: var(--text-muted);
    font-size: 8.5px;
    font-weight: 500;
    line-height: 1.4;
  }

  .lockfile-controls {
    display: flex;
    align-items: center;
    justify-content: flex-end;
    gap: 6px;
  }

  .inspect-button {
    min-height: 30px;
    border-color: var(--line-strong);
    background: var(--bg-elevated);
    color: var(--text-soft);
    padding: 0 9px;
    font-size: 8.5px;
  }

  .inspect-button:hover:not(:disabled) {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-soft);
    transform: none;
  }

  .lockfile-actions {
    position: relative;
  }

  .lockfile-actions summary {
    display: grid;
    width: 30px;
    height: 30px;
    place-items: center;
    border: 1px solid var(--line);
    border-radius: 7px;
    background: transparent;
    color: var(--text-muted);
    cursor: pointer;
    list-style: none;
  }

  .lockfile-actions summary::-webkit-details-marker {
    display: none;
  }

  .lockfile-actions summary:hover {
    border-color: var(--line-strong);
    background: var(--bg-elevated);
    color: var(--text);
  }

  .lockfile-actions summary svg {
    width: 14px;
    height: 14px;
    fill: currentColor;
  }

  .lockfile-actions > div {
    position: absolute;
    z-index: 20;
    top: calc(100% + 6px);
    right: 0;
    display: grid;
    width: 150px;
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: 8px;
    background: var(--bg-panel-strong);
    box-shadow: var(--shadow-deep);
  }

  .lockfile-actions button {
    min-height: 36px;
    justify-content: flex-start;
    border: 0;
    border-radius: 0;
    background: transparent;
    padding: 0 11px;
    font-size: 9.5px;
  }

  .lockfile-actions button:hover:not(:disabled) {
    background: var(--bg-elevated);
    transform: none;
  }

  .lockfile-actions .action-danger {
    color: var(--danger);
  }

  /* JSON inspector */

  .json-inspector {
    overflow: hidden;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    animation: inspector-enter 260ms var(--ease) both;
  }

  @keyframes inspector-enter {
    from {
      opacity: 0;
      transform: translateY(-5px);
    }
  }

  .json-inspector__header {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 18px;
    padding: 14px 16px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .json-inspector__header > div:first-child {
    display: grid;
    gap: 3px;
  }

  .json-inspector h2 {
    color: var(--text);
    font-size: 14px;
    font-weight: 610;
  }

  .json-inspector__header p:not(.panel-kicker) {
    color: var(--text-muted);
    font-size: 9px;
    line-height: 1.5;
  }

  .json-inspector__actions {
    display: flex;
    flex: 0 0 auto;
    align-items: center;
    gap: 7px;
  }

  .json-inspector__actions
    .button-secondary {
    min-height: 29px;
    padding: 0 9px;
    font-size: 8.5px;
  }

  .json-inspector__metadata {
    display: grid;
    grid-template-columns:
      minmax(0, 1.5fr)
      minmax(120px, 0.5fr);
    border-bottom: 1px solid var(--line-ink);
    background: var(--bg-ink);
  }

  .json-inspector__metadata > div {
    display: grid;
    min-width: 0;
    gap: 4px;
    padding: 10px 14px;
    border-right: 1px solid var(--line-ink);
  }

  .json-inspector__metadata > div:last-child {
    border-right: 0;
  }

  .json-inspector__metadata span {
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 7px;
    text-transform: uppercase;
  }

  .json-inspector__metadata code {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 8px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .json-inspector__metadata strong {
    color: var(--text-soft);
    font-size: 8.5px;
  }

  .json-inspector pre {
    max-height: 520px;
    margin: 0;
    overflow: auto;
    padding: 18px;
    background: var(--bg-ink);
    color: var(--text-soft);
    font-family: var(--font-mono);
    font-size: 10px;
    line-height: 1.7;
    tab-size: 2;
  }

  /* Modal */

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
    width: min(100%, 450px);
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
    border: 1px solid var(--danger-line);
    border-radius: 9px;
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

  .confirmation-field {
    display: grid;
    width: 100%;
    gap: 7px;
  }

  .confirmation-field > span {
    color: var(--text-soft);
    font-size: 9.5px;
    line-height: 1.5;
  }

  .confirmation-field code {
    color: var(--danger);
    font-size: 9px;
  }

  .confirm-modal__actions {
    display: flex;
    width: 100%;
    justify-content: flex-end;
    gap: 8px;
    padding-top: 15px;
    border-top: 1px solid var(--line-soft);
  }

  @media (prefers-reduced-motion: reduce) {
    .lockfiles-page,
    .upload-panel,
    .page-loading__bar,
    .detail-progress,
    .json-inspector {
      animation: none;
    }
  }

  @media (max-width: 1060px) {
    .upload-form {
      grid-template-columns: 1fr;
    }

    .upload-form__fields {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .upload-preview {
      grid-template-columns:
        auto minmax(0, 1fr);
      align-items: center;
    }

    .upload-preview > .panel-kicker,
    .upload-preview dl,
    .upload-preview__workflow {
      grid-column: 1 / -1;
    }

    .lockfile-onboarding {
      grid-template-columns: 1fr;
    }

    .lockfile-list__heading,
    .lockfile-row {
      grid-template-columns:
        minmax(250px, 1.2fr)
        minmax(100px, 0.4fr)
        minmax(125px, auto);
    }

    .lockfile-date {
      display: none;
    }
  }

  @media (max-width: 900px) {
    .lockfiles-layout {
      grid-template-columns: 1fr;
    }

    .project-directory {
      position: static;
    }

    .project-options {
      max-height: 260px;
    }
  }

  @media (max-width: 760px) {
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

    .project-overview__header {
      align-items: flex-start;
      flex-direction: column;
    }

    .project-link {
      width: 100%;
      justify-content: space-between;
    }

    .lockfile-metrics {
      grid-template-columns: 1fr;
    }

    .lockfile-metrics article {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .lockfile-metrics article:last-child {
      border-bottom: 0;
    }

    .history-toolbar {
      grid-template-columns: 1fr;
    }

    .lockfile-list__heading {
      display: none;
    }

    .lockfile-row {
      grid-template-columns:
        minmax(0, 1fr) auto;
      gap: 13px;
      padding: 14px;
    }

    .lockfile-checksum {
      grid-column: 1;
    }

    .lockfile-controls {
      grid-column: 2;
      grid-row: 1;
    }

    .lockfile-source {
      grid-column: 1 / -1;
      padding-top: 10px;
      border-top: 1px solid var(--line-soft);
    }

    .lockfile-onboarding__steps {
      grid-template-columns: 1fr;
      gap: 15px;
    }

    .lockfile-onboarding__steps article,
    .lockfile-onboarding__steps article:first-child {
      padding: 0;
      border-right: 0;
    }

    .json-inspector__metadata {
      grid-template-columns: 1fr;
    }

    .json-inspector__metadata > div {
      border-right: 0;
      border-bottom: 1px solid var(--line-ink);
    }

    .json-inspector__metadata > div:last-child {
      border-bottom: 0;
    }
  }

  @media (max-width: 620px) {
    .checksum-input {
      grid-template-columns: 1fr;
    }

    .upload-form__actions,
    .confirm-modal__actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .upload-form__actions button,
    .confirm-modal__actions button {
      width: 100%;
    }

    .lockfile-onboarding {
      padding: 26px 19px;
    }

    .lockfile-onboarding__copy h2 {
      font-size: clamp(31px, 10vw, 43px);
    }

    .lockfile-terminal > footer {
      align-items: flex-start;
      flex-direction: column;
      justify-content: center;
      padding-block: 10px;
    }

    .archived-notice {
      grid-template-columns:
        auto minmax(0, 1fr);
    }

    .archived-notice > a {
      grid-column: 2;
    }

    .json-inspector__header {
      align-items: stretch;
      flex-direction: column;
    }

    .json-inspector__actions {
      justify-content: space-between;
    }
  }

  @media (max-width: 500px) {
    .upload-form__fields,
    .upload-preview {
      padding: 16px;
    }

    .lockfile-row {
      grid-template-columns: 1fr;
    }

    .lockfile-controls {
      grid-column: 1;
      grid-row: auto;
      justify-content: flex-start;
    }

    .lockfile-source {
      grid-column: 1;
    }

    .lockfile-actions > div {
      position: fixed;
      right: 16px;
      bottom: 16px;
      top: auto;
      width: calc(100% - 32px);
    }

    .latest-snapshot {
      grid-template-columns:
        auto minmax(0, 1fr);
    }

    .latest-snapshot
      > .copy-icon-button {
      grid-column: 2;
      width: 100%;
    }
  }
</style>
