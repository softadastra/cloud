<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { listPackageVersions, publishPackageVersion } from '$lib/api/packageVersions';
  import { listPackages } from '$lib/api/packages';
  import { listWorkspaces } from '$lib/api/workspaces';
  import { ApiError, type Package, type PackageVersion, type Workspace } from '$lib/api/types';
  import { auth } from '$lib/stores/auth';

  let workspaces: Workspace[] = [];
  let packages: Package[] = [];
  let versions: PackageVersion[] = [];
  let selectedWorkspaceId = '';
  let selectedPackageId = '';
  let version = '';
  let checksumSha256 = '';
  let archiveUrl = '';
  let manifestJson = '{}';
  let sizeBytes = 0;
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
      await loadPackages(params.get('package_id'));
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to load package versions.';
    } finally {
      loading = false;
    }
  }

  async function loadPackages(packageId?: string | null) {
    if (!selectedWorkspaceId) {
      packages = [];
      versions = [];
      return;
    }
    const data = await listPackages(selectedWorkspaceId);
    packages = data.packages;
    selectedPackageId = packageId ?? packages[0]?.id ?? '';
    await loadVersions();
  }

  async function loadVersions() {
    if (!selectedWorkspaceId || !selectedPackageId) {
      versions = [];
      return;
    }
    const data = await listPackageVersions(selectedWorkspaceId, selectedPackageId);
    versions = data.package_versions;
  }

  async function submitVersion() {
    const user = $auth.user;
    if (!user || !selectedWorkspaceId || !selectedPackageId || !version.trim()) return;
    saving = true;
    error = '';
    try {
      const created = await publishPackageVersion({
        workspaceId: selectedWorkspaceId,
        packageId: selectedPackageId,
        publishedByUserId: user.id,
        version: version.trim(),
        archiveUrl: archiveUrl || `storage/packages/${selectedWorkspaceId}/${selectedPackageId}/${version.trim()}/package.tar.gz`,
        checksumSha256,
        manifestJson,
        sizeBytes
      });
      versions = [created.package_version, ...versions];
      version = '';
      archiveUrl = '';
      checksumSha256 = '';
      manifestJson = '{}';
      sizeBytes = 0;
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to publish version.';
    } finally {
      saving = false;
    }
  }

  onMount(load);
</script>

<svelte:head><title>Package versions | Softadastra Cloud</title></svelte:head>
<section class="page-header"><div><p class="eyebrow">Registry</p><h1>Package versions</h1></div></section>
{#if error}<p class="form-error">{error}</p>{/if}

<section class="dashboard-grid">
  <form class="panel" on:submit|preventDefault={submitVersion}>
    <div class="panel-header"><h2>Publish metadata</h2></div>
    <label>Workspace<select bind:value={selectedWorkspaceId} on:change={() => loadPackages()}>{#each workspaces as workspace}<option value={workspace.id}>{workspace.name}</option>{/each}</select></label>
    <label>Package<select bind:value={selectedPackageId} on:change={loadVersions}>{#each packages as pkg}<option value={pkg.id}>{pkg.name}</option>{/each}</select></label>
    <label>Version<input bind:value={version} placeholder="1.0.0" required /></label>
    <label>Checksum SHA256<input bind:value={checksumSha256} maxlength="64" required /></label>
    <label>Archive URL<input bind:value={archiveUrl} /></label>
    <label>Size bytes<input bind:value={sizeBytes} type="number" min="0" /></label>
    <label>Manifest JSON<textarea bind:value={manifestJson} rows="5"></textarea></label>
    <button type="submit" disabled={saving || !selectedPackageId}>{saving ? 'Publishing...' : 'Publish version'}</button>
  </form>

  <div class="panel span-2">
    <div class="panel-header"><h2>Versions</h2><span>{versions.length}</span></div>
    {#if loading}<p class="muted">Loading...</p>{:else}
      <div class="table-list">
        {#each versions as item}
          <div class="row"><span><strong>{item.version}</strong><small>{item.checksum_sha256}</small></span><small>{item.status}</small></div>
        {:else}<p class="muted padded">No versions.</p>{/each}
      </div>
    {/if}
  </div>
</section>
