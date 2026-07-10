<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
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
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import { canSubmitBuildReport } from '$lib/permissions';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  let workspaces: Workspace[] = [];
  let projects: Project[] = [];
  let reports: BuildReport[] = [];

  let selectedWorkspaceId = '';
  let selectedProjectId = '';

  let loading = true;
  let loadingReports = false;
  let initialized = false;
  let projectListRequestId = 0;
  let saving = false;
  let busyReportId = '';

  let error = '';
  let success = '';

  let projectQuery = '';
  let reportQuery = '';
  let statusFilter = 'all';

  let showSubmitForm = false;
  let openDiagnosticsId = '';
  let copiedKey = '';

  let status = 'success';
  let target = '';
  let profile = 'debug';
  let branch = '';
  let commitSha = '';
  let toolchain = '';
  let durationMs = 0;
  let warningsCount = 0;
  let errorsCount = 0;

  let reportRequestId = 0;
  let copyResetTimer: ReturnType<typeof setTimeout> | undefined;

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

  $: canSubmit =
    canSubmitBuildReport(currentRole);

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

  $: visibleReports = reports.filter((report) => {
    const query = reportQuery.trim().toLowerCase();

    const matchesQuery =
      !query ||
      report.target.toLowerCase().includes(query) ||
      report.profile?.toLowerCase().includes(query) ||
      report.branch?.toLowerCase().includes(query) ||
      report.commit_sha?.toLowerCase().includes(query) ||
      report.toolchain?.toLowerCase().includes(query);

    const matchesStatus =
      statusFilter === 'all' ||
      report.status === statusFilter;

    return matchesQuery && matchesStatus;
  });

  $: successfulReports = reports.filter(
    (report) => report.status === 'success'
  ).length;

  $: failedReports = reports.filter(
    (report) => report.status === 'failed'
  ).length;

  $: warningReports = reports.filter(
    (report) => report.status === 'warning'
  ).length;

  $: totalWarnings = reports.reduce(
    (total, report) =>
      total + Number(report.warnings_count || 0),
    0
  );

  $: totalErrors = reports.reduce(
    (total, report) =>
      total + Number(report.errors_count || 0),
    0
  );

  $: averageDuration = reports.length > 0
    ? reports.reduce(
        (total, report) =>
          total + Number(report.duration_ms || 0),
        0
      ) / reports.length
    : 0;

  function projectInitial(project: Project) {
    return project.name
      .slice(0, 1)
      .toUpperCase();
  }

  function formatDuration(value?: number) {
    const duration = Number(value || 0);

    if (duration < 1000) {
      return `${duration} ms`;
    }

    if (duration < 60_000) {
      return `${(duration / 1000).toFixed(
        duration >= 10_000 ? 0 : 1
      )} s`;
    }

    const minutes = Math.floor(duration / 60_000);
    const seconds = Math.round(
      (duration % 60_000) / 1000
    );

    return `${minutes}m ${seconds}s`;
  }

  function formatDate(value?: string | number) {
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
      const numericValue = Number(value);

      date =
        Number.isFinite(numericValue) &&
        value.trim() !== ''
          ? new Date(
              numericValue < 1_000_000_000_000
                ? numericValue * 1000
                : numericValue
            )
          : new Date(value);
    }

    if (Number.isNaN(date.getTime())) {
      return 'Unknown date';
    }

    return new Intl.DateTimeFormat(undefined, {
      dateStyle: 'medium',
      timeStyle: 'short'
    }).format(date);
  }

  function shortCommit(value?: string) {
    if (!value) {
      return '—';
    }

    return value.length > 12
      ? value.slice(0, 12)
      : value;
  }

  function diagnosticsFor(report: BuildReport) {
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

  function hasDiagnostics(report: BuildReport) {
    return Boolean(
      report.diagnostics_json ||
      report.summary_json
    );
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

  function toggleSubmitForm() {
    showSubmitForm = !showSubmitForm;
    error = '';
    success = '';

    if (!showSubmitForm) {
      resetSubmitForm();
    }
  }

  function closeSubmitForm() {
    if (saving) {
      return;
    }

    showSubmitForm = false;
    resetSubmitForm();
    error = '';
  }

  function handleSubmit(event: SubmitEvent) {
    event.preventDefault();
    void submitReport();
  }

  async function deleteReportRecord(report: BuildReport) {
    if (!selectedWorkspaceId || !selectedProjectId || !canSubmit) {
      return;
    }

    if (!window.confirm('Hide this build report? The historical record is kept for safety.')) {
      return;
    }

    busyReportId = report.id;
    error = '';
    success = '';

    try {
      await deleteBuildReport(selectedWorkspaceId, selectedProjectId, report.id);
      reports = reports.filter((item) => item.id !== report.id);
      success = 'Build report hidden.';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to hide build report.';
    } finally {
      busyReportId = '';
    }
  }

  function toggleDiagnostics(reportId: string) {
    openDiagnosticsId =
      openDiagnosticsId === reportId
        ? ''
        : reportId;
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

      const workspaceData =
        await listWorkspaces(user.id);

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
          : 'Unable to load build reports.';
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
      reports = [];
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

    await loadReports(selectedProjectId);
  }

  async function switchWorkspace(workspaceId: string) {
    selectedWorkspaceId = workspaceId;
    selectedProjectId = '';
    projects = [];
    reports = [];
    projectQuery = '';
    reportQuery = '';
    statusFilter = 'all';
    openDiagnosticsId = '';
    showSubmitForm = false;
    resetSubmitForm();
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
          : 'Unable to load build reports.';
    } finally {
      loading = false;
    }
  }

  async function selectProject(project: Project) {
    if (
      project.id === selectedProjectId ||
      loadingReports
    ) {
      return;
    }

    selectedProjectId = project.id;

    reportQuery = '';
    statusFilter = 'all';
    openDiagnosticsId = '';
    copiedKey = '';
    error = '';
    success = '';

    showSubmitForm = false;
    resetSubmitForm();

    updatePageUrl(project.id);

    await loadReports(project.id);
  }

  async function loadReports(projectId: string) {
    if (!selectedWorkspaceId || !projectId) {
      reports = [];
      return;
    }

    const requestId = ++reportRequestId;
    const workspaceId = selectedWorkspaceId;

    loadingReports = true;
    error = '';
    reports = [];

    try {
      const data = await listBuildReports(
        workspaceId,
        projectId
      );

      if (
        requestId !== reportRequestId ||
        selectedWorkspaceId !== workspaceId ||
        selectedProjectId !== projectId
      ) {
        return;
      }

      reports = data.build_reports;
    } catch (err) {
      if (requestId !== reportRequestId) {
        return;
      }

      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load build reports.';
    } finally {
      if (requestId === reportRequestId) {
        loadingReports = false;
      }
    }
  }

  async function submitReport() {
    const user = $auth.user;
    const cleanTarget = target.trim();

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
      const submitted = await submitBuildReport({
        workspaceId: selectedWorkspaceId,
        projectId: selectedProjectId,
        submittedByUserId: user.id,
        status,
        target: cleanTarget,
        profile: profile.trim() || 'debug',
        branch: branch.trim(),
        commitSha: commitSha.trim(),
        toolchain: toolchain.trim(),
        durationMs: Math.max(
          0,
          Number(durationMs) || 0
        ),
        warningsCount: Math.max(
          0,
          Number(warningsCount) || 0
        ),
        errorsCount: Math.max(
          0,
          Number(errorsCount) || 0
        )
      });

      reports = [
        submitted.build_report,
        ...reports
      ];

      success = 'Build report submitted.';

      resetSubmitForm();
      showSubmitForm = false;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to submit build report.';
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
  <title>Build reports | Softadastra Cloud</title>
</svelte:head>

<PageHeader
  eyebrow="Builds"
  title="Build reports"
  workspaceName={selectedWorkspace?.name ?? ''}
  role={currentRole}
>
  {#if canSubmit && selectedProject && !loading}
    <button
      class:secondary-button={showSubmitForm}
      class="submit-report-button"
      type="button"
      onclick={toggleSubmitForm}
    >
      {showSubmitForm ? 'Close' : 'Submit report'}
    </button>
  {/if}
</PageHeader>

<p class="page-description">
  Review build results, warnings, errors, toolchains and diagnostics
  recorded for each project.
</p>

<InlineError message={error} />

{#if success}
  <p class="success-message" role="status">
    {success}
  </p>
{/if}

{#if showSubmitForm && canSubmit && selectedProject}
  <section
    class="submit-section"
    aria-labelledby="submit-report-title"
  >
    <div class="submit-heading">
      <div>
        <h2 id="submit-report-title">
          Submit a build report
        </h2>

        <p>
          Recording a build for
          <strong>{selectedProject.name}</strong>.
        </p>
      </div>
    </div>

    <form
      class="submit-form"
      onsubmit={handleSubmit}
    >
      <label>
        Status

        <select bind:value={status}>
          <option value="success">
            Success
          </option>

          <option value="warning">
            Warning
          </option>

          <option value="failed">
            Failed
          </option>
        </select>
      </label>

      <label>
        Target

        <input
          bind:value={target}
          placeholder="app"
          required
          autocomplete="off"
        />
      </label>

      <label>
        Profile

        <input
          bind:value={profile}
          placeholder="debug"
          autocomplete="off"
        />
      </label>

      <label>
        Branch

        <input
          bind:value={branch}
          placeholder="main"
          autocomplete="off"
        />
      </label>

      <label>
        Commit SHA

        <input
          bind:value={commitSha}
          placeholder="a1b2c3d4"
          autocomplete="off"
          spellcheck="false"
        />
      </label>

      <label>
        Toolchain

        <input
          bind:value={toolchain}
          placeholder="gcc-14"
          autocomplete="off"
        />
      </label>

      <label>
        Duration in milliseconds

        <input
          bind:value={durationMs}
          type="number"
          min="0"
          step="1"
        />
      </label>

      <label>
        Warnings

        <input
          bind:value={warningsCount}
          type="number"
          min="0"
          step="1"
        />
      </label>

      <label>
        Errors

        <input
          bind:value={errorsCount}
          type="number"
          min="0"
          step="1"
        />
      </label>

      <div class="submit-actions">
        <button
          class="secondary-button"
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
            !selectedProjectId ||
            !target.trim()
          }
        >
          {saving
            ? 'Submitting…'
            : 'Submit report'}
        </button>
      </div>
    </form>
  </section>
{/if}

<div class="reports-layout">
  <aside
    class="project-directory"
    aria-label="Project directory"
  >
    <div class="directory-header">
      <div>
        <h2>Projects</h2>
        <p>Choose the build history to inspect.</p>
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
          body="Create a project before submitting build reports."
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

  <main class="report-detail">
    {#if loading}
      <section class="detail-section">
        <p class="loading-state">
          Loading build reports…
        </p>
      </section>
    {:else if !selectedProject}
      <section class="detail-section empty-detail">
        <EmptyState
          title="Select a project"
          body="Its build history and diagnostics will appear here."
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

        <dl class="build-summary">
          <div>
            <dt>Builds</dt>
            <dd>{reports.length}</dd>
          </div>

          <div>
            <dt>Successful</dt>
            <dd>{successfulReports}</dd>
          </div>

          <div>
            <dt>Warnings</dt>
            <dd>{warningReports}</dd>
          </div>

          <div>
            <dt>Failed</dt>
            <dd>{failedReports}</dd>
          </div>

          <div>
            <dt>Average duration</dt>
            <dd>{formatDuration(averageDuration)}</dd>
          </div>
        </dl>

        <div class="diagnostic-summary">
          <span>
            <strong>{totalWarnings}</strong>
            total warnings
          </span>

          <span>
            <strong>{totalErrors}</strong>
            total errors
          </span>
        </div>

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
            href={`/lockfiles?workspace_id=${selectedWorkspaceId}&project_id=${selectedProject.id}`}
          >
            Lockfiles
          </a>

          <a
            href={`/packages?workspace_id=${selectedWorkspaceId}`}
          >
            Workspace packages
          </a>
        </nav>
      </section>

      {#if !canSubmit}
        <ReadOnlyNotice
          message="You can review build reports, but your role cannot submit new reports."
        />
      {/if}

      <section
        class="detail-section reports-section"
        aria-labelledby="reports-title"
      >
        <div class="section-header">
          <div>
            <h2 id="reports-title">
              Build history
            </h2>

            <p>
              Recorded builds for the selected project.
            </p>
          </div>

          <span class="section-count">
            {visibleReports.length}
          </span>
        </div>

        {#if reports.length > 0}
          <div class="report-toolbar">
            <div class="report-search">
              <svg viewBox="0 0 24 24" aria-hidden="true">
                <circle cx="11" cy="11" r="7"></circle>
                <path d="m20 20-4-4"></path>
              </svg>

              <input
                bind:value={reportQuery}
                type="search"
                placeholder="Find a target, branch or commit"
                aria-label="Find a build report"
              />
            </div>

            <select
              bind:value={statusFilter}
              aria-label="Filter reports by status"
            >
              <option value="all">
                All statuses
              </option>

              <option value="success">
                Success
              </option>

              <option value="warning">
                Warning
              </option>

              <option value="failed">
                Failed
              </option>
            </select>
          </div>
        {/if}

        {#if loadingReports}
          <p class="loading-state">
            Loading build history…
          </p>
        {:else if reports.length === 0}
          <div class="empty-wrapper">
            <EmptyState
              title="No build reports"
              body={canSubmit
                ? 'Submit the first build report for this project.'
                : 'No build report has been submitted for this project.'}
            />
          </div>
        {:else if visibleReports.length === 0}
          <div class="no-results">
            <strong>No matching reports</strong>

            <p>
              Change the search text or selected status.
            </p>
          </div>
        {:else}
          <div class="report-table">
            <div class="report-table__scroll">
              <div class="table-heading">
                <span>Build</span>
                <span>Source</span>
                <span>Duration</span>
                <span>Diagnostics</span>
                <span>Status</span>
                <span class="align-right">Action</span>
              </div>

              {#each visibleReports as report (report.id)}
                <div class="report-entry">
                  <div class="report-row">
                    <div
                      class="build-cell"
                      data-label="Build"
                    >
                      <strong>{report.target}</strong>

                      <span>
                        {report.profile || 'default'}
                      </span>
                    </div>

                    <div
                      class="source-cell"
                      data-label="Source"
                    >
                      <span>
                        {report.branch || 'No branch'}
                      </span>

                      <code title={report.commit_sha}>
                        {shortCommit(report.commit_sha)}
                      </code>

                      {#if report.toolchain}
                        <small>{report.toolchain}</small>
                      {/if}

                      {#if canSubmit}
                        <button
                          class="danger-link"
                          type="button"
                          disabled={busyReportId === report.id}
                          onclick={() => deleteReportRecord(report)}
                        >
                          Delete
                        </button>
                      {/if}
                    </div>

                    <div
                      class="duration-cell"
                      data-label="Duration"
                    >
                      <strong>
                        {formatDuration(report.duration_ms)}
                      </strong>

                      <small>
                        {formatDate(report.created_at)}
                      </small>
                    </div>

                    <div
                      class="diagnostic-cell"
                      data-label="Diagnostics"
                    >
                      <span
                        class:has-warnings={
                          Number(report.warnings_count || 0) > 0
                        }
                      >
                        {report.warnings_count || 0}
                        warnings
                      </span>

                      <span
                        class:has-errors={
                          Number(report.errors_count || 0) > 0
                        }
                      >
                        {report.errors_count || 0}
                        errors
                      </span>
                    </div>

                    <div
                      class="status-cell"
                      data-label="Status"
                    >
                      <StatusBadge status={report.status} />
                    </div>

                    <div
                      class="row-actions"
                      data-label="Action"
                    >
                      {#if hasDiagnostics(report)}
                        <button
                          class="diagnostics-button"
                          type="button"
                          aria-expanded={
                            openDiagnosticsId === report.id
                          }
                          onclick={() =>
                            toggleDiagnostics(report.id)}
                        >
                          {openDiagnosticsId === report.id
                            ? 'Hide diagnostics'
                            : 'View diagnostics'}
                        </button>
                      {:else}
                        <span class="no-diagnostics">
                          No diagnostics
                        </span>
                      {/if}
                    </div>
                  </div>

                  {#if
                    openDiagnosticsId === report.id &&
                    hasDiagnostics(report)
                  }
                    <div class="diagnostics-panel">
                      <div class="diagnostics-panel__header">
                        <div>
                          <strong>
                            Build diagnostics
                          </strong>

                          <span>
                            {report.target}
                            ·
                            {report.profile || 'default'}
                          </span>
                        </div>

                        <button
                          class:copied={
                            copiedKey === `diagnostics:${report.id}`
                          }
                          class="copy-diagnostics-button"
                          type="button"
                          onclick={() =>
                            copyValue(
                              `diagnostics:${report.id}`,
                              diagnosticsFor(report)
                            )}
                        >
                          {copiedKey === `diagnostics:${report.id}`
                            ? 'Copied'
                            : 'Copy diagnostics'}
                        </button>
                      </div>

                      <pre>{diagnosticsFor(report)}</pre>
                    </div>
                  {/if}
                </div>
              {/each}
            </div>
          </div>
        {/if}
      </section>
    {/if}
  </main>
</div>

<style>
  .page-description {
    max-width: 760px;
    margin: -8px 0 18px;
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.6;
  }

  .submit-report-button {
    border-color: var(--brand);
    background: var(--brand);
    color: var(--brand-ink);
  }

  .submit-report-button:hover:not(:disabled) {
    border-color: var(--brand-soft);
    background: var(--brand-soft);
  }

  .submit-report-button.secondary-button {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-soft);
  }

  .submit-report-button.secondary-button:hover:not(:disabled) {
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

  /* Submit form */

  .submit-section {
    display: grid;
    gap: 17px;
    margin-bottom: 18px;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    padding: 18px;
  }

  .submit-heading {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 16px;
  }

  .submit-heading > div {
    display: grid;
    gap: 4px;
  }

  .submit-heading h2 {
    font-size: 14px;
  }

  .submit-heading p {
    color: var(--text-muted);
    font-size: 12px;
    line-height: 1.5;
  }

  .submit-heading strong {
    color: var(--text-soft);
    font-weight: 600;
  }

  .submit-form {
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 1fr));
    gap: 14px;
  }

  .submit-actions {
    grid-column: 1 / -1;
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    border-top: 1px solid var(--line-soft);
    padding-top: 14px;
  }

  /* Main layout */

  .reports-layout {
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

  .report-detail {
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

  .build-summary {
    display: grid;
    grid-template-columns: repeat(5, minmax(0, 1fr));
    margin: 0;
    border-bottom: 1px solid var(--line-soft);
  }

  .build-summary > div {
    display: grid;
    min-width: 0;
    gap: 4px;
    padding: 12px 14px;
    border-right: 1px solid var(--line-soft);
  }

  .build-summary > div:last-child {
    border-right: 0;
  }

  .build-summary dt {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .build-summary dd {
    margin: 0;
    color: var(--text-soft);
    font-size: 12px;
    font-weight: 550;
  }

  .diagnostic-summary {
    display: flex;
    align-items: center;
    gap: 20px;
    padding: 10px 14px;
    border-bottom: 1px solid var(--line-soft);
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .diagnostic-summary span {
    display: inline-flex;
    align-items: baseline;
    gap: 4px;
  }

  .diagnostic-summary strong {
    color: var(--text-soft);
    font-family: var(--font-mono);
    font-weight: 600;
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

  /* Report toolbar */

  .report-toolbar {
    display: grid;
    grid-template-columns:
      minmax(240px, 1fr)
      minmax(150px, 210px);
    gap: 10px;
    padding: 10px 12px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .report-search {
    position: relative;
  }

  .report-search svg {
    left: 11px;
  }

  .report-search input {
    min-height: 34px;
    padding-left: 33px;
    font-size: 12px;
  }

  .report-toolbar select {
    min-height: 34px;
    font-size: 12px;
  }

  /* Report table */

  .report-table {
    width: 100%;
    overflow-x: auto;
    overflow-y: hidden;
    scrollbar-width: thin;
    scrollbar-color: var(--line-strong) transparent;
  }

  .report-table::-webkit-scrollbar {
    height: 8px;
  }

  .report-table::-webkit-scrollbar-track {
    background: transparent;
  }

  .report-table::-webkit-scrollbar-thumb {
    border: 2px solid transparent;
    border-radius: 999px;
    background: var(--line-strong);
    background-clip: padding-box;
  }

  .report-table__scroll {
    min-width: 980px;
  }

  .table-heading,
  .report-row {
    display: grid;
    grid-template-columns:
      minmax(150px, 0.75fr)
      minmax(210px, 1fr)
      minmax(140px, 0.65fr)
      minmax(180px, 0.8fr)
      minmax(100px, 0.45fr)
      minmax(150px, auto);
    gap: 16px;
    align-items: center;
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

  .report-entry {
    border-bottom: 1px solid var(--line-soft);
  }

  .report-entry:last-child {
    border-bottom: 0;
  }

  .report-row {
    min-height: 70px;
    padding: 10px 16px;
  }

  .report-row:hover {
    background: rgba(255, 255, 255, 0.018);
  }

  .build-cell,
  .source-cell,
  .duration-cell,
  .diagnostic-cell {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .build-cell strong {
    color: var(--text);
    font-size: 12px;
    font-weight: 600;
    overflow-wrap: anywhere;
  }

  .build-cell span,
  .source-cell span,
  .source-cell small,
  .duration-cell small {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .source-cell code {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 9.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .duration-cell strong {
    color: var(--text-soft);
    font-family: var(--font-mono);
    font-size: 10.5px;
    font-weight: 550;
  }

  .diagnostic-cell span {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .diagnostic-cell .has-warnings {
    color: var(--warning);
  }

  .diagnostic-cell .has-errors {
    color: var(--danger);
  }

  .row-actions {
    display: flex;
    min-width: 150px;
    justify-content: flex-end;
  }

  .danger-link { min-height: 32px; border: 1px solid color-mix(in srgb, var(--danger) 45%, var(--line)); border-radius: var(--radius-sm); background: transparent; padding: 0 10px; color: var(--danger); font-size: 11px; font-weight: 700; }

  .diagnostics-button {
    min-height: 30px;
    border: 1px solid var(--info-line);
    background: var(--info-faint);
    color: var(--link);
    padding: 0 11px;
    font-size: 11px;
    font-weight: 600;
    white-space: nowrap;
  }

  .diagnostics-button:hover:not(:disabled) {
    border-color: var(--link);
    background: rgba(107, 184, 255, 0.14);
    color: var(--link-hover);
  }

  .no-diagnostics {
    color: var(--text-faint);
    font-size: 10.5px;
    white-space: nowrap;
  }

  /* Diagnostics */

  .diagnostics-panel {
    display: grid;
    border-top: 1px solid var(--line-soft);
    background: var(--bg-ink);
  }

  .diagnostics-panel__header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 14px;
    padding: 10px 14px;
    border-bottom: 1px solid var(--line-soft);
  }

  .diagnostics-panel__header > div {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .diagnostics-panel__header strong {
    color: var(--text-soft);
    font-size: 11.5px;
    font-weight: 600;
  }

  .diagnostics-panel__header span {
    color: var(--text-muted);
    font-size: 10px;
  }

  .copy-diagnostics-button {
    min-height: 28px;
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-muted);
    padding: 0 9px;
    font-size: 10.5px;
    white-space: nowrap;
  }

  .copy-diagnostics-button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
  }

  .copy-diagnostics-button.copied {
    border-color: var(--brand-line);
    color: var(--brand);
  }

  .diagnostics-panel pre {
    max-height: 460px;
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
    .submit-form {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }

    .build-summary {
      grid-template-columns: repeat(3, minmax(0, 1fr));
    }

    .build-summary > div:nth-child(3) {
      border-right: 0;
    }

    .build-summary > div:nth-child(-n + 3) {
      border-bottom: 1px solid var(--line-soft);
    }
  }

  @media (max-width: 880px) {
    .reports-layout {
      grid-template-columns: 1fr;
    }

    .project-directory {
      position: static;
    }

    .project-options {
      max-height: 280px;
    }
  }

  @media (max-width: 720px) {
    .submit-form {
      grid-template-columns: 1fr;
    }

    .submit-actions {
      grid-column: auto;
    }

    .project-overview__header {
      align-items: flex-start;
      flex-direction: column;
    }

    .build-summary {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }

    .build-summary > div,
    .build-summary > div:nth-child(3) {
      border-right: 1px solid var(--line-soft);
      border-bottom: 1px solid var(--line-soft);
    }

    .build-summary > div:nth-child(even) {
      border-right: 0;
    }

    .build-summary > div:last-child {
      border-bottom: 0;
    }

    .report-toolbar {
      grid-template-columns: 1fr;
    }
  }

  @media (max-width: 560px) {
    .submit-section {
      padding: 14px;
    }

    .submit-actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .submit-actions button {
      width: 100%;
    }

    .build-summary {
      grid-template-columns: 1fr;
    }

    .build-summary > div,
    .build-summary > div:nth-child(even) {
      border-right: 0;
    }

    .project-links {
      align-items: flex-start;
      flex-direction: column;
      gap: 9px;
    }

    .diagnostic-summary {
      align-items: flex-start;
      flex-direction: column;
      gap: 6px;
    }

    .diagnostics-panel__header {
      align-items: stretch;
      flex-direction: column;
    }

    .copy-diagnostics-button {
      width: 100%;
    }
  }
</style>
