<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { listBuildReports, submitBuildReport } from '$lib/api/buildReports';
  import { listProjects } from '$lib/api/projects';
  import { listWorkspaces } from '$lib/api/workspaces';
  import { ApiError, type BuildReport, type Project, type Workspace } from '$lib/api/types';
  import { auth } from '$lib/stores/auth';

  let workspaces: Workspace[] = [];
  let projects: Project[] = [];
  let reports: BuildReport[] = [];
  let selectedWorkspaceId = '';
  let selectedProjectId = '';
  let status = 'success';
  let target = '';
  let profile = 'debug';
  let branch = '';
  let commitSha = '';
  let toolchain = '';
  let durationMs = 0;
  let warningsCount = 0;
  let errorsCount = 0;
  let loading = true;
  let saving = false;
  let error = '';

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
      await loadProjects(params.get('project_id'));
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to load build reports.';
    } finally {
      loading = false;
    }
  }

  async function loadProjects(projectId?: string | null) {
    if (!selectedWorkspaceId) {
      projects = [];
      reports = [];
      return;
    }
    const data = await listProjects(selectedWorkspaceId);
    projects = data.projects;
    selectedProjectId = projectId ?? projects[0]?.id ?? '';
    await loadReports();
  }

  async function loadReports() {
    if (!selectedWorkspaceId || !selectedProjectId) {
      reports = [];
      return;
    }
    const data = await listBuildReports(selectedWorkspaceId, selectedProjectId);
    reports = data.build_reports;
  }

  async function submitReport() {
    const user = $auth.user;
    if (!user || !selectedWorkspaceId || !selectedProjectId || !target.trim()) return;
    saving = true;
    error = '';
    try {
      const submitted = await submitBuildReport({ workspaceId: selectedWorkspaceId, projectId: selectedProjectId, submittedByUserId: user.id, status, target, profile, branch, commitSha, toolchain, durationMs, warningsCount, errorsCount });
      reports = [submitted.build_report, ...reports];
      target = '';
      commitSha = '';
      durationMs = 0;
      warningsCount = 0;
      errorsCount = 0;
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to submit build report.';
    } finally {
      saving = false;
    }
  }

  onMount(load);
</script>

<svelte:head><title>Build reports | Softadastra Cloud</title></svelte:head>
<section class="page-header"><div><p class="eyebrow">Builds</p><h1>Build reports</h1></div></section>
{#if error}<p class="form-error">{error}</p>{/if}

<section class="dashboard-grid">
  <form class="panel" on:submit|preventDefault={submitReport}>
    <div class="panel-header"><h2>Submit report</h2></div>
    <label>Workspace<select bind:value={selectedWorkspaceId} on:change={() => loadProjects()}>{#each workspaces as workspace}<option value={workspace.id}>{workspace.name}</option>{/each}</select></label>
    <label>Project<select bind:value={selectedProjectId} on:change={loadReports}>{#each projects as project}<option value={project.id}>{project.name}</option>{/each}</select></label>
    <label>Status<select bind:value={status}><option value="success">success</option><option value="failed">failed</option><option value="warning">warning</option></select></label>
    <label>Target<input bind:value={target} placeholder="app" required /></label>
    <label>Profile<input bind:value={profile} /></label>
    <label>Branch<input bind:value={branch} /></label>
    <label>Commit SHA<input bind:value={commitSha} /></label>
    <label>Toolchain<input bind:value={toolchain} /></label>
    <label>Duration ms<input bind:value={durationMs} type="number" min="0" /></label>
    <label>Warnings<input bind:value={warningsCount} type="number" min="0" /></label>
    <label>Errors<input bind:value={errorsCount} type="number" min="0" /></label>
    <button type="submit" disabled={saving || !selectedProjectId}>{saving ? 'Submitting...' : 'Submit report'}</button>
  </form>

  <div class="panel span-2">
    <div class="panel-header"><h2>Reports</h2><span>{reports.length}</span></div>
    {#if loading}<p class="muted">Loading...</p>{:else}
      <div class="table-list">
        {#each reports as report}
          <div class="row"><span><strong>{report.status}</strong><small>{report.target} / {report.profile}</small></span><small>{report.errors_count} errors</small></div>
        {:else}<p class="muted padded">No build reports.</p>{/each}
      </div>
    {/if}
  </div>
</section>
