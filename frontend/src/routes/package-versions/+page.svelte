<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { listPackageVersions, publishPackageVersion } from '$lib/api/packageVersions';
  import { listPackages } from '$lib/api/packages';
  import { listWorkspaces } from '$lib/api/workspaces';
  import { ApiError, type Package, type PackageVersion, type Workspace } from '$lib/api/types';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import { canPublishPackageVersion } from '$lib/permissions';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  let workspaces: Workspace[] = []; let packages: Package[] = []; let versions: PackageVersion[] = [];
  let selectedWorkspaceId = ''; let selectedPackageId = ''; let version = ''; let checksumSha256 = ''; let archiveUrl = ''; let manifestJson = '{}'; let sizeBytes = 0;
  let loading = true; let saving = false; let error = ''; let success = '';
  $: selectedWorkspace = workspaces.find((workspace) => workspace.id === selectedWorkspaceId) ?? null;
  $: currentRole = selectedWorkspace?.current_user_role ?? 'viewer';

  async function load() {
    const user = $auth.user; if (!$auth.session || !user) { await goto('/login'); return; }
    loading = true; error = '';
    try { const params = new URLSearchParams(window.location.search); const workspaceData = await listWorkspaces(user.id); workspaces = workspaceData.workspaces; selectedWorkspaceId = params.get('workspace_id') ?? workspaces[0]?.id ?? ''; workspaceContext.setWorkspaces(workspaces, selectedWorkspaceId); await loadPackages(params.get('package_id')); }
    catch (err) { error = err instanceof ApiError ? err.message : 'Unable to load package versions.'; } finally { loading = false; }
  }
  async function loadPackages(packageId?: string | null) { workspaceContext.setSelectedWorkspace(selectedWorkspaceId); if (!selectedWorkspaceId) { packages = []; versions = []; return; } const data = await listPackages(selectedWorkspaceId); packages = data.packages; selectedPackageId = packageId ?? packages[0]?.id ?? ''; await loadVersions(); }
  async function loadVersions() { if (!selectedWorkspaceId || !selectedPackageId) { versions = []; return; } const data = await listPackageVersions(selectedWorkspaceId, selectedPackageId); versions = data.package_versions; }
  async function submitVersion() {
    const user = $auth.user; if (!user || !selectedWorkspaceId || !selectedPackageId || !version.trim() || !canPublishPackageVersion(currentRole)) return;
    saving = true; error = ''; success = '';
    try { const created = await publishPackageVersion({ workspaceId: selectedWorkspaceId, packageId: selectedPackageId, publishedByUserId: user.id, version: version.trim(), archiveUrl: archiveUrl || `storage/packages/${selectedWorkspaceId}/${selectedPackageId}/${version.trim()}/package.tar.gz`, checksumSha256, manifestJson, sizeBytes }); versions = [created.package_version, ...versions]; success = `Version ${created.package_version.version} published.`; version = ''; archiveUrl = ''; checksumSha256 = ''; manifestJson = '{}'; sizeBytes = 0; }
    catch (err) { error = err instanceof ApiError ? err.message : 'Unable to publish version.'; } finally { saving = false; }
  }
  onMount(load);
</script>

<svelte:head><title>Package versions | Softadastra Cloud</title></svelte:head>
<PageHeader eyebrow="Registry" title="Package versions" workspaceName={selectedWorkspace?.name ?? ''} role={currentRole} />
<InlineError message={error} />{#if success}<p class="success-message">{success}</p>{/if}
<section class="dashboard-grid">
  <div class="panel">
    <div class="panel-header"><h2>Package</h2></div>
    <label>Workspace<select bind:value={selectedWorkspaceId} on:change={() => loadPackages()}>{#each workspaces as workspace}<option value={workspace.id}>{workspace.name}</option>{/each}</select></label>
    <label>Package<select bind:value={selectedPackageId} on:change={loadVersions}>{#each packages as pkg}<option value={pkg.id}>{pkg.name}</option>{/each}</select></label>
    {#if canPublishPackageVersion(currentRole)}
      <form on:submit|preventDefault={submitVersion}>
        <div class="panel-header"><h2>Publish metadata</h2></div><p class="muted">Published versions are immutable. Publish a new version if metadata is wrong.</p>
        <label>Version<input bind:value={version} placeholder="1.0.0" required /></label><label>Checksum SHA256<input bind:value={checksumSha256} maxlength="64" required /></label><label>Archive URL<input bind:value={archiveUrl} /></label><label>Size bytes<input bind:value={sizeBytes} type="number" min="0" /></label><label>Manifest JSON<textarea bind:value={manifestJson} rows="5"></textarea></label>
        <button type="submit" disabled={saving || !selectedPackageId}>{saving ? 'Publishing...' : 'Publish version'}</button>
      </form>
    {:else}<ReadOnlyNotice message="You can list and download versions, but you cannot publish package versions." />{/if}
  </div>
  <div class="panel span-2"><div class="panel-header"><h2>Versions</h2><span>{versions.length}</span></div>{#if loading}<p class="muted">Loading...</p>{:else}<div class="table-list">{#each versions as item}<div class="row"><span><strong>{item.version}</strong><small>{item.checksum_sha256}</small></span><span class="actions"><StatusBadge status={item.status} /><a class="small" href={item.archive_url}>Download</a></span></div>{:else}<EmptyState title="No package versions yet." body={canPublishPackageVersion(currentRole) ? 'Publish the first version.' : 'No package version has been published yet.'} />{/each}</div>{/if}</div>
</section>
