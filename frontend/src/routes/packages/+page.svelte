<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import {
    archivePackage,
    changePackageVisibility,
    createPackage,
    deletePackage,
    listPackages,
    reactivatePackage
  } from '$lib/api/packages';
  import { listWorkspaces } from '$lib/api/workspaces';
  import {
    ApiError,
    type Package,
    type Workspace
  } from '$lib/api/types';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import { canCreatePackage } from '$lib/permissions';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  type PackageFilter =
    | 'all'
    | 'private'
    | 'public'
    | 'active'
    | 'archived';

  let workspaces: Workspace[] = [];
  let selectedWorkspaceId = '';
  let packages: Package[] = [];

  let loading = true;
  let saving = false;
  let busyPackageId = '';
  let initialized = false;
  let packageRequestId = 0;
  let error = '';
  let success = '';

  let showForm = false;
  let packageQuery = '';
  let packageFilter: PackageFilter = 'all';

  let name = '';
  let description = '';
  let repositoryUrl = '';
  let visibility: 'private' | 'public' = 'private';

  $: selectedWorkspace =
    workspaces.find(
      (workspace) => workspace.id === selectedWorkspaceId
    ) ?? null;

  $: currentRole =
    selectedWorkspace?.current_user_role ?? 'viewer';

  $: canCreate = canCreatePackage(currentRole);

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

  $: activePackages = packages.filter(
    (pkg) => pkg.active !== false
  );

  $: archivedPackages = packages.filter(
    (pkg) => pkg.active === false
  );

  $: privatePackages = packages.filter(
    (pkg) => pkg.visibility !== 'public'
  );

  $: publicPackages = packages.filter(
    (pkg) => pkg.visibility === 'public'
  );

  $: visiblePackages = packages.filter((pkg) => {
    const query = packageQuery.trim().toLowerCase();

    const matchesQuery =
      !query ||
      pkg.name.toLowerCase().includes(query) ||
      pkg.description?.toLowerCase().includes(query) ||
      pkg.repository_url?.toLowerCase().includes(query);

    if (!matchesQuery) {
      return false;
    }

    if (packageFilter === 'private') {
      return pkg.visibility !== 'public';
    }

    if (packageFilter === 'public') {
      return pkg.visibility === 'public';
    }

    if (packageFilter === 'active') {
      return pkg.active !== false;
    }

    if (packageFilter === 'archived') {
      return pkg.active === false;
    }

    return true;
  });

  function packageInitial(pkg: Package) {
    const value = pkg.name
      .split('/')
      .filter(Boolean)
      .at(-1);

    return (value || pkg.name || 'P')
      .slice(0, 1)
      .toUpperCase();
  }

  function visibilityLabel(value?: string) {
    return value === 'public'
      ? 'Public'
      : 'Private';
  }

  function packageStatus(pkg: Package) {
    return pkg.status || (pkg.active === false ? 'archived' : 'active');
  }

  function canWritePackages() {
    return canCreate;
  }

  function syncPackage(updated: Package) {
    packages = packages.map((pkg) =>
      pkg.id === updated.id ? updated : pkg
    );
  }

  function resetForm() {
    name = '';
    description = '';
    repositoryUrl = '';
    visibility = 'private';
  }

  function toggleForm() {
    showForm = !showForm;
    error = '';
    success = '';

    if (!showForm) {
      resetForm();
    }
  }

  function closeForm() {
    if (saving) {
      return;
    }

    showForm = false;
    resetForm();
    error = '';
  }

  function handleSubmit(event: SubmitEvent) {
    event.preventDefault();
    void submitPackage();
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
      const workspaceData = await listWorkspaces(user.id);

      workspaces = workspaceData.workspaces;

      const requestedWorkspaceId =
        new URLSearchParams(
          window.location.search
        ).get('workspace_id');

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

      await loadPackages(selectedWorkspaceId);
      initialized = true;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load packages.';
    } finally {
      loading = false;
    }
  }

  async function loadPackages(workspaceId: string) {
    const requestId = ++packageRequestId;

    if (!workspaceId) {
      packages = [];
      return;
    }

    workspaceContext.setSelectedWorkspace(workspaceId);

    const data = await listPackages(workspaceId);

    if (
      requestId !== packageRequestId ||
      workspaceId !== selectedWorkspaceId
    ) {
      return;
    }

    packages = data.packages;
  }

  async function switchWorkspace(workspaceId: string) {
    selectedWorkspaceId = workspaceId;
    packages = [];
    showForm = false;
    packageQuery = '';
    packageFilter = 'all';
    error = '';
    success = '';
    loading = true;

    const url = new URL(window.location.href);
    url.searchParams.set('workspace_id', workspaceId);
    history.replaceState(null, '', `${url.pathname}${url.search}`);

    try {
      await loadPackages(workspaceId);
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load packages.';
    } finally {
      loading = false;
    }
  }

  async function handlePackageAction(event: MouseEvent, pkg: Package, action: 'archive' | 'reactivate' | 'delete' | 'public' | 'private') {
    event.preventDefault();
    event.stopPropagation();

    if (!selectedWorkspaceId || !canWritePackages()) {
      return;
    }

    const message =
      action === 'delete'
        ? `Delete package ${pkg.name}? Versions and archives are kept for safety.`
        : action === 'archive'
          ? `Archive package ${pkg.name}?`
          : action === 'reactivate'
            ? `Reactivate package ${pkg.name}?`
            : action === 'public'
              ? `Make package ${pkg.name} public?`
              : `Make package ${pkg.name} private? It will disappear from public profiles and pins.`;

    if (!window.confirm(message)) {
      return;
    }

    busyPackageId = pkg.id;
    error = '';
    success = '';

    try {
      const updated =
        action === 'archive'
          ? await archivePackage(selectedWorkspaceId, pkg.id)
          : action === 'reactivate'
            ? await reactivatePackage(selectedWorkspaceId, pkg.id)
            : action === 'delete'
              ? await deletePackage(selectedWorkspaceId, pkg.id)
              : await changePackageVisibility(
                  selectedWorkspaceId,
                  pkg.id,
                  action === 'public' ? 'public' : 'private'
                );

      if (action === 'delete') {
        packages = packages.filter((item) => item.id !== pkg.id);
      } else {
        syncPackage(updated.package);
      }

      success =
        action === 'archive'
          ? 'Package archived.'
          : action === 'reactivate'
            ? 'Package reactivated.'
            : action === 'delete'
              ? 'Package deleted.'
              : 'Package visibility changed.';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to update package.';
    } finally {
      busyPackageId = '';
    }
  }

  async function submitPackage() {
    const user = $auth.user;
    const packageName = name.trim();

    if (
      !user ||
      !selectedWorkspaceId ||
      !packageName ||
      !canCreate
    ) {
      return;
    }

    saving = true;
    error = '';
    success = '';

    try {
      const created = await createPackage({
        workspaceId: selectedWorkspaceId,
        ownerUserId: user.id,
        name: packageName,
        description: description.trim(),
        repositoryUrl: repositoryUrl.trim(),
        visibility
      });

      packages = [
        created.package,
        ...packages
      ];

      success = `Package "${created.package.name}" created.`;

      resetForm();
      showForm = false;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to create package.';
    } finally {
      saving = false;
    }
  }

  onMount(load);
</script>

<svelte:head>
  <title>Packages | Softadastra Cloud</title>
</svelte:head>

<PageHeader
  eyebrow="Registry"
  title="Packages"
  workspaceName={selectedWorkspace?.name ?? ''}
  role={currentRole}
>
  {#if canCreate && !loading}
    <button
      class:secondary-button={showForm}
      class="new-package-button"
      type="button"
      onclick={toggleForm}
    >
      {showForm ? 'Close' : 'New package'}
    </button>
  {/if}
</PageHeader>

<p class="page-description">
  Manage the packages published in this workspace and open their
  version history.
</p>

<InlineError message={error} />

{#if success}
  <p class="success-message" role="status">
    {success}
  </p>
{/if}

{#if showForm && canCreate}
  <section
    class="package-form-section"
    aria-labelledby="new-package-title"
  >
    <div class="form-heading">
      <div>
        <h2 id="new-package-title">
          Create a package
        </h2>

        <p>
          Register the package before publishing its first version.
        </p>
      </div>
    </div>

    <form
      class="package-form"
      onsubmit={handleSubmit}
    >
      <label>
        Package name

        <input
          bind:value={name}
          placeholder="vix/http"
          required
          autocomplete="off"
        />
      </label>

      <label>
        Visibility

        <select bind:value={visibility}>
          <option value="private">
            Private
          </option>

          <option value="public">
            Public
          </option>
        </select>
      </label>

      <label class="wide-field">
        Description

        <input
          bind:value={description}
          placeholder="HTTP client and server utilities for Vix"
          autocomplete="off"
        />
      </label>

      <label class="wide-field">
        Repository URL

        <input
          bind:value={repositoryUrl}
          type="url"
          placeholder="https://github.com/vixcpp/vix"
          autocomplete="off"
        />
      </label>

      <div class="package-preview">
        <span>Package identifier</span>
        <code>{name.trim() || 'namespace/package'}</code>
      </div>

      <div class="form-actions">
        <button
          class="secondary-button"
          type="button"
          disabled={saving}
          onclick={closeForm}
        >
          Cancel
        </button>

        <button
          type="submit"
          disabled={
            saving ||
            !selectedWorkspaceId ||
            !name.trim()
          }
        >
          {saving
            ? 'Creating…'
            : 'Create package'}
        </button>
      </div>
    </form>
  </section>
{/if}

{#if !canCreate && selectedWorkspace}
  <ReadOnlyNotice
    message="Your role can view packages in this workspace, but cannot create or edit them."
  />
{/if}

<section
  class="registry-summary"
  aria-label="Package registry summary"
>
  <div class="summary-item">
    <span>Total packages</span>
    <strong>{packages.length}</strong>
    <small>Current workspace</small>
  </div>

  <div class="summary-item">
    <span>Active</span>
    <strong>{activePackages.length}</strong>
    <small>Available packages</small>
  </div>

  <div class="summary-item">
    <span>Private</span>
    <strong>{privatePackages.length}</strong>
    <small>Workspace access</small>
  </div>

  <div class="summary-item">
    <span>Public</span>
    <strong>{publicPackages.length}</strong>
    <small>Public registry</small>
  </div>
</section>

<section
  class="packages-section"
  aria-labelledby="packages-title"
>
  <div class="section-header">
    <div>
      <h2 id="packages-title">
        Package registry
      </h2>

      <p>
        Packages available in
        {selectedWorkspace?.name ?? 'the current workspace'}.
      </p>
    </div>

    <span class="section-count">
      {visiblePackages.length}
    </span>
  </div>

  {#if packages.length > 0}
    <div class="package-toolbar">
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

      <label class="filter-field">
        <span>Filter</span>

        <select
          bind:value={packageFilter}
          aria-label="Filter packages"
        >
          <option value="all">
            All packages
          </option>

          <option value="active">
            Active
          </option>

          <option value="archived">
            Archived
          </option>

          <option value="private">
            Private
          </option>

          <option value="public">
            Public
          </option>
        </select>
      </label>
    </div>
  {/if}

  {#if loading}
    <p class="loading-state">
      Loading packages…
    </p>
  {:else if packages.length === 0}
    <div class="empty-wrapper">
      <EmptyState
        title="No packages yet"
        body={canCreate
          ? 'Create a package, then publish its first version with vix publish --cloud.'
          : 'No packages have been published in this workspace.'}
      />
    </div>
  {:else if visiblePackages.length === 0}
    <div class="no-results">
      <strong>No matching packages</strong>
      <p>Change the search text or selected filter.</p>
    </div>
  {:else}
    <div class="package-table">
      <div class="table-heading">
        <span>Package</span>
        <span>Repository</span>
        <span>Visibility</span>
        <span>Status</span>
        <span>Actions</span>
      </div>

      {#each visiblePackages as pkg (pkg.id)}
        <a
          class="package-row"
          href={`/package-versions?workspace_id=${pkg.workspace_id}&package_id=${pkg.id}`}
        >
          <div class="package-identity">
            <span
              class="package-mark"
              aria-hidden="true"
            >
              {packageInitial(pkg)}
            </span>

            <div>
              <strong>{pkg.name}</strong>

              {#if pkg.description}
                <p>{pkg.description}</p>
              {:else}
                <p>No description provided.</p>
              {/if}
            </div>
          </div>

          <div
            class="repository-cell"
            data-label="Repository"
          >
            {#if pkg.repository_url}
              <span title={pkg.repository_url}>
                {pkg.repository_url}
              </span>
            {:else}
              <span class="empty-value">
                Not connected
              </span>
            {/if}
          </div>

          <div
            class="visibility-cell"
            data-label="Visibility"
          >
            <span
              class:public={pkg.visibility === 'public'}
              class="visibility-badge"
            >
              {visibilityLabel(pkg.visibility)}
            </span>
          </div>

          <div
            class="status-cell"
            data-label="Status"
          >
            <StatusBadge status={packageStatus(pkg)} />
          </div>

          <div class="package-row-actions">
            {#if canWritePackages()}
              <button
                type="button"
                disabled={busyPackageId === pkg.id}
                onclick={(event) =>
                  handlePackageAction(
                    event,
                    pkg,
                    pkg.visibility === 'public' ? 'private' : 'public'
                  )}
              >
                {pkg.visibility === 'public' ? 'Make private' : 'Make public'}
              </button>

              <button
                type="button"
                disabled={busyPackageId === pkg.id}
                onclick={(event) =>
                  handlePackageAction(
                    event,
                    pkg,
                    packageStatus(pkg) === 'archived' ? 'reactivate' : 'archive'
                  )}
              >
                {packageStatus(pkg) === 'archived' ? 'Reactivate' : 'Archive'}
              </button>

              <button
                class="danger-link"
                type="button"
                disabled={busyPackageId === pkg.id}
                onclick={(event) => handlePackageAction(event, pkg, 'delete')}
              >
                Delete
              </button>
            {/if}
          </div>
        </a>
      {/each}
    </div>
  {/if}
</section>

{#if archivedPackages.length > 0}
  <p class="registry-footnote">
    {archivedPackages.length}
    {archivedPackages.length === 1
      ? ' archived package is hidden by the Active filter.'
      : ' archived packages are hidden by the Active filter.'}
  </p>
{/if}

<style>
  .page-description {
    max-width: 720px;
    margin: -8px 0 18px;
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.6;
  }

  .new-package-button {
    border-color: var(--brand);
    background: var(--brand);
    color: var(--brand-ink);
  }

  .new-package-button:hover:not(:disabled) {
    border-color: var(--brand-soft);
    background: var(--brand-soft);
  }

  .new-package-button.secondary-button {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-soft);
  }

  .new-package-button.secondary-button:hover:not(:disabled) {
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

  /* Package form */

  .package-form-section {
    display: grid;
    gap: 17px;
    margin-bottom: 18px;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    padding: 18px;
  }

  .form-heading {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 16px;
  }

  .form-heading > div {
    display: grid;
    gap: 4px;
  }

  .form-heading h2 {
    font-size: 14px;
  }

  .form-heading p {
    color: var(--text-muted);
    font-size: 12px;
  }

  .package-form {
    display: grid;
    grid-template-columns:
      minmax(0, 1.4fr)
      minmax(180px, 0.6fr);
    gap: 14px;
  }

  .wide-field,
  .package-preview,
  .form-actions {
    grid-column: 1 / -1;
  }

  .package-preview {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 8px;
    color: var(--text-muted);
    font-size: 11.5px;
  }

  .package-preview code {
    min-width: 0;
    color: var(--text-soft);
    overflow-wrap: anywhere;
  }

  .form-actions {
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    border-top: 1px solid var(--line-soft);
    padding-top: 14px;
  }

  /* Summary */

  .registry-summary {
    display: grid;
    grid-template-columns: repeat(4, minmax(0, 1fr));
    margin-bottom: 16px;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    overflow: hidden;
  }

  .summary-item {
    display: grid;
    min-width: 0;
    gap: 3px;
    padding: 14px 16px;
    border-right: 1px solid var(--line-soft);
  }

  .summary-item:last-child {
    border-right: 0;
  }

  .summary-item > span {
    color: var(--text-muted);
    font-size: 11px;
  }

  .summary-item > strong {
    color: var(--text);
    font-family: var(--font-mono);
    font-size: 19px;
    font-weight: 600;
  }

  .summary-item > small {
    color: var(--text-faint);
    font-size: 10.5px;
  }

  /* Package registry */

  .packages-section {
    min-width: 0;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    overflow: hidden;
  }

  .section-header {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 16px;
    padding: 14px 16px;
    border-bottom: 1px solid var(--line-soft);
  }

  .section-header > div {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .section-header h2 {
    color: var(--text);
    font-size: 13.5px;
  }

  .section-header p {
    color: var(--text-muted);
    font-size: 11.5px;
    line-height: 1.5;
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

  .package-toolbar {
    display: grid;
    grid-template-columns:
      minmax(220px, 1fr)
      minmax(160px, 220px);
    gap: 10px;
    align-items: end;
    padding: 10px 12px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .package-search {
    position: relative;
  }

  .package-search svg {
    position: absolute;
    top: 50%;
    left: 11px;
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

  .package-search input {
    min-height: 34px;
    padding-left: 33px;
    font-size: 12px;
  }

  .filter-field {
    display: grid;
    gap: 5px;
  }

  .filter-field > span {
    position: absolute;
    width: 1px;
    height: 1px;
    overflow: hidden;
    clip: rect(0 0 0 0);
  }

  .filter-field select {
    min-height: 34px;
    font-size: 12px;
  }

  /* Table */

  .package-table {
    min-width: 0;
  }

  .table-heading,
  .package-row {
    display: grid;
    grid-template-columns:
      minmax(260px, 1.35fr)
      minmax(180px, 0.85fr)
      minmax(95px, 0.4fr)
      minmax(90px, 0.4fr)
      minmax(220px, 0.8fr);
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

  .package-row {
    min-height: 68px;
    padding: 10px 16px;
    border-bottom: 1px solid var(--line-soft);
    color: var(--text-muted);
    transition: background var(--speed) var(--ease);
  }

  .package-row:last-child {
    border-bottom: 0;
  }

  .package-row:hover {
    background: var(--info-faint);
  }

  .package-identity {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 11px;
  }

  .package-mark {
    display: grid;
    width: 34px;
    height: 34px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    background: var(--bg-elevated);
    color: var(--text-soft);
    font-size: 12px;
    font-weight: 650;
  }

  .package-identity > div {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .package-identity strong {
    color: var(--link);
    font-family: var(--font-mono);
    font-size: 12.5px;
    font-weight: 600;
    overflow-wrap: anywhere;
    transition: color var(--speed) var(--ease);
  }

  .package-row:hover .package-identity strong {
    color: var(--link-hover);
  }

  .package-identity p {
    display: -webkit-box;
    color: var(--text-muted);
    font-size: 10.5px;
    line-height: 1.45;
    overflow: hidden;
    line-clamp: 2;
    -webkit-line-clamp: 2;
    -webkit-box-orient: vertical;
  }

  .repository-cell {
    min-width: 0;
  }

  .repository-cell span {
    display: block;
    overflow: hidden;
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 9.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .repository-cell .empty-value {
    color: var(--text-faint);
    font-family: inherit;
  }

  .visibility-badge {
    display: inline-flex;
    width: fit-content;
    min-height: 21px;
    align-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    background: var(--bg-elevated);
    color: var(--text-muted);
    padding: 0 8px;
    font-size: 10.5px;
  }

  .visibility-badge.public {
    border-color: var(--info-line);
    background: var(--info-faint);
    color: var(--info);
  }

  .loading-state {
    min-height: 90px;
    padding: 22px 16px;
    color: var(--text-muted);
    font-size: 12px;
  }

  .empty-wrapper {
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

  .registry-footnote {
    margin-top: 10px;
    color: var(--text-faint);
    font-size: 10.5px;
  }


  .package-row-actions {
    display: flex;
    flex-wrap: wrap;
    justify-content: flex-end;
    gap: 6px;
  }

  .package-row-actions button {
    min-height: 28px;
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
    background: var(--bg-elevated);
    padding: 0 8px;
    color: var(--text-strong);
    font-size: 10.5px;
    font-weight: 700;
    cursor: pointer;
  }

  .package-row-actions button:hover:not(:disabled) {
    border-color: var(--line-strong);
    background: var(--bg-panel);
  }

  .package-row-actions button:disabled {
    cursor: wait;
    opacity: 0.6;
  }

  .package-row-actions .danger-link {
    border-color: color-mix(in srgb, var(--danger) 45%, var(--line));
    color: var(--danger);
  }

  @media (max-width: 960px) {
    .table-heading,
    .package-row {
      grid-template-columns:
        minmax(240px, 1.3fr)
        minmax(150px, 0.8fr)
        minmax(90px, 0.4fr)
        minmax(85px, 0.4fr)
        minmax(200px, 0.8fr);
      gap: 12px;
    }
  }

  @media (max-width: 780px) {
    .registry-summary {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }

    .summary-item:nth-child(2) {
      border-right: 0;
    }

    .summary-item:nth-child(-n + 2) {
      border-bottom: 1px solid var(--line-soft);
    }

    .table-heading {
      display: none;
    }

    .package-row {
      grid-template-columns: minmax(0, 1fr);
      gap: 12px;
      padding: 14px;
    }

    .repository-cell,
    .visibility-cell,
    .status-cell,
    .package-row-actions {
      grid-column: 1 / -1;
      display: grid;
      grid-template-columns: 94px minmax(0, 1fr);
      align-items: center;
      gap: 10px;
      padding-left: 45px;
    }

    .repository-cell::before,
    .visibility-cell::before,
    .status-cell::before {
      content: attr(data-label);
      color: var(--text-muted);
      font-size: 10px;
    }

    .package-row-actions {
      justify-content: flex-start;
      padding-left: 45px;
    }
  }

  @media (max-width: 620px) {
    .package-form {
      grid-template-columns: 1fr;
    }

    .wide-field,
    .package-preview,
    .form-actions {
      grid-column: auto;
    }

    .package-toolbar {
      grid-template-columns: 1fr;
    }

    .form-actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .form-actions button {
      width: 100%;
    }
  }

  @media (max-width: 460px) {
    .registry-summary {
      grid-template-columns: 1fr;
    }

    .summary-item {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .summary-item:last-child {
      border-bottom: 0;
    }

    .repository-cell,
    .visibility-cell,
    .status-cell,
    .package-row-actions {
      grid-template-columns: 1fr;
      gap: 4px;
      padding-left: 45px;
    }

    .package-form-section {
      padding: 14px;
    }
  }
</style>
