<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import {
    listPackageVersions,
    publishPackageVersion
  } from '$lib/api/packageVersions';
  import { listPackages } from '$lib/api/packages';
  import { listWorkspaces } from '$lib/api/workspaces';
  import {
    ApiError,
    type Package,
    type PackageVersion,
    type Workspace
  } from '$lib/api/types';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import { canPublishPackageVersion } from '$lib/permissions';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  let workspaces: Workspace[] = [];
  let packages: Package[] = [];
  let versions: PackageVersion[] = [];

  let selectedWorkspaceId = '';
  let selectedPackageId = '';

  let loading = true;
  let loadingVersions = false;
  let saving = false;

  let error = '';
  let success = '';

  let packageQuery = '';
  let versionQuery = '';
  let versionStatusFilter = 'all';

  let showPublishForm = false;

  let version = '';
  let checksumSha256 = '';
  let archiveUrl = '';
  let manifestJson = '{}';
  let sizeBytes = 0;

  let copiedChecksumId = '';
  let copyResetTimer: ReturnType<typeof setTimeout> | undefined;
  let versionRequestId = 0;

  $: selectedWorkspace =
    workspaces.find(
      (workspace) => workspace.id === selectedWorkspaceId
    ) ?? null;

  $: selectedPackage =
    packages.find(
      (pkg) => pkg.id === selectedPackageId
    ) ?? null;

  $: currentRole =
    selectedWorkspace?.current_user_role ?? 'viewer';

  $: canPublish =
    canPublishPackageVersion(currentRole);

  $: visiblePackages = packages.filter((pkg) => {
    const query = packageQuery.trim().toLowerCase();

    if (!query) {
      return true;
    }

    return (
      pkg.name.toLowerCase().includes(query) ||
      pkg.description?.toLowerCase().includes(query) ||
      pkg.repository_url?.toLowerCase().includes(query)
    );
  });

  $: versionStatuses = Array.from(
    new Set(
      versions
        .map((item) => item.status)
        .filter((status): status is string => Boolean(status))
    )
  );

  $: visibleVersions = versions.filter((item) => {
    const query = versionQuery.trim().toLowerCase();

    const matchesQuery =
      !query ||
      item.version.toLowerCase().includes(query) ||
      item.checksum_sha256.toLowerCase().includes(query) ||
      item.archive_url?.toLowerCase().includes(query);

    const matchesStatus =
      versionStatusFilter === 'all' ||
      item.status === versionStatusFilter;

    return matchesQuery && matchesStatus;
  });

  $: totalSize = versions.reduce(
    (total, item) => total + Number(item.size_bytes || 0),
    0
  );

  $: latestVersion =
    versions[0]?.version ?? '—';

  function packageInitial(pkg: Package) {
    const segment = pkg.name
      .split('/')
      .filter(Boolean)
      .at(-1);

    return (segment || pkg.name || 'P')
      .slice(0, 1)
      .toUpperCase();
  }

  function visibilityLabel(value?: string) {
    return value === 'public'
      ? 'Public'
      : 'Private';
  }

  function statusLabel(value?: string) {
    if (!value) {
      return 'Unknown';
    }

    return value.charAt(0).toUpperCase() + value.slice(1);
  }

  function shortChecksum(value: string) {
    if (value.length <= 20) {
      return value;
    }

    return `${value.slice(0, 12)}…${value.slice(-8)}`;
  }

  function formatBytes(value: number) {
    if (!Number.isFinite(value) || value <= 0) {
      return '0 B';
    }

    const units = ['B', 'KB', 'MB', 'GB'];
    const unitIndex = Math.min(
      Math.floor(Math.log(value) / Math.log(1024)),
      units.length - 1
    );

    const size = value / Math.pow(1024, unitIndex);

    return `${size >= 10 ? size.toFixed(0) : size.toFixed(1)} ${
      units[unitIndex]
    }`;
  }

  function updatePackageUrl(packageId: string) {
    const url = new URL(window.location.href);

    url.searchParams.set(
      'workspace_id',
      selectedWorkspaceId
    );

    url.searchParams.set(
      'package_id',
      packageId
    );

    history.replaceState(
      null,
      '',
      `${url.pathname}${url.search}`
    );
  }

  function resetPublishForm() {
    version = '';
    checksumSha256 = '';
    archiveUrl = '';
    manifestJson = '{}';
    sizeBytes = 0;
  }

  function togglePublishForm() {
    showPublishForm = !showPublishForm;
    error = '';
    success = '';

    if (!showPublishForm) {
      resetPublishForm();
    }
  }

  function closePublishForm() {
    if (saving) {
      return;
    }

    showPublishForm = false;
    resetPublishForm();
    error = '';
  }

  function handlePublishSubmit(event: SubmitEvent) {
    event.preventDefault();
    void submitVersion();
  }

  async function copyChecksum(item: PackageVersion) {
    try {
      await navigator.clipboard.writeText(
        item.checksum_sha256
      );

      copiedChecksumId = item.id;

      if (copyResetTimer) {
        clearTimeout(copyResetTimer);
      }

      copyResetTimer = setTimeout(() => {
        copiedChecksumId = '';
      }, 1800);
    } catch {
      error = 'Unable to copy the checksum.';
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

      await loadPackages(
        params.get('package_id')
      );
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load package versions.';
    } finally {
      loading = false;
    }
  }

  async function loadPackages(
    requestedPackageId?: string | null
  ) {
    if (!selectedWorkspaceId) {
      packages = [];
      versions = [];
      selectedPackageId = '';
      return;
    }

    workspaceContext.setSelectedWorkspace(
      selectedWorkspaceId
    );

    const data = await listPackages(
      selectedWorkspaceId
    );

    packages = data.packages;

    selectedPackageId =
      (
        requestedPackageId &&
        packages.some(
          (pkg) => pkg.id === requestedPackageId
        )
      )
        ? requestedPackageId
        : packages[0]?.id ?? '';

    if (selectedPackageId) {
      updatePackageUrl(selectedPackageId);
    }

    await loadVersions(selectedPackageId);
  }

  async function selectPackage(pkg: Package) {
    if (
      pkg.id === selectedPackageId ||
      loadingVersions
    ) {
      return;
    }

    selectedPackageId = pkg.id;
    versionQuery = '';
    versionStatusFilter = 'all';
    copiedChecksumId = '';
    error = '';
    success = '';

    showPublishForm = false;
    resetPublishForm();

    updatePackageUrl(pkg.id);

    await loadVersions(pkg.id);
  }

  async function loadVersions(packageId: string) {
    if (!selectedWorkspaceId || !packageId) {
      versions = [];
      return;
    }

    const requestId = ++versionRequestId;
    const workspaceId = selectedWorkspaceId;

    loadingVersions = true;
    error = '';
    versions = [];

    try {
      const data = await listPackageVersions(
        workspaceId,
        packageId
      );

      if (
        requestId !== versionRequestId ||
        selectedPackageId !== packageId ||
        selectedWorkspaceId !== workspaceId
      ) {
        return;
      }

      versions = data.package_versions;
    } catch (err) {
      if (requestId !== versionRequestId) {
        return;
      }

      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load versions.';
    } finally {
      if (requestId === versionRequestId) {
        loadingVersions = false;
      }
    }
  }

  async function submitVersion() {
    const user = $auth.user;

    const cleanVersion = version.trim();
    const cleanChecksum = checksumSha256.trim();
    const cleanArchiveUrl = archiveUrl.trim();

    if (
      !user ||
      !selectedWorkspaceId ||
      !selectedPackageId ||
      !cleanVersion ||
      !canPublish
    ) {
      return;
    }

    if (!/^[a-fA-F0-9]{64}$/.test(cleanChecksum)) {
      error =
        'Checksum SHA256 must contain exactly 64 hexadecimal characters.';
      return;
    }

    let normalizedManifest = '';

    try {
      normalizedManifest = JSON.stringify(
        JSON.parse(manifestJson || '{}')
      );
    } catch {
      error = 'Manifest JSON is not valid.';
      return;
    }

    saving = true;
    error = '';
    success = '';

    try {
      const created = await publishPackageVersion({
        workspaceId: selectedWorkspaceId,
        packageId: selectedPackageId,
        publishedByUserId: user.id,
        version: cleanVersion,
        archiveUrl:
          cleanArchiveUrl ||
          `storage/packages/${selectedWorkspaceId}/${selectedPackageId}/${cleanVersion}/package.tar.gz`,
        checksumSha256: cleanChecksum,
        manifestJson: normalizedManifest,
        sizeBytes: Math.max(
          0,
          Number(sizeBytes) || 0
        )
      });

      versions = [
        created.package_version,
        ...versions
      ];

      success =
        `Version ${created.package_version.version} published.`;

      resetPublishForm();
      showPublishForm = false;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to publish version.';
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
  <title>Package versions | Softadastra Cloud</title>
</svelte:head>

<PageHeader
  eyebrow="Registry"
  title="Package versions"
  workspaceName={selectedWorkspace?.name ?? ''}
  role={currentRole}
>
  {#if canPublish && selectedPackage && !loading}
    <button
      class:secondary-button={showPublishForm}
      class="publish-version-button"
      type="button"
      onclick={togglePublishForm}
    >
      {showPublishForm ? 'Close' : 'Publish version'}
    </button>
  {/if}
</PageHeader>

<p class="page-description">
  Review immutable package releases, their checksums, archives and
  publication metadata.
</p>

<InlineError message={error} />

{#if success}
  <p class="success-message" role="status">
    {success}
  </p>
{/if}

{#if showPublishForm && canPublish && selectedPackage}
  <section
    class="publish-section"
    aria-labelledby="publish-version-title"
  >
    <div class="publish-heading">
      <div>
        <h2 id="publish-version-title">
          Publish a version
        </h2>

        <p>
          Publishing to
          <strong>{selectedPackage.name}</strong>.
          Published versions cannot be edited.
        </p>
      </div>
    </div>

    <form
      class="publish-form"
      onsubmit={handlePublishSubmit}
    >
      <label>
        Version

        <input
          bind:value={version}
          placeholder="1.0.0"
          required
          autocomplete="off"
        />
      </label>

      <label>
        Size in bytes

        <input
          bind:value={sizeBytes}
          type="number"
          min="0"
          step="1"
          placeholder="0"
        />
      </label>

      <label class="wide-field">
        Checksum SHA256

        <input
          bind:value={checksumSha256}
          maxlength="64"
          minlength="64"
          placeholder="64 hexadecimal characters"
          required
          autocomplete="off"
          spellcheck="false"
        />
      </label>

      <label class="wide-field">
        Archive URL

        <input
          bind:value={archiveUrl}
          placeholder="Leave empty to use the default storage path"
          autocomplete="off"
          spellcheck="false"
        />
      </label>

      <label class="wide-field">
        Manifest JSON

        <textarea
          bind:value={manifestJson}
          rows="7"
          spellcheck="false"
        ></textarea>
      </label>

      <div class="archive-preview">
        <span>Archive path</span>

        <code>
          {archiveUrl.trim() ||
            `storage/packages/${selectedWorkspaceId}/${selectedPackageId}/${version.trim() || 'version'}/package.tar.gz`}
        </code>
      </div>

      <div class="publish-actions">
        <button
          class="secondary-button"
          type="button"
          disabled={saving}
          onclick={closePublishForm}
        >
          Cancel
        </button>

        <button
          type="submit"
          disabled={
            saving ||
            !selectedPackageId ||
            !version.trim() ||
            checksumSha256.trim().length !== 64
          }
        >
          {saving
            ? 'Publishing…'
            : 'Publish version'}
        </button>
      </div>
    </form>
  </section>
{/if}

<div class="versions-layout">
  <aside
    class="package-directory"
    aria-label="Package directory"
  >
    <div class="directory-header">
      <div>
        <h2>Packages</h2>
        <p>{selectedWorkspace?.name ?? 'Current workspace'}</p>
      </div>

      <span class="section-count">
        {packages.length}
      </span>
    </div>

    {#if packages.length > 1}
      <div class="package-search">
        <svg viewBox="0 0 24 24" aria-hidden="true">
          <circle cx="11" cy="11" r="7"></circle>
          <path d="m20 20-4-4"></path>
        </svg>

        <input
          bind:value={packageQuery}
          type="search"
          placeholder="Find a package"
          aria-label="Find a package"
        />
      </div>
    {/if}

    {#if loading}
      <p class="directory-loading">
        Loading packages…
      </p>
    {:else if packages.length === 0}
      <div class="directory-empty">
        <EmptyState
          title="No packages"
          body="Create a package before publishing versions."
        />
      </div>
    {:else if visiblePackages.length === 0}
      <p class="directory-loading">
        No package matches your search.
      </p>
    {:else}
      <div class="package-options">
        {#each visiblePackages as pkg (pkg.id)}
          <button
            class:selected={
              pkg.id === selectedPackageId
            }
            class="package-option"
            type="button"
            aria-pressed={
              pkg.id === selectedPackageId
            }
            onclick={() => selectPackage(pkg)}
          >
            <span class="package-option__mark">
              {packageInitial(pkg)}
            </span>

            <span class="package-option__content">
              <strong>{pkg.name}</strong>

              <span>
                {visibilityLabel(pkg.visibility)}
              </span>
            </span>
          </button>
        {/each}
      </div>
    {/if}
  </aside>

  <main class="version-detail">
    {#if loading}
      <section class="detail-section">
        <p class="loading-state">
          Loading package…
        </p>
      </section>
    {:else if !selectedPackage}
      <section class="detail-section empty-detail">
        <EmptyState
          title="Select a package"
          body="Its published versions will appear here."
        />
      </section>
    {:else}
      <section
        class="package-overview"
        aria-labelledby="selected-package-name"
      >
        <div class="package-overview__header">
          <div class="package-identity">
            <span class="package-mark">
              {packageInitial(selectedPackage)}
            </span>

            <div>
              <div class="package-name-line">
                <h2 id="selected-package-name">
                  {selectedPackage.name}
                </h2>

                <span
                  class:public={
                    selectedPackage.visibility === 'public'
                  }
                  class="visibility-badge"
                >
                  {visibilityLabel(selectedPackage.visibility)}
                </span>
              </div>

              <p>
                {selectedPackage.description ||
                  'No package description provided.'}
              </p>
            </div>
          </div>

          <a
            class="package-link"
            href={`/packages?workspace_id=${selectedWorkspaceId}`}
          >
            Open packages

            <svg viewBox="0 0 24 24" aria-hidden="true">
              <path d="m9 18 6-6-6-6"></path>
            </svg>
          </a>
        </div>

        <dl class="package-summary">
          <div>
            <dt>Versions</dt>
            <dd>{versions.length}</dd>
          </div>

          <div>
            <dt>Latest version</dt>
            <dd>{latestVersion}</dd>
          </div>

          <div>
            <dt>Total archive size</dt>
            <dd>{formatBytes(totalSize)}</dd>
          </div>

          <div>
            <dt>Visibility</dt>
            <dd>
              {visibilityLabel(selectedPackage.visibility)}
            </dd>
          </div>
        </dl>

        {#if selectedPackage.repository_url}
          <div class="repository-row">
            <span>Repository</span>

            <a
              href={selectedPackage.repository_url}
              target="_blank"
              rel="noreferrer"
            >
              {selectedPackage.repository_url}
            </a>
          </div>
        {/if}
      </section>

      {#if !canPublish}
        <ReadOnlyNotice
          message="You can review and download package versions, but your role cannot publish new versions."
        />
      {/if}

      <section
        class="detail-section versions-section"
        aria-labelledby="versions-title"
      >
        <div class="section-header">
          <div>
            <h2 id="versions-title">
              Published versions
            </h2>

            <p>
              Immutable releases published for this package.
            </p>
          </div>

          <span class="section-count">
            {visibleVersions.length}
          </span>
        </div>

        {#if versions.length > 0}
          <div class="version-toolbar">
            <div class="version-search">
              <svg viewBox="0 0 24 24" aria-hidden="true">
                <circle cx="11" cy="11" r="7"></circle>
                <path d="m20 20-4-4"></path>
              </svg>

              <input
                bind:value={versionQuery}
                type="search"
                placeholder="Find a version or checksum"
                aria-label="Find a package version"
              />
            </div>

            <select
              bind:value={versionStatusFilter}
              aria-label="Filter versions by status"
            >
              <option value="all">
                All statuses
              </option>

              {#each versionStatuses as status}
                <option value={status}>
                  {statusLabel(status)}
                </option>
              {/each}
            </select>
          </div>
        {/if}

        {#if loadingVersions}
          <p class="loading-state">
            Loading versions…
          </p>
        {:else if versions.length === 0}
          <div class="empty-wrapper">
            <EmptyState
              title="No package versions yet"
              body={canPublish
                ? 'Publish the first immutable version of this package.'
                : 'No version has been published for this package.'}
            />
          </div>
        {:else if visibleVersions.length === 0}
          <div class="no-results">
            <strong>No matching versions</strong>
            <p>
              Change the search text or selected status.
            </p>
          </div>
        {:else}
          <div class="version-table">
            <div class="table-heading">
              <span>Version</span>
              <span>Checksum</span>
              <span>Size</span>
              <span>Status</span>
              <span class="align-right">Archive</span>
            </div>

            {#each visibleVersions as item (item.id)}
              <div class="version-row">
                <div
                  class="version-identity"
                  data-label="Version"
                >
                  <strong>{item.version}</strong>
                  <small>Immutable release</small>
                </div>

                <div
                  class="checksum-cell"
                  data-label="Checksum"
                >
                  <code title={item.checksum_sha256}>
                    {shortChecksum(item.checksum_sha256)}
                  </code>

                  <button
                    class:copied={
                      copiedChecksumId === item.id
                    }
                    class="copy-checksum-button"
                    type="button"
                    aria-label={
                      copiedChecksumId === item.id
                        ? 'Checksum copied'
                        : `Copy checksum for version ${item.version}`
                    }
                    title={
                      copiedChecksumId === item.id
                        ? 'Copied'
                        : 'Copy checksum'
                    }
                    onclick={() => copyChecksum(item)}
                  >
                    {#if copiedChecksumId === item.id}
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

                        <path
                          d="M16 8V5a2 2 0 0 0-2-2H5a2 2 0 0 0-2 2v9a2 2 0 0 0 2 2h3"
                        ></path>
                      </svg>
                    {/if}
                  </button>
                </div>

                <div
                  class="size-cell"
                  data-label="Size"
                >
                  {formatBytes(Number(item.size_bytes || 0))}
                </div>

                <div
                  class="status-cell"
                  data-label="Status"
                >
                  <StatusBadge status={item.status} />
                </div>

                <div
                  class="archive-cell"
                  data-label="Archive"
                >
                  {#if item.archive_url}
                    <a
                      href={item.archive_url}
                      target="_blank"
                      rel="noreferrer"
                    >
                      Download

                      <svg
                        viewBox="0 0 24 24"
                        aria-hidden="true"
                      >
                        <path d="M12 3v12"></path>
                        <path d="m7 10 5 5 5-5"></path>
                        <path d="M5 21h14"></path>
                      </svg>
                    </a>
                  {:else}
                    <span class="unavailable">
                      Unavailable
                    </span>
                  {/if}
                </div>
              </div>
            {/each}
          </div>
        {/if}
      </section>
    {/if}
  </main>
</div>

<style>
  .page-description {
    max-width: 740px;
    margin: -8px 0 18px;
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.6;
  }

  .publish-version-button {
    border-color: var(--brand);
    background: var(--brand);
    color: var(--brand-ink);
  }

  .publish-version-button:hover:not(:disabled) {
    border-color: var(--brand-soft);
    background: var(--brand-soft);
  }

  .publish-version-button.secondary-button {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-soft);
  }

  .publish-version-button.secondary-button:hover:not(:disabled) {
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

  /* Publication form */

  .publish-section {
    display: grid;
    gap: 17px;
    margin-bottom: 18px;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    padding: 18px;
  }

  .publish-heading {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 16px;
  }

  .publish-heading > div {
    display: grid;
    gap: 4px;
  }

  .publish-heading h2 {
    font-size: 14px;
  }

  .publish-heading p {
    color: var(--text-muted);
    font-size: 12px;
    line-height: 1.5;
  }

  .publish-heading strong {
    color: var(--text-soft);
    font-weight: 600;
  }

  .publish-form {
    display: grid;
    grid-template-columns:
      minmax(0, 1fr)
      minmax(180px, 0.45fr);
    gap: 14px;
  }

  .wide-field,
  .archive-preview,
  .publish-actions {
    grid-column: 1 / -1;
  }

  .wide-field textarea {
    min-height: 150px;
    resize: vertical;
    font-family: var(--font-mono);
    font-size: 11px;
    line-height: 1.55;
  }

  .archive-preview {
    display: flex;
    min-width: 0;
    align-items: baseline;
    gap: 9px;
    color: var(--text-muted);
    font-size: 11px;
  }

  .archive-preview code {
    min-width: 0;
    color: var(--text-soft);
    font-size: 10.5px;
    overflow-wrap: anywhere;
  }

  .publish-actions {
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    border-top: 1px solid var(--line-soft);
    padding-top: 14px;
  }

  /* Main layout */

  .versions-layout {
    display: grid;
    grid-template-columns: 270px minmax(0, 1fr);
    gap: 16px;
    align-items: start;
  }

  /* Package directory */

  .package-directory {
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

  .package-search {
    position: relative;
    padding: 10px;
    border-bottom: 1px solid var(--line-soft);
  }

  .package-search svg,
  .version-search svg {
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

  .package-search svg {
    left: 21px;
  }

  .package-search input {
    min-height: 34px;
    padding-left: 33px;
    font-size: 12px;
  }

  .package-options {
    display: grid;
    max-height: calc(100vh - 220px);
    overflow-y: auto;
  }

  .package-option {
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

  .package-option:last-child {
    border-bottom: 0;
  }

  .package-option:hover:not(:disabled) {
    background: var(--bg-elevated);
  }

  .package-option.selected {
    background: var(--brand-faint);
    box-shadow: inset 2px 0 0 var(--brand);
  }

  .package-option__mark {
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

  .package-option.selected .package-option__mark {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-bright);
  }

  .package-option__content {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .package-option__content strong {
    overflow: hidden;
    color: var(--text);
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 600;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .package-option.selected .package-option__content strong {
    color: var(--brand-bright);
  }

  .package-option__content span {
    color: var(--text-muted);
    font-size: 9.5px;
  }

  .directory-loading {
    padding: 18px 14px;
    color: var(--text-muted);
    font-size: 12px;
  }

  .directory-empty {
    padding: 10px;
  }

  /* Package overview */

  .version-detail {
    display: grid;
    min-width: 0;
    gap: 16px;
  }

  .package-overview,
  .detail-section {
    min-width: 0;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    overflow: hidden;
  }

  .package-overview__header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    padding: 16px;
    border-bottom: 1px solid var(--line-soft);
  }

  .package-identity {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 11px;
  }

  .package-mark {
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

  .package-identity > div {
    display: grid;
    min-width: 0;
    gap: 4px;
  }

  .package-name-line {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 8px;
    flex-wrap: wrap;
  }

  .package-name-line h2 {
    color: var(--text);
    font-family: var(--font-mono);
    font-size: 14px;
    font-weight: 600;
    overflow-wrap: anywhere;
  }

  .package-identity p {
    color: var(--text-muted);
    font-size: 11px;
    line-height: 1.45;
  }

  .visibility-badge {
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

  .visibility-badge.public {
    border-color: var(--info-line);
    background: var(--info-faint);
    color: var(--info);
  }

  .package-link,
  .repository-row a,
  .archive-cell a {
    color: var(--link);
    font-weight: 550;
  }

  .package-link:hover,
  .repository-row a:hover,
  .archive-cell a:hover {
    color: var(--link-hover);
  }

  .package-link {
    display: inline-flex;
    flex: 0 0 auto;
    align-items: center;
    gap: 5px;
    font-size: 11.5px;
  }

  .package-link svg,
  .archive-cell svg {
    width: 15px;
    height: 15px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .package-summary {
    display: grid;
    grid-template-columns: repeat(4, minmax(0, 1fr));
    margin: 0;
    border-bottom: 1px solid var(--line-soft);
  }

  .package-summary > div {
    display: grid;
    min-width: 0;
    gap: 4px;
    padding: 12px 14px;
    border-right: 1px solid var(--line-soft);
  }

  .package-summary > div:last-child {
    border-right: 0;
  }

  .package-summary dt {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .package-summary dd {
    margin: 0;
    color: var(--text-soft);
    font-size: 12px;
    font-weight: 550;
    overflow-wrap: anywhere;
  }

  .repository-row {
    display: grid;
    grid-template-columns: auto minmax(0, 1fr);
    gap: 12px;
    align-items: center;
    padding: 10px 14px;
  }

  .repository-row > span {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .repository-row a {
    min-width: 0;
    overflow: hidden;
    font-family: var(--font-mono);
    font-size: 10.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  /* Version toolbar */

  .version-toolbar {
    display: grid;
    grid-template-columns:
      minmax(220px, 1fr)
      minmax(150px, 210px);
    gap: 10px;
    padding: 10px 12px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .version-search {
    position: relative;
  }

  .version-search svg {
    left: 11px;
  }

  .version-search input {
    min-height: 34px;
    padding-left: 33px;
    font-size: 12px;
  }

  .version-toolbar select {
    min-height: 34px;
    font-size: 12px;
  }

  /* Version table */

  .table-heading,
  .version-row {
    display: grid;
    grid-template-columns:
      minmax(120px, 0.55fr)
      minmax(230px, 1.15fr)
      minmax(90px, 0.4fr)
      minmax(100px, 0.45fr)
      minmax(100px, 0.45fr);
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

  .version-row {
    min-height: 64px;
    padding: 10px 16px;
    border-bottom: 1px solid var(--line-soft);
  }

  .version-row:last-child {
    border-bottom: 0;
  }

  .version-row:hover {
    background: rgba(255, 255, 255, 0.018);
  }

  .version-identity {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .version-identity strong {
    color: var(--text);
    font-family: var(--font-mono);
    font-size: 12.5px;
    font-weight: 600;
  }

  .version-identity small {
    color: var(--text-muted);
    font-size: 10px;
  }

  .checksum-cell {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 7px;
  }

  .checksum-cell code {
    min-width: 0;
    overflow: hidden;
    color: var(--text-soft);
    font-size: 9.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .copy-checksum-button {
    display: grid;
    width: 27px;
    height: 27px;
    min-height: 27px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--line-strong);
    background: transparent;
    color: var(--text-muted);
    padding: 0;
  }

  .copy-checksum-button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
  }

  .copy-checksum-button.copied {
    border-color: var(--brand-line);
    color: var(--brand);
  }

  .copy-checksum-button svg {
    width: 13px;
    height: 13px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .size-cell {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 10.5px;
  }

  .archive-cell {
    display: flex;
    justify-content: flex-end;
  }

  .archive-cell a {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    font-size: 11px;
  }

  .unavailable {
    color: var(--text-faint);
    font-size: 10.5px;
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
    .table-heading,
    .version-row {
      grid-template-columns:
        minmax(110px, 0.55fr)
        minmax(190px, 1fr)
        minmax(80px, 0.4fr)
        minmax(90px, 0.45fr)
        minmax(90px, 0.45fr);
      gap: 11px;
    }
  }

  @media (max-width: 880px) {
    .versions-layout {
      grid-template-columns: 1fr;
    }

    .package-directory {
      position: static;
    }

    .package-options {
      max-height: 280px;
    }
  }

  @media (max-width: 760px) {
    .publish-form {
      grid-template-columns: 1fr;
    }

    .wide-field,
    .archive-preview,
    .publish-actions {
      grid-column: auto;
    }

    .package-overview__header {
      align-items: flex-start;
      flex-direction: column;
    }

    .package-summary {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }

    .package-summary > div:nth-child(2) {
      border-right: 0;
    }

    .package-summary > div:nth-child(-n + 2) {
      border-bottom: 1px solid var(--line-soft);
    }

    .table-heading {
      display: none;
    }

    .version-row {
      grid-template-columns: 1fr;
      gap: 10px;
      padding: 14px;
    }

    .version-identity[data-label],
    .checksum-cell[data-label],
    .size-cell[data-label],
    .status-cell[data-label],
    .archive-cell[data-label] {
      display: grid;
      grid-template-columns: 100px minmax(0, 1fr);
      align-items: center;
      gap: 10px;
    }

    .version-identity[data-label]::before,
    .checksum-cell[data-label]::before,
    .size-cell[data-label]::before,
    .status-cell[data-label]::before,
    .archive-cell[data-label]::before {
      content: attr(data-label);
      color: var(--text-muted);
      font-size: 10px;
    }

    .checksum-cell {
      display: grid;
    }

    .checksum-cell code {
      grid-column: 2;
      grid-row: 1;
    }

    .copy-checksum-button {
      grid-column: 3;
      grid-row: 1;
    }

    .archive-cell {
      justify-content: start;
    }
  }

  @media (max-width: 600px) {
    .version-toolbar {
      grid-template-columns: 1fr;
    }

    .publish-actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .publish-actions button {
      width: 100%;
    }

    .archive-preview {
      align-items: flex-start;
      flex-direction: column;
    }

    .repository-row {
      grid-template-columns: 1fr;
      gap: 4px;
    }
  }

  @media (max-width: 460px) {
    .publish-section {
      padding: 14px;
    }

    .package-summary {
      grid-template-columns: 1fr;
    }

    .package-summary > div,
    .package-summary > div:nth-child(2) {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .package-summary > div:last-child {
      border-bottom: 0;
    }

    .version-identity[data-label],
    .checksum-cell[data-label],
    .size-cell[data-label],
    .status-cell[data-label],
    .archive-cell[data-label] {
      grid-template-columns: 1fr;
      gap: 4px;
    }

    .checksum-cell code,
    .copy-checksum-button {
      grid-column: auto;
      grid-row: auto;
    }
  }
</style>
