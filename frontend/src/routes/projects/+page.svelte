<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';

  import { usePageRefresh } from '$lib/stores/pageRefresh';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  import { listBuildReports } from '$lib/api/buildReports';
  import { listLockfiles } from '$lib/api/lockfiles';

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
    type Project,
    type Workspace
  } from '$lib/api/types';

  import InlineError from '$lib/components/InlineError.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import WorkspaceAvatar from '$lib/components/WorkspaceAvatar.svelte';

  import { canCreateProject } from '$lib/permissions';

  type ProjectAction =
    | 'archive'
    | 'reactivate'
    | 'delete';

  let workspaces: Workspace[] = [];
  let selectedWorkspaceId = '';

  let projects: Project[] = [];
  let selectedProject: Project | null = null;

  let buildReports: BuildReport[] = [];
  let lockfiles: Lockfile[] = [];

  let projectQuery = '';

  let loading = true;
  let loadingDetail = false;
  let initialized = false;

  let projectRequestId = 0;
  let detailRequestId = 0;

  let savingProject = false;
  let busyProjectAction = false;

  let showProjectForm = false;

  let projectName = '';
  let projectSlug = '';
  let projectRepository = '';
  let slugTouched = false;

  let copiedProjectId = false;

  let copyResetTimer:
    | ReturnType<typeof setTimeout>
    | undefined;

  let pendingProjectAction:
    | ProjectAction
    | null = null;

  let deleteConfirmation = '';

  let error = '';
  let success = '';

  $: selectedWorkspace =
    workspaces.find(
      (workspace) =>
        workspace.id === selectedWorkspaceId
    ) ?? null;

  $: currentRole =
    selectedWorkspace?.current_user_role ??
    'viewer';

  $: canCreate =
    canCreateProject(currentRole);

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

  $: effectiveSlug = slugTouched
    ? slugify(projectSlug)
    : slugify(projectName);

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

  $: latestBuild =
    buildReports[0] ?? null;

  $: latestLockfile =
    lockfiles[0] ?? null;

  $: recentBuildReports =
    buildReports.slice(0, 5);

  $: successfulBuilds =
    buildReports.filter(
      (report) =>
        report.status === 'success'
    ).length;

  $: failedBuilds =
    buildReports.filter(
      (report) =>
        report.status === 'failed'
    ).length;

  $: buildReportsHref =
    selectedProject
      ? `/build-reports?workspace_id=${encodeURIComponent(
          selectedWorkspaceId
        )}&project_id=${encodeURIComponent(
          selectedProject.id
        )}`
      : '';

  $: lockfilesHref =
    selectedProject
      ? `/lockfiles?workspace_id=${encodeURIComponent(
          selectedWorkspaceId
        )}&project_id=${encodeURIComponent(
          selectedProject.id
        )}`
      : '';

  $: deleteCanContinue =
    pendingProjectAction !== 'delete' ||
    deleteConfirmation ===
      selectedProject?.slug;

  function slugify(value: string) {
    return value
      .trim()
      .toLowerCase()
      .replace(/[^a-z0-9]+/g, '-')
      .replace(/^-|-$/g, '');
  }

  function shortId(
    value: string,
    length = 13
  ) {
    return value.length > length
      ? `${value.slice(0, length)}…`
      : value;
  }

  function projectStatus(
    project: Project
  ) {
    return (
      project.status ||
      (
        project.active === false
          ? 'archived'
          : 'active'
      )
    );
  }

  function projectInitial(
    project: Project
  ) {
    return project.name
      .slice(0, 1)
      .toUpperCase();
  }

  function clearProjectDetail() {
    buildReports = [];
    lockfiles = [];
  }

  function resetProjectForm() {
    projectName = '';
    projectSlug = '';
    projectRepository = '';
    slugTouched = false;
  }

  function openProjectForm() {
    showProjectForm = true;
    error = '';
    success = '';
  }

  function closeProjectForm() {
    if (savingProject) {
      return;
    }

    showProjectForm = false;
    resetProjectForm();
    error = '';
  }

  function handleSlugInput(
    event: Event
  ) {
    const input =
      event.currentTarget as HTMLInputElement;

    slugTouched = true;
    projectSlug = input.value;
  }

  function updateProjectUrl(
    projectId?: string
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
      error =
        'Unable to copy the project ID.';
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
          : 'Unable to load projects.';
    } finally {
      loading = false;
    }
  }

  async function loadProjects(
    requestedProjectId?: string | null
  ) {
    const requestId =
      ++projectRequestId;

    const workspaceId =
      selectedWorkspaceId;

    if (!workspaceId) {
      projects = [];
      selectedProject = null;

      clearProjectDetail();
      updateProjectUrl();

      return;
    }

    const data =
      await listProjects(workspaceId);

    if (
      requestId !== projectRequestId ||
      selectedWorkspaceId !== workspaceId
    ) {
      return;
    }

    projects = data.projects;

    selectedProject =
      (
        requestedProjectId
          ? projects.find(
              (project) =>
                project.id ===
                requestedProjectId
            )
          : null
      ) ??
      projects[0] ??
      null;

    updateProjectUrl(
      selectedProject?.id
    );

    await loadProjectDetail(
      selectedProject
    );
  }

  async function switchWorkspace(
    workspaceId: string
  ) {
    if (
      workspaceId === selectedWorkspaceId
    ) {
      return;
    }

    selectedWorkspaceId = workspaceId;

    workspaceContext.setSelectedWorkspace(
      workspaceId
    );

    projects = [];
    selectedProject = null;

    clearProjectDetail();

    showProjectForm = false;
    resetProjectForm();

    projectQuery = '';
    copiedProjectId = false;

    pendingProjectAction = null;
    deleteConfirmation = '';

    error = '';
    success = '';

    loading = true;

    updateProjectUrl();

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

  async function selectProject(
    project: Project
  ) {
    if (
      project.id ===
        selectedProject?.id ||
      loadingDetail
    ) {
      return;
    }

    selectedProject = project;

    copiedProjectId = false;
    pendingProjectAction = null;
    deleteConfirmation = '';

    error = '';
    success = '';

    updateProjectUrl(project.id);

    await loadProjectDetail(project);
  }

  async function loadProjectDetail(
    project: Project | null
  ) {
    if (
      !selectedWorkspaceId ||
      !project
    ) {
      clearProjectDetail();
      return;
    }

    const requestId =
      ++detailRequestId;

    const workspaceId =
      selectedWorkspaceId;

    const projectId =
      project.id;

    loadingDetail = true;
    error = '';

    clearProjectDetail();

    try {
      const [
        buildReportData,
        lockfileData
      ] = await Promise.all([
        listBuildReports(
          workspaceId,
          projectId
        ),

        listLockfiles(
          workspaceId,
          projectId
        )
      ]);

      if (
        requestId !== detailRequestId ||
        selectedWorkspaceId !==
          workspaceId ||
        selectedProject?.id !== projectId
      ) {
        return;
      }

      buildReports =
        buildReportData.build_reports;

      lockfiles =
        lockfileData.lockfiles;
    } catch (err) {
      if (
        requestId !== detailRequestId
      ) {
        return;
      }

      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load project details.';
    } finally {
      if (
        requestId === detailRequestId
      ) {
        loadingDetail = false;
      }
    }
  }

  async function submitProject() {
    const user = $auth.user;

    const name =
      projectName.trim();

    const slug =
      effectiveSlug.trim();

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
      const created =
        await createProject({
          workspaceId:
            selectedWorkspaceId,

          ownerUserId:
            user.id,

          name,
          slug,

          repositoryUrl:
            projectRepository.trim(),

          defaultBranch:
            'main'
        });

      projects = [
        created.project,
        ...projects
      ];

      selectedProject =
        created.project;

      showProjectForm = false;
      resetProjectForm();

      updateProjectUrl(
        created.project.id
      );

      await loadProjectDetail(
        created.project
      );

      success =
        `${created.project.name} is ready. ` +
        'You can now send build reports and lockfiles.';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to create project.';
    } finally {
      savingProject = false;
    }
  }

  function requestProjectAction(
    action: ProjectAction
  ) {
    pendingProjectAction = action;
    deleteConfirmation = '';

    error = '';
    success = '';
  }

  function cancelProjectAction() {
    pendingProjectAction = null;
    deleteConfirmation = '';
  }

  async function confirmProjectAction() {
    if (
      !selectedProject ||
      !selectedWorkspaceId ||
      !canCreate ||
      !pendingProjectAction ||
      !deleteCanContinue
    ) {
      return;
    }

    const action =
      pendingProjectAction;

    const projectId =
      selectedProject.id;

    busyProjectAction = true;
    error = '';
    success = '';

    try {
      if (action === 'delete') {
        await deleteProject(
          selectedWorkspaceId,
          projectId
        );

        projects =
          projects.filter(
            (project) =>
              project.id !== projectId
          );

        selectedProject =
          projects[0] ?? null;

        updateProjectUrl(
          selectedProject?.id
        );

        await loadProjectDetail(
          selectedProject
        );

        success =
          'Project deleted.';
      } else if (
        action === 'archive'
      ) {
        const updated =
          await archiveProject(
            selectedWorkspaceId,
            projectId
          );

        selectedProject =
          updated.project;

        projects =
          projects.map(
            (project) =>
              project.id ===
              updated.project.id
                ? updated.project
                : project
          );

        success =
          'Project archived.';
      } else {
        const updated =
          await reactivateProject(
            selectedWorkspaceId,
            projectId
          );

        selectedProject =
          updated.project;

        projects =
          projects.map(
            (project) =>
              project.id ===
              updated.project.id
                ? updated.project
                : project
          );

        success =
          'Project reactivated.';
      }

      cancelProjectAction();
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to update project.';
    } finally {
      busyProjectAction = false;
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
    Projects | Softadastra Cloud
  </title>

  <meta
    name="description"
    content="Connect Vix projects to Softadastra Cloud and review their build reports, lockfiles, repository information and project status."
  />
</svelte:head>

<div class="projects-page">
  <!-- ===================================================
       HEADER
       =================================================== -->
  <header class="page-header">
    <div class="page-heading">
      <p class="page-eyebrow">
        <span aria-hidden="true"></span>
        Development
      </p>

      <h1>Projects</h1>

      <p>
        {#if loading}
          Loading projects…
        {:else if selectedWorkspace}
          Projects connected to
          <strong>
            {selectedWorkspace.name}
          </strong>.
        {:else}
          Create a workspace before connecting
          a project.
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
        selectedWorkspace &&
        canCreate &&
        projects.length > 0 &&
        !loading
      }
        <button
          class:button-secondary={
            showProjectForm
          }
          class="header-action"
          type="button"
          onclick={() => {
            if (showProjectForm) {
              closeProjectForm();
            } else {
              openProjectForm();
            }
          }}
        >
          {#if !showProjectForm}
            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path d="M12 5v14"></path>
              <path d="M5 12h14"></path>
            </svg>
          {/if}

          {showProjectForm
            ? 'Close'
            : 'New project'}
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
       MANUAL PROJECT FORM
       Secondary to vix cloud init
       =================================================== -->
  {#if
    !loading &&
    selectedWorkspace &&
    showProjectForm &&
    canCreate
  }
    <section
      class="project-form-panel"
      aria-labelledby="project-form-title"
    >
      <div class="project-form-panel__intro">
        <span
          class="project-form-panel__icon"
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
            Manual setup
          </p>

          <h2 id="project-form-title">
            Create a cloud project
          </h2>

          <p>
            Use this form when the project cannot be
            initialized directly from the Vix CLI.
          </p>
        </div>
      </div>

      <form
        class="project-form"
        onsubmit={(event) => {
          event.preventDefault();
          void submitProject();
        }}
      >
        <label>
          <span>Project name</span>

          <input
            bind:value={projectName}
            placeholder="Native service"
            required
            autocomplete="off"
          />
        </label>

        <label>
          <span>Project slug</span>

          <input
            value={effectiveSlug}
            placeholder="native-service"
            required
            autocomplete="off"
            oninput={handleSlugInput}
          />
        </label>

        <label class="repository-field">
          <span>Repository URL</span>

          <input
            bind:value={projectRepository}
            type="url"
            placeholder="https://github.com/example/native-service"
            autocomplete="off"
          />
        </label>

        <div class="slug-preview">
          <span>Project identifier</span>

          <code>
            {effectiveSlug ||
              'project-slug'}
          </code>
        </div>

        <div class="project-form__actions">
          <button
            class="button-secondary"
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
              !projectName.trim() ||
              !effectiveSlug
            }
          >
            {savingProject
              ? 'Creating project…'
              : 'Create project'}
          </button>
        </div>
      </form>
    </section>
  {/if}

  <!-- ===================================================
       LOADING
       =================================================== -->
  {#if loading}
    <section class="page-loading">
      <span class="page-loading__mark">
        P
      </span>

      <div>
        <strong>
          Loading projects
        </strong>

        <p>
          Preparing project access, builds and
          dependency history.
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
    <section class="workspace-required">
      <span
        class="workspace-required__icon"
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
          Projects need a workspace first.
        </h2>

        <p>
          The workspace owns the projects, packages,
          members, access rules and CLI credentials.
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
       EMPTY WORKSPACE
       =================================================== -->
  {:else if
    projects.length === 0 &&
    !showProjectForm
  }
    <section class="project-onboarding">
      <div class="project-onboarding__copy">
        <p class="panel-kicker">
          First project
        </p>

        <h2>
          Connect the project
          <span>from your local Vix workflow.</span>
        </h2>

        <p>
          Run the initialization command inside the
          local C++ project. Vix will connect it to
          <strong>{selectedWorkspace.name}</strong>
          without moving the source code or build
          process into Cloud.
        </p>

        <div class="project-onboarding__actions">
          {#if canCreate}
            <button
              class="manual-action"
              type="button"
              onclick={openProjectForm}
            >
              Create manually
            </button>
          {:else}
            <ReadOnlyNotice
              message="Your role can view projects, but a workspace owner or administrator must connect the first project."
            />
          {/if}
        </div>
      </div>

      <div class="cli-setup">
        <header class="cli-setup__header">
          <div>
            <span aria-hidden="true"></span>
            <span aria-hidden="true"></span>
            <span aria-hidden="true"></span>
          </div>

          <strong>
            project terminal
          </strong>

          <small>
            local
          </small>
        </header>

        <div class="cli-setup__body">
          <div class="cli-command">
            <span>$</span>
            <code>cd native-service</code>
          </div>

          <div class="cli-output">
            <span>→</span>
            <code>Local project selected</code>
          </div>

          <div class="cli-command cli-command--primary">
            <span>$</span>
            <code>vix cloud init</code>
          </div>

          <div class="cli-output cli-output--success">
            <span>✓</span>
            <code>
              Project connected to
              {selectedWorkspace.slug}
            </code>
          </div>
        </div>

        <footer class="cli-setup__footer">
          <span>
            Source remains local
          </span>

          <span>
            Workspace context shared
          </span>
        </footer>
      </div>

      <div class="project-onboarding__steps">
        <article class="active">
          <span>01</span>

          <div>
            <strong>
              Connect the project
            </strong>

            <p>
              Initialize Cloud from the local project.
            </p>
          </div>
        </article>

        <article>
          <span>02</span>

          <div>
            <strong>
              Send a build report
            </strong>

            <p>
              Run
              <code>vix build --report</code>.
            </p>
          </div>
        </article>

        <article>
          <span>03</span>

          <div>
            <strong>
              Upload the lockfile
            </strong>

            <p>
              Preserve the dependency state used by
              the project.
            </p>
          </div>
        </article>
      </div>
    </section>

  <!-- ===================================================
       PROJECT CONTROL
       =================================================== -->
  {:else if selectedProject}
    {@const currentProject = selectedProject}
    <div class="projects-layout">
      <!-- Directory -->
      <aside
        class="project-directory"
        aria-label="Project directory"
      >
        <header class="directory-header">
          <div>
            <p class="panel-kicker">
              Directory
            </p>

            <h2>
              Workspace projects
            </h2>
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
                  selectedProject?.id
                }
                class="project-option"
                type="button"
                aria-pressed={
                  project.id ===
                  selectedProject?.id
                }
                onclick={() =>
                  selectProject(project)}
              >
                <span class="project-option__mark">
                  {projectInitial(project)}
                </span>

                <span class="project-option__copy">
                  <strong>
                    {project.name}
                  </strong>

                  <code>
                    {project.slug}
                  </code>
                </span>

                {#if
                  projectStatus(project) ===
                  'archived'
                }
                  <span class="project-option__state">
                    Archived
                  </span>
                {/if}
              </button>
            {/each}
          </div>
        {/if}
      </aside>

      <!-- Selected project -->
      <main class="project-control">
        <section class="project-card">
          <header class="project-card__header">
            <div class="project-identity">
              <span class="project-identity__mark">
                {projectInitial(
                  selectedProject
                )}
              </span>

              <div>
                <p class="panel-kicker">
                  Current project
                </p>

                <div class="project-title">
                  <h2>
                    {selectedProject.name}
                  </h2>

                  <StatusBadge
                    status={projectStatus(
                        currentProject
                      )}
                  />
                </div>

                <code>
                  {selectedProject.slug}
                </code>
              </div>
            </div>

            <a
              class="primary-link primary-link--compact"
              href={buildReportsHref}
            >
              Open build reports

              <svg
                viewBox="0 0 24 24"
                aria-hidden="true"
              >
                <path d="m9 18 6-6-6-6"></path>
              </svg>
            </a>
          </header>

          {#if loadingDetail}
            <span
              class="detail-progress"
              aria-hidden="true"
            ></span>
          {/if}

          <div class="project-metadata">
            <div>
              <span>Default branch</span>

              <strong>
                {selectedProject.default_branch ||
                  'main'}
              </strong>
            </div>

            <div class="repository-meta">
              <span>Repository</span>

              {#if selectedProject.repository_url}
                <a
                  href={selectedProject.repository_url}
                  target="_blank"
                  rel="noopener noreferrer"
                >
                  {selectedProject.repository_url}

                  <svg
                    viewBox="0 0 24 24"
                    aria-hidden="true"
                  >
                    <path d="M14 5h5v5"></path>
                    <path d="M10 14 19 5"></path>
                    <path d="M19 14v5H5V5h5"></path>
                  </svg>
                </a>
              {:else}
                <strong>
                  Not connected
                </strong>
              {/if}
            </div>

            <div class="project-id-meta">
              <span>Project ID</span>

              <code>
                {shortId(
                  selectedProject.id,
                  18
                )}
              </code>

              <button
                class:copied={copiedProjectId}
                type="button"
                aria-label={
                  copiedProjectId
                    ? 'Project ID copied'
                    : 'Copy project ID'
                }
                onclick={copyProjectId}
              >
                {#if copiedProjectId}
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
          </div>

          <section
            class="project-metrics"
            aria-label="Project summary"
          >
            <article>
              <span>Latest build</span>

              {#if latestBuild}
                <StatusBadge
                  status={latestBuild.status}
                />
              {:else}
                <strong>
                  No report
                </strong>
              {/if}

              <small>
                {latestBuild
                  ? latestBuild.target
                  : 'Run vix build --report'}
              </small>
            </article>

            <article>
              <span>Build reports</span>

              <strong>
                {buildReports.length}
              </strong>

              <small>
                {successfulBuilds} successful
                ·
                {failedBuilds} failed
              </small>
            </article>

            <article>
              <span>Lockfiles</span>

              <strong>
                {lockfiles.length}
              </strong>

              <small>
                {latestLockfile
                  ? 'Dependency history available'
                  : 'No dependency state uploaded'}
              </small>
            </article>
          </section>

          {#if !canCreate}
            <div class="read-only-wrapper">
              <ReadOnlyNotice
                message="Your role can inspect this project, but cannot create, archive or delete projects."
              />
            </div>
          {/if}

          <div class="project-resources">
            <!-- Build reports -->
            <section
              class="resource-panel build-panel"
              aria-labelledby="build-panel-title"
            >
              <header class="resource-panel__header">
                <div>
                  <p class="panel-kicker">
                    Compilation
                  </p>

                  <h3 id="build-panel-title">
                    Recent build reports
                  </h3>

                  <p>
                    Results sent from local development
                    or existing CI.
                  </p>
                </div>

                {#if buildReports.length > 0}
                  <a href={buildReportsHref}>
                    View all
                  </a>
                {/if}
              </header>

              {#if loadingDetail}
                <p class="resource-loading">
                  Loading build reports…
                </p>
              {:else if recentBuildReports.length === 0}
                <div class="resource-empty">
                  <span
                    class="resource-empty__icon"
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
                      <path d="M4 19V5"></path>
                      <path d="M4 19h16"></path>
                      <path d="m7 15 3-4 3 2 4-6"></path>
                    </svg>
                  </span>

                  <div>
                    <strong>
                      Send the first build report
                    </strong>

                    <p>
                      Build locally and record the
                      result in this project.
                    </p>
                  </div>

                  <code>
                    <span>$</span>
                    vix build --report
                  </code>
                </div>
              {:else}
                <div class="build-list">
                  {#each recentBuildReports as report (report.id)}
                    <a
                      class="build-row"
                      href={buildReportsHref}
                    >
                      <span
                        class="build-row__indicator"
                        class:is-success={
                          report.status ===
                          'success'
                        }
                        class:is-failed={
                          report.status ===
                          'failed'
                        }
                        aria-hidden="true"
                      ></span>

                      <div>
                        <strong>
                          {report.target}
                        </strong>

                        <span>
                          {report.profile ||
                            'default'}

                          <i aria-hidden="true"></i>

                          {report.errors_count}
                          {report.errors_count === 1
                            ? ' error'
                            : ' errors'}
                        </span>
                      </div>

                      <StatusBadge
                        status={report.status}
                      />

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

            <!-- Lockfile -->
            <section
              class="resource-panel lockfile-panel"
              aria-labelledby="lockfile-panel-title"
            >
              <header class="resource-panel__header">
                <div>
                  <p class="panel-kicker">
                    Dependencies
                  </p>

                  <h3 id="lockfile-panel-title">
                    Lockfile state
                  </h3>

                  <p>
                    The dependency state recorded for
                    this project.
                  </p>
                </div>

                {#if lockfiles.length > 0}
                  <a href={lockfilesHref}>
                    History
                  </a>
                {/if}
              </header>

              {#if loadingDetail}
                <p class="resource-loading">
                  Loading lockfiles…
                </p>
              {:else if latestLockfile}
                <div class="lockfile-state">
                  <span
                    class="lockfile-state__icon"
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
                    <span>
                      Latest checksum
                    </span>

                    <code>
                      {shortId(
                        latestLockfile
                          .checksum_sha256,
                        28
                      )}
                    </code>
                  </div>

                  <span class="lockfile-state__ready">
                    <i aria-hidden="true"></i>
                    Recorded
                  </span>
                </div>

                <dl class="lockfile-details">
                  <div>
                    <dt>Source</dt>

                    <dd>
                      {latestLockfile.source ||
                        'Unknown source'}
                    </dd>
                  </div>

                  <div>
                    <dt>History</dt>

                    <dd>
                      {lockfiles.length}
                      {lockfiles.length === 1
                        ? ' lockfile'
                        : ' lockfiles'}
                    </dd>
                  </div>
                </dl>

                <a
                  class="resource-action"
                  href={lockfilesHref}
                >
                  Open lockfile history

                  <svg
                    viewBox="0 0 24 24"
                    aria-hidden="true"
                  >
                    <path d="M5 12h14"></path>
                    <path d="m13 6 6 6-6 6"></path>
                  </svg>
                </a>
              {:else}
                <div class="resource-empty resource-empty--compact">
                  <span
                    class="resource-empty__icon"
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
                    </svg>
                  </span>

                  <div>
                    <strong>
                      Upload the first lockfile
                    </strong>

                    <p>
                      Preserve the dependency state
                      used by this project.
                    </p>
                  </div>

                  <code>
                    <span>$</span>
                    vix cloud lockfile upload
                  </code>
                </div>
              {/if}
            </section>
          </div>

          <!-- Administration -->
          {#if canCreate}
            <details class="project-administration">
              <summary>
                <span>
                  <strong>
                    Project administration
                  </strong>

                  <small>
                    Archive, reactivate or delete this
                    project.
                  </small>
                </span>

                <svg
                  viewBox="0 0 24 24"
                  aria-hidden="true"
                >
                  <path d="m6 9 6 6 6-6"></path>
                </svg>
              </summary>

              <div class="project-administration__content">
                <article>
                  <div>
                    <strong>
                      {projectStatus(
                        currentProject
                      ) === 'archived'
                        ? 'Reactivate project'
                        : 'Archive project'}
                    </strong>

                    <p>
                      {projectStatus(
                        currentProject
                      ) === 'archived'
                        ? 'Return the project to active use.'
                        : 'Hide the project from normal active workflows without removing its history.'}
                    </p>
                  </div>

                  <button
                    class="button-secondary"
                    type="button"
                    disabled={busyProjectAction}
                    onclick={() =>
                      requestProjectAction(
                        projectStatus(
                        currentProject
                      ) === 'archived'
                          ? 'reactivate'
                          : 'archive'
                      )}
                  >
                    {projectStatus(
                        currentProject
                      ) === 'archived'
                      ? 'Reactivate'
                      : 'Archive'}
                  </button>
                </article>

                <article class="delete-project-action">
                  <div>
                    <strong>
                      Delete project
                    </strong>

                    <p>
                      Remove the project from normal
                      use while preserving build reports
                      and lockfiles for safety.
                    </p>
                  </div>

                  <button
                    class="danger-button"
                    type="button"
                    disabled={busyProjectAction}
                    onclick={() =>
                      requestProjectAction(
                        'delete'
                      )}
                  >
                    Delete
                  </button>
                </article>

                {#if pendingProjectAction}
                  <div
                    class="project-confirmation"
                    role="alert"
                  >
                    <div>
                      <strong>
                        Confirm
                        {pendingProjectAction}
                      </strong>

                      <p>
                        {#if pendingProjectAction === 'delete'}
                          Enter
                          <code>
                            {selectedProject.slug}
                          </code>
                          to confirm removal.
                        {:else if pendingProjectAction === 'archive'}
                          The project will leave active
                          workflows but its records will
                          remain available.
                        {:else}
                          The project will become active
                          again.
                        {/if}
                      </p>
                    </div>

                    {#if pendingProjectAction === 'delete'}
                      <input
                        bind:value={deleteConfirmation}
                        placeholder={selectedProject.slug}
                        autocomplete="off"
                      />
                    {/if}

                    <div class="project-confirmation__actions">
                      <button
                        class="button-secondary"
                        type="button"
                        disabled={busyProjectAction}
                        onclick={cancelProjectAction}
                      >
                        Cancel
                      </button>

                      <button
                        class:danger-button={
                          pendingProjectAction ===
                          'delete'
                        }
                        type="button"
                        disabled={
                          busyProjectAction ||
                          !deleteCanContinue
                        }
                        onclick={() =>
                          void confirmProjectAction()}
                      >
                        {busyProjectAction
                          ? 'Working…'
                          : `Confirm ${pendingProjectAction}`}
                      </button>
                    </div>
                  </div>
                {/if}
              </div>
            </details>
          {/if}
        </section>
      </main>
    </div>
  {/if}
</div>

<style>
  /* =====================================================
     PAGE
     ===================================================== */

  .projects-page {
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
    gap: 25px;
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
     BUTTONS
     ===================================================== */

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
  .manual-action {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-soft);
  }

  .button-secondary:hover:not(:disabled),
  .manual-action:hover:not(:disabled) {
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
    stroke-linejoin: round;
  }

  .primary-link--compact {
    min-height: 34px;
    padding-inline: 11px;
    font-size: 10px;
  }

  .panel-kicker {
    color: var(--brand-soft);
    font-family: var(--font-mono);
    font-size: 7px;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }

  /* =====================================================
     MESSAGES
     ===================================================== */

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
     CREATION FORM
     ===================================================== */

  .project-form-panel {
    display: grid;
    grid-template-columns:
      minmax(230px, 0.72fr)
      minmax(0, 1.28fr);
    gap: 28px;
    padding: 23px;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    box-shadow: inset 3px 0 0 var(--brand);
  }

  .project-form-panel__intro {
    display: flex;
    align-items: flex-start;
    gap: 12px;
  }

  .project-form-panel__icon {
    display: grid;
    width: 43px;
    height: 43px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 10px;
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .project-form-panel__icon svg {
    width: 20px;
    height: 20px;
  }

  .project-form-panel__intro > div {
    display: grid;
    gap: 5px;
  }

  .project-form-panel h2 {
    color: var(--text);
    font-size: 18px;
    font-weight: 620;
    letter-spacing: -0.03em;
  }

  .project-form-panel__intro p:not(.panel-kicker) {
    max-width: 330px;
    color: var(--text-muted);
    font-size: 10.5px;
    line-height: 1.55;
  }

  .project-form {
    display: grid;
    grid-template-columns:
      repeat(2, minmax(0, 1fr));
    gap: 13px;
  }

  .project-form label,
  .project-confirmation {
    min-width: 0;
  }

  .project-form label {
    display: grid;
    gap: 7px;
    color: var(--text-soft);
    font-size: 10.5px;
    font-weight: 550;
  }

  .project-form input,
  .project-confirmation input {
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

  .project-form input:focus,
  .project-confirmation input:focus {
    border-color: var(--brand);
    box-shadow: 0 0 0 3px var(--brand-faint);
  }

  .repository-field,
  .slug-preview,
  .project-form__actions {
    grid-column: 1 / -1;
  }

  .slug-preview {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 9px;
    padding: 9px 10px;
    border: 1px solid var(--line-soft);
    border-radius: 6px;
    background: var(--bg-ink-soft);
  }

  .slug-preview span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 7px;
    text-transform: uppercase;
  }

  .slug-preview code {
    color: var(--brand-bright);
    font-size: 10.5px;
  }

  .project-form__actions {
    display: flex;
    justify-content: flex-end;
    gap: 8px;
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
     WORKSPACE REQUIRED
     ===================================================== */

  .workspace-required {
    display: grid;
    max-width: 760px;
    min-height: 310px;
    align-content: center;
    justify-items: start;
    gap: 15px;
    padding: clamp(28px, 6vw, 56px);
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-lg);
    background: var(--bg-panel);
    box-shadow: var(--shadow-panel);
  }

  .workspace-required__icon {
    display: grid;
    width: 49px;
    height: 49px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 11px;
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .workspace-required__icon svg {
    width: 22px;
    height: 22px;
  }

  .workspace-required > div {
    display: grid;
    gap: 7px;
  }

  .workspace-required h2 {
    color: var(--text);
    font-size: clamp(27px, 4vw, 42px);
    font-weight: 630;
    letter-spacing: -0.045em;
  }

  .workspace-required div > p:not(.panel-kicker) {
    max-width: 570px;
    color: var(--text-muted);
    font-size: 12px;
    line-height: 1.65;
  }

  /* =====================================================
     FIRST PROJECT ONBOARDING
     ===================================================== */

  .project-onboarding {
    display: grid;
    grid-template-columns:
      minmax(0, 0.82fr)
      minmax(430px, 1.18fr);
    gap: clamp(38px, 7vw, 80px);
    align-items: center;
    padding: clamp(30px, 5vw, 54px);
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-lg);
    background: var(--bg-panel);
    box-shadow: var(--shadow-panel);
  }

  .project-onboarding__copy {
    display: grid;
    align-content: center;
  }

  .project-onboarding__copy h2 {
    max-width: 560px;
    margin-top: 10px;
    color: var(--text);
    font-size: clamp(32px, 4.8vw, 53px);
    font-weight: 640;
    line-height: 1;
    letter-spacing: -0.052em;
  }

  .project-onboarding__copy h2 span {
    display: block;
    color: var(--brand-soft);
  }

  .project-onboarding__copy > p:not(.panel-kicker) {
    max-width: 550px;
    margin-top: 17px;
    color: var(--text-muted);
    font-size: 12.5px;
    line-height: 1.7;
  }

  .project-onboarding__copy strong {
    color: var(--text-soft);
    font-weight: 600;
  }

  .project-onboarding__actions {
    margin-top: 24px;
  }

  .cli-setup {
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: 11px;
    background: var(--bg-ink);
    box-shadow: var(--shadow-panel);
  }

  .cli-setup__header {
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

  .cli-setup__header > div {
    display: flex;
    gap: 5px;
  }

  .cli-setup__header > div span {
    width: 7px;
    height: 7px;
    border-radius: 50%;
    background: var(--text-faint);
  }

  .cli-setup__header > div span:first-child {
    background: var(--brand);
  }

  .cli-setup__header strong,
  .cli-setup__header small {
    font-family: var(--font-mono);
    font-size: 7px;
  }

  .cli-setup__header strong {
    overflow: hidden;
    color: var(--text-muted);
    text-align: center;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .cli-setup__header small {
    color: var(--text-faint);
  }

  .cli-setup__body {
    display: grid;
    align-content: center;
    min-height: 235px;
    padding: 24px 20px;
  }

  .cli-command,
  .cli-output {
    display: grid;
    min-height: 35px;
    grid-template-columns: 23px minmax(0, 1fr);
    align-items: center;
    font-family: var(--font-mono);
    font-size: 9px;
  }

  .cli-command span {
    color: var(--brand);
    font-weight: 800;
  }

  .cli-command code {
    color: var(--text);
  }

  .cli-output span,
  .cli-output code {
    color: var(--text-muted);
  }

  .cli-command--primary {
    margin-top: 17px;
  }

  .cli-output--success span,
  .cli-output--success code {
    color: var(--green-soft);
  }

  .cli-setup__footer {
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

  .project-onboarding__steps {
    grid-column: 1 / -1;
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    border-top: 1px solid var(--line);
    padding-top: 25px;
  }

  .project-onboarding__steps article {
    display: grid;
    grid-template-columns:
      auto minmax(0, 1fr);
    gap: 10px;
    padding: 0 18px;
    border-right: 1px solid var(--line);
  }

  .project-onboarding__steps article:first-child {
    padding-left: 0;
  }

  .project-onboarding__steps article:last-child {
    border-right: 0;
  }

  .project-onboarding__steps article > span {
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 8px;
    font-weight: 700;
  }

  .project-onboarding__steps article.active > span {
    color: var(--brand-soft);
  }

  .project-onboarding__steps article > div {
    display: grid;
    gap: 4px;
  }

  .project-onboarding__steps strong {
    color: var(--text-soft);
    font-size: 10px;
  }

  .project-onboarding__steps p {
    color: var(--text-muted);
    font-size: 8.5px;
    line-height: 1.45;
  }

  .project-onboarding__steps code {
    color: var(--brand-bright);
    font-size: 8px;
  }

  /* =====================================================
     PROJECT LAYOUT
     ===================================================== */

  .projects-layout {
    display: grid;
    grid-template-columns:
      250px minmax(0, 1fr);
    gap: 16px;
    align-items: start;
  }

  /* =====================================================
     PROJECT DIRECTORY
     ===================================================== */

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

  .directory-count {
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
    width: 100%;
    min-height: 34px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    outline: 0;
    background: var(--bg-ink-soft);
    color: var(--text);
    padding: 0 10px 0 33px;
    font: inherit;
    font-size: 11px;
  }

  .project-search input:focus {
    border-color: var(--brand);
  }

  .project-options {
    display: grid;
    max-height: calc(100vh - 220px);
    overflow-y: auto;
  }

  .project-option {
    display: grid;
    width: 100%;
    min-height: 59px;
    grid-template-columns:
      31px minmax(0, 1fr) auto;
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

  .project-option__copy {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .project-option__copy strong {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 11.5px;
    font-weight: 600;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .project-option.selected
    .project-option__copy strong {
    color: var(--brand-bright);
  }

  .project-option__copy code {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 9px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .project-option__state {
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 6.5px;
    text-transform: uppercase;
  }

  .directory-message {
    padding: 18px 14px;
    color: var(--text-muted);
    font-size: 11px;
  }

  /* =====================================================
     PROJECT CARD
     ===================================================== */

  .project-control {
    min-width: 0;
  }

  .project-card {
    position: relative;
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    box-shadow: var(--shadow-soft);
  }

  .project-card__header {
    display: flex;
    min-height: 93px;
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

  .project-identity__mark {
    display: grid;
    width: 45px;
    height: 45px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 10px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 15px;
    font-weight: 750;
  }

  .project-identity > div {
    display: grid;
    min-width: 0;
    gap: 5px;
  }

  .project-title {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 9px;
  }

  .project-title h2 {
    color: var(--text);
    font-size: 20px;
    font-weight: 620;
    letter-spacing: -0.035em;
    overflow-wrap: anywhere;
  }

  .project-identity > div > code {
    color: var(--text-muted);
    font-size: 9px;
  }

  .detail-progress {
    position: absolute;
    z-index: 3;
    top: 92px;
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

  /* =====================================================
     METADATA
     ===================================================== */

  .project-metadata {
    display: grid;
    grid-template-columns:
      minmax(120px, 0.55fr)
      minmax(230px, 1.35fr)
      minmax(190px, 0.9fr);
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .project-metadata > div {
    display: grid;
    min-width: 0;
    align-content: center;
    gap: 4px;
    min-height: 64px;
    padding: 11px 15px;
    border-right: 1px solid var(--line-soft);
  }

  .project-metadata > div:last-child {
    border-right: 0;
  }

  .project-metadata span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6px;
    text-transform: uppercase;
  }

  .project-metadata strong {
    color: var(--text-soft);
    font-size: 9.5px;
    font-weight: 550;
  }

  .repository-meta a {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 5px;
    color: var(--brand-soft);
    font-size: 9px;
    text-decoration: none;
  }

  .repository-meta a:hover {
    color: var(--brand-bright);
  }

  .repository-meta a {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .repository-meta svg {
    width: 11px;
    height: 11px;
    flex: 0 0 auto;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .project-id-meta {
    grid-template-columns:
      minmax(0, 1fr) 28px;
  }

  .project-id-meta > span {
    grid-column: 1 / -1;
  }

  .project-id-meta code {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 8.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .project-id-meta button {
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

  .project-id-meta button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
    transform: none;
  }

  .project-id-meta button.copied {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .project-id-meta button svg {
    width: 13px;
    height: 13px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  /* =====================================================
     METRICS
     ===================================================== */

  .project-metrics {
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    border-bottom: 1px solid var(--line-soft);
  }

  .project-metrics article {
    display: grid;
    min-width: 0;
    gap: 5px;
    padding: 16px 19px;
    border-right: 1px solid var(--line-soft);
  }

  .project-metrics article:last-child {
    border-right: 0;
  }

  .project-metrics article > span:first-child {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6.5px;
    letter-spacing: 0.06em;
    text-transform: uppercase;
  }

  .project-metrics strong {
    color: var(--text);
    font-size: 18px;
    font-weight: 620;
  }

  .project-metrics small {
    overflow: hidden;
    color: var(--text-faint);
    font-size: 8.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .read-only-wrapper {
    padding: 12px 16px 0;
  }

  /* =====================================================
     PROJECT RESOURCES
     ===================================================== */

  .project-resources {
    display: grid;
    grid-template-columns:
      minmax(0, 1.25fr)
      minmax(290px, 0.75fr);
    gap: 14px;
    padding: 16px;
  }

  .resource-panel {
    overflow: hidden;
    border: 1px solid var(--line);
    border-radius: 9px;
    background: var(--bg-ink-soft);
  }

  .resource-panel__header {
    display: flex;
    min-height: 76px;
    align-items: flex-start;
    justify-content: space-between;
    gap: 16px;
    padding: 13px 15px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .resource-panel__header > div {
    display: grid;
    gap: 3px;
  }

  .resource-panel h3 {
    color: var(--text);
    font-size: 12.5px;
    font-weight: 600;
  }

  .resource-panel__header p:not(.panel-kicker) {
    color: var(--text-muted);
    font-size: 8.5px;
    line-height: 1.45;
  }

  .resource-panel__header > a {
    flex: 0 0 auto;
    color: var(--brand-soft);
    font-size: 9px;
    font-weight: 600;
    text-decoration: none;
  }

  .resource-panel__header > a:hover {
    color: var(--brand-bright);
  }

  .resource-loading {
    min-height: 120px;
    padding: 20px 15px;
    color: var(--text-muted);
    font-size: 10px;
  }

  /* Build list */

  .build-list {
    display: grid;
  }

  .build-row {
    display: grid;
    min-height: 60px;
    grid-template-columns:
      7px minmax(0, 1fr) auto auto;
    align-items: center;
    gap: 10px;
    padding: 10px 14px;
    border-bottom: 1px solid var(--line-soft);
    color: inherit;
    text-decoration: none;
    transition: background var(--speed) var(--ease);
  }

  .build-row:last-child {
    border-bottom: 0;
  }

  .build-row:hover {
    background: var(--bg-elevated);
  }

  .build-row__indicator {
    width: 6px;
    height: 6px;
    border: 1px solid var(--line-strong);
    border-radius: 50%;
  }

  .build-row__indicator.is-success {
    border-color: var(--green-soft);
    background: var(--green-soft);
    box-shadow: 0 0 0 3px var(--green-faint);
  }

  .build-row__indicator.is-failed {
    border-color: var(--danger);
    background: var(--danger);
    box-shadow: 0 0 0 3px var(--danger-faint);
  }

  .build-row > div {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .build-row strong {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 10.5px;
    font-weight: 600;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .build-row div > span {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 6px;
    color: var(--text-muted);
    font-size: 8.5px;
  }

  .build-row div > span i {
    width: 3px;
    height: 3px;
    border-radius: 50%;
    background: var(--text-faint);
  }

  .build-row > svg {
    width: 13px;
    height: 13px;
    fill: none;
    stroke: var(--text-faint);
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .build-row:hover > svg {
    stroke: var(--brand);
  }

  /* Empty resources */

  .resource-empty {
    display: grid;
    grid-template-columns:
      auto minmax(0, 1fr);
    gap: 11px;
    align-content: center;
    min-height: 210px;
    padding: 22px;
  }

  .resource-empty--compact {
    min-height: 260px;
  }

  .resource-empty__icon {
    display: grid;
    width: 39px;
    height: 39px;
    grid-row: 1 / 3;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 9px;
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .resource-empty__icon svg {
    width: 18px;
    height: 18px;
  }

  .resource-empty > div {
    display: grid;
    gap: 4px;
  }

  .resource-empty strong {
    color: var(--text-soft);
    font-size: 10.5px;
  }

  .resource-empty p {
    color: var(--text-muted);
    font-size: 8.5px;
    line-height: 1.5;
  }

  .resource-empty > code {
    grid-column: 2;
    display: flex;
    min-height: 38px;
    align-items: center;
    gap: 8px;
    margin-top: 5px;
    padding: 0 10px;
    border: 1px solid var(--line-ink);
    border-radius: 6px;
    background: var(--bg-ink);
    color: var(--text-soft);
    font-size: 8.5px;
  }

  .resource-empty > code span {
    color: var(--brand);
    font-weight: 800;
  }

  /* Lockfile state */

  .lockfile-state {
    display: grid;
    grid-template-columns:
      auto minmax(0, 1fr) auto;
    align-items: center;
    gap: 9px;
    margin: 14px;
    padding: 11px;
    border: 1px solid var(--green-line);
    border-radius: 8px;
    background: var(--green-faint);
  }

  .lockfile-state__icon {
    display: grid;
    width: 34px;
    height: 34px;
    place-items: center;
    border-radius: 7px;
    background: var(--bg-panel-strong);
    color: var(--green-soft);
  }

  .lockfile-state__icon svg {
    width: 16px;
    height: 16px;
  }

  .lockfile-state > div {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .lockfile-state div > span {
    color: var(--text-muted);
    font-size: 7px;
  }

  .lockfile-state code {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 8px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .lockfile-state__ready {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    color: var(--green-soft);
    font-size: 7px;
    font-weight: 600;
  }

  .lockfile-state__ready i {
    width: 5px;
    height: 5px;
    border-radius: 50%;
    background: currentColor;
  }

  .lockfile-details {
    display: grid;
    grid-template-columns:
      repeat(2, minmax(0, 1fr));
    margin: 0 14px;
    border: 1px solid var(--line-soft);
    border-radius: 7px;
  }

  .lockfile-details > div {
    display: grid;
    gap: 4px;
    padding: 10px;
    border-right: 1px solid var(--line-soft);
  }

  .lockfile-details > div:last-child {
    border-right: 0;
  }

  .lockfile-details dt {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6px;
    text-transform: uppercase;
  }

  .lockfile-details dd {
    overflow: hidden;
    margin: 0;
    color: var(--text-soft);
    font-size: 8.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .resource-action {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    margin: 14px;
    color: var(--brand-soft);
    font-size: 9px;
    font-weight: 600;
    text-decoration: none;
  }

  .resource-action:hover {
    color: var(--brand-bright);
  }

  .resource-action svg {
    width: 12px;
    height: 12px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
  }

  /* =====================================================
     ADMINISTRATION
     ===================================================== */

  .project-administration {
    margin: 0 16px 16px;
    overflow: hidden;
    border: 1px solid var(--line);
    border-radius: 9px;
    background: var(--bg-ink-soft);
  }

  .project-administration summary {
    display: flex;
    min-height: 61px;
    align-items: center;
    justify-content: space-between;
    gap: 15px;
    padding: 12px 15px;
    cursor: pointer;
    list-style: none;
  }

  .project-administration summary::-webkit-details-marker {
    display: none;
  }

  .project-administration summary > span {
    display: grid;
    gap: 3px;
  }

  .project-administration summary strong {
    color: var(--text-soft);
    font-size: 10.5px;
  }

  .project-administration summary small {
    color: var(--text-muted);
    font-size: 8.5px;
  }

  .project-administration summary > svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: var(--text-muted);
    stroke-width: 1.8;
    transition: transform var(--speed) var(--ease);
  }

  .project-administration[open]
    summary > svg {
    transform: rotate(180deg);
  }

  .project-administration__content {
    display: grid;
    border-top: 1px solid var(--line-soft);
  }

  .project-administration__content > article {
    display: flex;
    min-height: 72px;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    padding: 13px 15px;
    border-bottom: 1px solid var(--line-soft);
  }

  .project-administration__content
    article > div {
    display: grid;
    gap: 4px;
  }

  .project-administration__content
    article strong {
    color: var(--text);
    font-size: 10px;
  }

  .project-administration__content
    article p {
    max-width: 580px;
    color: var(--text-muted);
    font-size: 8.5px;
    line-height: 1.5;
  }

  .delete-project-action strong {
    color: var(--danger) !important;
  }

  .project-confirmation {
    display: grid;
    gap: 12px;
    padding: 15px;
    background: var(--danger-faint);
  }

  .project-confirmation > div:first-child {
    display: grid;
    gap: 4px;
  }

  .project-confirmation strong {
    color: var(--danger);
    font-size: 10.5px;
    text-transform: capitalize;
  }

  .project-confirmation p {
    color: var(--text-soft);
    font-size: 9px;
    line-height: 1.55;
  }

  .project-confirmation code {
    color: var(--danger);
  }

  .project-confirmation input {
    max-width: 360px;
    border-color: var(--danger-line);
  }

  .project-confirmation__actions {
    display: flex;
    gap: 8px;
  }

  /* =====================================================
     REDUCED MOTION
     ===================================================== */

  @media (prefers-reduced-motion: reduce) {
    .projects-page,
    .page-loading__bar,
    .detail-progress {
      animation: none;
    }
  }

  /* =====================================================
     RESPONSIVE
     ===================================================== */

  @media (max-width: 1040px) {
    .project-onboarding,
    .project-form-panel {
      grid-template-columns: 1fr;
    }

    .projects-layout {
      grid-template-columns: 1fr;
    }

    .project-directory {
      position: static;
    }

    .project-options {
      max-height: 260px;
    }

    .project-resources {
      grid-template-columns: 1fr;
    }

    .resource-empty--compact {
      min-height: 200px;
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

    .project-form {
      grid-template-columns: 1fr;
    }

    .repository-field,
    .slug-preview,
    .project-form__actions {
      grid-column: auto;
    }

    .project-card__header {
      align-items: flex-start;
      flex-direction: column;
    }

    .primary-link--compact {
      width: 100%;
    }

    .project-metadata,
    .project-metrics {
      grid-template-columns: 1fr;
    }

    .project-metadata > div,
    .project-metrics article {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .project-metadata > div:last-child,
    .project-metrics article:last-child {
      border-bottom: 0;
    }

    .project-onboarding__steps {
      grid-template-columns: 1fr;
      gap: 15px;
    }

    .project-onboarding__steps article,
    .project-onboarding__steps article:first-child {
      padding: 0;
      border-right: 0;
    }
  }

  @media (max-width: 620px) {
    .project-form__actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .project-form__actions button {
      width: 100%;
    }

    .project-onboarding {
      padding: 26px 19px;
    }

    .project-onboarding__copy h2 {
      font-size: clamp(31px, 10vw, 43px);
    }

    .build-row {
      grid-template-columns:
        7px minmax(0, 1fr) auto;
    }

    .build-row > svg {
      display: none;
    }

    .resource-empty {
      grid-template-columns: 1fr;
    }

    .resource-empty__icon {
      grid-row: auto;
    }

    .resource-empty > code {
      grid-column: auto;
    }

    .project-administration__content
      > article {
      align-items: stretch;
      flex-direction: column;
    }

    .project-administration__content
      > article button {
      width: 100%;
    }

    .project-confirmation__actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .project-confirmation__actions button {
      width: 100%;
    }
  }

  @media (max-width: 460px) {
    .project-form-panel {
      padding: 18px;
    }

    .project-option {
      grid-template-columns:
        31px minmax(0, 1fr);
    }

    .project-option__state {
      display: none;
    }

    .lockfile-state {
      grid-template-columns:
        auto minmax(0, 1fr);
    }

    .lockfile-state__ready {
      grid-column: 2;
    }

    .lockfile-details {
      grid-template-columns: 1fr;
    }

    .lockfile-details > div {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .lockfile-details > div:last-child {
      border-bottom: 0;
    }
  }
</style>
