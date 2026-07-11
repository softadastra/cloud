<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { listBuildReports } from '$lib/api/buildReports';
  import { listLockfiles } from '$lib/api/lockfiles';
  import { listPackageVersions } from '$lib/api/packageVersions';
  import {
    createPackage,
    listPackages
  } from '$lib/api/packages';
  import {
    archiveProject,
    createProject,
    deleteProject,
    listProjects,
    reactivateProject
  } from '$lib/api/projects';
  import { listWorkspaces } from '$lib/api/workspaces';
  import {
    ApiError,
    type BuildReport,
    type Lockfile,
    type Package,
    type PackageVersion,
    type Project,
    type Workspace
  } from '$lib/api/types';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import {
    canCreatePackage,
    canCreateProject
  } from '$lib/permissions';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  let workspaces: Workspace[] = [];
  let selectedWorkspaceId = '';

  let projects: Project[] = [];
  let selectedProject: Project | null = null;

  let packages: Package[] = [];
  let packageVersions: PackageVersion[] = [];
  let lockfiles: Lockfile[] = [];
  let buildReports: BuildReport[] = [];

  let projectQuery = '';

  let loading = true;
  let loadingDetail = false;
  let initialized = false;
  let projectRequestId = 0;
  let savingProject = false;
  let savingPackage = false;
  let busyProjectAction = false;

  let showProjectForm = false;
  let showPackageForm = false;

  let projectName = '';
  let projectSlug = '';
  let projectRepository = '';
  let slugTouched = false;

  let packageName = '';

  let copiedProjectId = false;
  let copyResetTimer: ReturnType<typeof setTimeout> | undefined;

  let detailRequestId = 0;
  let error = '';
  let success = '';

  $: selectedWorkspace =
    workspaces.find(
      (workspace) => workspace.id === selectedWorkspaceId
    ) ?? null;

  $: currentRole =
    selectedWorkspace?.current_user_role ?? 'viewer';

  $: canCreate = canCreateProject(currentRole);
  $: canPublish = canCreatePackage(currentRole);

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

  $: effectiveSlug = slugTouched
    ? slugify(projectSlug)
    : slugify(projectName);

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

  $: recentBuildReports = buildReports.slice(0, 8);
  $: recentLockfiles = lockfiles.slice(0, 6);
  $: recentPackages = packages.slice(0, 6);
  $: recentPackageVersions = packageVersions.slice(0, 6);
  $: selectedProjectBuildReportsHref = selectedProject
    ? `/build-reports?workspace_id=${selectedWorkspaceId}&project_id=${selectedProject.id}`
    : '';
  $: selectedProjectLockfilesHref = selectedProject
    ? `/lockfiles?workspace_id=${selectedWorkspaceId}&project_id=${selectedProject.id}`
    : '';

  $: successfulBuilds = buildReports.filter(
    (report) => report.status === 'success'
  ).length;

  $: failedBuilds = buildReports.filter(
    (report) => report.status === 'failed'
  ).length;

  function slugify(value: string) {
    return value
      .trim()
      .toLowerCase()
      .replace(/[^a-z0-9]+/g, '-')
      .replace(/^-|-$/g, '');
  }

  function shortId(value: string, length = 12) {
    return value.length > length
      ? `${value.slice(0, length)}…`
      : value;
  }

  function projectStatus(project: Project) {
    return project.status || (project.active === false ? 'archived' : 'active');
  }

  function projectInitial(project: Project) {
    return project.name.slice(0, 1).toUpperCase();
  }

  function packageNameForId(packageId: string) {
    return (
      packages.find((pkg) => pkg.id === packageId)?.name ??
      shortId(packageId)
    );
  }

  function updateProjectUrl(projectId: string) {
    const url = new URL(window.location.href);

    url.searchParams.set(
      'workspace_id',
      selectedWorkspaceId
    );

    url.searchParams.set('project_id', projectId);

    history.replaceState(
      null,
      '',
      `${url.pathname}${url.search}`
    );
  }

  function resetProjectForm() {
    projectName = '';
    projectSlug = '';
    projectRepository = '';
    slugTouched = false;
    showProjectForm = false;
  }

  function closeProjectForm() {
    if (savingProject) {
      return;
    }

    resetProjectForm();
    error = '';
  }

  function toggleProjectForm() {
    showProjectForm = !showProjectForm;
    error = '';
    success = '';

    if (!showProjectForm) {
      resetProjectForm();
    }
  }

  function closePackageForm() {
    if (savingPackage) {
      return;
    }

    showPackageForm = false;
    packageName = '';
    error = '';
  }

  function handleSlugInput(event: Event) {
    const input = event.currentTarget as HTMLInputElement;

    slugTouched = true;
    projectSlug = input.value;
  }

  function handleProjectSubmit(event: SubmitEvent) {
    event.preventDefault();
    void submitProject();
  }

  function handlePackageSubmit(event: SubmitEvent) {
    event.preventDefault();
    void submitPackage();
  }

  async function copyProjectId() {
    if (!selectedProject) {
      return;
    }

    try {
      await navigator.clipboard.writeText(
        selectedProject.id
      );

      copiedProjectId = true;

      if (copyResetTimer) {
        clearTimeout(copyResetTimer);
      }

      copyResetTimer = setTimeout(() => {
        copiedProjectId = false;
      }, 1800);
    } catch {
      error = 'Unable to copy the project ID.';
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

      await loadProjects(params.get('project_id'));
      initialized = true;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load projects.';
    } finally {
      loading = false;
    }
  }

  async function loadProjects(
    requestedProjectId?: string | null
  ) {
    const requestId = ++projectRequestId;

    if (!selectedWorkspaceId) {
      projects = [];
      selectedProject = null;
      clearProjectDetail();
      return;
    }

    const data = await listProjects(
      selectedWorkspaceId
    );

    if (
      requestId !== projectRequestId ||
      data.projects.some((project) => project.workspace_id !== selectedWorkspaceId)
    ) {
      return;
    }

    projects = data.projects;

    selectedProject =
      (
        requestedProjectId
          ? projects.find(
              (project) =>
                project.id === requestedProjectId
            )
          : null
      ) ??
      projects[0] ??
      null;

    if (selectedProject) {
      updateProjectUrl(selectedProject.id);
    }

    await loadProjectDetail(selectedProject);
  }

  async function switchWorkspace(workspaceId: string) {
    selectedWorkspaceId = workspaceId;
    projects = [];
    selectedProject = null;
    clearProjectDetail();
    showProjectForm = false;
    showPackageForm = false;
    projectQuery = '';
    copiedProjectId = false;
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
          : 'Unable to load projects.';
    } finally {
      loading = false;
    }
  }

  function clearProjectDetail() {
    packages = [];
    packageVersions = [];
    lockfiles = [];
    buildReports = [];
  }

  async function selectProject(project: Project) {
    if (
      project.id === selectedProject?.id ||
      loadingDetail
    ) {
      return;
    }

    selectedProject = project;
    copiedProjectId = false;
    error = '';
    success = '';

    updateProjectUrl(project.id);

    await loadProjectDetail(project);
  }

  async function loadProjectDetail(
    project: Project | null
  ) {
    if (!selectedWorkspaceId || !project) {
      clearProjectDetail();
      return;
    }

    const requestId = ++detailRequestId;
    const workspaceId = selectedWorkspaceId;

    loadingDetail = true;
    error = '';

    clearProjectDetail();

    try {
      const [
        packageData,
        lockfileData,
        buildReportData
      ] = await Promise.all([
        listPackages(workspaceId),
        listLockfiles(workspaceId, project.id),
        listBuildReports(workspaceId, project.id)
      ]);

      const versionResults = await Promise.all(
        packageData.packages.map((pkg) =>
          listPackageVersions(
            workspaceId,
            pkg.id
          ).catch(() => ({
            package_versions: [] as PackageVersion[]
          }))
        )
      );

      if (
        requestId !== detailRequestId ||
        selectedProject?.id !== project.id ||
        selectedWorkspaceId !== workspaceId
      ) {
        return;
      }

      packages = packageData.packages;
      lockfiles = lockfileData.lockfiles;
      buildReports = buildReportData.build_reports;

      packageVersions = versionResults.flatMap(
        (result) => result.package_versions
      );
    } catch (err) {
      if (requestId !== detailRequestId) {
        return;
      }

      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load project details.';
    } finally {
      if (requestId === detailRequestId) {
        loadingDetail = false;
      }
    }
  }

  async function handleProjectAction(action: 'archive' | 'reactivate' | 'delete') {
    if (!selectedProject || !selectedWorkspaceId || !canCreate) {
      return;
    }

    const message =
      action === 'delete'
        ? `Delete project ${selectedProject.name}? Build reports and lockfiles are kept for safety.`
        : action === 'archive'
          ? `Archive project ${selectedProject.name}?`
          : `Reactivate project ${selectedProject.name}?`;

    if (!window.confirm(message)) {
      return;
    }

    busyProjectAction = true;
    error = '';
    success = '';

    try {
      const updated =
        action === 'delete'
          ? await deleteProject(selectedWorkspaceId, selectedProject.id)
          : action === 'archive'
            ? await archiveProject(selectedWorkspaceId, selectedProject.id)
            : await reactivateProject(selectedWorkspaceId, selectedProject.id);

      if (action === 'delete') {
        projects = projects.filter((project) => project.id !== selectedProject?.id);
        selectedProject = projects[0] ?? null;
        updateProjectUrl(selectedProject?.id ?? '');
        await loadProjectDetail(selectedProject);
      } else {
        selectedProject = updated.project;
        projects = projects.map((project) =>
          project.id === updated.project.id ? updated.project : project
        );
      }

      success =
        action === 'delete'
          ? 'Project deleted.'
          : action === 'archive'
            ? 'Project archived.'
            : 'Project reactivated.';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to update project.';
    } finally {
      busyProjectAction = false;
    }
  }

  async function submitProject() {
    const user = $auth.user;
    const name = projectName.trim();
    const slug = effectiveSlug.trim();

    if (
      !user ||
      !selectedWorkspaceId ||
      !name ||
      !slug ||
      !canCreate
    ) {
      return;
    }

    savingProject = true;
    error = '';
    success = '';

    try {
      const created = await createProject({
        workspaceId: selectedWorkspaceId,
        ownerUserId: user.id,
        name,
        slug,
        repositoryUrl: projectRepository.trim(),
        defaultBranch: 'main'
      });

      projects = [
        created.project,
        ...projects
      ];

      selectedProject = created.project;
      resetProjectForm();

      updateProjectUrl(created.project.id);
      await loadProjectDetail(created.project);

      success = `Project "${created.project.name}" created.`;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to create project.';
    } finally {
      savingProject = false;
    }
  }

  async function submitPackage() {
    const user = $auth.user;
    const name = packageName.trim();

    if (
      !user ||
      !selectedWorkspaceId ||
      !name ||
      !canPublish
    ) {
      return;
    }

    savingPackage = true;
    error = '';
    success = '';

    try {
      const created = await createPackage({
        workspaceId: selectedWorkspaceId,
        ownerUserId: user.id,
        name,
        visibility: 'private'
      });

      packages = [
        created.package,
        ...packages
      ];

      packageName = '';
      showPackageForm = false;

      success = `Package "${created.package.name}" created.`;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to create package.';
    } finally {
      savingPackage = false;
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
  <title>Projects | Softadastra Cloud</title>
</svelte:head>

<PageHeader
  eyebrow="Workspace"
  title="Projects"
  workspaceName={selectedWorkspace?.name ?? ''}
  role={currentRole}
>
  {#if canCreate && !loading}
    <button
      class:secondary-button={showProjectForm}
      class="new-project-button"
      type="button"
      onclick={toggleProjectForm}
    >
      {showProjectForm ? 'Close' : 'New project'}
    </button>
  {/if}
</PageHeader>

<p class="page-description">
  Review project builds, lockfiles and the packages available in the
  current workspace.
</p>

<InlineError message={error} />

{#if success}
  <p class="success-message" role="status">
    {success}
  </p>
{/if}

{#if showProjectForm && canCreate}
  <section
    class="project-form-section"
    aria-labelledby="new-project-title"
  >
    <div class="form-section-head">
      <div>
        <h2 id="new-project-title">Create a project</h2>
        <p>Connect a C++ project to the current workspace.</p>
      </div>
    </div>

    <form class="project-form" onsubmit={handleProjectSubmit}>
      <label>
        <span>Project name</span>
        <input
          class="field"
          bind:value={projectName}
          placeholder="Vix Runtime"
          required
          autocomplete="off"
        />
      </label>

      <label>
        <span>Slug</span>
        <input
          class="field"
          value={effectiveSlug}
          placeholder="vix-runtime"
          required
          autocomplete="off"
          oninput={handleSlugInput}
        />
      </label>

      <label class="repository-field">
        <span>Repository URL</span>
        <input
          class="field"
          bind:value={projectRepository}
          type="url"
          placeholder="https://github.com/vixcpp/vix"
          autocomplete="off"
        />
      </label>

      <div class="slug-preview">
        <span>Identifier</span>
        <i class="preview-leader" aria-hidden="true"></i>
        <code>{effectiveSlug || 'project-slug'}</code>
      </div>

      <div class="project-form-actions">
        <button
          class="btn"
          type="button"
          disabled={savingProject}
          onclick={closeProjectForm}
        >
          Cancel
        </button>

        <button
          class="btn btn-primary"
          type="submit"
          disabled={
            savingProject ||
            !selectedWorkspaceId ||
            !projectName.trim() ||
            !effectiveSlug
          }
        >
          {savingProject ? 'Creating…' : 'Create project'}
        </button>
      </div>
    </form>
  </section>
{/if}

<div class="projects-shell">
  <!-- ============================================================== -->
  <!-- Project rail                                                    -->
  <!-- ============================================================== -->
  <aside class="project-rail" aria-label="Project directory">
    <div class="rail-head">
      <p class="rail-label">Projects</p>
      <span class="rail-count">{projects.length}</span>
    </div>

    {#if projects.length > 1}
      <div class="rail-search">
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
      <p class="rail-note">Loading projects…</p>
    {:else if projects.length === 0}
      <div class="rail-empty">
        <EmptyState
          title="No projects yet"
          body={canCreate
            ? 'Create a project here or run vix cloud init locally.'
            : 'A workspace administrator can create the first project.'}
        />
      </div>
    {:else if visibleProjects.length === 0}
      <p class="rail-note">No project matches your search.</p>
    {:else}
      <div class="rail-list">
        {#each visibleProjects as project (project.id)}
          <button
            class:selected={project.id === selectedProject?.id}
            class="rail-item"
            type="button"
            aria-pressed={project.id === selectedProject?.id}
            onclick={() => selectProject(project)}
          >
            <span class="rail-item__copy">
              <strong>{project.name}</strong>
              <code>{project.slug}</code>
            </span>

            {#if projectStatus(project) === 'archived'}
              <span class="rail-item__flag">archived</span>
            {/if}
          </button>
        {/each}
      </div>
    {/if}
  </aside>

  <!-- ============================================================== -->
  <!-- Detail                                                          -->
  <!-- ============================================================== -->
  <main class="project-detail">
    {#if loading}
      <div class="detail-loading">Loading project…</div>
    {:else if !selectedProject}
      <section class="panel empty-detail">
        <EmptyState
          title="Select a project"
          body="Its builds, lockfiles and related workspace resources will appear here."
        />
      </section>
    {:else}
      <!-- Identity bar -->
      <section
        class="identity"
        aria-labelledby="selected-project-title"
      >
        <div class="identity__main">
          <div class="identity__title-line">
            <h2 id="selected-project-title">
              {selectedProject.name}
            </h2>

            <StatusBadge status={projectStatus(selectedProject)} />

            {#if !canCreate}
              <span class="badge no-dot">Read only</span>
            {/if}
          </div>

          <div class="identity__meta">
            <code class="identity__slug">{selectedProject.slug}</code>

            <span class="identity__sep" aria-hidden="true"></span>

            <span class="identity__branch">
              {selectedProject.default_branch || 'main'}
            </span>

            <span class="identity__sep" aria-hidden="true"></span>

            <button
              class:copied={copiedProjectId}
              class="id-chip"
              type="button"
              title={copiedProjectId ? 'Copied' : 'Copy project ID'}
              onclick={copyProjectId}
            >
              <code>{shortId(selectedProject.id, 14)}</code>

              {#if copiedProjectId}
                <svg viewBox="0 0 24 24" aria-hidden="true">
                  <path d="m5 12 4 4L19 6"></path>
                </svg>
              {:else}
                <svg viewBox="0 0 24 24" aria-hidden="true">
                  <rect x="8" y="8" width="11" height="11" rx="2"></rect>
                  <path
                    d="M16 8V5a2 2 0 0 0-2-2H5a2 2 0 0 0-2 2v9a2 2 0 0 0 2 2h3"
                  ></path>
                </svg>
              {/if}
            </button>
          </div>
        </div>

        <div class="identity__actions">
          {#if canCreate}
            {#if projectStatus(selectedProject) === 'archived'}
              <button
                class="btn"
                type="button"
                disabled={busyProjectAction}
                onclick={() => handleProjectAction('reactivate')}
              >
                Reactivate
              </button>
            {:else}
              <button
                class="btn"
                type="button"
                disabled={busyProjectAction}
                onclick={() => handleProjectAction('archive')}
              >
                Archive
              </button>
            {/if}

            <button
              class="btn btn-danger"
              type="button"
              disabled={busyProjectAction}
              onclick={() => handleProjectAction('delete')}
            >
              Delete
            </button>
          {/if}

          <a
            class="btn btn-primary"
            href={`/build-reports?workspace_id=${selectedWorkspaceId}&project_id=${selectedProject.id}`}
          >
            Build reports

            <svg viewBox="0 0 24 24" aria-hidden="true">
              <path d="m9 18 6-6-6-6"></path>
            </svg>
          </a>
        </div>
      </section>

      <!-- Metrics strip -->
      <section class="metrics" aria-label="Project statistics">
        <div class="metric">
          <small>Build reports</small>
          <strong>{buildReports.length}</strong>
        </div>

        <div class="metric">
          <small>Successful</small>
          <strong class="is-ok">{successfulBuilds}</strong>
        </div>

        <div class="metric">
          <small>Failed</small>
          <strong class:is-bad={failedBuilds > 0}>{failedBuilds}</strong>
        </div>

        <div class="metric">
          <small>Lockfiles</small>
          <strong>{lockfiles.length}</strong>
        </div>

        <div class="metric">
          <small>Packages</small>
          <strong>{packages.length}</strong>
        </div>
      </section>

      {#if !canCreate}
        <ReadOnlyNotice
          message="Your role can review this project but cannot create or modify projects."
        />
      {/if}

      <!-- Resource grid -->
      <div class="resource-grid">
        <div class="primary-column">
          <!-- Build reports -->
          <section class="panel" aria-labelledby="build-reports-title">
            <header class="panel__head">
              <div>
                <h2 id="build-reports-title">Recent build reports</h2>
                <p>Latest recorded builds for this project.</p>
              </div>

              <div class="panel__head-actions">
                <span class="panel__meta">{buildReports.length}</span>
                <a
                  class="panel__link"
                  href={`/build-reports?workspace_id=${selectedWorkspaceId}&project_id=${selectedProject.id}`}
                >
                  View all
                </a>
              </div>
            </header>

            {#if loadingDetail}
              <p class="panel-note">Loading build reports…</p>
            {:else if recentBuildReports.length === 0}
              <div class="panel-empty">
                <EmptyState
                  title="No build reports"
                  body="Record the first report with vix build --report."
                />
              </div>
            {:else}
              <div class="table-wrap">
                <table class="data-table">
                  <thead>
                    <tr>
                      <th>Target</th>
                      <th>Profile</th>
                      <th class="is-right">Errors</th>
                      <th class="is-right">Status</th>
                    </tr>
                  </thead>

                  <tbody>
                    {#each recentBuildReports as report (report.id)}
                      <tr
                        class="is-link"
                        onclick={() =>
                          selectedProjectBuildReportsHref && goto(selectedProjectBuildReportsHref)}
                      >
                        <td class="is-primary">{report.target}</td>
                        <td class="cell-muted is-mono">
                          {report.profile || 'default'}
                        </td>
                        <td class="is-right is-mono">
                          {#if report.errors_count > 0}
                            <span class="err">{report.errors_count}</span>
                          {:else}
                            <span class="cell-faint">0</span>
                          {/if}
                        </td>
                        <td class="is-right">
                          <StatusBadge status={report.status} />
                        </td>
                      </tr>
                    {/each}
                  </tbody>
                </table>
              </div>
            {/if}
          </section>

          <!-- Lockfiles -->
          <section class="panel" aria-labelledby="lockfiles-title">
            <header class="panel__head">
              <div>
                <h2 id="lockfiles-title">Lockfiles</h2>
                <p>Dependency states uploaded for this project.</p>
              </div>

              <div class="panel__head-actions">
                <span class="panel__meta">{lockfiles.length}</span>
                <a
                  class="panel__link"
                  href={`/lockfiles?workspace_id=${selectedWorkspaceId}&project_id=${selectedProject.id}`}
                >
                  View all
                </a>
              </div>
            </header>

            {#if loadingDetail}
              <p class="panel-note">Loading lockfiles…</p>
            {:else if recentLockfiles.length === 0}
              <div class="panel-empty">
                <EmptyState
                  title="No lockfiles"
                  body="Upload one with vix cloud lockfile upload."
                />
              </div>
            {:else}
              <div class="table-wrap">
                <table class="data-table">
                  <thead>
                    <tr>
                      <th>Checksum</th>
                      <th>Source</th>
                      <th class="is-right"></th>
                    </tr>
                  </thead>

                  <tbody>
                    {#each recentLockfiles as lockfile (lockfile.id)}
                      <tr
                        class="is-link"
                        onclick={() =>
                          selectedProjectLockfilesHref && goto(selectedProjectLockfilesHref)}
                      >
                        <td class="is-primary is-mono">
                          {shortId(lockfile.checksum_sha256, 20)}
                        </td>
                        <td class="cell-muted">
                          {lockfile.source || 'Unknown source'}
                        </td>
                        <td class="is-right">
                          <svg class="row-chevron" viewBox="0 0 24 24" aria-hidden="true">
                            <path d="m9 18 6-6-6-6"></path>
                          </svg>
                        </td>
                      </tr>
                    {/each}
                  </tbody>
                </table>
              </div>
            {/if}
          </section>
        </div>

        <div class="secondary-column">
          <!-- Packages -->
          <section class="panel" aria-labelledby="packages-title">
            <header class="panel__head">
              <div>
                <h2 id="packages-title">Workspace packages</h2>
                <p>Private packages available to workspace projects.</p>
              </div>

              <div class="panel__head-actions">
                <span class="panel__meta">{packages.length}</span>

                {#if canPublish && !showPackageForm}
                  <button
                    class="btn btn-primary btn-sm"
                    type="button"
                    onclick={() => {
                      showPackageForm = true;
                      error = '';
                      success = '';
                    }}
                  >
                    New
                  </button>
                {/if}
              </div>
            </header>

            {#if showPackageForm && canPublish}
              <form class="package-form" onsubmit={handlePackageSubmit}>
                <label>
                  <span>Package name</span>
                  <input
                    class="field"
                    bind:value={packageName}
                    placeholder="vix/package-name"
                    required
                    autocomplete="off"
                  />
                </label>

                <div class="package-form-actions">
                  <button
                    class="btn btn-sm"
                    type="button"
                    disabled={savingPackage}
                    onclick={closePackageForm}
                  >
                    Cancel
                  </button>

                  <button
                    class="btn btn-primary btn-sm"
                    type="submit"
                    disabled={savingPackage || !packageName.trim()}
                  >
                    {savingPackage ? 'Creating…' : 'Create'}
                  </button>
                </div>
              </form>
            {/if}

            {#if loadingDetail}
              <p class="panel-note">Loading packages…</p>
            {:else if recentPackages.length === 0}
              <div class="panel-empty">
                <EmptyState
                  title="No packages"
                  body={canPublish
                    ? 'Publish with vix publish --cloud.'
                    : 'No packages are available in this workspace.'}
                />
              </div>
            {:else}
              <div class="list">
                {#each recentPackages as pkg (pkg.id)}
                  <a
                    class="list-row"
                    href={`/packages?workspace_id=${selectedWorkspaceId}`}
                  >
                    <span class="list-row__copy">
                      <strong>{pkg.name}</strong>
                      <span
                        class={`badge ${pkg.visibility === 'public' ? 'is-yes' : 'no-dot'}`}
                      >
                        {pkg.visibility}
                      </span>
                    </span>

                    <svg class="row-chevron" viewBox="0 0 24 24" aria-hidden="true">
                      <path d="m9 18 6-6-6-6"></path>
                    </svg>
                  </a>
                {/each}
              </div>

              <div class="panel-footer">
                <a href={`/packages?workspace_id=${selectedWorkspaceId}`}>
                  View all packages
                </a>
              </div>
            {/if}
          </section>

          <!-- Versions -->
          <section class="panel" aria-labelledby="versions-title">
            <header class="panel__head">
              <div>
                <h2 id="versions-title">Recent package versions</h2>
                <p>Versions published in the current workspace.</p>
              </div>

              <span class="panel__meta">{packageVersions.length}</span>
            </header>

            {#if loadingDetail}
              <p class="panel-note">Loading versions…</p>
            {:else if recentPackageVersions.length === 0}
              <div class="panel-empty">
                <EmptyState
                  title="No versions"
                  body="Publish a package version with vix publish --cloud."
                />
              </div>
            {:else}
              <div class="list">
                {#each recentPackageVersions as version (version.id)}
                  <a
                    class="list-row"
                    href={`/package-versions?workspace_id=${selectedWorkspaceId}`}
                  >
                    <span class="list-row__copy">
                      <strong>{packageNameForId(version.package_id)}</strong>
                      <code>{version.version}</code>
                    </span>

                    <StatusBadge status={version.status} />
                  </a>
                {/each}
              </div>

              <div class="panel-footer">
                <a href={`/package-versions?workspace_id=${selectedWorkspaceId}`}>
                  View all versions
                </a>
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

  .success-message {
    margin-bottom: 14px;
    border: 1px solid var(--green-line);
    border-radius: var(--radius-sm);
    background: var(--green-faint);
    color: var(--green-soft);
    padding: 9px 13px;
    font-size: 11.5px;
  }

  .new-project-button {
    border-color: var(--brand);
    background: var(--brand);
    color: var(--brand-ink);
  }

  .new-project-button:hover:not(:disabled) {
    border-color: var(--brand-soft);
    background: var(--brand-soft);
  }

  .new-project-button.secondary-button {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-soft);
  }

  .new-project-button.secondary-button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
  }

  /* ------------------------------------------------------------------ */
  /* Shared button system                                                */
  /* ------------------------------------------------------------------ */

  .btn {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    min-height: 32px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    background: var(--bg-elevated);
    color: var(--text-soft);
    padding: 0 11px;
    font-size: 11.5px;
    font-weight: 600;
    cursor: pointer;
    transition:
      color var(--speed) var(--ease),
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease);
  }

  .btn:hover:not(:disabled) {
    border-color: var(--brand);
    color: var(--text);
  }

  .btn:disabled {
    opacity: 0.55;
    cursor: not-allowed;
  }

  .btn-sm {
    min-height: 28px;
    padding: 0 9px;
    font-size: 11px;
  }

  .btn-primary {
    border-color: var(--brand);
    background: var(--brand);
    color: var(--brand-ink);
  }

  .btn-primary:hover:not(:disabled) {
    border-color: var(--brand-soft);
    background: var(--brand-soft);
    color: var(--brand-ink);
  }

  .btn-danger {
    border-color: color-mix(in srgb, var(--danger) 45%, var(--line-strong));
    color: var(--danger);
  }

  .btn-danger:hover:not(:disabled) {
    border-color: var(--danger);
    background: color-mix(in srgb, var(--danger) 12%, transparent);
    color: var(--danger);
  }

  .btn svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  /* ------------------------------------------------------------------ */
  /* Fields                                                              */
  /* ------------------------------------------------------------------ */

  .field {
    width: 100%;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    background: var(--bg-ink-soft);
    color: var(--text);
    padding: 8px 11px;
    font-size: 12px;
    font-family: inherit;
    transition: border-color var(--speed) var(--ease);
  }

  .field:focus {
    outline: none;
    border-color: var(--brand);
  }

  /* ------------------------------------------------------------------ */
  /* Project creation form                                               */
  /* ------------------------------------------------------------------ */

  .project-form-section {
    display: grid;
    gap: 16px;
    margin-bottom: 18px;
    border: 1px solid var(--brand-line);
    border-left: 2px solid var(--brand);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    padding: 18px;
  }

  .form-section-head h2 {
    font-size: 14px;
  }

  .form-section-head p {
    margin-top: 3px;
    color: var(--text-muted);
    font-size: 12px;
  }

  .project-form {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 14px;
  }

  .project-form label {
    display: grid;
    gap: 5px;
  }

  .project-form label > span {
    color: var(--text-muted);
    font-size: 10.5px;
    font-weight: 550;
  }

  .repository-field,
  .slug-preview,
  .project-form-actions {
    grid-column: 1 / -1;
  }

  .slug-preview {
    display: flex;
    min-width: 0;
    align-items: baseline;
    gap: 9px;
    font-family: var(--font-mono);
    font-size: 11px;
  }

  .slug-preview > span {
    color: var(--text-faint);
    text-transform: uppercase;
    letter-spacing: 0.08em;
  }

  .preview-leader {
    flex: 1;
    border-bottom: 1px dotted var(--line-strong);
    transform: translateY(-3px);
  }

  .slug-preview code {
    color: var(--text-soft);
    overflow-wrap: anywhere;
  }

  .project-form-actions {
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    border-top: 1px solid var(--line-soft);
    padding-top: 14px;
  }

  /* ------------------------------------------------------------------ */
  /* Shell                                                               */
  /* ------------------------------------------------------------------ */

  .projects-shell {
    display: grid;
    grid-template-columns: 244px minmax(0, 1fr);
    gap: 18px;
    align-items: start;
  }

  /* ------------------------------------------------------------------ */
  /* Project rail                                                        */
  /* ------------------------------------------------------------------ */

  .project-rail {
    position: sticky;
    top: 24px;
    min-width: 0;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    overflow: hidden;
  }

  .rail-head {
    display: flex;
    align-items: center;
    justify-content: space-between;
    border-bottom: 1px solid var(--line-soft);
    padding: 12px 14px;
  }

  .rail-label {
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 9px;
    letter-spacing: 0.14em;
    text-transform: uppercase;
  }

  .rail-count {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 10.5px;
  }

  .rail-search {
    position: relative;
    border-bottom: 1px solid var(--line-soft);
    padding: 10px;
  }

  .rail-search svg {
    position: absolute;
    top: 50%;
    left: 21px;
    width: 14px;
    height: 14px;
    fill: none;
    stroke: var(--text-faint);
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
    transform: translateY(-50%);
    pointer-events: none;
  }

  .rail-search input {
    width: 100%;
    min-height: 32px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    background: var(--bg-ink-soft);
    color: var(--text);
    padding: 0 10px 0 32px;
    font-size: 12px;
    font-family: inherit;
  }

  .rail-search input:focus {
    outline: none;
    border-color: var(--brand);
  }

  .rail-list {
    display: grid;
    max-height: calc(100vh - 200px);
    overflow-y: auto;
  }

  .rail-item {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
    width: 100%;
    min-height: 50px;
    border: 0;
    border-bottom: 1px solid var(--line-soft);
    border-left: 2px solid transparent;
    border-radius: 0;
    background: transparent;
    color: var(--text);
    padding: 8px 13px;
    text-align: left;
    cursor: pointer;
    transition: background var(--speed) var(--ease);
  }

  .rail-item:last-child {
    border-bottom: 0;
  }

  .rail-item:hover:not(:disabled) {
    background: var(--bg-elevated);
  }

  .rail-item.selected {
    border-left-color: var(--brand);
    background: var(--brand-faint);
  }

  .rail-item__copy {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .rail-item__copy strong {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 12px;
    font-weight: 600;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .rail-item.selected .rail-item__copy strong {
    color: var(--brand-bright);
  }

  .rail-item__copy code {
    overflow: hidden;
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 9.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .rail-item__flag {
    flex: 0 0 auto;
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 8.5px;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }

  .rail-note {
    padding: 16px 14px;
    color: var(--text-muted);
    font-size: 12px;
  }

  .rail-empty {
    padding: 10px;
  }

  /* ------------------------------------------------------------------ */
  /* Detail column                                                       */
  /* ------------------------------------------------------------------ */

  .project-detail {
    display: grid;
    min-width: 0;
    gap: 16px;
  }

  .detail-loading {
    min-height: 120px;
    display: grid;
    place-items: center;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    color: var(--text-muted);
    font-size: 12px;
  }

  /* ------------------------------------------------------------------ */
  /* Identity bar                                                        */
  /* ------------------------------------------------------------------ */

  .identity {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    padding: 15px 17px;
  }

  .identity__main {
    display: grid;
    min-width: 0;
    gap: 8px;
  }

  .identity__title-line {
    display: flex;
    align-items: center;
    gap: 10px;
    flex-wrap: wrap;
  }

  .identity__title-line h2 {
    color: var(--text);
    font-size: 17px;
    font-weight: 650;
    letter-spacing: -0.01em;
    overflow-wrap: anywhere;
  }

  .identity__meta {
    display: flex;
    align-items: center;
    gap: 10px;
    flex-wrap: wrap;
  }

  .identity__slug {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 11px;
  }

  .identity__branch {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 11px;
  }

  .identity__branch::before {
    content: '⎇ ';
    color: var(--text-faint);
  }

  .identity__sep {
    width: 3px;
    height: 3px;
    border-radius: 50%;
    background: var(--line-strong);
  }

  .id-chip {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    background: var(--bg-ink-soft);
    color: var(--text-muted);
    padding: 3px 8px;
    cursor: pointer;
    transition:
      color var(--speed) var(--ease),
      border-color var(--speed) var(--ease);
  }

  .id-chip:hover {
    border-color: var(--brand);
    color: var(--text-soft);
  }

  .id-chip.copied {
    border-color: var(--brand-line);
    color: var(--brand);
  }

  .id-chip code {
    font-family: var(--font-mono);
    font-size: 10px;
  }

  .id-chip svg {
    width: 12px;
    height: 12px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.9;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .identity__actions {
    display: flex;
    flex: 0 0 auto;
    align-items: center;
    justify-content: flex-end;
    gap: 8px;
    flex-wrap: wrap;
  }

  /* ------------------------------------------------------------------ */
  /* Metrics strip                                                       */
  /* ------------------------------------------------------------------ */

  .metrics {
    display: grid;
    grid-template-columns: repeat(5, minmax(0, 1fr));
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    overflow: hidden;
  }

  .metric {
    display: grid;
    gap: 6px;
    border-right: 1px solid var(--line-soft);
    padding: 13px 16px;
  }

  .metric:last-child {
    border-right: 0;
  }

  .metric small {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 9px;
    letter-spacing: 0.1em;
    text-transform: uppercase;
  }

  .metric strong {
    color: var(--text);
    font-size: 22px;
    font-weight: 650;
    letter-spacing: -0.02em;
    line-height: 1.1;
    font-variant-numeric: tabular-nums;
  }

  .metric strong.is-ok {
    color: var(--green-soft);
  }

  .metric strong.is-bad {
    color: var(--danger);
  }

  /* ------------------------------------------------------------------ */
  /* Resource grid                                                       */
  /* ------------------------------------------------------------------ */

  .resource-grid {
    display: grid;
    grid-template-columns: minmax(0, 1.35fr) minmax(300px, 0.85fr);
    gap: 16px;
    align-items: start;
  }

  .primary-column,
  .secondary-column {
    display: grid;
    min-width: 0;
    gap: 16px;
  }

  /* ------------------------------------------------------------------ */
  /* Panels                                                              */
  /* ------------------------------------------------------------------ */

  .panel {
    min-width: 0;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    overflow: hidden;
  }

  .empty-detail {
    padding: 16px;
  }

  .panel__head {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 14px;
    border-bottom: 1px solid var(--line-soft);
    padding: 13px 16px;
  }

  .panel__head h2 {
    font-size: 13px;
    font-weight: 650;
  }

  .panel__head p {
    margin-top: 2px;
    color: var(--text-muted);
    font-size: 11px;
  }

  .panel__head-actions {
    display: flex;
    flex: 0 0 auto;
    align-items: center;
    gap: 10px;
  }

  .panel__meta {
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 10px;
  }

  .panel__link {
    color: var(--link);
    font-size: 11px;
    font-weight: 600;
  }

  .panel__link:hover {
    color: var(--link-hover);
    text-decoration: underline;
    text-underline-offset: 3px;
  }

  .panel-note {
    min-height: 60px;
    padding: 18px 16px;
    color: var(--text-muted);
    font-size: 12px;
  }

  .panel-empty {
    padding: 11px;
  }

  .panel-footer {
    display: flex;
    justify-content: flex-end;
    border-top: 1px solid var(--line-soft);
    padding: 9px 16px;
  }

  .panel-footer a {
    color: var(--link);
    font-size: 10.5px;
    font-weight: 600;
  }

  .panel-footer a:hover {
    color: var(--link-hover);
  }

  /* ------------------------------------------------------------------ */
  /* Data table                                                          */
  /* ------------------------------------------------------------------ */

  .table-wrap {
    overflow-x: auto;
  }

  .data-table {
    width: 100%;
    border-collapse: collapse;
  }

  .data-table th {
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
    color: var(--text-faint);
    padding: 8px 16px;
    font-family: var(--font-mono);
    font-size: 9px;
    font-weight: 600;
    letter-spacing: 0.11em;
    text-align: left;
    text-transform: uppercase;
    white-space: nowrap;
  }

  .data-table td {
    border-bottom: 1px solid var(--line-soft);
    color: var(--text-muted);
    padding: 11px 16px;
    font-size: 11.5px;
    white-space: nowrap;
  }

  .data-table tbody tr:last-child td {
    border-bottom: 0;
  }

  .data-table tbody tr.is-link {
    cursor: pointer;
  }

  .data-table tbody tr.is-link:hover td {
    background: var(--bg-elevated);
  }

  .data-table .is-primary {
    color: var(--text-soft);
    font-weight: 600;
  }

  .data-table tbody tr.is-link:hover .is-primary {
    color: var(--link);
  }

  .is-right {
    text-align: right;
  }

  .is-mono {
    font-family: var(--font-mono);
    font-size: 10.5px;
  }

  .cell-muted {
    color: var(--text-muted);
  }

  .cell-faint {
    color: var(--text-faint);
  }

  .err {
    color: var(--danger);
    font-weight: 600;
  }

  .row-chevron {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: var(--text-faint);
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  /* ------------------------------------------------------------------ */
  /* Badges                                                              */
  /* ------------------------------------------------------------------ */

  .badge {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    background: var(--bg-elevated);
    color: var(--text-muted);
    padding: 2px 7px;
    font-family: var(--font-mono);
    font-size: 9.5px;
    font-weight: 600;
    white-space: nowrap;
    text-transform: capitalize;
  }

  .badge::before {
    content: '';
    width: 5px;
    height: 5px;
    border-radius: 50%;
    background: var(--text-faint);
  }

  .badge.no-dot::before {
    display: none;
  }

  .badge.is-yes {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .badge.is-yes::before {
    background: var(--green-soft);
  }

  /* ------------------------------------------------------------------ */
  /* Package / version lists                                             */
  /* ------------------------------------------------------------------ */

  .list {
    display: grid;
  }

  .list-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    min-height: 50px;
    border-bottom: 1px solid var(--line-soft);
    padding: 9px 15px;
    transition: background var(--speed) var(--ease);
  }

  .list-row:last-child {
    border-bottom: 0;
  }

  .list-row:hover {
    background: var(--bg-elevated);
  }

  .list-row__copy {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 8px;
  }

  .list-row__copy strong {
    overflow: hidden;
    color: var(--link);
    font-size: 11.5px;
    font-weight: 600;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .list-row:hover .list-row__copy strong {
    color: var(--link-hover);
  }

  .list-row__copy code {
    flex: 0 0 auto;
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 10px;
  }

  /* ------------------------------------------------------------------ */
  /* Package form                                                        */
  /* ------------------------------------------------------------------ */

  .package-form {
    display: grid;
    gap: 11px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
    padding: 13px 15px;
  }

  .package-form label {
    display: grid;
    gap: 5px;
  }

  .package-form label > span {
    color: var(--text-muted);
    font-size: 10.5px;
    font-weight: 550;
  }

  .package-form-actions {
    display: flex;
    justify-content: flex-end;
    gap: 7px;
  }

  /* ------------------------------------------------------------------ */
  /* Responsive                                                          */
  /* ------------------------------------------------------------------ */

  @media (max-width: 1080px) {
    .resource-grid {
      grid-template-columns: 1fr;
    }

    .secondary-column {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }
  }

  @media (max-width: 880px) {
    .projects-shell {
      grid-template-columns: 1fr;
    }

    .project-rail {
      position: static;
    }

    .rail-list {
      max-height: 260px;
    }
  }

  @media (max-width: 720px) {
    .project-form {
      grid-template-columns: 1fr;
    }

    .repository-field,
    .slug-preview,
    .project-form-actions {
      grid-column: auto;
    }

    .identity {
      align-items: flex-start;
      flex-direction: column;
    }

    .identity__actions {
      justify-content: flex-start;
    }

    .metrics {
      grid-template-columns: repeat(3, minmax(0, 1fr));
    }

    .metric:nth-child(3n) {
      border-right: 0;
    }

    .metric:nth-child(n + 4) {
      border-top: 1px solid var(--line-soft);
    }

    .secondary-column {
      grid-template-columns: 1fr;
    }
  }

  @media (max-width: 520px) {
    .project-form-section {
      padding: 14px;
    }

    .project-form-actions {
      flex-direction: column-reverse;
    }

    .project-form-actions .btn {
      width: 100%;
      justify-content: center;
    }

    .metrics {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }

    .metric:nth-child(3n) {
      border-right: 1px solid var(--line-soft);
    }

    .metric:nth-child(2n) {
      border-right: 0;
    }

    .metric:nth-child(n + 3) {
      border-top: 1px solid var(--line-soft);
    }

    .package-form-actions {
      flex-direction: column-reverse;
    }

    .package-form-actions .btn {
      width: 100%;
      justify-content: center;
    }
  }
</style>
