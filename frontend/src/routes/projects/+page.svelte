<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { listBuildReports } from '$lib/api/buildReports';
  import { listLockfiles } from '$lib/api/lockfiles';
  import { listPackageVersions } from '$lib/api/packageVersions';
  import { createPackage, listPackages } from '$lib/api/packages';
  import { createProject, listProjects } from '$lib/api/projects';
  import { listWorkspaces } from '$lib/api/workspaces';
  import { ApiError, type BuildReport, type Lockfile, type Package, type PackageVersion, type Project, type Workspace } from '$lib/api/types';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import RoleBadge from '$lib/components/RoleBadge.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import { canCreatePackage, canCreateProject } from '$lib/permissions';
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
  let loading = true;
  let savingProject = false;
  let savingPackage = false;
  let error = '';
  let projectName = '';
  let projectSlug = '';
  let projectRepository = '';
  let packageName = '';

  $: projectSlug = projectSlug || slugify(projectName);
  $: selectedWorkspace = workspaces.find((workspace) => workspace.id === selectedWorkspaceId) ?? null;
  $: currentRole = selectedWorkspace?.current_user_role ?? 'viewer';

  function slugify(value: string) {
    return value
      .trim()
      .toLowerCase()
      .replace(/[^a-z0-9]+/g, '-')
      .replace(/^-|-$/g, '');
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
      const params = new URLSearchParams(window.location.search);
      const workspaceData = await listWorkspaces(user.id);
      workspaces = workspaceData.workspaces;
      selectedWorkspaceId = params.get('workspace_id') ?? workspaces[0]?.id ?? '';
      workspaceContext.setWorkspaces(workspaces, selectedWorkspaceId);
      await loadProjects(params.get('project_id'));
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to load projects.';
    } finally {
      loading = false;
    }
  }

  async function loadProjects(projectId?: string | null) {
    if (!selectedWorkspaceId) {
      projects = [];
      selectedProject = null;
      return;
    }

    const data = await listProjects(selectedWorkspaceId);
    projects = data.projects;
    selectedProject = projects.find((project) => project.id === projectId) ?? projects[0] ?? null;
    await loadProjectDetail();
  }

  async function selectWorkspace() {
    workspaceContext.setSelectedWorkspace(selectedWorkspaceId);
    history.replaceState(null, '', `/projects?workspace_id=${selectedWorkspaceId}`);
    await loadProjects();
  }

  async function selectProject(project: Project) {
    selectedProject = project;
    history.replaceState(null, '', `/projects?workspace_id=${selectedWorkspaceId}&project_id=${project.id}`);
    await loadProjectDetail();
  }

  async function loadProjectDetail() {
    if (!selectedWorkspaceId || !selectedProject) {
      packages = [];
      packageVersions = [];
      lockfiles = [];
      buildReports = [];
      return;
    }

    const [packageData, lockfileData, buildReportData] = await Promise.all([
      listPackages(selectedWorkspaceId),
      listLockfiles(selectedWorkspaceId, selectedProject.id),
      listBuildReports(selectedWorkspaceId, selectedProject.id)
    ]);

    packages = packageData.packages;
    lockfiles = lockfileData.lockfiles;
    buildReports = buildReportData.build_reports;
    const versions = await Promise.all(packages.slice(0, 5).map((pkg) => listPackageVersions(selectedWorkspaceId, pkg.id).catch(() => ({ package_versions: [] }))));
    packageVersions = versions.flatMap((item) => item.package_versions);
  }

  async function submitProject() {
    const user = $auth.user;

    if (!user || !selectedWorkspaceId || !projectName.trim() || !canCreateProject(currentRole)) {
      return;
    }

    savingProject = true;
    error = '';

    try {
      const created = await createProject({
        workspaceId: selectedWorkspaceId,
        ownerUserId: user.id,
        name: projectName.trim(),
        slug: projectSlug || slugify(projectName),
        repositoryUrl: projectRepository.trim(),
        defaultBranch: 'main'
      });
      projects = [created.project, ...projects];
      await selectProject(created.project);
      projectName = '';
      projectSlug = '';
      projectRepository = '';
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to create project.';
    } finally {
      savingProject = false;
    }
  }

  async function submitPackage() {
    const user = $auth.user;

    if (!user || !selectedWorkspaceId || !packageName.trim() || !canCreatePackage(currentRole)) {
      return;
    }

    savingPackage = true;
    error = '';

    try {
      const created = await createPackage({
        workspaceId: selectedWorkspaceId,
        ownerUserId: user.id,
        name: packageName.trim(),
        visibility: 'private'
      });
      packages = [created.package, ...packages];
      packageName = '';
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to create package.';
    } finally {
      savingPackage = false;
    }
  }

  onMount(load);
</script>

<svelte:head><title>Projects | Softadastra Cloud</title></svelte:head>

<PageHeader eyebrow="Projects" title="Projects" workspaceName={selectedWorkspace?.name ?? ''} role={currentRole} />
<InlineError message={error} />

<section class="dashboard-grid">
  <div class="panel">
    <div class="panel-header"><h2>Workspace</h2></div>
    <label>
      Select workspace
      <select bind:value={selectedWorkspaceId} on:change={selectWorkspace}>
        {#each workspaces as workspace}
          <option value={workspace.id}>{workspace.name}</option>
        {/each}
      </select>
    </label>

    <div class="panel-header"><h2>Projects</h2><span>{projects.length}</span></div>
    {#if loading}
      <p class="muted">Loading...</p>
    {:else}
      <div class="table-list compact">
        {#each projects as project}
          <button class="row-button" type="button" on:click={() => selectProject(project)}><span><strong>{project.name}</strong><small>{project.slug}</small></span><small>{project.id === selectedProject?.id ? 'selected' : 'open'}</small></button>
        {:else}<EmptyState title={canCreateProject(currentRole) ? 'No projects yet.' : 'No projects have been created yet.'} body={canCreateProject(currentRole) ? 'Create the first project.' : ''} />{/each}
      </div>
    {/if}
  </div>

  <div class="panel">
    {#if canCreateProject(currentRole)}
      <form on:submit|preventDefault={submitProject}>
        <div class="panel-header"><h2>New project</h2></div>
        <label>Name<input bind:value={projectName} required /></label>
        <label>Slug<input bind:value={projectSlug} required /></label>
        <label>Repository URL<input bind:value={projectRepository} /></label>
        <button type="submit" disabled={savingProject || !selectedWorkspaceId}>{savingProject ? 'Creating...' : 'Create project'}</button>
      </form>
    {:else}
      <div class="panel-header"><h2>Projects</h2></div>
      <ReadOnlyNotice message="You can view projects in this workspace, but you cannot create or edit them." />
    {/if}
  </div>
</section>

{#if selectedProject}
  <section class="panel detail-panel">
    <div class="panel-header"><h2>{selectedProject.name}</h2><span><RoleBadge role={currentRole} /></span></div>
    <div class="meta-grid"><span><strong>ID</strong>{selectedProject.id}</span><span><strong>Slug</strong>{selectedProject.slug}</span><span><strong>Branch</strong>{selectedProject.default_branch || 'main'}</span></div>
  </section>

  <section class="triple-grid">
    <div class="panel">
      <div class="panel-header"><h2>Packages</h2><span>{packages.length}</span></div>
      {#if canCreatePackage(currentRole)}<form class="inline-form" on:submit|preventDefault={submitPackage}><input bind:value={packageName} placeholder="vix/package-name" /><button type="submit" disabled={savingPackage}>{savingPackage ? '...' : 'Create'}</button></form>{/if}
      <div class="table-list compact">{#each packages as pkg}<div class="row"><span><strong>{pkg.name}</strong><small>{pkg.visibility}</small></span></div>{:else}<EmptyState title="No packages yet." body={canCreatePackage(currentRole) ? 'Create the first package.' : 'No packages have been created yet.'} />{/each}</div>
    </div>

    <div class="panel">
      <div class="panel-header"><h2>Lockfiles</h2><span>{lockfiles.length}</span></div>
      <div class="table-list compact">{#each lockfiles as lockfile}<div class="row"><span><strong>{lockfile.checksum_sha256.slice(0, 12)}</strong><small>{lockfile.source}</small></span></div>{:else}<EmptyState title="No lockfiles uploaded yet." body={currentRole === 'viewer' ? 'No lockfile has been uploaded yet.' : 'Upload the first lockfile from the Lockfiles page.'} />{/each}</div>
    </div>

    <div class="panel">
      <div class="panel-header"><h2>Build reports</h2><span>{buildReports.length}</span></div>
      <div class="table-list compact">{#each buildReports as report}<div class="row"><span><strong>{report.status}</strong><small>{report.target} {report.profile}</small></span><span class="actions"><StatusBadge status={report.status} /><small>{report.errors_count} errors</small></span></div>{:else}<EmptyState title="No build reports yet." body={currentRole === 'viewer' ? 'No build reports have been submitted yet.' : 'Submit the first report from the Build reports page.'} />{/each}</div>
    </div>
  </section>

  <section class="panel">
    <div class="panel-header"><h2>Package versions</h2><span>{packageVersions.length}</span></div>
    <div class="table-list compact">{#each packageVersions as version}<div class="row"><span><strong>{version.version}</strong><small>{version.package_id}</small></span><small>{version.status}</small></div>{:else}<EmptyState title="No package versions yet." body={currentRole === 'viewer' ? 'No package version has been published yet.' : 'Publish the first version from the Versions page.'} />{/each}</div>
  </section>
{/if}
