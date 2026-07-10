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
    <div class="section-heading">
      <div>
        <h2 id="new-project-title">
          Create a project
        </h2>

        <p>
          Connect a C++ project to the current workspace.
        </p>
      </div>
    </div>

    <form
      class="project-form"
      onsubmit={handleProjectSubmit}
    >
      <label>
        Project name

        <input
          bind:value={projectName}
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

      <label class="repository-field">
        Repository URL

        <input
          bind:value={projectRepository}
          type="url"
          placeholder="https://github.com/vixcpp/vix"
          autocomplete="off"
        />
      </label>

      <div class="slug-preview">
        <span>Identifier</span>
        <code>{effectiveSlug || 'project-slug'}</code>
      </div>

      <div class="project-form-actions">
        <button
          class="secondary-button"
          type="button"
          disabled={savingProject}
          onclick={closeProjectForm}
        >
          Cancel
        </button>

        <button
          type="submit"
          disabled={
            savingProject ||
            !selectedWorkspaceId ||
            !projectName.trim() ||
            !effectiveSlug
          }
        >
          {savingProject
            ? 'Creating…'
            : 'Create project'}
        </button>
      </div>
    </form>
  </section>
{/if}

<div class="projects-layout">
  <aside
    class="project-directory"
    aria-label="Project directory"
  >
    <div class="directory-header">
      <div>
        <h2>Projects</h2>
        <p>{selectedWorkspace?.name ?? 'Current workspace'}</p>
      </div>

      <span class="section-count">
        {projects.length}
      </span>
    </div>

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
          title="No projects yet"
          body={canCreate
            ? 'Create a project here or run vix cloud init locally.'
            : 'A workspace administrator can create the first project.'}
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
              project.id === selectedProject?.id
            }
            class="project-option"
            type="button"
            aria-pressed={
              project.id === selectedProject?.id
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

  <main class="project-detail">
    {#if loading}
      <section class="detail-section">
        <p class="detail-loading">
          Loading project…
        </p>
      </section>
    {:else if !selectedProject}
      <section class="detail-section empty-detail">
        <EmptyState
          title="Select a project"
          body="Its builds, lockfiles and related workspace resources will appear here."
        />
      </section>
    {:else}
      <section
        class="project-overview"
        aria-labelledby="selected-project-title"
      >
        <div class="project-overview__header">
          <div class="project-identity">
            <span class="project-mark">
              {projectInitial(selectedProject)}
            </span>

            <div>
              <div class="project-name-line">
                <h2 id="selected-project-title">
                  {selectedProject.name}
                </h2>

                <StatusBadge status={projectStatus(selectedProject)} />

                {#if !canCreate}
                  <span class="neutral-badge">
                    Read only
                  </span>
                {/if}
              </div>

              <code>{selectedProject.slug}</code>
            </div>
          </div>

          <div class="project-header-actions">
            {#if canCreate}
              {#if projectStatus(selectedProject) === 'archived'}
                <button
                  class="secondary-button"
                  type="button"
                  disabled={busyProjectAction}
                  onclick={() => handleProjectAction('reactivate')}
                >
                  Reactivate
                </button>
              {:else}
                <button
                  class="secondary-button"
                  type="button"
                  disabled={busyProjectAction}
                  onclick={() => handleProjectAction('archive')}
                >
                  Archive
                </button>
              {/if}

              <button
                class="danger-button"
                type="button"
                disabled={busyProjectAction}
                onclick={() => handleProjectAction('delete')}
              >
                Delete
              </button>
            {/if}

            <a
              class="primary-link"
              href={`/build-reports?workspace_id=${selectedWorkspaceId}&project_id=${selectedProject.id}`}
            >
              View build reports

              <svg viewBox="0 0 24 24" aria-hidden="true">
                <path d="m9 18 6-6-6-6"></path>
              </svg>
            </a>
          </div>
        </div>

        <dl class="project-summary">
          <div>
            <dt>Default branch</dt>
            <dd>
              {selectedProject.default_branch || 'main'}
            </dd>
          </div>

          <div>
            <dt>Build reports</dt>
            <dd>{buildReports.length}</dd>
          </div>

          <div>
            <dt>Successful</dt>
            <dd>{successfulBuilds}</dd>
          </div>

          <div>
            <dt>Failed</dt>
            <dd>{failedBuilds}</dd>
          </div>

          <div>
            <dt>Lockfiles</dt>
            <dd>{lockfiles.length}</dd>
          </div>
        </dl>

        <div class="project-id-row">
          <span>Project ID</span>

          <code>{selectedProject.id}</code>

          <button
            class:copied={copiedProjectId}
            class="copy-id-button"
            type="button"
            aria-label={
              copiedProjectId
                ? 'Project ID copied'
                : 'Copy project ID'
            }
            title={
              copiedProjectId
                ? 'Copied'
                : 'Copy project ID'
            }
            onclick={copyProjectId}
          >
            {#if copiedProjectId}
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
          class="project-links"
          aria-label="Project resources"
        >
          <a
            href={`/build-reports?workspace_id=${selectedWorkspaceId}&project_id=${selectedProject.id}`}
          >
            Build reports
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

          <a
            href={`/package-versions?workspace_id=${selectedWorkspaceId}`}
          >
            Package versions
          </a>
        </nav>
      </section>

      {#if !canCreate}
        <ReadOnlyNotice
          message="Your role can review this project but cannot create or modify projects."
        />
      {/if}

      <div class="project-content-grid">
        <div class="primary-column">
          <section
            class="detail-section"
            aria-labelledby="build-reports-title"
          >
            <div class="section-header">
              <div>
                <h2 id="build-reports-title">
                  Recent build reports
                </h2>

                <p>
                  Latest recorded builds for this project.
                </p>
              </div>

              <div class="section-header__actions">
                <span class="section-count">
                  {buildReports.length}
                </span>

                <a
                  class="section-link"
                  href={`/build-reports?workspace_id=${selectedWorkspaceId}&project_id=${selectedProject.id}`}
                >
                  View all
                </a>
              </div>
            </div>

            {#if loadingDetail}
              <p class="detail-loading">
                Loading build reports…
              </p>
            {:else if recentBuildReports.length === 0}
              <div class="section-empty">
                <EmptyState
                  title="No build reports"
                  body="Record the first report with vix build --report."
                />
              </div>
            {:else}
              <div class="build-list">
                {#each recentBuildReports as report (report.id)}
                  <a
                    class="build-row"
                    href={`/build-reports?workspace_id=${selectedWorkspaceId}&project_id=${selectedProject.id}`}
                  >
                    <div class="build-identity">
                      <strong>{report.target}</strong>

                      <span>
                        {report.profile || 'default'}
                      </span>
                    </div>

                    <div class="build-result">
                      {#if report.errors_count > 0}
                        <span class="error-count">
                          {report.errors_count}
                          {report.errors_count === 1
                            ? ' error'
                            : ' errors'}
                        </span>
                      {/if}

                      <StatusBadge status={report.status} />
                    </div>
                  </a>
                {/each}
              </div>
            {/if}
          </section>

          <section
            class="detail-section"
            aria-labelledby="lockfiles-title"
          >
            <div class="section-header">
              <div>
                <h2 id="lockfiles-title">
                  Lockfiles
                </h2>

                <p>
                  Dependency states uploaded for this project.
                </p>
              </div>

              <div class="section-header__actions">
                <span class="section-count">
                  {lockfiles.length}
                </span>

                <a
                  class="section-link"
                  href={`/lockfiles?workspace_id=${selectedWorkspaceId}&project_id=${selectedProject.id}`}
                >
                  View all
                </a>
              </div>
            </div>

            {#if loadingDetail}
              <p class="detail-loading">
                Loading lockfiles…
              </p>
            {:else if recentLockfiles.length === 0}
              <div class="section-empty">
                <EmptyState
                  title="No lockfiles"
                  body="Upload one with vix cloud lockfile upload."
                />
              </div>
            {:else}
              <div class="resource-list">
                {#each recentLockfiles as lockfile (lockfile.id)}
                  <a
                    class="resource-row"
                    href={`/lockfiles?workspace_id=${selectedWorkspaceId}&project_id=${selectedProject.id}`}
                  >
                    <div>
                      <strong>
                        {shortId(lockfile.checksum_sha256, 18)}
                      </strong>

                      <span>
                        {lockfile.source || 'Unknown source'}
                      </span>
                    </div>

                    <svg viewBox="0 0 24 24" aria-hidden="true">
                      <path d="m9 18 6-6-6-6"></path>
                    </svg>
                  </a>
                {/each}
              </div>
            {/if}
          </section>
        </div>

        <div class="secondary-column">
          <section
            class="detail-section"
            aria-labelledby="packages-title"
          >
            <div class="section-header">
              <div>
                <h2 id="packages-title">
                  Workspace packages
                </h2>

                <p>
                  Private packages available to workspace projects.
                </p>
              </div>

              <div class="section-header__actions">
                <span class="section-count">
                  {packages.length}
                </span>

                {#if canPublish && !showPackageForm}
                  <button
                    class="small-action-button"
                    type="button"
                    onclick={() => {
                      showPackageForm = true;
                      error = '';
                      success = '';
                    }}
                  >
                    New package
                  </button>
                {/if}
              </div>
            </div>

            {#if showPackageForm && canPublish}
              <form
                class="package-form"
                onsubmit={handlePackageSubmit}
              >
                <label>
                  Package name

                  <input
                    bind:value={packageName}
                    placeholder="vix/package-name"
                    required
                    autocomplete="off"
                  />
                </label>

                <div class="package-form-actions">
                  <button
                    class="secondary-button compact-button"
                    type="button"
                    disabled={savingPackage}
                    onclick={closePackageForm}
                  >
                    Cancel
                  </button>

                  <button
                    class="compact-button"
                    type="submit"
                    disabled={
                      savingPackage ||
                      !packageName.trim()
                    }
                  >
                    {savingPackage ? 'Creating…' : 'Create'}
                  </button>
                </div>
              </form>
            {/if}

            {#if loadingDetail}
              <p class="detail-loading">
                Loading packages…
              </p>
            {:else if recentPackages.length === 0}
              <div class="section-empty">
                <EmptyState
                  title="No packages"
                  body={canPublish
                    ? 'Publish with vix publish --cloud.'
                    : 'No packages are available in this workspace.'}
                />
              </div>
            {:else}
              <div class="resource-list">
                {#each recentPackages as pkg (pkg.id)}
                  <a
                    class="resource-row"
                    href={`/packages?workspace_id=${selectedWorkspaceId}`}
                  >
                    <div>
                      <strong>{pkg.name}</strong>
                      <span>{pkg.visibility}</span>
                    </div>

                    <svg viewBox="0 0 24 24" aria-hidden="true">
                      <path d="m9 18 6-6-6-6"></path>
                    </svg>
                  </a>
                {/each}
              </div>

              <div class="section-footer">
                <a
                  href={`/packages?workspace_id=${selectedWorkspaceId}`}
                >
                  View all packages
                </a>
              </div>
            {/if}
          </section>

          <section
            class="detail-section"
            aria-labelledby="versions-title"
          >
            <div class="section-header">
              <div>
                <h2 id="versions-title">
                  Recent package versions
                </h2>

                <p>
                  Versions published in the current workspace.
                </p>
              </div>

              <span class="section-count">
                {packageVersions.length}
              </span>
            </div>

            {#if loadingDetail}
              <p class="detail-loading">
                Loading versions…
              </p>
            {:else if recentPackageVersions.length === 0}
              <div class="section-empty">
                <EmptyState
                  title="No versions"
                  body="Publish a package version with vix publish --cloud."
                />
              </div>
            {:else}
              <div class="version-list">
                {#each recentPackageVersions as version (version.id)}
                  <a
                    class="version-row"
                    href={`/package-versions?workspace_id=${selectedWorkspaceId}`}
                  >
                    <div>
                      <strong>
                        {packageNameForId(version.package_id)}
                      </strong>

                      <code>{version.version}</code>
                    </div>

                    <StatusBadge status={version.status} />
                  </a>
                {/each}
              </div>

              <div class="section-footer">
                <a
                  href={`/package-versions?workspace_id=${selectedWorkspaceId}`}
                >
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

  .projects-layout {
    display: grid;
    grid-template-columns: 270px minmax(0, 1fr);
    gap: 16px;
    align-items: start;
  }

  /* Project creation */

  .project-form-section {
    display: grid;
    gap: 17px;
    margin-bottom: 18px;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    padding: 18px;
  }

  .section-heading {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 16px;
  }

  .section-heading > div {
    display: grid;
    gap: 4px;
  }

  .section-heading h2 {
    font-size: 14px;
  }

  .section-heading p {
    color: var(--text-muted);
    font-size: 12px;
  }

  .project-form {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 14px;
  }

  .repository-field,
  .slug-preview,
  .project-form-actions {
    grid-column: 1 / -1;
  }

  .slug-preview {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 8px;
    color: var(--text-muted);
    font-size: 11.5px;
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

  .project-search {
    position: relative;
    padding: 10px;
    border-bottom: 1px solid var(--line-soft);
  }

  .project-search svg {
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

  .project-search input {
    min-height: 34px;
    padding-left: 33px;
    font-size: 12px;
  }

  .project-options {
    display: grid;
    max-height: calc(100vh - 220px);
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

  .project-detail {
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

  .project-name-line {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 8px;
    flex-wrap: wrap;
  }

  .project-name-line h2 {
    color: var(--text);
    font-size: 16px;
    font-weight: 600;
    overflow-wrap: anywhere;
  }

  .project-identity code {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .neutral-badge {
    display: inline-flex;
    min-height: 20px;
    align-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    background: var(--bg-elevated);
    color: var(--text-muted);
    padding: 0 7px;
    font-size: 10px;
  }


  .project-header-actions {
    display: flex;
    flex: 0 0 auto;
    flex-wrap: wrap;
    align-items: center;
    justify-content: flex-end;
    gap: 8px;
  }

  .project-header-actions button {
    min-height: 34px;
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
    background: var(--bg-elevated);
    padding: 0 10px;
    color: var(--text-strong);
    font-size: 12px;
    font-weight: 700;
    cursor: pointer;
  }

  .project-header-actions button:disabled {
    cursor: wait;
    opacity: 0.65;
  }

  .project-header-actions .danger-button {
    border-color: color-mix(in srgb, var(--danger) 55%, var(--line));
    background: color-mix(in srgb, var(--danger) 12%, transparent);
    color: var(--danger);
  }

  .primary-link,
  .section-link,
  .project-links a,
  .section-footer a {
    color: var(--link);
    font-weight: 550;
  }

  .primary-link:hover,
  .section-link:hover,
  .project-links a:hover,
  .section-footer a:hover {
    color: var(--link-hover);
  }

  .primary-link {
    display: inline-flex;
    flex: 0 0 auto;
    align-items: center;
    gap: 5px;
    font-size: 11.5px;
  }

  .primary-link svg,
  .resource-row > svg {
    width: 15px;
    height: 15px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .project-summary {
    display: grid;
    grid-template-columns: repeat(5, minmax(0, 1fr));
    margin: 0;
    border-bottom: 1px solid var(--line-soft);
  }

  .project-summary > div {
    display: grid;
    min-width: 0;
    gap: 4px;
    padding: 12px 14px;
    border-right: 1px solid var(--line-soft);
  }

  .project-summary > div:last-child {
    border-right: 0;
  }

  .project-summary dt {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .project-summary dd {
    margin: 0;
    color: var(--text-soft);
    font-size: 12px;
    font-weight: 550;
    overflow-wrap: anywhere;
  }

  .project-id-row {
    display: grid;
    grid-template-columns: auto minmax(0, 1fr) 30px;
    gap: 10px;
    align-items: center;
    padding: 10px 14px;
    border-bottom: 1px solid var(--line-soft);
  }

  .project-id-row > span {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .project-id-row > code {
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

  /* Project content */

  .project-content-grid {
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

  .section-header__actions {
    display: flex;
    align-items: center;
    gap: 10px;
    flex: 0 0 auto;
  }

  .section-link {
    font-size: 11px;
  }

  .small-action-button {
    min-height: 28px;
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-bright);
    padding: 0 9px;
    font-size: 11px;
  }

  .small-action-button:hover:not(:disabled) {
    border-color: var(--brand);
    background: rgba(249, 115, 22, 0.17);
    color: var(--brand-bright);
  }

  /* Build reports */

  .build-list,
  .resource-list,
  .version-list {
    display: grid;
  }

  .build-row,
  .resource-row,
  .version-row {
    min-width: 0;
    border-bottom: 1px solid var(--line-soft);
  }

  .build-row:last-child,
  .resource-row:last-child,
  .version-row:last-child {
    border-bottom: 0;
  }

  .build-row {
    display: flex;
    min-height: 56px;
    align-items: center;
    justify-content: space-between;
    gap: 14px;
    padding: 10px 15px;
  }

  .build-row:hover,
  .resource-row:hover,
  .version-row:hover {
    background: var(--info-faint);
  }

  .build-identity {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .build-identity strong {
    color: var(--link);
    font-size: 12px;
    font-weight: 600;
    overflow-wrap: anywhere;
  }

  .build-row:hover .build-identity strong {
    color: var(--link-hover);
  }

  .build-identity span {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .build-result {
    display: flex;
    flex: 0 0 auto;
    align-items: center;
    justify-content: flex-end;
    gap: 8px;
  }

  .error-count {
    color: var(--danger);
    font-size: 10.5px;
    font-weight: 550;
  }

  /* Lockfiles and packages */

  .resource-row {
    display: flex;
    min-height: 53px;
    align-items: center;
    justify-content: space-between;
    gap: 14px;
    padding: 9px 14px;
    color: var(--link);
  }

  .resource-row > div {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .resource-row strong {
    color: var(--link);
    font-family: var(--font-mono);
    font-size: 10.5px;
    font-weight: 550;
    overflow-wrap: anywhere;
  }

  .resource-row:hover strong {
    color: var(--link-hover);
  }

  .resource-row span {
    color: var(--text-muted);
    font-size: 10.5px;
    text-transform: capitalize;
  }

  /* Package form */

  .package-form {
    display: grid;
    gap: 11px;
    padding: 13px 14px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .package-form-actions {
    display: flex;
    justify-content: flex-end;
    gap: 7px;
  }

  .compact-button {
    min-height: 30px;
    padding: 0 11px;
    font-size: 12px;
  }

  /* Versions */

  .version-row {
    display: flex;
    min-height: 52px;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    padding: 9px 14px;
  }

  .version-row > div {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .version-row strong {
    color: var(--link);
    font-size: 11.5px;
    font-weight: 550;
    overflow-wrap: anywhere;
  }

  .version-row:hover strong {
    color: var(--link-hover);
  }

  .version-row code {
    color: var(--text-muted);
    font-size: 10px;
  }

  .section-footer {
    display: flex;
    justify-content: flex-end;
    border-top: 1px solid var(--line-soft);
    padding: 9px 14px;
  }

  .section-footer a {
    font-size: 10.5px;
  }

  .section-empty {
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
    .project-summary {
      grid-template-columns: repeat(3, minmax(0, 1fr));
    }

    .project-summary > div:nth-child(3) {
      border-right: 0;
    }

    .project-summary > div:nth-child(-n + 3) {
      border-bottom: 1px solid var(--line-soft);
    }

    .project-content-grid {
      grid-template-columns: 1fr;
    }

    .secondary-column {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }
  }

  @media (max-width: 880px) {
    .projects-layout {
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
    .project-form {
      grid-template-columns: 1fr;
    }

    .repository-field,
    .slug-preview,
    .project-form-actions {
      grid-column: auto;
    }

    .project-overview__header {
      align-items: flex-start;
      flex-direction: column;
    }

    .project-header-actions {
      justify-content: flex-start;
    }

    .project-summary {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }

    .project-summary > div,
    .project-summary > div:nth-child(3) {
      border-right: 1px solid var(--line-soft);
      border-bottom: 1px solid var(--line-soft);
    }

    .project-summary > div:nth-child(even) {
      border-right: 0;
    }

    .project-summary > div:last-child {
      border-bottom: 0;
    }

    .secondary-column {
      grid-template-columns: 1fr;
    }

    .project-id-row {
      grid-template-columns: 1fr 30px;
    }

    .project-id-row > span {
      grid-column: 1 / -1;
    }
  }

  @media (max-width: 520px) {
    .project-form-section {
      padding: 14px;
    }

    .project-form-actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .project-form-actions button {
      width: 100%;
    }

    .project-summary {
      grid-template-columns: 1fr;
    }

    .project-summary > div,
    .project-summary > div:nth-child(even) {
      border-right: 0;
    }

    .project-links {
      align-items: flex-start;
      flex-direction: column;
      gap: 9px;
    }

    .build-row {
      align-items: flex-start;
      flex-direction: column;
    }

    .build-result {
      justify-content: flex-start;
    }

    .package-form-actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .package-form-actions button {
      width: 100%;
    }
  }
</style>
