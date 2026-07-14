<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount, tick } from 'svelte';

  import { usePageRefresh } from '$lib/stores/pageRefresh';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  import {
    deleteBuildReport,
    listBuildReports,
    submitBuildReport
  } from '$lib/api/buildReports';

  import { listProjects } from '$lib/api/projects';
  import { listWorkspaces } from '$lib/api/workspaces';

  import {
    ApiError,
    type BuildReport,
    type Project,
    type Workspace
  } from '$lib/api/types';

  import InlineError from '$lib/components/InlineError.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import WorkspaceAvatar from '$lib/components/WorkspaceAvatar.svelte';

  import { canSubmitBuildReport } from '$lib/permissions';

  type ReportStatus =
    | 'success'
    | 'warning'
    | 'failed';

  type ReportFilter =
    | 'all'
    | ReportStatus;

  type Confirmation = {
    report: BuildReport;
  };

  let workspaces: Workspace[] = [];
  let projects: Project[] = [];
  let reports: BuildReport[] = [];

  let selectedWorkspaceId = '';
  let selectedProjectId = '';
  let selectedReportId = '';

  let loading = true;
  let loadingReports = false;
  let initialized = false;

  let projectListRequestId = 0;
  let reportRequestId = 0;

  let saving = false;
  let busyReportId = '';

  let error = '';
  let success = '';

  let projectQuery = '';
  let reportQuery = '';
  let statusFilter: ReportFilter = 'all';

  let showSubmitForm = false;

  let status: ReportStatus = 'success';
  let target = '';
  let profile = 'debug';
  let branch = '';
  let commitSha = '';
  let toolchain = '';
  let durationMs = 0;
  let warningsCount = 0;
  let errorsCount = 0;

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

  $: selectedReport =
    reports.find(
      (report) =>
        report.id === selectedReportId
    ) ?? null;

  $: currentRole =
    selectedWorkspace?.current_user_role ??
    'viewer';

  $: canSubmit =
    canSubmitBuildReport(currentRole);

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

  $: visibleReports = reports.filter(
    (report) => {
      const query =
        reportQuery.trim().toLowerCase();

      const matchesQuery =
        !query ||
        report.target
          .toLowerCase()
          .includes(query) ||
        report.profile
          ?.toLowerCase()
          .includes(query) ||
        report.branch
          ?.toLowerCase()
          .includes(query) ||
        report.commit_sha
          ?.toLowerCase()
          .includes(query) ||
        report.toolchain
          ?.toLowerCase()
          .includes(query);

      const matchesStatus =
        statusFilter === 'all' ||
        report.status === statusFilter;

      return matchesQuery && matchesStatus;
    }
  );

  $: successfulReports =
    reports.filter(
      (report) =>
        report.status === 'success'
    ).length;

  $: warningReports =
    reports.filter(
      (report) =>
        report.status === 'warning'
    ).length;

  $: failedReports =
    reports.filter(
      (report) =>
        report.status === 'failed'
    ).length;

  $: totalWarnings =
    reports.reduce(
      (total, report) =>
        total +
        Number(
          report.warnings_count || 0
        ),
      0
    );

  $: totalErrors =
    reports.reduce(
      (total, report) =>
        total +
        Number(
          report.errors_count || 0
        ),
      0
    );

  $: averageDuration =
    reports.length > 0
      ? reports.reduce(
          (total, report) =>
            total +
            Number(
              report.duration_ms || 0
            ),
          0
        ) / reports.length
      : 0;

  $: successRate =
    reports.length > 0
      ? Math.round(
          (
            successfulReports /
            reports.length
          ) * 100
        )
      : 0;

  $: latestReport =
    reports[0] ?? null;

  $: latestStatus =
    latestReport?.status ?? '';

  $: confirmationToken =
    confirmation
      ? reportConfirmationToken(
          confirmation.report
        )
      : '';

  $: confirmationCanContinue =
    Boolean(confirmation) &&
    confirmationValue === confirmationToken;

  $: if (
    selectedReportId &&
    !reports.some(
      (report) =>
        report.id === selectedReportId
    )
  ) {
    selectedReportId = '';
  }

  function projectInitial(
    project: Project
  ) {
    return project.name
      .slice(0, 1)
      .toUpperCase();
  }

  function statusLabel(
    value?: string
  ) {
    if (value === 'success') {
      return 'Successful';
    }

    if (value === 'warning') {
      return 'Completed with warnings';
    }

    if (value === 'failed') {
      return 'Failed';
    }

    return 'Unknown';
  }

  function formatDuration(
    value?: number
  ) {
    const duration =
      Number(value || 0);

    if (duration < 1000) {
      return `${duration} ms`;
    }

    if (duration < 60_000) {
      return `${(
        duration / 1000
      ).toFixed(
        duration >= 10_000
          ? 0
          : 1
      )} s`;
    }

    const minutes =
      Math.floor(
        duration / 60_000
      );

    const seconds =
      Math.round(
        (
          duration % 60_000
        ) / 1000
      );

    return `${minutes}m ${seconds}s`;
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
        value <
        1_000_000_000_000
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
      Number.isNaN(
        date.getTime()
      )
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

  function shortCommit(
    value?: string
  ) {
    if (!value) {
      return 'No commit';
    }

    return value.length > 12
      ? value.slice(0, 12)
      : value;
  }

  function diagnosticsFor(
    report: BuildReport
  ) {
    const value =
      report.diagnostics_json ||
      report.summary_json ||
      '';

    if (!value) {
      return '';
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

  function hasDiagnostics(
    report: BuildReport
  ) {
    return Boolean(
      report.diagnostics_json ||
      report.summary_json
    );
  }

  function reportConfirmationToken(
    report: BuildReport
  ) {
    const commit =
      report.commit_sha?.trim();

    if (commit) {
      return shortCommit(commit);
    }

    return report.target
      .trim()
      .slice(0, 16);
  }

  function resetSubmitForm() {
    status = 'success';
    target = '';
    profile = 'debug';
    branch = '';
    commitSha = '';
    toolchain = '';
    durationMs = 0;
    warningsCount = 0;
    errorsCount = 0;
  }

  function openSubmitForm() {
    if (!canSubmit) {
      return;
    }

    showSubmitForm = true;
    error = '';
    success = '';
  }

  function closeSubmitForm() {
    if (saving) {
      return;
    }

    showSubmitForm = false;
    resetSubmitForm();
    error = '';
  }

  function toggleSubmitForm() {
    if (showSubmitForm) {
      closeSubmitForm();
      return;
    }

    openSubmitForm();
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
        clearTimeout(
          copyResetTimer
        );
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

  async function load() {
    const user = $auth.user;

    if (
      !$auth.session ||
      !user
    ) {
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
          : 'Unable to load build reports.';
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
      reports = [];

      selectedProjectId = '';
      selectedReportId = '';

      updatePageUrl();
      return;
    }

    workspaceContext.setSelectedWorkspace(
      workspaceId
    );

    const data =
      await listProjects(workspaceId);

    if (
      requestId !==
        projectListRequestId ||
      selectedWorkspaceId !==
        workspaceId
    ) {
      return;
    }

    projects = data.projects;

    selectedProjectId =
      requestedProjectId &&
      projects.some(
        (project) =>
          project.id ===
          requestedProjectId
      )
        ? requestedProjectId
        : projects[0]?.id ?? '';

    updatePageUrl(
      selectedProjectId
    );

    await loadReports(
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
    selectedReportId = '';

    projects = [];
    reports = [];

    projectQuery = '';
    reportQuery = '';
    statusFilter = 'all';

    showSubmitForm = false;
    resetSubmitForm();

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
          : 'Unable to load build reports.';
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

    selectedReportId = '';

    reportQuery = '';
    statusFilter = 'all';
    copiedKey = '';

    showSubmitForm = false;
    resetSubmitForm();

    confirmation = null;

    error = '';
    success = '';

    updatePageUrl(project.id);

    await loadReports(project.id);
  }

  async function loadReports(
    projectId: string
  ) {
    if (
      !selectedWorkspaceId ||
      !projectId
    ) {
      reports = [];
      selectedReportId = '';
      return;
    }

    const requestId =
      ++reportRequestId;

    const workspaceId =
      selectedWorkspaceId;

    loadingReports = true;
    error = '';

    reports = [];
    selectedReportId = '';

    try {
      const data =
        await listBuildReports(
          workspaceId,
          projectId
        );

      if (
        requestId !==
          reportRequestId ||
        selectedWorkspaceId !==
          workspaceId ||
        selectedProjectId !==
          projectId
      ) {
        return;
      }

      reports =
        data.build_reports;
    } catch (err) {
      if (
        requestId !==
        reportRequestId
      ) {
        return;
      }

      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load build history.';
    } finally {
      if (
        requestId ===
        reportRequestId
      ) {
        loadingReports = false;
      }
    }
  }

  async function submitReport() {
    const user = $auth.user;

    const cleanTarget =
      target.trim();

    if (
      !user ||
      !selectedWorkspaceId ||
      !selectedProjectId ||
      !cleanTarget ||
      !canSubmit
    ) {
      return;
    }

    saving = true;
    error = '';
    success = '';

    try {
      const submitted =
        await submitBuildReport({
          workspaceId:
            selectedWorkspaceId,

          projectId:
            selectedProjectId,

          submittedByUserId:
            user.id,

          status,

          target:
            cleanTarget,

          profile:
            profile.trim() ||
            'debug',

          branch:
            branch.trim(),

          commitSha:
            commitSha.trim(),

          toolchain:
            toolchain.trim(),

          durationMs:
            Math.max(
              0,
              Number(durationMs) || 0
            ),

          warningsCount:
            Math.max(
              0,
              Number(warningsCount) || 0
            ),

          errorsCount:
            Math.max(
              0,
              Number(errorsCount) || 0
            )
        });

      reports = [
        submitted.build_report,
        ...reports
      ];

      selectedReportId =
        submitted.build_report.id;

      success =
        'Build result recorded successfully.';

      resetSubmitForm();
      showSubmitForm = false;
      statusFilter = 'all';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to submit build report.';
    } finally {
      saving = false;
    }
  }

  function inspectReport(
    report: BuildReport
  ) {
    selectedReportId =
      selectedReportId === report.id
        ? ''
        : report.id;
  }

  async function openConfirmation(
    report: BuildReport
  ) {
    confirmation = {
      report
    };

    confirmationValue = '';

    await tick();

    confirmationDialog?.focus();
  }

  function closeConfirmation() {
    if (busyReportId) {
      return;
    }

    confirmation = null;
    confirmationValue = '';
  }

  async function confirmHideReport() {
    if (
      !confirmation ||
      !selectedWorkspaceId ||
      !selectedProjectId ||
      !canSubmit ||
      !confirmationCanContinue
    ) {
      return;
    }

    const report =
      confirmation.report;

    busyReportId =
      report.id;

    error = '';
    success = '';

    try {
      await deleteBuildReport(
        selectedWorkspaceId,
        selectedProjectId,
        report.id
      );

      reports =
        reports.filter(
          (item) =>
            item.id !== report.id
        );

      if (
        selectedReportId ===
        report.id
      ) {
        selectedReportId = '';
      }

      success =
        'Build report hidden from normal history.';

      confirmation = null;
      confirmationValue = '';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to hide build report.';
    } finally {
      busyReportId = '';
    }
  }

  function handleModalKeydown(
    event: KeyboardEvent
  ) {
    if (
      event.key === 'Escape' &&
      confirmation &&
      !busyReportId
    ) {
      event.preventDefault();
      closeConfirmation();
    }
  }

  usePageRefresh(load);

  onMount(() => {
    return () => {
      if (copyResetTimer) {
        clearTimeout(
          copyResetTimer
        );
      }
    };
  });
</script>

<svelte:head>
  <title>
    Build reports | Softadastra Cloud
  </title>

  <meta
    name="description"
    content="Review local and CI build results, diagnostics, warnings, errors, duration, branches and toolchains in Softadastra Cloud."
  />
</svelte:head>

<svelte:window
  onkeydown={handleModalKeydown}
/>

<div class="reports-page">
  <!-- ===================================================
       HEADER
       =================================================== -->
  <header class="page-header">
    <div class="page-heading">
      <p class="page-eyebrow">
        <span aria-hidden="true"></span>
        Build health
      </p>

      <h1>Build reports</h1>

      <p>
        {#if loading}
          Loading build history…
        {:else if selectedProject}
          Local and CI build results for
          <strong>
            {selectedProject.name}
          </strong>.
        {:else if selectedWorkspace}
          Connect a project before recording build
          results.
        {:else}
          Create a workspace before collecting build
          history.
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
        canSubmit
      }
        <button
          class:button-secondary={
            showSubmitForm
          }
          class="header-action"
          type="button"
          onclick={toggleSubmitForm}
        >
          {#if !showSubmitForm}
            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path d="M12 5v14"></path>
              <path d="M5 12h14"></path>
            </svg>
          {/if}

          {showSubmitForm
            ? 'Close'
            : reports.length === 0
              ? 'Record first build'
              : 'Submit manually'}
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
        B
      </span>

      <div>
        <strong>
          Loading build history
        </strong>

        <p>
          Preparing projects, build status and
          diagnostics.
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
          Build history belongs to a workspace.
        </h2>

        <p>
          The workspace connects projects, members
          and the build reports shared by the team.
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
          Connect the project before recording its
          builds.
        </h2>

        <p>
          Each report belongs to one project so its
          branch, commit, toolchain and diagnostics
          remain attached to the correct history.
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
         MANUAL SUBMISSION
         ================================================= -->
    {#if
      showSubmitForm &&
      selectedProject &&
      canSubmit
    }
      <section
        class="submit-panel"
        aria-labelledby="submit-title"
      >
        <header class="submit-panel__header">
          <span
            class="submit-panel__icon"
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
              <path d="M5 4h14v16H5z"></path>
              <path d="m8 9 2 2-2 2"></path>
              <path d="M12 13h4"></path>
            </svg>
          </span>

          <div>
            <p class="panel-kicker">
              Manual build record
            </p>

            <h2 id="submit-title">
              Record a build result
            </h2>

            <p>
              Use this for imported or external
              builds. The preferred Vix workflow
              submits the report automatically.
            </p>
          </div>
        </header>

        <form
          class="submit-form"
          onsubmit={(event) => {
            event.preventDefault();
            void submitReport();
          }}
        >
          <div class="submit-form__fields">
            <fieldset>
              <legend>Build result</legend>

              <div class="status-options">
                <button
                  class:selected={
                    status === 'success'
                  }
                  class="status-option status-option--success"
                  type="button"
                  aria-pressed={
                    status === 'success'
                  }
                  onclick={() => {
                    status = 'success';
                  }}
                >
                  <span
                    class="status-option__indicator"
                    aria-hidden="true"
                  ></span>

                  <span>
                    <strong>Success</strong>
                    <small>
                      The target completed without
                      errors.
                    </small>
                  </span>
                </button>

                <button
                  class:selected={
                    status === 'warning'
                  }
                  class="status-option status-option--warning"
                  type="button"
                  aria-pressed={
                    status === 'warning'
                  }
                  onclick={() => {
                    status = 'warning';
                  }}
                >
                  <span
                    class="status-option__indicator"
                    aria-hidden="true"
                  ></span>

                  <span>
                    <strong>Warning</strong>
                    <small>
                      The build completed with
                      warnings.
                    </small>
                  </span>
                </button>

                <button
                  class:selected={
                    status === 'failed'
                  }
                  class="status-option status-option--failed"
                  type="button"
                  aria-pressed={
                    status === 'failed'
                  }
                  onclick={() => {
                    status = 'failed';
                  }}
                >
                  <span
                    class="status-option__indicator"
                    aria-hidden="true"
                  ></span>

                  <span>
                    <strong>Failed</strong>
                    <small>
                      The target did not complete.
                    </small>
                  </span>
                </button>
              </div>
            </fieldset>

            <div class="field-grid">
              <label>
                <span>Target</span>

                <input
                  bind:value={target}
                  placeholder="app"
                  required
                  autocomplete="off"
                />
              </label>

              <label>
                <span>Profile</span>

                <input
                  bind:value={profile}
                  placeholder="debug"
                  autocomplete="off"
                />
              </label>

              <label>
                <span>Branch</span>

                <input
                  bind:value={branch}
                  placeholder="main"
                  autocomplete="off"
                />
              </label>

              <label>
                <span>Commit SHA</span>

                <input
                  bind:value={commitSha}
                  placeholder="a1b2c3d4"
                  autocomplete="off"
                  spellcheck="false"
                />
              </label>

              <label>
                <span>Toolchain</span>

                <input
                  bind:value={toolchain}
                  placeholder="gcc-14"
                  autocomplete="off"
                />
              </label>

              <label>
                <span>Duration in milliseconds</span>

                <input
                  bind:value={durationMs}
                  type="number"
                  min="0"
                  step="1"
                />
              </label>

              <label>
                <span>Warnings</span>

                <input
                  bind:value={warningsCount}
                  type="number"
                  min="0"
                  step="1"
                />
              </label>

              <label>
                <span>Errors</span>

                <input
                  bind:value={errorsCount}
                  type="number"
                  min="0"
                  step="1"
                />
              </label>
            </div>
          </div>

          <aside class="submit-preview">
            <p class="panel-kicker">
              Build preview
            </p>

            <span
              class:submit-preview__mark--success={
                status === 'success'
              }
              class:submit-preview__mark--warning={
                status === 'warning'
              }
              class:submit-preview__mark--failed={
                status === 'failed'
              }
              class="submit-preview__mark"
            >
              {#if status === 'success'}
                <svg
                  viewBox="0 0 24 24"
                  aria-hidden="true"
                >
                  <path d="m5 12 4 4L19 6"></path>
                </svg>
              {:else if status === 'warning'}
                <svg
                  viewBox="0 0 24 24"
                  aria-hidden="true"
                >
                  <path d="M12 9v4"></path>
                  <path d="M12 17h.01"></path>
                  <path d="M12 3 2 21h20L12 3Z"></path>
                </svg>
              {:else}
                <svg
                  viewBox="0 0 24 24"
                  aria-hidden="true"
                >
                  <path d="M6 6l12 12"></path>
                  <path d="M18 6 6 18"></path>
                </svg>
              {/if}
            </span>

            <div class="submit-preview__identity">
              <strong>
                {target.trim() || 'app'}
              </strong>

              <span>
                {statusLabel(status)}
              </span>
            </div>

            <dl>
              <div>
                <dt>Profile</dt>

                <dd>
                  {profile.trim() || 'debug'}
                </dd>
              </div>

              <div>
                <dt>Source</dt>

                <dd>
                  {branch.trim() || 'No branch'}
                  ·
                  {shortCommit(
                    commitSha.trim()
                  )}
                </dd>
              </div>

              <div>
                <dt>Toolchain</dt>

                <dd>
                  {toolchain.trim() ||
                    'Not provided'}
                </dd>
              </div>

              <div>
                <dt>Duration</dt>

                <dd>
                  {formatDuration(
                    Number(durationMs) || 0
                  )}
                </dd>
              </div>
            </dl>

            <div class="submit-preview__diagnostics">
              <span>
                <strong>
                  {Number(warningsCount) || 0}
                </strong>
                warnings
              </span>

              <span>
                <strong>
                  {Number(errorsCount) || 0}
                </strong>
                errors
              </span>
            </div>

            <div class="submit-preview__workflow">
              <span>Preferred workflow</span>

              <code>
                <i>$</i>
                vix build --report
              </code>
            </div>
          </aside>

          <div class="submit-form__actions">
            <button
              class="button-secondary"
              type="button"
              disabled={saving}
              onclick={closeSubmitForm}
            >
              Cancel
            </button>

            <button
              type="submit"
              disabled={
                saving ||
                !target.trim()
              }
            >
              {saving
                ? 'Recording build…'
                : 'Record build'}
            </button>
          </div>
        </form>
      </section>
    {/if}

    <div class="reports-layout">
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
           BUILD DETAIL
           =============================================== -->
      <main class="report-detail">
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

            {#if loadingReports}
              <span
                class="detail-progress"
                aria-hidden="true"
              ></span>
            {/if}

            <div class="build-metrics">
              <article>
                <span>Latest build</span>

                <strong
                  class:metric-success={
                    latestStatus === 'success'
                  }
                  class:metric-warning={
                    latestStatus === 'warning'
                  }
                  class:metric-failed={
                    latestStatus === 'failed'
                  }
                >
                  {latestReport
                    ? statusLabel(
                        latestReport.status
                      )
                    : 'No build'}
                </strong>

                <small>
                  {latestReport
                    ? latestReport.target
                    : 'Record the first result'}
                </small>
              </article>

              <article>
                <span>Success rate</span>

                <strong>
                  {successRate}%
                </strong>

                <small>
                  {successfulReports} of
                  {reports.length} builds
                </small>
              </article>

              <article>
                <span>Average duration</span>

                <strong>
                  {formatDuration(
                    averageDuration
                  )}
                </strong>

                <small>
                  Across recorded builds
                </small>
              </article>

              <article>
                <span>Diagnostics</span>

                <strong>
                  {totalWarnings + totalErrors}
                </strong>

                <small>
                  {totalWarnings} warnings ·
                  {totalErrors} errors
                </small>
              </article>
            </div>

            {#if latestReport}
              <div
                class:latest-build--success={
                  latestReport.status ===
                  'success'
                }
                class:latest-build--warning={
                  latestReport.status ===
                  'warning'
                }
                class:latest-build--failed={
                  latestReport.status ===
                  'failed'
                }
                class="latest-build"
              >
                <span
                  class="latest-build__status"
                  aria-hidden="true"
                >
                  {#if latestReport.status === 'success'}
                    <svg
                      viewBox="0 0 24 24"
                    >
                      <path d="m5 12 4 4L19 6"></path>
                    </svg>
                  {:else if latestReport.status === 'warning'}
                    <svg
                      viewBox="0 0 24 24"
                    >
                      <path d="M12 9v4"></path>
                      <path d="M12 17h.01"></path>
                      <path d="M12 3 2 21h20L12 3Z"></path>
                    </svg>
                  {:else}
                    <svg
                      viewBox="0 0 24 24"
                    >
                      <path d="M6 6l12 12"></path>
                      <path d="M18 6 6 18"></path>
                    </svg>
                  {/if}
                </span>

                <div class="latest-build__identity">
                  <span>Latest recorded build</span>

                  <strong>
                    {latestReport.target}
                    ·
                    {latestReport.profile ||
                      'default'}
                  </strong>

                  <small>
                    {latestReport.branch ||
                      'No branch'}
                    ·
                    {shortCommit(
                      latestReport.commit_sha
                    )}
                    ·
                    {formatDate(
                      latestReport.created_at
                    )}
                  </small>
                </div>

                <div class="latest-build__result">
                  <strong>
                    {formatDuration(
                      latestReport.duration_ms
                    )}
                  </strong>

                  <span>
                    {latestReport.warnings_count || 0}
                    warnings ·
                    {latestReport.errors_count || 0}
                    errors
                  </span>
                </div>

                <button
                  type="button"
                  onclick={() =>
                    inspectReport(
                      latestReport
                    )}
                >
                  Inspect
                </button>
              </div>
            {/if}
          </section>

          {#if !canSubmit}
            <ReadOnlyNotice
              message="Your role can inspect build history and diagnostics, but cannot submit or hide build reports."
            />
          {/if}

          <!-- ===========================================
               FIRST BUILD
               =========================================== -->
          {#if
            !loadingReports &&
            reports.length === 0 &&
            !showSubmitForm
          }
            <section class="build-onboarding">
              <div class="build-onboarding__copy">
                <p class="panel-kicker">
                  First build report
                </p>

                <h2>
                  Build locally.
                  <span>
                    Share the result with the team.
                  </span>
                </h2>

                <p>
                  Softadastra Cloud does not replace
                  the local Vix build. It records the
                  target, toolchain, duration,
                  warnings, errors and diagnostics so
                  the result remains visible after the
                  terminal is closed.
                </p>

                {#if canSubmit}
                  <button
                    class="manual-submit-action"
                    type="button"
                    onclick={openSubmitForm}
                  >
                    Record manually
                  </button>
                {/if}
              </div>

              <div class="build-terminal">
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

                <div class="build-terminal__body">
                  <div class="terminal-command">
                    <span>$</span>

                    <code>
                      cd {selectedProject.slug}
                    </code>
                  </div>

                  <div class="terminal-command terminal-command--primary">
                    <span>$</span>

                    <code>
                      vix build --report
                    </code>
                  </div>

                  <div class="terminal-output">
                    <span>→</span>

                    <code>
                      Building target app with gcc-14
                    </code>
                  </div>

                  <div class="terminal-progress">
                    <span></span>
                  </div>

                  <div class="terminal-output terminal-output--success">
                    <span>✓</span>

                    <code>
                      Build completed · report uploaded
                    </code>
                  </div>
                </div>

                <footer>
                  <span>
                    Compilation stays local
                  </span>

                  <span>
                    Results become shared
                  </span>
                </footer>
              </div>

              <div class="build-onboarding__steps">
                <article class="active">
                  <span>01</span>

                  <div>
                    <strong>
                      Run the build
                    </strong>

                    <p>
                      Compile the target through the
                      normal Vix workflow.
                    </p>
                  </div>
                </article>

                <article>
                  <span>02</span>

                  <div>
                    <strong>
                      Capture the result
                    </strong>

                    <p>
                      Record duration, toolchain and
                      diagnostics.
                    </p>
                  </div>
                </article>

                <article>
                  <span>03</span>

                  <div>
                    <strong>
                      Keep the history
                    </strong>

                    <p>
                      Compare later builds without
                      moving compilation to Cloud.
                    </p>
                  </div>
                </article>
              </div>
            </section>

          <!-- ===========================================
               BUILD HISTORY
               =========================================== -->
          {:else if reports.length > 0}
            <section class="history-panel">
              <header class="history-panel__header">
                <div>
                  <p class="panel-kicker">
                    Project builds
                  </p>

                  <h2>Build history</h2>

                  <p>
                    Local and CI results recorded for
                    this project.
                  </p>
                </div>

                <span class="history-count">
                  {visibleReports.length}
                </span>
              </header>

              <div class="history-toolbar">
                <label class="report-search">
                  <span class="visually-hidden">
                    Find a report
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
                    bind:value={reportQuery}
                    type="search"
                    placeholder="Find target, branch or commit"
                  />
                </label>

                <nav
                  class="status-filters"
                  aria-label="Build status filters"
                >
                  <button
                    class:active={
                      statusFilter === 'all'
                    }
                    type="button"
                    onclick={() => {
                      statusFilter = 'all';
                    }}
                  >
                    All

                    <span>
                      {reports.length}
                    </span>
                  </button>

                  <button
                    class:active={
                      statusFilter ===
                      'success'
                    }
                    type="button"
                    onclick={() => {
                      statusFilter =
                        'success';
                    }}
                  >
                    Success

                    <span>
                      {successfulReports}
                    </span>
                  </button>

                  {#if warningReports > 0}
                    <button
                      class:active={
                        statusFilter ===
                        'warning'
                      }
                      type="button"
                      onclick={() => {
                        statusFilter =
                          'warning';
                      }}
                    >
                      Warning

                      <span>
                        {warningReports}
                      </span>
                    </button>
                  {/if}

                  {#if failedReports > 0}
                    <button
                      class:active={
                        statusFilter ===
                        'failed'
                      }
                      type="button"
                      onclick={() => {
                        statusFilter =
                          'failed';
                      }}
                    >
                      Failed

                      <span>
                        {failedReports}
                      </span>
                    </button>
                  {/if}
                </nav>
              </div>

              {#if visibleReports.length === 0}
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
                    No matching build reports
                  </strong>

                  <p>
                    Change the search text or selected
                    build status.
                  </p>
                </div>
              {:else}
                <div class="report-list">
                  <div class="report-list__heading">
                    <span>Build</span>
                    <span>Source</span>
                    <span>Diagnostics</span>
                    <span>Duration</span>
                    <span>Status</span>
                    <span></span>
                  </div>

                  {#each visibleReports as report (report.id)}
                    <article
                      class:selected={
                        selectedReportId ===
                        report.id
                      }
                      class="report-row"
                    >
                      <div class="report-build">
                        <span
                          class:report-icon--success={
                            report.status ===
                            'success'
                          }
                          class:report-icon--warning={
                            report.status ===
                            'warning'
                          }
                          class:report-icon--failed={
                            report.status ===
                            'failed'
                          }
                          class="report-icon"
                          aria-hidden="true"
                        >
                          {#if report.status === 'success'}
                            <svg
                              viewBox="0 0 24 24"
                            >
                              <path d="m5 12 4 4L19 6"></path>
                            </svg>
                          {:else if report.status === 'warning'}
                            <svg
                              viewBox="0 0 24 24"
                            >
                              <path d="M12 9v4"></path>
                              <path d="M12 17h.01"></path>
                              <path d="M12 3 2 21h20L12 3Z"></path>
                            </svg>
                          {:else}
                            <svg
                              viewBox="0 0 24 24"
                            >
                              <path d="M6 6l12 12"></path>
                              <path d="M18 6 6 18"></path>
                            </svg>
                          {/if}
                        </span>

                        <div>
                          <strong>
                            {report.target}
                          </strong>

                          <small>
                            {report.profile ||
                              'default'}
                            ·
                            {formatDate(
                              report.created_at
                            )}
                          </small>
                        </div>
                      </div>

                      <div class="report-source">
                        <span>Source</span>

                        <strong>
                          {report.branch ||
                            'No branch'}
                        </strong>

                        <code
                          title={report.commit_sha}
                        >
                          {shortCommit(
                            report.commit_sha
                          )}
                        </code>

                        {#if report.toolchain}
                          <small>
                            {report.toolchain}
                          </small>
                        {/if}
                      </div>

                      <div class="report-diagnostics">
                        <span>Diagnostics</span>

                        <div>
                          <strong
                            class:has-warnings={
                              Number(
                                report.warnings_count ||
                                0
                              ) > 0
                            }
                          >
                            {report.warnings_count ||
                              0}
                            warnings
                          </strong>

                          <strong
                            class:has-errors={
                              Number(
                                report.errors_count ||
                                0
                              ) > 0
                            }
                          >
                            {report.errors_count ||
                              0}
                            errors
                          </strong>
                        </div>
                      </div>

                      <div class="report-duration">
                        <span>Duration</span>

                        <strong>
                          {formatDuration(
                            report.duration_ms
                          )}
                        </strong>
                      </div>

                      <div class="report-state">
                        <span>Status</span>

                        <StatusBadge
                          status={report.status}
                        />
                      </div>

                      <div class="report-controls">
                        <button
                          class="inspect-button"
                          type="button"
                          aria-expanded={
                            selectedReportId ===
                            report.id
                          }
                          onclick={() =>
                            inspectReport(report)}
                        >
                          {selectedReportId ===
                          report.id
                            ? 'Close'
                            : 'Inspect'}
                        </button>

                        {#if canSubmit}
                          <details class="report-actions">
                            <summary
                              aria-label={`Manage build report for ${report.target}`}
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
                                  busyReportId ===
                                  report.id
                                }
                                onclick={() =>
                                  openConfirmation(
                                    report
                                  )}
                              >
                                Hide report
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
                 REPORT INSPECTOR
                 ========================================= -->
            {#if selectedReport}
              <section
                class:report-inspector--success={
                  selectedReport.status ===
                  'success'
                }
                class:report-inspector--warning={
                  selectedReport.status ===
                  'warning'
                }
                class:report-inspector--failed={
                  selectedReport.status ===
                  'failed'
                }
                class="report-inspector"
                aria-labelledby="report-inspector-title"
              >
                <header class="report-inspector__header">
                  <div class="report-inspector__title">
                    <span
                      class="report-inspector__status"
                      aria-hidden="true"
                    >
                      {#if selectedReport.status === 'success'}
                        <svg
                          viewBox="0 0 24 24"
                        >
                          <path d="m5 12 4 4L19 6"></path>
                        </svg>
                      {:else if selectedReport.status === 'warning'}
                        <svg
                          viewBox="0 0 24 24"
                        >
                          <path d="M12 9v4"></path>
                          <path d="M12 17h.01"></path>
                          <path d="M12 3 2 21h20L12 3Z"></path>
                        </svg>
                      {:else}
                        <svg
                          viewBox="0 0 24 24"
                        >
                          <path d="M6 6l12 12"></path>
                          <path d="M18 6 6 18"></path>
                        </svg>
                      {/if}
                    </span>

                    <div>
                      <p class="panel-kicker">
                        Selected build
                      </p>

                      <h2 id="report-inspector-title">
                        {selectedReport.target}
                        ·
                        {selectedReport.profile ||
                          'default'}
                      </h2>

                      <p>
                        {statusLabel(
                          selectedReport.status
                        )}
                        on
                        {formatDate(
                          selectedReport.created_at
                        )}.
                      </p>
                    </div>
                  </div>

                  <button
                    class="icon-close-button"
                    type="button"
                    aria-label="Close build inspector"
                    onclick={() => {
                      selectedReportId = '';
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
                </header>

                <div class="report-inspector__metrics">
                  <article>
                    <span>Status</span>

                    <strong>
                      {statusLabel(
                        selectedReport.status
                      )}
                    </strong>
                  </article>

                  <article>
                    <span>Duration</span>

                    <strong>
                      {formatDuration(
                        selectedReport.duration_ms
                      )}
                    </strong>
                  </article>

                  <article>
                    <span>Warnings</span>

                    <strong
                      class:metric-warning={
                        Number(
                          selectedReport.warnings_count ||
                          0
                        ) > 0
                      }
                    >
                      {selectedReport.warnings_count ||
                        0}
                    </strong>
                  </article>

                  <article>
                    <span>Errors</span>

                    <strong
                      class:metric-failed={
                        Number(
                          selectedReport.errors_count ||
                          0
                        ) > 0
                      }
                    >
                      {selectedReport.errors_count ||
                        0}
                    </strong>
                  </article>
                </div>

                <dl class="report-metadata">
                  <div>
                    <dt>Project</dt>

                    <dd>
                      {selectedProject.name}
                    </dd>
                  </div>

                  <div>
                    <dt>Target</dt>

                    <dd>
                      {selectedReport.target}
                    </dd>
                  </div>

                  <div>
                    <dt>Profile</dt>

                    <dd>
                      {selectedReport.profile ||
                        'default'}
                    </dd>
                  </div>

                  <div>
                    <dt>Branch</dt>

                    <dd>
                      {selectedReport.branch ||
                        'Not provided'}
                    </dd>
                  </div>

                  <div>
                    <dt>Commit</dt>

                    <dd class="metadata-copy">
                      <code>
                        {selectedReport.commit_sha ||
                          'Not provided'}
                      </code>

                      {#if selectedReport.commit_sha}
                        <button
                          class:copied={
                            copiedKey ===
                            `commit:${selectedReport.id}`
                          }
                          class="copy-icon-button"
                          type="button"
                          aria-label="Copy commit SHA"
                          onclick={() =>
                            copyValue(
                              `commit:${selectedReport.id}`,
                              selectedReport.commit_sha ||
                                ''
                            )}
                        >
                          {#if
                            copiedKey ===
                            `commit:${selectedReport.id}`
                          }
                            <svg
                              viewBox="0 0 24 24"
                            >
                              <path d="m5 12 4 4L19 6"></path>
                            </svg>
                          {:else}
                            <svg
                              viewBox="0 0 24 24"
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
                      {/if}
                    </dd>
                  </div>

                  <div>
                    <dt>Toolchain</dt>

                    <dd>
                      {selectedReport.toolchain ||
                        'Not provided'}
                    </dd>
                  </div>
                </dl>

                <section class="diagnostics-inspector">
                  <header>
                    <div>
                      <p class="panel-kicker">
                        Build output
                      </p>

                      <h3>Diagnostics</h3>

                      <p>
                        Raw diagnostic and summary
                        information attached to this
                        report.
                      </p>
                    </div>

                    {#if hasDiagnostics(selectedReport)}
                      <button
                        class:copied={
                          copiedKey ===
                          `diagnostics:${selectedReport.id}`
                        }
                        class="button-secondary"
                        type="button"
                        onclick={() =>
                          copyValue(
                            `diagnostics:${selectedReport.id}`,
                            diagnosticsFor(
                              selectedReport
                            )
                          )}
                      >
                        {copiedKey ===
                        `diagnostics:${selectedReport.id}`
                          ? 'Diagnostics copied'
                          : 'Copy diagnostics'}
                      </button>
                    {/if}
                  </header>

                  {#if hasDiagnostics(selectedReport)}
                    <pre><code>{diagnosticsFor(
                      selectedReport
                    )}</code></pre>
                  {:else}
                    <div class="no-diagnostics-state">
                      <span aria-hidden="true">
                        <svg
                          viewBox="0 0 24 24"
                          fill="none"
                          stroke="currentColor"
                          stroke-width="1.8"
                          stroke-linecap="round"
                          stroke-linejoin="round"
                        >
                          <path d="m5 12 4 4L19 6"></path>
                        </svg>
                      </span>

                      <div>
                        <strong>
                          No diagnostic payload
                        </strong>

                        <p>
                          This build report contains
                          its status and metrics, but
                          no raw diagnostics or summary
                          JSON.
                        </p>
                      </div>
                    </div>
                  {/if}
                </section>
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
          Hide this build report?
        </h2>

        <p id="confirmation-description">
          The report will disappear from normal
          project history. Its historical data may
          still be retained for safety and auditing.
        </p>
      </div>

      <label class="confirmation-field">
        <span>
          Enter
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
            busyReportId
          )}
          onclick={closeConfirmation}
        >
          Cancel
        </button>

        <button
          class="danger-button"
          type="button"
          disabled={
            Boolean(busyReportId) ||
            !confirmationCanContinue
          }
          onclick={confirmHideReport}
        >
          {busyReportId
            ? 'Hiding report…'
            : 'Hide report'}
        </button>
      </div>
    </dialog>
  </div>
{/if}

<style>
  .reports-page {
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
  .manual-submit-action {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-soft);
  }

  .button-secondary:hover:not(:disabled),
  .manual-submit-action:hover:not(:disabled) {
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

  /* Manual form */

  .submit-panel {
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

  .submit-panel__header {
    display: flex;
    align-items: flex-start;
    gap: 12px;
    padding: 17px 19px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .submit-panel__icon {
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

  .submit-panel__icon svg {
    width: 20px;
    height: 20px;
  }

  .submit-panel__header > div {
    display: grid;
    gap: 4px;
  }

  .submit-panel h2 {
    color: var(--text);
    font-size: 16px;
    font-weight: 620;
  }

  .submit-panel__header p:not(.panel-kicker) {
    max-width: 650px;
    color: var(--text-muted);
    font-size: 10px;
    line-height: 1.5;
  }

  .submit-form {
    display: grid;
    grid-template-columns:
      minmax(0, 1.4fr)
      minmax(280px, 0.6fr);
  }

  .submit-form__fields {
    display: grid;
    gap: 18px;
    padding: 20px;
    border-right: 1px solid var(--line-soft);
  }

  .submit-form fieldset {
    display: grid;
    gap: 9px;
    margin: 0;
    border: 0;
    padding: 0;
  }

  .submit-form legend,
  .submit-form label > span {
    color: var(--text-soft);
    font-size: 10.5px;
    font-weight: 550;
  }

  .status-options {
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    gap: 8px;
  }

  .status-option {
    display: grid;
    min-height: 85px;
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

  .status-option:hover:not(:disabled) {
    border-color: var(--line-strong);
    background: var(--bg-elevated);
    transform: none;
  }

  .status-option.selected {
    border-color: var(--brand-line);
    background: var(--brand-faint);
  }

  .status-option--success.selected {
    border-color: var(--green-line);
    background: var(--green-faint);
  }

  .status-option--warning.selected {
    border-color: var(--warning-line);
    background: var(--warning-faint);
  }

  .status-option--failed.selected {
    border-color: var(--danger-line);
    background: var(--danger-faint);
  }

  .status-option__indicator {
    width: 11px;
    height: 11px;
    margin-top: 2px;
    border: 1px solid var(--line-strong);
    border-radius: 50%;
  }

  .status-option.selected
    .status-option__indicator {
    border: 3px solid var(--bg-panel);
    background: var(--brand);
    box-shadow: 0 0 0 1px var(--brand);
  }

  .status-option--success.selected
    .status-option__indicator {
    background: var(--green-soft);
    box-shadow: 0 0 0 1px var(--green-soft);
  }

  .status-option--warning.selected
    .status-option__indicator {
    background: var(--warning);
    box-shadow: 0 0 0 1px var(--warning);
  }

  .status-option--failed.selected
    .status-option__indicator {
    background: var(--danger);
    box-shadow: 0 0 0 1px var(--danger);
  }

  .status-option > span:last-child {
    display: grid;
    gap: 4px;
  }

  .status-option strong {
    color: var(--text-soft);
    font-size: 10px;
    font-weight: 600;
  }

  .status-option small {
    color: var(--text-muted);
    font-size: 8px;
    line-height: 1.45;
  }

  .field-grid {
    display: grid;
    grid-template-columns:
      repeat(2, minmax(0, 1fr));
    gap: 13px;
  }

  .submit-form label {
    display: grid;
    gap: 7px;
  }

  .submit-form input,
  .confirmation-field input {
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

  .submit-form input:focus,
  .confirmation-field input:focus {
    border-color: var(--brand);
    box-shadow: 0 0 0 3px var(--brand-faint);
  }

  .submit-preview {
    display: grid;
    align-content: start;
    justify-items: start;
    gap: 12px;
    padding: 20px;
    background: var(--bg-ink-soft);
  }

  .submit-preview__mark {
    display: grid;
    width: 48px;
    height: 48px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 12px;
    background: var(--bg-elevated);
    color: var(--text-soft);
  }

  .submit-preview__mark--success {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .submit-preview__mark--warning {
    border-color: var(--warning-line);
    background: var(--warning-faint);
    color: var(--warning);
  }

  .submit-preview__mark--failed {
    border-color: var(--danger-line);
    background: var(--danger-faint);
    color: var(--danger);
  }

  .submit-preview__mark svg {
    width: 21px;
    height: 21px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.9;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .submit-preview__identity {
    display: grid;
    gap: 3px;
  }

  .submit-preview__identity strong {
    color: var(--text);
    font-size: 11px;
  }

  .submit-preview__identity span {
    color: var(--text-muted);
    font-size: 8.5px;
  }

  .submit-preview dl {
    display: grid;
    width: 100%;
    margin: 3px 0 0;
    border: 1px solid var(--line-soft);
    border-radius: 8px;
  }

  .submit-preview dl > div {
    display: grid;
    gap: 4px;
    padding: 10px;
    border-bottom: 1px solid var(--line-soft);
  }

  .submit-preview dl > div:last-child {
    border-bottom: 0;
  }

  .submit-preview dt {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 7px;
    text-transform: uppercase;
  }

  .submit-preview dd {
    overflow-wrap: anywhere;
    margin: 0;
    color: var(--text-soft);
    font-size: 8.5px;
  }

  .submit-preview__diagnostics {
    display: grid;
    width: 100%;
    grid-template-columns:
      repeat(2, minmax(0, 1fr));
    gap: 8px;
  }

  .submit-preview__diagnostics span {
    display: grid;
    gap: 3px;
    padding: 9px;
    border: 1px solid var(--line-soft);
    border-radius: 7px;
    background: var(--bg-panel);
    color: var(--text-muted);
    font-size: 7.5px;
  }

  .submit-preview__diagnostics strong {
    color: var(--text-soft);
    font-size: 12px;
  }

  .submit-preview__workflow {
    display: grid;
    width: 100%;
    gap: 7px;
  }

  .submit-preview__workflow > span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 7px;
    text-transform: uppercase;
  }

  .submit-preview__workflow code {
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

  .submit-preview__workflow i {
    color: var(--brand);
    font-style: normal;
    font-weight: 800;
  }

  .submit-form__actions {
    grid-column: 1 / -1;
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    padding: 13px 20px;
    border-top: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  /* Main layout */

  .reports-layout {
    display: grid;
    grid-template-columns:
      260px minmax(0, 1fr);
    gap: 16px;
    align-items: start;
  }

  /* Project directory */

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
  .report-search svg {
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
  .report-search input {
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
  .report-search input:focus {
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

  .report-detail {
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

  .build-metrics {
    display: grid;
    grid-template-columns:
      repeat(4, minmax(0, 1fr));
    border-bottom: 1px solid var(--line-soft);
  }

  .build-metrics article {
    display: grid;
    min-width: 0;
    gap: 4px;
    padding: 15px 18px;
    border-right: 1px solid var(--line-soft);
  }

  .build-metrics article:last-child {
    border-right: 0;
  }

  .build-metrics span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 7px;
    letter-spacing: 0.06em;
    text-transform: uppercase;
  }

  .build-metrics strong {
    overflow-wrap: anywhere;
    color: var(--text);
    font-size: 16px;
    font-weight: 620;
  }

  .build-metrics small {
    overflow: hidden;
    color: var(--text-faint);
    font-size: 8px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .metric-success {
    color: var(--green-soft) !important;
  }

  .metric-warning {
    color: var(--warning) !important;
  }

  .metric-failed {
    color: var(--danger) !important;
  }

  .latest-build {
    display: grid;
    grid-template-columns:
      auto minmax(0, 1fr) auto auto;
    align-items: center;
    gap: 12px;
    padding: 12px 15px;
    background: var(--bg-panel-strong);
  }

  .latest-build--success {
    box-shadow: inset 3px 0 0 var(--green-soft);
  }

  .latest-build--warning {
    box-shadow: inset 3px 0 0 var(--warning);
  }

  .latest-build--failed {
    box-shadow: inset 3px 0 0 var(--danger);
  }

  .latest-build__status {
    display: grid;
    width: 34px;
    height: 34px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 8px;
    background: var(--bg-elevated);
    color: var(--text-soft);
  }

  .latest-build--success
    .latest-build__status {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .latest-build--warning
    .latest-build__status {
    border-color: var(--warning-line);
    background: var(--warning-faint);
    color: var(--warning);
  }

  .latest-build--failed
    .latest-build__status {
    border-color: var(--danger-line);
    background: var(--danger-faint);
    color: var(--danger);
  }

  .latest-build__status svg {
    width: 16px;
    height: 16px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.9;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .latest-build__identity {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .latest-build__identity > span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 7px;
    text-transform: uppercase;
  }

  .latest-build__identity strong {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 10px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .latest-build__identity small {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 8px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .latest-build__result {
    display: grid;
    min-width: 120px;
    gap: 3px;
    text-align: right;
  }

  .latest-build__result strong {
    color: var(--text-soft);
    font-family: var(--font-mono);
    font-size: 10px;
  }

  .latest-build__result span {
    color: var(--text-muted);
    font-size: 7.5px;
  }

  .latest-build > button {
    min-height: 30px;
    border-color: var(--line-strong);
    background: var(--bg-elevated);
    color: var(--text-soft);
    padding: 0 9px;
    font-size: 8.5px;
  }

  .latest-build > button:hover:not(:disabled) {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-soft);
    transform: none;
  }

  /* Onboarding */

  .build-onboarding {
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

  .build-onboarding__copy {
    display: grid;
    align-content: center;
  }

  .build-onboarding__copy h2 {
    max-width: 550px;
    margin-top: 10px;
    color: var(--text);
    font-size: clamp(31px, 4.6vw, 50px);
    font-weight: 640;
    line-height: 1;
    letter-spacing: -0.052em;
  }

  .build-onboarding__copy h2 span {
    display: block;
    color: var(--brand-soft);
  }

  .build-onboarding__copy > p:not(.panel-kicker) {
    max-width: 540px;
    margin-top: 17px;
    color: var(--text-muted);
    font-size: 12px;
    line-height: 1.7;
  }

  .manual-submit-action {
    width: fit-content;
    margin-top: 24px;
  }

  .build-terminal {
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: 11px;
    background: var(--bg-ink);
    box-shadow: var(--shadow-panel);
  }

  .build-terminal > header {
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

  .build-terminal header > div {
    display: flex;
    gap: 5px;
  }

  .build-terminal header > div span {
    width: 7px;
    height: 7px;
    border-radius: 50%;
    background: var(--text-faint);
  }

  .build-terminal header > div span:first-child {
    background: var(--brand);
  }

  .build-terminal header strong,
  .build-terminal header small {
    font-family: var(--font-mono);
    font-size: 7px;
  }

  .build-terminal header strong {
    overflow: hidden;
    color: var(--text-muted);
    text-align: center;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .build-terminal header small {
    color: var(--text-faint);
  }

  .build-terminal__body {
    display: grid;
    align-content: center;
    min-height: 235px;
    padding: 24px 20px;
  }

  .terminal-command,
  .terminal-output {
    display: grid;
    min-height: 34px;
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
    margin-top: 7px;
  }

  .terminal-output--success span,
  .terminal-output--success code {
    color: var(--green-soft);
  }

  .terminal-progress {
    height: 3px;
    overflow: hidden;
    margin: 8px 0 9px 23px;
    border-radius: 999px;
    background: var(--line-ink);
  }

  .terminal-progress span {
    display: block;
    width: 100%;
    height: 100%;
    border-radius: inherit;
    background: var(--brand);
    transform-origin: left;
    animation: terminal-build 2.8s ease-in-out infinite;
  }

  @keyframes terminal-build {
    0% {
      transform: scaleX(0);
    }

    68%,
    100% {
      transform: scaleX(1);
    }
  }

  .build-terminal > footer {
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

  .build-onboarding__steps {
    grid-column: 1 / -1;
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    padding-top: 25px;
    border-top: 1px solid var(--line);
  }

  .build-onboarding__steps article {
    display: grid;
    grid-template-columns:
      auto minmax(0, 1fr);
    gap: 10px;
    padding: 0 18px;
    border-right: 1px solid var(--line);
  }

  .build-onboarding__steps article:first-child {
    padding-left: 0;
  }

  .build-onboarding__steps article:last-child {
    border-right: 0;
  }

  .build-onboarding__steps article > span {
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 8px;
    font-weight: 700;
  }

  .build-onboarding__steps article.active > span {
    color: var(--brand-soft);
  }

  .build-onboarding__steps article > div {
    display: grid;
    gap: 4px;
  }

  .build-onboarding__steps strong {
    color: var(--text-soft);
    font-size: 10px;
  }

  .build-onboarding__steps p {
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
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 15px;
    padding: 9px 13px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .report-search {
    position: relative;
    width: min(290px, 100%);
  }

  .report-search svg {
    left: 11px;
  }

  .status-filters {
    display: flex;
    flex-wrap: wrap;
    gap: 4px;
  }

  .status-filters button {
    min-height: 31px;
    gap: 7px;
    border-color: transparent;
    background: transparent;
    color: var(--text-muted);
    padding: 0 9px;
    font-size: 9px;
  }

  .status-filters button:hover:not(:disabled) {
    border-color: var(--line);
    background: var(--bg-elevated);
    color: var(--text-soft);
    transform: none;
  }

  .status-filters button.active {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-bright);
  }

  .status-filters button span {
    display: grid;
    min-width: 18px;
    height: 18px;
    place-items: center;
    border-radius: 999px;
    background: var(--bg-panel);
    padding-inline: 5px;
    font-family: var(--font-mono);
    font-size: 7px;
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

  /* Report rows */

  .report-list {
    display: grid;
  }

  .report-list__heading,
  .report-row {
    display: grid;
    grid-template-columns:
      minmax(190px, 1fr)
      minmax(155px, 0.7fr)
      minmax(145px, 0.6fr)
      minmax(90px, 0.35fr)
      minmax(90px, 0.4fr)
      minmax(115px, auto);
    align-items: center;
    gap: 13px;
  }

  .report-list__heading {
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

  .report-row {
    min-height: 78px;
    padding: 10px 14px;
    border-bottom: 1px solid var(--line-soft);
    transition:
      background var(--speed) var(--ease),
      box-shadow var(--speed) var(--ease);
  }

  .report-row:last-child {
    border-bottom: 0;
  }

  .report-row:hover {
    background: rgba(255, 255, 255, 0.018);
  }

  .report-row.selected {
    background: var(--brand-faint);
    box-shadow: inset 2px 0 0 var(--brand);
  }

  .report-build {
    display: grid;
    min-width: 0;
    grid-template-columns:
      auto minmax(0, 1fr);
    align-items: center;
    gap: 9px;
  }

  .report-icon {
    display: grid;
    width: 33px;
    height: 33px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 8px;
    background: var(--bg-elevated);
    color: var(--text-soft);
  }

  .report-icon--success {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .report-icon--warning {
    border-color: var(--warning-line);
    background: var(--warning-faint);
    color: var(--warning);
  }

  .report-icon--failed {
    border-color: var(--danger-line);
    background: var(--danger-faint);
    color: var(--danger);
  }

  .report-icon svg {
    width: 15px;
    height: 15px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.9;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .report-build > div {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .report-build strong {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 10.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .report-build small {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 7.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .report-source,
  .report-diagnostics,
  .report-duration,
  .report-state {
    display: grid;
    min-width: 0;
    gap: 4px;
  }

  .report-source > span,
  .report-diagnostics > span,
  .report-duration > span,
  .report-state > span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6.5px;
    text-transform: uppercase;
  }

  .report-source strong {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 8.5px;
    font-weight: 550;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .report-source code {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 8px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .report-source small {
    overflow: hidden;
    color: var(--text-faint);
    font-size: 7.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .report-diagnostics > div {
    display: grid;
    gap: 3px;
  }

  .report-diagnostics strong {
    color: var(--text-muted);
    font-size: 8px;
    font-weight: 500;
  }

  .report-diagnostics .has-warnings {
    color: var(--warning);
  }

  .report-diagnostics .has-errors {
    color: var(--danger);
  }

  .report-duration strong {
    color: var(--text-soft);
    font-family: var(--font-mono);
    font-size: 9px;
  }

  .report-controls {
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

  .report-actions {
    position: relative;
  }

  .report-actions summary {
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

  .report-actions summary::-webkit-details-marker {
    display: none;
  }

  .report-actions summary:hover {
    border-color: var(--line-strong);
    background: var(--bg-elevated);
    color: var(--text);
  }

  .report-actions summary svg {
    width: 14px;
    height: 14px;
    fill: currentColor;
  }

  .report-actions > div {
    position: absolute;
    z-index: 20;
    top: calc(100% + 6px);
    right: 0;
    display: grid;
    width: 145px;
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: 8px;
    background: var(--bg-panel-strong);
    box-shadow: var(--shadow-deep);
  }

  .report-actions button {
    min-height: 36px;
    justify-content: flex-start;
    border: 0;
    border-radius: 0;
    background: transparent;
    padding: 0 11px;
    font-size: 9.5px;
  }

  .report-actions button:hover:not(:disabled) {
    background: var(--bg-elevated);
    transform: none;
  }

  .report-actions .action-danger {
    color: var(--danger);
  }

  /* Report inspector */

  .report-inspector {
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    animation: inspector-enter 260ms var(--ease) both;
  }

  .report-inspector--success {
    border-color: var(--green-line);
  }

  .report-inspector--warning {
    border-color: var(--warning-line);
  }

  .report-inspector--failed {
    border-color: var(--danger-line);
  }

  @keyframes inspector-enter {
    from {
      opacity: 0;
      transform: translateY(-5px);
    }
  }

  .report-inspector__header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    padding: 15px 16px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .report-inspector__title {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 11px;
  }

  .report-inspector__status {
    display: grid;
    width: 39px;
    height: 39px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 9px;
    background: var(--bg-elevated);
    color: var(--text-soft);
  }

  .report-inspector--success
    .report-inspector__status {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .report-inspector--warning
    .report-inspector__status {
    border-color: var(--warning-line);
    background: var(--warning-faint);
    color: var(--warning);
  }

  .report-inspector--failed
    .report-inspector__status {
    border-color: var(--danger-line);
    background: var(--danger-faint);
    color: var(--danger);
  }

  .report-inspector__status svg {
    width: 18px;
    height: 18px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.9;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .report-inspector__title > div {
    display: grid;
    min-width: 0;
    gap: 4px;
  }

  .report-inspector h2 {
    overflow-wrap: anywhere;
    color: var(--text);
    font-size: 14px;
    font-weight: 610;
  }

  .report-inspector__title p:not(.panel-kicker) {
    color: var(--text-muted);
    font-size: 8.5px;
  }

  .icon-close-button,
  .copy-icon-button {
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

  .icon-close-button:hover:not(:disabled),
  .copy-icon-button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
    transform: none;
  }

  .copy-icon-button.copied {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .icon-close-button svg,
  .copy-icon-button svg {
    width: 13px;
    height: 13px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .report-inspector__metrics {
    display: grid;
    grid-template-columns:
      repeat(4, minmax(0, 1fr));
    border-bottom: 1px solid var(--line-soft);
  }

  .report-inspector__metrics article {
    display: grid;
    gap: 4px;
    padding: 13px 15px;
    border-right: 1px solid var(--line-soft);
  }

  .report-inspector__metrics article:last-child {
    border-right: 0;
  }

  .report-inspector__metrics span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6.5px;
    text-transform: uppercase;
  }

  .report-inspector__metrics strong {
    color: var(--text-soft);
    font-size: 12px;
    font-weight: 600;
  }

  .report-metadata {
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    margin: 0;
    border-bottom: 1px solid var(--line-soft);
  }

  .report-metadata > div {
    display: grid;
    min-width: 0;
    gap: 4px;
    padding: 12px 14px;
    border-right: 1px solid var(--line-soft);
    border-bottom: 1px solid var(--line-soft);
  }

  .report-metadata > div:nth-child(3n) {
    border-right: 0;
  }

  .report-metadata > div:nth-last-child(-n + 3) {
    border-bottom: 0;
  }

  .report-metadata dt {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6.5px;
    text-transform: uppercase;
  }

  .report-metadata dd {
    min-width: 0;
    overflow-wrap: anywhere;
    margin: 0;
    color: var(--text-soft);
    font-size: 8.5px;
    line-height: 1.45;
  }

  .report-metadata code {
    color: var(--text-soft);
    font-size: 8px;
  }

  .metadata-copy {
    display: flex;
    align-items: center;
    gap: 7px;
  }

  .metadata-copy code {
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .diagnostics-inspector {
    display: grid;
  }

  .diagnostics-inspector > header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 15px;
    padding: 13px 15px;
    border-bottom: 1px solid var(--line-ink);
    background: var(--bg-ink-soft);
  }

  .diagnostics-inspector header > div {
    display: grid;
    gap: 3px;
  }

  .diagnostics-inspector h3 {
    color: var(--text);
    font-size: 12px;
    font-weight: 600;
  }

  .diagnostics-inspector header p:not(.panel-kicker) {
    color: var(--text-muted);
    font-size: 8.5px;
  }

  .diagnostics-inspector
    .button-secondary {
    min-height: 29px;
    padding: 0 9px;
    font-size: 8.5px;
  }

  .diagnostics-inspector pre {
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

  .no-diagnostics-state {
    display: flex;
    min-height: 145px;
    align-items: center;
    gap: 11px;
    padding: 22px;
    background: var(--bg-ink);
  }

  .no-diagnostics-state > span {
    display: grid;
    width: 35px;
    height: 35px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--green-line);
    border-radius: 8px;
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .no-diagnostics-state svg {
    width: 16px;
    height: 16px;
  }

  .no-diagnostics-state > div {
    display: grid;
    gap: 4px;
  }

  .no-diagnostics-state strong {
    color: var(--text-soft);
    font-size: 10px;
  }

  .no-diagnostics-state p {
    max-width: 520px;
    color: var(--text-muted);
    font-size: 8.5px;
    line-height: 1.5;
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
    .reports-page,
    .submit-panel,
    .page-loading__bar,
    .detail-progress,
    .terminal-progress span,
    .report-inspector {
      animation: none;
    }
  }

  @media (max-width: 1120px) {
    .report-list__heading,
    .report-row {
      grid-template-columns:
        minmax(185px, 1fr)
        minmax(145px, 0.65fr)
        minmax(120px, 0.5fr)
        minmax(85px, 0.35fr)
        minmax(110px, auto);
    }

    .report-duration {
      display: none;
    }
  }

  @media (max-width: 1060px) {
    .submit-form {
      grid-template-columns: 1fr;
    }

    .submit-form__fields {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .submit-preview {
      grid-template-columns:
        auto minmax(0, 1fr);
      align-items: center;
    }

    .submit-preview > .panel-kicker,
    .submit-preview dl,
    .submit-preview__diagnostics,
    .submit-preview__workflow {
      grid-column: 1 / -1;
    }

    .build-onboarding {
      grid-template-columns: 1fr;
    }

    .build-metrics {
      grid-template-columns:
        repeat(2, minmax(0, 1fr));
    }

    .build-metrics article:nth-child(2) {
      border-right: 0;
    }

    .build-metrics article:nth-child(-n + 2) {
      border-bottom: 1px solid var(--line-soft);
    }
  }

  @media (max-width: 900px) {
    .reports-layout {
      grid-template-columns: 1fr;
    }

    .project-directory {
      position: static;
    }

    .project-options {
      max-height: 260px;
    }
  }

  @media (max-width: 800px) {
    .history-toolbar {
      align-items: stretch;
      flex-direction: column;
    }

    .report-search {
      width: 100%;
    }

    .status-filters {
      overflow-x: auto;
      flex-wrap: nowrap;
    }

    .status-filters button {
      flex: 0 0 auto;
    }

    .report-list__heading {
      display: none;
    }

    .report-row {
      grid-template-columns:
        minmax(0, 1fr) auto;
      gap: 13px;
      padding: 14px;
    }

    .report-build {
      grid-column: 1;
    }

    .report-controls {
      grid-column: 2;
      grid-row: 1;
    }

    .report-source,
    .report-diagnostics,
    .report-state {
      grid-column: 1 / -1;
      padding-top: 9px;
      border-top: 1px solid var(--line-soft);
    }

    .report-diagnostics > div {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
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

    .field-grid,
    .status-options {
      grid-template-columns: 1fr;
    }

    .project-overview__header {
      align-items: flex-start;
      flex-direction: column;
    }

    .project-link {
      width: 100%;
      justify-content: space-between;
    }

    .latest-build {
      grid-template-columns:
        auto minmax(0, 1fr);
    }

    .latest-build__result {
      grid-column: 2;
      text-align: left;
    }

    .latest-build > button {
      grid-column: 2;
      width: fit-content;
    }

    .build-onboarding__steps {
      grid-template-columns: 1fr;
      gap: 15px;
    }

    .build-onboarding__steps article,
    .build-onboarding__steps article:first-child {
      padding: 0;
      border-right: 0;
    }

    .report-inspector__metrics {
      grid-template-columns:
        repeat(2, minmax(0, 1fr));
    }

    .report-inspector__metrics article:nth-child(2) {
      border-right: 0;
    }

    .report-inspector__metrics article:nth-child(-n + 2) {
      border-bottom: 1px solid var(--line-soft);
    }

    .report-metadata {
      grid-template-columns:
        repeat(2, minmax(0, 1fr));
    }

    .report-metadata > div,
    .report-metadata > div:nth-child(3n) {
      border-right: 1px solid var(--line-soft);
      border-bottom: 1px solid var(--line-soft);
    }

    .report-metadata > div:nth-child(even) {
      border-right: 0;
    }

    .report-metadata > div:nth-last-child(-n + 2) {
      border-bottom: 0;
    }
  }

  @media (max-width: 620px) {
    .submit-form__actions,
    .confirm-modal__actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .submit-form__actions button,
    .confirm-modal__actions button {
      width: 100%;
    }

    .build-onboarding {
      padding: 26px 19px;
    }

    .build-onboarding__copy h2 {
      font-size: clamp(31px, 10vw, 43px);
    }

    .build-terminal > footer {
      align-items: flex-start;
      flex-direction: column;
      justify-content: center;
      padding-block: 10px;
    }

    .diagnostics-inspector > header {
      align-items: stretch;
      flex-direction: column;
    }

    .diagnostics-inspector
      .button-secondary {
      width: 100%;
    }
  }

  @media (max-width: 500px) {
    .submit-form__fields,
    .submit-preview {
      padding: 16px;
    }

    .build-metrics {
      grid-template-columns: 1fr;
    }

    .build-metrics article,
    .build-metrics article:nth-child(2) {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .build-metrics article:last-child {
      border-bottom: 0;
    }

    .latest-build {
      grid-template-columns: 1fr;
    }

    .latest-build__result,
    .latest-build > button {
      grid-column: 1;
    }

    .report-row {
      grid-template-columns: 1fr;
    }

    .report-controls {
      grid-column: 1;
      grid-row: auto;
      justify-content: flex-start;
    }

    .report-source,
    .report-diagnostics,
    .report-state {
      grid-column: 1;
    }

    .report-actions > div {
      position: fixed;
      right: 16px;
      bottom: 16px;
      top: auto;
      width: calc(100% - 32px);
    }

    .report-inspector__header {
      align-items: flex-start;
    }

    .report-inspector__title {
      align-items: flex-start;
    }

    .report-inspector__metrics,
    .report-metadata {
      grid-template-columns: 1fr;
    }

    .report-inspector__metrics article,
    .report-inspector__metrics article:nth-child(2),
    .report-metadata > div,
    .report-metadata > div:nth-child(3n),
    .report-metadata > div:nth-child(even) {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .report-inspector__metrics article:last-child,
    .report-metadata > div:last-child {
      border-bottom: 0;
    }
  }
</style>
