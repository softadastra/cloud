<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { listBuildReports, submitBuildReport } from '$lib/api/buildReports';
  import { listProjects } from '$lib/api/projects';
  import { listWorkspaces } from '$lib/api/workspaces';
  import { ApiError, type BuildReport, type Project, type Workspace } from '$lib/api/types';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import { canSubmitBuildReport } from '$lib/permissions';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  let workspaces: Workspace[] = []; let projects: Project[] = []; let reports: BuildReport[] = [];
  let selectedWorkspaceId = ''; let selectedProjectId = ''; let status = 'success'; let target = ''; let profile = 'debug'; let branch = ''; let commitSha = ''; let toolchain = ''; let durationMs = 0; let warningsCount = 0; let errorsCount = 0;
  let loading = true; let saving = false; let error = ''; let success = ''; let openDiagnosticsId = '';
  $: selectedWorkspace = workspaces.find((workspace) => workspace.id === selectedWorkspaceId) ?? null;
  $: currentRole = selectedWorkspace?.current_user_role ?? 'viewer';

  async function load() { const user = $auth.user; if (!$auth.session || !user) { await goto('/login'); return; } loading = true; error = ''; try { const params = new URLSearchParams(window.location.search); const workspaceData = await listWorkspaces(user.id); workspaces = workspaceData.workspaces; selectedWorkspaceId = params.get('workspace_id') ?? workspaces[0]?.id ?? ''; workspaceContext.setWorkspaces(workspaces, selectedWorkspaceId); await loadProjects(params.get('project_id')); } catch (err) { error = err instanceof ApiError ? err.message : 'Unable to load build reports.'; } finally { loading = false; } }
  async function loadProjects(projectId?: string | null) { workspaceContext.setSelectedWorkspace(selectedWorkspaceId); if (!selectedWorkspaceId) { projects = []; reports = []; return; } const data = await listProjects(selectedWorkspaceId); projects = data.projects; selectedProjectId = projectId ?? projects[0]?.id ?? ''; await loadReports(); }
  async function loadReports() { if (!selectedWorkspaceId || !selectedProjectId) { reports = []; return; } const data = await listBuildReports(selectedWorkspaceId, selectedProjectId); reports = data.build_reports; }
  async function submitReport() { const user = $auth.user; if (!user || !selectedWorkspaceId || !selectedProjectId || !target.trim() || !canSubmitBuildReport(currentRole)) return; saving = true; error = ''; success = ''; try { const submitted = await submitBuildReport({ workspaceId: selectedWorkspaceId, projectId: selectedProjectId, submittedByUserId: user.id, status, target, profile, branch, commitSha, toolchain, durationMs, warningsCount, errorsCount }); reports = [submitted.build_report, ...reports]; success = 'Build report submitted.'; target = ''; commitSha = ''; durationMs = 0; warningsCount = 0; errorsCount = 0; } catch (err) { error = err instanceof ApiError ? err.message : 'Unable to submit build report.'; } finally { saving = false; } }
  onMount(load);
</script>

<svelte:head><title>Build reports | Softadastra Cloud</title></svelte:head>
<PageHeader eyebrow="Builds" title="Build reports" workspaceName={selectedWorkspace?.name ?? ''} role={currentRole} />
<InlineError message={error} />{#if success}<p class="success-message">{success}</p>{/if}
<section class="dashboard-grid">
  <div class="panel">
    <div class="panel-header"><h2>Project</h2></div>
    <label>Workspace<select bind:value={selectedWorkspaceId} on:change={() => loadProjects()}>{#each workspaces as workspace}<option value={workspace.id}>{workspace.name}</option>{/each}</select></label>
    <label>Project<select bind:value={selectedProjectId} on:change={loadReports}>{#each projects as project}<option value={project.id}>{project.name}</option>{/each}</select></label>
    {#if canSubmitBuildReport(currentRole)}
      <form on:submit|preventDefault={submitReport}><div class="panel-header"><h2>Submit report</h2></div><label>Status<select bind:value={status}><option value="success">success</option><option value="failed">failed</option><option value="warning">warning</option></select></label><label>Target<input bind:value={target} placeholder="app" required /></label><label>Profile<input bind:value={profile} /></label><label>Branch<input bind:value={branch} /></label><label>Commit SHA<input bind:value={commitSha} /></label><label>Toolchain<input bind:value={toolchain} /></label><label>Duration ms<input bind:value={durationMs} type="number" min="0" /></label><label>Warnings<input bind:value={warningsCount} type="number" min="0" /></label><label>Errors<input bind:value={errorsCount} type="number" min="0" /></label><button type="submit" disabled={saving || !selectedProjectId}>{saving ? 'Submitting...' : 'Submit report'}</button></form>
    {:else}<ReadOnlyNotice message="You can view build reports, but you cannot submit reports." />{/if}
  </div>
  <div class="panel span-2"><div class="panel-header"><h2>Reports</h2><span>{reports.length}</span></div>{#if loading}<p class="muted">Loading...</p>{:else}<div class="table-list">{#each reports as report}<div class="row"><span><strong><StatusBadge status={report.status} /></strong><small>{report.target} / {report.profile} · {report.warnings_count} warnings · {report.errors_count} errors</small>{#if openDiagnosticsId === report.id}<pre class="json-preview">{report.diagnostics_json || report.summary_json}</pre>{/if}</span><button class="small" type="button" on:click={() => openDiagnosticsId = openDiagnosticsId === report.id ? '' : report.id}>View diagnostics</button></div>{:else}<EmptyState title="No build reports yet." body={canSubmitBuildReport(currentRole) ? 'Submit the first report.' : 'No build reports have been submitted yet.'} />{/each}</div>{/if}</div>
</section>
