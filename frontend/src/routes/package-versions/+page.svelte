<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount, tick } from 'svelte';

  import { usePageRefresh } from '$lib/stores/pageRefresh';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  import {
    deletePackageVersion,
    deprecatePackageVersion,
    listPackageVersions,
    publishPackageVersion,
    unyankPackageVersion,
    yankPackageVersion
  } from '$lib/api/packageVersions';

  import { listPackages } from '$lib/api/packages';
  import { listWorkspaces } from '$lib/api/workspaces';

  import {
    ApiError,
    type Package,
    type PackageVersion,
    type Workspace
  } from '$lib/api/types';

  import InlineError from '$lib/components/InlineError.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import WorkspaceAvatar from '$lib/components/WorkspaceAvatar.svelte';

  import { canPublishPackageVersion } from '$lib/permissions';

  type VersionFilter =
    | 'all'
    | 'available'
    | 'deprecated'
    | 'yanked';

  type VersionAction =
    | 'yank'
    | 'unyank'
    | 'deprecate'
    | 'delete';

  type Confirmation = {
    item: PackageVersion;
    action: VersionAction;
  };

  let workspaces: Workspace[] = [];
  let packages: Package[] = [];
  let versions: PackageVersion[] = [];

  let selectedWorkspaceId = '';
  let selectedPackageId = '';

  let loading = true;
  let loadingVersions = false;
  let initialized = false;

  let packageListRequestId = 0;
  let versionRequestId = 0;

  let saving = false;
  let busyVersionId = '';

  let error = '';
  let success = '';

  let packageQuery = '';
  let versionQuery = '';
  let versionFilter: VersionFilter = 'all';

  let showPublishForm = false;

  let version = '';
  let checksumSha256 = '';
  let archiveUrl = '';
  let manifestJson = '{}';
  let sizeBytes = 0;

  let copiedChecksumId = '';

  let copyResetTimer:
    | ReturnType<typeof setTimeout>
    | undefined;

  let confirmation: Confirmation | null = null;
  let confirmationDialog: HTMLDialogElement | null = null;

  let deprecationMessage = '';
  let deleteConfirmation = '';

  $: selectedWorkspace =
    workspaces.find(
      (workspace) =>
        workspace.id === selectedWorkspaceId
    ) ?? null;

  $: selectedPackage =
    packages.find(
      (pkg) =>
        pkg.id === selectedPackageId
    ) ?? null;

  $: currentRole =
    selectedWorkspace?.current_user_role ??
    'viewer';

  $: canPublish =
    canPublishPackageVersion(currentRole);

  $: canPublishCurrent =
    canPublish &&
    selectedPackage?.active !== false;

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

  $: visiblePackages = packages.filter(
    (pkg) => {
      const query =
        packageQuery.trim().toLowerCase();

      if (!query) {
        return true;
      }

      return (
        pkg.name
          .toLowerCase()
          .includes(query) ||
        pkg.description
          ?.toLowerCase()
          .includes(query) ||
        pkg.repository_url
          ?.toLowerCase()
          .includes(query)
      );
    }
  );

  $: availableVersions = versions.filter(
    (item) =>
      item.status !== 'yanked' &&
      item.status !== 'deprecated'
  );

  $: deprecatedVersions = versions.filter(
    (item) =>
      item.status === 'deprecated'
  );

  $: yankedVersions = versions.filter(
    (item) =>
      item.status === 'yanked'
  );

  $: visibleVersions = versions.filter(
    (item) => {
      const query =
        versionQuery.trim().toLowerCase();

      const matchesQuery =
        !query ||
        item.version
          .toLowerCase()
          .includes(query) ||
        item.checksum_sha256
          .toLowerCase()
          .includes(query) ||
        item.archive_url
          ?.toLowerCase()
          .includes(query);

      if (!matchesQuery) {
        return false;
      }

      if (versionFilter === 'available') {
        return (
          item.status !== 'yanked' &&
          item.status !== 'deprecated'
        );
      }

      if (versionFilter === 'deprecated') {
        return item.status === 'deprecated';
      }

      if (versionFilter === 'yanked') {
        return item.status === 'yanked';
      }

      return true;
    }
  );

  $: totalSize = versions.reduce(
    (total, item) =>
      total +
      Number(item.size_bytes || 0),
    0
  );

  $: latestVersion =
    versions[0] ?? null;

  $: confirmationNeedsVersion =
    confirmation?.action === 'delete';

  $: confirmationNeedsMessage =
    confirmation?.action === 'deprecate';

  $: confirmationCanContinue =
    Boolean(confirmation) &&
    (
      !confirmationNeedsVersion ||
      deleteConfirmation ===
        confirmation?.item.version
    ) &&
    (
      !confirmationNeedsMessage ||
      Boolean(
        deprecationMessage.trim()
      )
    );

  function packageInitial(
    pkg: Package
  ) {
    const segment = pkg.name
      .split('/')
      .filter(Boolean)
      .at(-1);

    return (
      segment ||
      pkg.name ||
      'P'
    )
      .slice(0, 1)
      .toUpperCase();
  }

  function visibilityLabel(
    value?: string
  ) {
    return value === 'public'
      ? 'Public'
      : 'Private';
  }

  function versionStatus(
    item: PackageVersion
  ) {
    return item.status || 'published';
  }

  function versionIsYanked(
    item: PackageVersion
  ) {
    return versionStatus(item) === 'yanked';
  }

  function versionIsDeprecated(
    item: PackageVersion
  ) {
    return versionStatus(item) === 'deprecated';
  }

  function shortChecksum(
    value: string
  ) {
    if (value.length <= 24) {
      return value;
    }

    return (
      `${value.slice(0, 13)}…` +
      value.slice(-8)
    );
  }

  function formatBytes(
    value: number
  ) {
    if (
      !Number.isFinite(value) ||
      value <= 0
    ) {
      return '0 B';
    }

    const units = [
      'B',
      'KB',
      'MB',
      'GB'
    ];

    const unitIndex = Math.min(
      Math.floor(
        Math.log(value) /
        Math.log(1024)
      ),
      units.length - 1
    );

    const size =
      value /
      Math.pow(1024, unitIndex);

    return (
      `${size >= 10
        ? size.toFixed(0)
        : size.toFixed(1)} ` +
      units[unitIndex]
    );
  }

  function repositoryLabel(
    value: string
  ) {
    try {
      const url = new URL(value);

      return (
        url.hostname +
        url.pathname.replace(/\/$/, '')
      );
    } catch {
      return value;
    }
  }

  function archiveLabel(
    value: string
  ) {
    try {
      const url = new URL(value);

      return (
        url.hostname +
        url.pathname
          .split('/')
          .slice(-2)
          .join('/')
      );
    } catch {
      const parts =
        value.split('/');

      return parts
        .slice(-3)
        .join('/');
    }
  }

  function syncVersion(
    updated: PackageVersion
  ) {
    versions = versions.map(
      (item) =>
        item.id === updated.id
          ? updated
          : item
    );
  }

  function updatePageUrl(
    packageId = ''
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

    if (packageId) {
      url.searchParams.set(
        'package_id',
        packageId
      );
    } else {
      url.searchParams.delete(
        'package_id'
      );
    }

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

  function openPublishForm() {
    if (!canPublishCurrent) {
      return;
    }

    showPublishForm = true;
    error = '';
    success = '';
  }

  function closePublishForm() {
    if (saving) {
      return;
    }

    showPublishForm = false;
    resetPublishForm();
    error = '';
  }

  function togglePublishForm() {
    if (showPublishForm) {
      closePublishForm();
      return;
    }

    openPublishForm();
  }

  async function copyChecksum(
    item: PackageVersion
  ) {
    try {
      await navigator.clipboard.writeText(
        item.checksum_sha256
      );

      copiedChecksumId =
        item.id;

      if (copyResetTimer) {
        clearTimeout(
          copyResetTimer
        );
      }

      copyResetTimer = setTimeout(
        () => {
          copiedChecksumId = '';
        },
        1800
      );
    } catch {
      error =
        'Unable to copy the checksum.';
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

      await loadPackages(
        params.get('package_id')
      );

      initialized = true;
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
    const requestId =
      ++packageListRequestId;

    const workspaceId =
      selectedWorkspaceId;

    if (!workspaceId) {
      packages = [];
      versions = [];
      selectedPackageId = '';

      updatePageUrl();
      return;
    }

    workspaceContext.setSelectedWorkspace(
      workspaceId
    );

    const data =
      await listPackages(workspaceId);

    if (
      requestId !== packageListRequestId ||
      selectedWorkspaceId !== workspaceId
    ) {
      return;
    }

    packages = data.packages;

    selectedPackageId =
      requestedPackageId &&
      packages.some(
        (pkg) =>
          pkg.id === requestedPackageId
      )
        ? requestedPackageId
        : packages[0]?.id ?? '';

    updatePageUrl(
      selectedPackageId
    );

    await loadVersions(
      selectedPackageId
    );
  }

  async function switchWorkspace(
    workspaceId: string
  ) {
    if (
      workspaceId ===
      selectedWorkspaceId
    ) {
      return;
    }

    selectedWorkspaceId =
      workspaceId;

    selectedPackageId = '';

    packages = [];
    versions = [];

    packageQuery = '';
    versionQuery = '';
    versionFilter = 'all';

    copiedChecksumId = '';

    showPublishForm = false;
    resetPublishForm();

    confirmation = null;

    error = '';
    success = '';
    loading = true;

    workspaceContext.setSelectedWorkspace(
      workspaceId
    );

    updatePageUrl();

    try {
      await loadPackages();
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load package versions.';
    } finally {
      loading = false;
    }
  }

  async function selectPackage(
    pkg: Package
  ) {
    if (
      pkg.id === selectedPackageId
    ) {
      return;
    }

    selectedPackageId =
      pkg.id;

    versionQuery = '';
    versionFilter = 'all';

    copiedChecksumId = '';

    showPublishForm = false;
    resetPublishForm();

    confirmation = null;

    error = '';
    success = '';

    updatePageUrl(pkg.id);

    await loadVersions(pkg.id);
  }

  async function loadVersions(
    packageId: string
  ) {
    if (
      !selectedWorkspaceId ||
      !packageId
    ) {
      versions = [];
      return;
    }

    const requestId =
      ++versionRequestId;

    const workspaceId =
      selectedWorkspaceId;

    loadingVersions = true;
    error = '';
    versions = [];

    try {
      const data =
        await listPackageVersions(
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

      versions =
        data.package_versions;
    } catch (err) {
      if (
        requestId !== versionRequestId
      ) {
        return;
      }

      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load versions.';
    } finally {
      if (
        requestId === versionRequestId
      ) {
        loadingVersions = false;
      }
    }
  }

  async function submitVersion() {
    const user = $auth.user;

    const cleanVersion =
      version.trim();

    const cleanChecksum =
      checksumSha256.trim();

    const cleanArchiveUrl =
      archiveUrl.trim();

    if (
      !user ||
      !selectedWorkspaceId ||
      !selectedPackageId ||
      !cleanVersion ||
      !canPublishCurrent
    ) {
      return;
    }

    if (
      !/^[a-fA-F0-9]{64}$/.test(
        cleanChecksum
      )
    ) {
      error =
        'Checksum SHA256 must contain exactly 64 hexadecimal characters.';

      return;
    }

    let normalizedManifest = '';

    try {
      normalizedManifest =
        JSON.stringify(
          JSON.parse(
            manifestJson || '{}'
          )
        );
    } catch {
      error =
        'Manifest JSON is not valid.';

      return;
    }

    saving = true;
    error = '';
    success = '';

    try {
      const created =
        await publishPackageVersion({
          workspaceId:
            selectedWorkspaceId,

          packageId:
            selectedPackageId,

          publishedByUserId:
            user.id,

          version:
            cleanVersion,

          archiveUrl:
            cleanArchiveUrl ||
            (
              `storage/packages/` +
              `${selectedWorkspaceId}/` +
              `${selectedPackageId}/` +
              `${cleanVersion}/` +
              `package.tar.gz`
            ),

          checksumSha256:
            cleanChecksum,

          manifestJson:
            normalizedManifest,

          sizeBytes:
            Math.max(
              0,
              Number(sizeBytes) || 0
            )
        });

      versions = [
        created.package_version,
        ...versions
      ];

      success =
        `Version ${created.package_version.version} ` +
        'was published successfully.';

      resetPublishForm();
      showPublishForm = false;
      versionFilter = 'all';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to publish version.';
    } finally {
      saving = false;
    }
  }

  async function openConfirmation(
    item: PackageVersion,
    action: VersionAction
  ) {
    confirmation = {
      item,
      action
    };

    deprecationMessage =
      action === 'deprecate'
        ? item.deprecation_message ||
          'Use a newer version.'
        : '';

    deleteConfirmation = '';

    await tick();

    confirmationDialog?.focus();
  }

  function closeConfirmation() {
    if (busyVersionId) {
      return;
    }

    confirmation = null;
    deprecationMessage = '';
    deleteConfirmation = '';
  }

  function confirmationTitle(
    value: Confirmation
  ) {
    if (value.action === 'delete') {
      return 'Delete package version?';
    }

    if (value.action === 'deprecate') {
      return 'Deprecate this version?';
    }

    if (value.action === 'yank') {
      return 'Yank this version?';
    }

    return 'Restore this version?';
  }

  function confirmationMessage(
    value: Confirmation
  ) {
    const versionNumber =
      value.item.version;

    if (value.action === 'delete') {
      return (
        `Version ${versionNumber} will be removed ` +
        `from normal registry use. The archive is ` +
        `retained according to registry safety rules.`
      );
    }

    if (value.action === 'deprecate') {
      return (
        `Version ${versionNumber} will remain ` +
        `available, but users will see a message ` +
        `advising them to choose another release.`
      );
    }

    if (value.action === 'yank') {
      return (
        `Version ${versionNumber} will be hidden ` +
        `from normal dependency resolution. Existing ` +
        `references can still be inspected.`
      );
    }

    return (
      `Version ${versionNumber} will return to ` +
      `normal dependency resolution.`
    );
  }

  function confirmationButtonLabel(
    value: Confirmation
  ) {
    if (value.action === 'delete') {
      return 'Delete version';
    }

    if (value.action === 'deprecate') {
      return 'Deprecate version';
    }

    if (value.action === 'yank') {
      return 'Yank version';
    }

    return 'Restore version';
  }

  function confirmationIsDanger(
    value: Confirmation
  ) {
    return value.action === 'delete';
  }

  function confirmationIsWarning(
    value: Confirmation
  ) {
    return (
      value.action === 'yank' ||
      value.action === 'deprecate'
    );
  }

  async function confirmVersionAction() {
    if (
      !confirmation ||
      !selectedWorkspaceId ||
      !selectedPackageId ||
      !canPublishCurrent ||
      !confirmationCanContinue
    ) {
      return;
    }

    const {
      item,
      action
    } = confirmation;

    busyVersionId = item.id;
    error = '';
    success = '';

    try {
      const updated =
        action === 'delete'
          ? await deletePackageVersion(
              selectedWorkspaceId,
              selectedPackageId,
              item.id
            )
          : action === 'deprecate'
            ? await deprecatePackageVersion(
                selectedWorkspaceId,
                selectedPackageId,
                item.id,
                deprecationMessage.trim()
              )
            : action === 'yank'
              ? await yankPackageVersion(
                  selectedWorkspaceId,
                  selectedPackageId,
                  item.id
                )
              : await unyankPackageVersion(
                  selectedWorkspaceId,
                  selectedPackageId,
                  item.id
                );

      if (action === 'delete') {
        versions = versions.filter(
          (versionItem) =>
            versionItem.id !== item.id
        );
      } else {
        syncVersion(
          updated.package_version
        );
      }

      success =
        action === 'delete'
          ? 'Package version deleted.'
          : action === 'deprecate'
            ? 'Package version deprecated.'
            : action === 'yank'
              ? 'Package version yanked.'
              : 'Package version restored.';

      closeConfirmation();
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to update package version.';
    } finally {
      busyVersionId = '';
    }
  }

  function handleModalKeydown(
    event: KeyboardEvent
  ) {
    if (
      event.key === 'Escape' &&
      confirmation &&
      !busyVersionId
    ) {
      event.preventDefault();
      closeConfirmation();
    }
  }

  usePageRefresh(load);

  onMount(() => {
    return () => {
      if (copyResetTimer) {
        clearTimeout(
          copyResetTimer
        );
      }
    };
  });
</script>

<svelte:head>
  <title>
    Package versions | Softadastra Cloud
  </title>

  <meta
    name="description"
    content="Review immutable Vix package releases, checksums, archives, release status and package version history in Softadastra Cloud."
  />
</svelte:head>

<svelte:window
  onkeydown={handleModalKeydown}
/>

<div class="versions-page">
  <!-- ===================================================
       PAGE HEADER
       =================================================== -->
  <header class="page-header">
    <div class="page-heading">
      <p class="page-eyebrow">
        <span aria-hidden="true"></span>
        Registry releases
      </p>

      <h1>Package versions</h1>

      <p>
        {#if loading}
          Loading release history…
        {:else if selectedPackage}
          Immutable releases for
          <strong>
            {selectedPackage.name}
          </strong>.
        {:else if selectedWorkspace}
          Select or create a package to begin
          publishing versions.
        {:else}
          Create a workspace before publishing
          package versions.
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
        !loading &&
        selectedPackage &&
        canPublishCurrent
      }
        <button
          class:button-secondary={
            showPublishForm
          }
          class="header-action"
          type="button"
          onclick={togglePublishForm}
        >
          {#if !showPublishForm}
            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path d="M12 5v14"></path>
              <path d="M5 12h14"></path>
            </svg>
          {/if}

          {showPublishForm
            ? 'Close'
            : versions.length === 0
              ? 'Publish first version'
              : 'Publish version'}
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
       LOADING
       =================================================== -->
  {#if loading}
    <section class="page-loading">
      <span class="page-loading__mark">
        V
      </span>

      <div>
        <strong>
          Loading release history
        </strong>

        <p>
          Preparing packages, checksums and archive
          metadata.
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
    <section class="required-state">
      <span
        class="required-state__icon"
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
          Release history begins inside a
          workspace.
        </h2>

        <p>
          The workspace owns the package,
          published versions, archive metadata
          and registry permissions.
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
       NO PACKAGE
       =================================================== -->
  {:else if packages.length === 0}
    <section class="required-state">
      <span
        class="required-state__icon"
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
          <path d="M21 16V8l-9-5-9 5v8l9 5 9-5Z"></path>
          <path d="m3.3 8.3 8.7 4.8 8.7-4.8"></path>
          <path d="M12 13v8"></path>
        </svg>
      </span>

      <div>
        <p class="panel-kicker">
          Package required
        </p>

        <h2>
          Create the package before publishing
          releases.
        </h2>

        <p>
          The package record defines the name,
          repository and visibility shared by every
          immutable version.
        </p>
      </div>

      <a
        class="primary-link"
        href={`/packages?workspace_id=${encodeURIComponent(
          selectedWorkspace.id
        )}`}
      >
        Open package registry

        <svg
          viewBox="0 0 24 24"
          aria-hidden="true"
        >
          <path d="M5 12h14"></path>
          <path d="m13 6 6 6-6 6"></path>
        </svg>
      </a>
    </section>

  {:else}
    <!-- =================================================
         MANUAL PUBLISH FORM
         ================================================= -->
    {#if
      showPublishForm &&
      selectedPackage &&
      canPublishCurrent
    }
      <section
        class="publish-panel"
        aria-labelledby="publish-title"
      >
        <header class="publish-panel__header">
          <span
            class="publish-panel__icon"
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
              <path d="M12 3v12"></path>
              <path d="m7 8 5-5 5 5"></path>
              <path d="M5 14v6h14v-6"></path>
            </svg>
          </span>

          <div>
            <p class="panel-kicker">
              Manual publication
            </p>

            <h2 id="publish-title">
              Publish
              {selectedPackage.name}
            </h2>

            <p>
              Enter the immutable release metadata.
              Published version numbers cannot be
              edited afterward.
            </p>
          </div>
        </header>

        <form
          class="publish-form"
          onsubmit={(event) => {
            event.preventDefault();
            void submitVersion();
          }}
        >
          <div class="publish-form__fields">
            <div class="publish-form__primary">
              <label>
                <span>Version</span>

                <input
                  bind:value={version}
                  placeholder="1.0.0"
                  required
                  autocomplete="off"
                />

                <small>
                  Use the package release version,
                  such as 1.0.0 or 2.1.3.
                </small>
              </label>

              <label>
                <span>Archive size in bytes</span>

                <input
                  bind:value={sizeBytes}
                  type="number"
                  min="0"
                  step="1"
                  placeholder="0"
                />
              </label>
            </div>

            <label>
              <span>Checksum SHA256</span>

              <input
                bind:value={checksumSha256}
                maxlength="64"
                minlength="64"
                placeholder="64 hexadecimal characters"
                required
                autocomplete="off"
                spellcheck="false"
              />

              <small>
                The checksum verifies that the
                downloaded archive is unchanged.
              </small>
            </label>

            <label>
              <span>Archive URL</span>

              <input
                bind:value={archiveUrl}
                placeholder="Leave empty to use the default storage path"
                autocomplete="off"
                spellcheck="false"
              />
            </label>

            <label>
              <span>Manifest JSON</span>

              <textarea
                bind:value={manifestJson}
                rows="8"
                spellcheck="false"
              ></textarea>
            </label>
          </div>

          <aside class="publish-preview">
            <p class="panel-kicker">
              Release preview
            </p>

            <span class="publish-preview__mark">
              {version.trim() || '1.0.0'}
            </span>

            <div class="publish-preview__identity">
              <strong>
                {selectedPackage.name}
              </strong>

              <span>
                Immutable package release
              </span>
            </div>

            <dl>
              <div>
                <dt>Version</dt>

                <dd>
                  {version.trim() ||
                    'Not defined'}
                </dd>
              </div>

              <div>
                <dt>Archive size</dt>

                <dd>
                  {formatBytes(
                    Number(sizeBytes) || 0
                  )}
                </dd>
              </div>

              <div>
                <dt>Checksum</dt>

                <dd>
                  {checksumSha256.trim()
                    ? shortChecksum(
                        checksumSha256.trim()
                      )
                    : 'Not defined'}
                </dd>
              </div>
            </dl>

            <div class="publish-preview__archive">
              <span>Archive path</span>

              <code>
                {archiveUrl.trim() ||
                  (
                    `storage/packages/` +
                    `${selectedWorkspaceId}/` +
                    `${selectedPackageId}/` +
                    `${version.trim() || 'version'}/` +
                    `package.tar.gz`
                  )}
              </code>
            </div>
          </aside>

          <div class="publish-form__actions">
            <button
              class="button-secondary"
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
                !version.trim() ||
                checksumSha256.trim().length !== 64
              }
            >
              {saving
                ? 'Publishing version…'
                : 'Publish version'}
            </button>
          </div>
        </form>
      </section>
    {/if}

    <div class="versions-layout">
      <!-- ===============================================
           PACKAGE DIRECTORY
           =============================================== -->
      <aside
        class="package-directory"
        aria-label="Package directory"
      >
        <header class="directory-header">
          <div>
            <p class="panel-kicker">
              Registry
            </p>

            <h2>Packages</h2>
          </div>

          <span class="directory-count">
            {packages.length}
          </span>
        </header>

        {#if packages.length > 4}
          <label class="package-search">
            <span class="visually-hidden">
              Find a package
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
              bind:value={packageQuery}
              type="search"
              placeholder="Find a package"
            />
          </label>
        {/if}

        {#if visiblePackages.length === 0}
          <p class="directory-message">
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
                onclick={() =>
                  selectPackage(pkg)}
              >
                <span class="package-option__mark">
                  {packageInitial(pkg)}
                </span>

                <span class="package-option__content">
                  <strong>
                    {pkg.name}
                  </strong>

                  <small>
                    {visibilityLabel(
                      pkg.visibility
                    )}

                    {#if pkg.active === false}
                      · Archived
                    {/if}
                  </small>
                </span>
              </button>
            {/each}
          </div>
        {/if}

        <a
          class="directory-footer"
          href={`/packages?workspace_id=${encodeURIComponent(
            selectedWorkspaceId
          )}`}
        >
          Manage packages

          <svg
            viewBox="0 0 24 24"
            aria-hidden="true"
          >
            <path d="m9 18 6-6-6-6"></path>
          </svg>
        </a>
      </aside>

      <!-- ===============================================
           VERSION DETAIL
           =============================================== -->
      <main class="version-detail">
        {#if selectedPackage}
          <section class="package-overview">
            <header class="package-overview__header">
              <div class="package-identity">
                <span class="package-mark">
                  {packageInitial(
                    selectedPackage
                  )}
                </span>

                <div>
                  <p class="panel-kicker">
                    Current package
                  </p>

                  <div class="package-title-line">
                    <h2>
                      {selectedPackage.name}
                    </h2>

                    <span
                      class:public={
                        selectedPackage.visibility ===
                        'public'
                      }
                      class="visibility-badge"
                    >
                      {visibilityLabel(
                        selectedPackage.visibility
                      )}
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
                href={`/packages?workspace_id=${encodeURIComponent(
                  selectedWorkspaceId
                )}`}
              >
                Package settings

                <svg
                  viewBox="0 0 24 24"
                  aria-hidden="true"
                >
                  <path d="m9 18 6-6-6-6"></path>
                </svg>
              </a>
            </header>

            {#if loadingVersions}
              <span
                class="detail-progress"
                aria-hidden="true"
              ></span>
            {/if}

            <div class="release-metrics">
              <article>
                <span>Latest version</span>

                <strong>
                  {latestVersion?.version ||
                    'No release'}
                </strong>

                <small>
                  Most recent published version
                </small>
              </article>

              <article>
                <span>Total versions</span>

                <strong>
                  {versions.length}
                </strong>

                <small>
                  Immutable release records
                </small>
              </article>

              <article>
                <span>Archive storage</span>

                <strong>
                  {formatBytes(totalSize)}
                </strong>

                <small>
                  Combined recorded archive size
                </small>
              </article>
            </div>

            {#if selectedPackage.repository_url}
              <div class="repository-row">
                <span>Repository</span>

                <a
                  href={selectedPackage.repository_url}
                  target="_blank"
                  rel="noopener noreferrer"
                >
                  {repositoryLabel(
                    selectedPackage.repository_url
                  )}

                  <svg
                    viewBox="0 0 24 24"
                    aria-hidden="true"
                  >
                    <path d="M14 5h5v5"></path>
                    <path d="M10 14 19 5"></path>
                    <path d="M19 14v5H5V5h5"></path>
                  </svg>
                </a>
              </div>
            {/if}
          </section>

          {#if selectedPackage.active === false}
            <div class="archived-notice">
              <span aria-hidden="true">
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="1.8"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                >
                  <path d="M4 7h16"></path>
                  <path d="M6 7v13h12V7"></path>
                  <path d="M9 11h6"></path>
                  <path d="M3 3h18v4H3z"></path>
                </svg>
              </span>

              <div>
                <strong>
                  This package is archived
                </strong>

                <p>
                  Existing versions remain visible,
                  but new publication is disabled
                  until the package is reactivated.
                </p>
              </div>

              <a
                href={`/packages?workspace_id=${encodeURIComponent(
                  selectedWorkspaceId
                )}`}
              >
                Manage package
              </a>
            </div>
          {:else if !canPublish}
            <ReadOnlyNotice
              message="Your role can inspect package versions and download archives, but cannot publish or change release status."
            />
          {/if}

          <!-- ===========================================
               FIRST VERSION ONBOARDING
               =========================================== -->
          {#if
            !loadingVersions &&
            versions.length === 0 &&
            !showPublishForm
          }
            <section class="version-onboarding">
              <div class="version-onboarding__copy">
                <p class="panel-kicker">
                  First release
                </p>

                <h2>
                  Publish the first immutable
                  <span>
                    version of this package.
                  </span>
                </h2>

                <p>
                  A release records its version,
                  archive, checksum and manifest.
                  The checksum allows every download
                  to be verified later.
                </p>

                {#if canPublishCurrent}
                  <div class="version-onboarding__actions">
                    <button
                      type="button"
                      onclick={openPublishForm}
                    >
                      Enter release metadata

                      <svg
                        viewBox="0 0 24 24"
                        aria-hidden="true"
                      >
                        <path d="M5 12h14"></path>
                        <path d="m13 6 6 6-6 6"></path>
                      </svg>
                    </button>
                  </div>
                {/if}
              </div>

              <div class="publish-terminal">
                <header>
                  <div aria-hidden="true">
                    <span></span>
                    <span></span>
                    <span></span>
                  </div>

                  <strong>
                    package terminal
                  </strong>

                  <small>local</small>
                </header>

                <div class="publish-terminal__body">
                  <div class="terminal-command">
                    <span>$</span>

                    <code>
                      cd
                      {selectedPackage.name
                        .split('/')
                        .at(-1)}
                    </code>
                  </div>

                  <div class="terminal-output">
                    <span>→</span>

                    <code>
                      Package source selected
                    </code>
                  </div>

                  <div class="terminal-command terminal-command--primary">
                    <span>$</span>

                    <code>
                      vix cloud publish
                    </code>
                  </div>

                  <div class="terminal-output terminal-output--success">
                    <span>✓</span>

                    <code>
                      Release uploaded and checksum
                      recorded
                    </code>
                  </div>
                </div>

                <footer>
                  <span>
                    Build remains local
                  </span>

                  <span>
                    Release history in Cloud
                  </span>
                </footer>
              </div>

              <div class="version-onboarding__steps">
                <article class="active">
                  <span>01</span>

                  <div>
                    <strong>
                      Prepare the archive
                    </strong>

                    <p>
                      Build the package release from
                      the local source.
                    </p>
                  </div>
                </article>

                <article>
                  <span>02</span>

                  <div>
                    <strong>
                      Publish the version
                    </strong>

                    <p>
                      Upload the archive, manifest and
                      checksum.
                    </p>
                  </div>
                </article>

                <article>
                  <span>03</span>

                  <div>
                    <strong>
                      Verify every download
                    </strong>

                    <p>
                      Use the recorded SHA256 value to
                      confirm integrity.
                    </p>
                  </div>
                </article>
              </div>
            </section>

          <!-- ===========================================
               RELEASE HISTORY
               =========================================== -->
          {:else if versions.length > 0}
            <section class="history-panel">
              <header class="history-panel__header">
                <div>
                  <p class="panel-kicker">
                    Release history
                  </p>

                  <h2>
                    Published versions
                  </h2>

                  <p>
                    Immutable archives and their
                    integrity metadata.
                  </p>
                </div>

                <span class="history-count">
                  {visibleVersions.length}
                </span>
              </header>

              <div class="history-toolbar">
                <label class="version-search">
                  <span class="visually-hidden">
                    Find a version
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
                    bind:value={versionQuery}
                    type="search"
                    placeholder="Find a version or checksum"
                  />
                </label>

                <nav
                  class="version-filters"
                  aria-label="Version filters"
                >
                  <button
                    class:active={
                      versionFilter === 'all'
                    }
                    type="button"
                    onclick={() => {
                      versionFilter = 'all';
                    }}
                  >
                    All

                    <span>
                      {versions.length}
                    </span>
                  </button>

                  <button
                    class:active={
                      versionFilter ===
                      'available'
                    }
                    type="button"
                    onclick={() => {
                      versionFilter =
                        'available';
                    }}
                  >
                    Available

                    <span>
                      {availableVersions.length}
                    </span>
                  </button>

                  {#if deprecatedVersions.length > 0}
                    <button
                      class:active={
                        versionFilter ===
                        'deprecated'
                      }
                      type="button"
                      onclick={() => {
                        versionFilter =
                          'deprecated';
                      }}
                    >
                      Deprecated

                      <span>
                        {deprecatedVersions.length}
                      </span>
                    </button>
                  {/if}

                  {#if yankedVersions.length > 0}
                    <button
                      class:active={
                        versionFilter ===
                        'yanked'
                      }
                      type="button"
                      onclick={() => {
                        versionFilter =
                          'yanked';
                      }}
                    >
                      Yanked

                      <span>
                        {yankedVersions.length}
                      </span>
                    </button>
                  {/if}
                </nav>
              </div>

              {#if visibleVersions.length === 0}
                <div class="history-empty">
                  <span
                    class="history-empty__icon"
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
                      <circle
                        cx="11"
                        cy="11"
                        r="7"
                      ></circle>

                      <path d="m20 20-4-4"></path>
                    </svg>
                  </span>

                  <strong>
                    No matching versions
                  </strong>

                  <p>
                    Change the search text or selected
                    release status.
                  </p>
                </div>
              {:else}
                <div class="version-list">
                  <div class="version-list__heading">
                    <span>Release</span>
                    <span>Checksum</span>
                    <span>Archive</span>
                    <span>Status</span>
                    <span></span>
                  </div>

                  {#each visibleVersions as item (item.id)}
                    <article class="version-row">
                      <div class="version-row__main">
                        <div class="version-identity">
                          <span class="version-icon">
                            v
                          </span>

                          <div>
                            <strong>
                              {item.version}
                            </strong>

                            <small>
                              Immutable release
                              ·
                              {formatBytes(
                                Number(
                                  item.size_bytes || 0
                                )
                              )}
                            </small>
                          </div>
                        </div>

                        <div class="checksum-cell">
                          <span>SHA256</span>

                          <div>
                            <code
                              title={item.checksum_sha256}
                            >
                              {shortChecksum(
                                item.checksum_sha256
                              )}
                            </code>

                            <button
                              class:copied={
                                copiedChecksumId ===
                                item.id
                              }
                              class="copy-button"
                              type="button"
                              aria-label={
                                copiedChecksumId ===
                                item.id
                                  ? 'Checksum copied'
                                  : `Copy checksum for version ${item.version}`
                              }
                              onclick={() =>
                                copyChecksum(item)}
                            >
                              {#if
                                copiedChecksumId ===
                                item.id
                              }
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
                        </div>

                        <div class="archive-cell">
                          <span>Archive</span>

                          {#if item.archive_url}
                            <a
                              href={item.archive_url}
                              target="_blank"
                              rel="noopener noreferrer"
                              title={item.archive_url}
                            >
                              {archiveLabel(
                                item.archive_url
                              )}

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
                              Unavailable
                            </strong>
                          {/if}
                        </div>

                        <div class="version-state">
                          <span>Status</span>

                          <StatusBadge
                            status={versionStatus(
                              item
                            )}
                          />
                        </div>

                        <div class="version-controls">
                          {#if item.archive_url}
                            <a
                              class="download-button"
                              href={item.archive_url}
                              target="_blank"
                              rel="noopener noreferrer"
                            >
                              Download
                            </a>
                          {/if}

                          {#if canPublishCurrent}
                            <details class="version-actions">
                              <summary
                                aria-label={`Manage version ${item.version}`}
                              >
                                <svg
                                  viewBox="0 0 24 24"
                                  aria-hidden="true"
                                >
                                  <circle
                                    cx="5"
                                    cy="12"
                                    r="1"
                                  ></circle>

                                  <circle
                                    cx="12"
                                    cy="12"
                                    r="1"
                                  ></circle>

                                  <circle
                                    cx="19"
                                    cy="12"
                                    r="1"
                                  ></circle>
                                </svg>
                              </summary>

                              <div>
                                <button
                                  type="button"
                                  disabled={
                                    busyVersionId ===
                                    item.id
                                  }
                                  onclick={() =>
                                    openConfirmation(
                                      item,
                                      versionIsYanked(
                                        item
                                      )
                                        ? 'unyank'
                                        : 'yank'
                                    )}
                                >
                                  {versionIsYanked(item)
                                    ? 'Restore version'
                                    : 'Yank version'}
                                </button>

                                {#if
                                  !versionIsDeprecated(
                                    item
                                  )
                                }
                                  <button
                                    type="button"
                                    disabled={
                                      busyVersionId ===
                                      item.id
                                    }
                                    onclick={() =>
                                      openConfirmation(
                                        item,
                                        'deprecate'
                                      )}
                                  >
                                    Deprecate
                                  </button>
                                {/if}

                                <button
                                  class="action-danger"
                                  type="button"
                                  disabled={
                                    busyVersionId ===
                                    item.id
                                  }
                                  onclick={() =>
                                    openConfirmation(
                                      item,
                                      'delete'
                                    )}
                                >
                                  Delete version
                                </button>
                              </div>
                            </details>
                          {/if}
                        </div>
                      </div>

                      {#if
                        versionIsDeprecated(item) &&
                        item.deprecation_message
                      }
                        <div class="version-message">
                          <span aria-hidden="true">
                            <svg
                              viewBox="0 0 24 24"
                              fill="none"
                              stroke="currentColor"
                              stroke-width="1.8"
                              stroke-linecap="round"
                              stroke-linejoin="round"
                            >
                              <path d="M12 9v4"></path>
                              <path d="M12 17h.01"></path>

                              <path
                                d="m10.3 3.7-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.7-3.3l-8-14a2 2 0 0 0-3.4 0Z"
                              ></path>
                            </svg>
                          </span>

                          <p>
                            {item.deprecation_message}
                          </p>
                        </div>
                      {/if}
                    </article>
                  {/each}
                </div>
              {/if}
            </section>
          {/if}
        {/if}
      </main>
    </div>
  {/if}
</div>

<!-- =====================================================
     ACTION CONFIRMATION
     ===================================================== -->
{#if confirmation}
  <div class="modal-layer">
    <button
      class="modal-backdrop"
      type="button"
      aria-label="Close confirmation"
      onclick={closeConfirmation}
    ></button>

    <dialog
      bind:this={confirmationDialog}
      class="confirm-modal"
      open
      aria-modal="true"
      aria-labelledby="confirmation-title"
      aria-describedby="confirmation-description"
      tabindex="-1"
    >
      <span
        class:confirm-modal__icon--danger={
          confirmationIsDanger(
            confirmation
          )
        }
        class:confirm-modal__icon--warning={
          confirmationIsWarning(
            confirmation
          )
        }
        class="confirm-modal__icon"
        aria-hidden="true"
      >
        {#if
          confirmationIsDanger(
            confirmation
          ) ||
          confirmationIsWarning(
            confirmation
          )
        }
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="1.9"
            stroke-linecap="round"
            stroke-linejoin="round"
          >
            <path d="M12 9v4"></path>
            <path d="M12 17h.01"></path>

            <path
              d="m10.3 3.7-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.7-3.3l-8-14a2 2 0 0 0-3.4 0Z"
            ></path>
          </svg>
        {:else}
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="1.9"
            stroke-linecap="round"
            stroke-linejoin="round"
          >
            <path d="m5 12 4 4L19 6"></path>
          </svg>
        {/if}
      </span>

      <div class="confirm-modal__content">
        <h2 id="confirmation-title">
          {confirmationTitle(
            confirmation
          )}
        </h2>

        <p id="confirmation-description">
          {confirmationMessage(
            confirmation
          )}
        </p>
      </div>

      {#if confirmationNeedsMessage}
        <label class="confirmation-field">
          <span>Deprecation message</span>

          <textarea
            bind:value={deprecationMessage}
            rows="3"
            placeholder="Use a newer version."
          ></textarea>
        </label>
      {/if}

      {#if confirmationNeedsVersion}
        <label class="confirmation-field">
          <span>
            Enter
            <code>
              {confirmation.item.version}
            </code>
            to confirm
          </span>

          <input
            bind:value={deleteConfirmation}
            placeholder={confirmation.item.version}
            autocomplete="off"
          />
        </label>
      {/if}

      <div class="confirm-modal__actions">
        <button
          class="button-secondary"
          type="button"
          disabled={Boolean(
            busyVersionId
          )}
          onclick={closeConfirmation}
        >
          Cancel
        </button>

        <button
          class:danger-button={
            confirmationIsDanger(
              confirmation
            )
          }
          class:warning-button={
            confirmationIsWarning(
              confirmation
            )
          }
          type="button"
          disabled={
            Boolean(busyVersionId) ||
            !confirmationCanContinue
          }
          onclick={confirmVersionAction}
        >
          {busyVersionId
            ? 'Working…'
            : confirmationButtonLabel(
                confirmation
              )}
        </button>
      </div>
    </dialog>
  </div>
{/if}

<style>
  /* =====================================================
     PAGE
     ===================================================== */

  .versions-page {
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
    gap: 24px;
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

  .header-action svg {
    width: 13px;
    height: 13px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
  }

  /* =====================================================
     SHARED BUTTONS
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

  .button-secondary {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-soft);
  }

  .button-secondary:hover:not(:disabled) {
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

  .warning-button {
    border-color: var(--warning-line);
    background: var(--warning-faint);
    color: var(--warning);
  }

  .warning-button:hover:not(:disabled) {
    border-color: var(--warning);
    background: rgba(245, 158, 11, 0.15);
    color: var(--warning);
  }

  .primary-link svg,
  .version-onboarding__actions button svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
    stroke-linejoin: round;
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
     FEEDBACK
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
     REQUIRED STATES
     ===================================================== */

  .required-state {
    display: grid;
    max-width: 780px;
    min-height: 320px;
    align-content: center;
    justify-items: start;
    gap: 15px;
    padding: clamp(28px, 6vw, 56px);
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-lg);
    background: var(--bg-panel);
    box-shadow: var(--shadow-panel);
  }

  .required-state__icon {
    display: grid;
    width: 49px;
    height: 49px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 11px;
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .required-state__icon svg {
    width: 22px;
    height: 22px;
  }

  .required-state > div {
    display: grid;
    gap: 7px;
  }

  .required-state h2 {
    max-width: 650px;
    color: var(--text);
    font-size: clamp(27px, 4vw, 42px);
    font-weight: 630;
    line-height: 1.04;
    letter-spacing: -0.045em;
  }

  .required-state div > p:not(.panel-kicker) {
    max-width: 590px;
    color: var(--text-muted);
    font-size: 12px;
    line-height: 1.65;
  }

  /* =====================================================
     PUBLISH FORM
     ===================================================== */

  .publish-panel {
    overflow: hidden;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    box-shadow: inset 3px 0 0 var(--brand);
    animation: publish-enter 300ms var(--ease) both;
  }

  @keyframes publish-enter {
    from {
      opacity: 0;
      transform: translateY(-6px);
    }
  }

  .publish-panel__header {
    display: flex;
    align-items: flex-start;
    gap: 12px;
    padding: 17px 19px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .publish-panel__icon {
    display: grid;
    width: 42px;
    height: 42px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 10px;
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .publish-panel__icon svg {
    width: 20px;
    height: 20px;
  }

  .publish-panel__header > div {
    display: grid;
    gap: 4px;
  }

  .publish-panel__header h2 {
    color: var(--text);
    font-size: 16px;
    font-weight: 620;
    letter-spacing: -0.025em;
  }

  .publish-panel__header p:not(.panel-kicker) {
    color: var(--text-muted);
    font-size: 10px;
    line-height: 1.5;
  }

  .publish-form {
    display: grid;
    grid-template-columns:
      minmax(0, 1.35fr)
      minmax(280px, 0.65fr);
  }

  .publish-form__fields {
    display: grid;
    gap: 16px;
    padding: 20px;
    border-right: 1px solid var(--line-soft);
  }

  .publish-form__primary {
    display: grid;
    grid-template-columns:
      minmax(0, 1fr)
      minmax(170px, 0.45fr);
    gap: 12px;
  }

  .publish-form label {
    display: grid;
    gap: 7px;
  }

  .publish-form label > span {
    color: var(--text-soft);
    font-size: 10.5px;
    font-weight: 550;
  }

  .publish-form label > small {
    color: var(--text-muted);
    font-size: 8.5px;
    line-height: 1.45;
  }

  .publish-form input,
  .publish-form textarea,
  .confirmation-field input,
  .confirmation-field textarea {
    width: 100%;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    outline: 0;
    background: var(--bg-ink-soft);
    color: var(--text);
    padding: 0 11px;
    font: inherit;
    font-size: 12px;
  }

  .publish-form input,
  .confirmation-field input {
    min-height: 41px;
  }

  .publish-form textarea,
  .confirmation-field textarea {
    resize: vertical;
    padding-block: 10px;
    font-family: var(--font-mono);
    font-size: 10.5px;
    line-height: 1.55;
  }

  .publish-form input:focus,
  .publish-form textarea:focus,
  .confirmation-field input:focus,
  .confirmation-field textarea:focus {
    border-color: var(--brand);
    box-shadow: 0 0 0 3px var(--brand-faint);
  }

  .publish-preview {
    display: grid;
    align-content: start;
    justify-items: start;
    gap: 12px;
    padding: 20px;
    background: var(--bg-ink-soft);
  }

  .publish-preview__mark {
    display: grid;
    min-width: 70px;
    min-height: 48px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 12px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    padding: 0 12px;
    font-family: var(--font-mono);
    font-size: 13px;
    font-weight: 700;
  }

  .publish-preview__identity {
    display: grid;
    gap: 3px;
  }

  .publish-preview__identity strong {
    overflow-wrap: anywhere;
    color: var(--text);
    font-family: var(--font-mono);
    font-size: 11px;
  }

  .publish-preview__identity span {
    color: var(--text-muted);
    font-size: 8.5px;
  }

  .publish-preview dl {
    display: grid;
    width: 100%;
    margin: 3px 0 0;
    border: 1px solid var(--line-soft);
    border-radius: 8px;
  }

  .publish-preview dl > div {
    display: grid;
    gap: 4px;
    padding: 10px;
    border-bottom: 1px solid var(--line-soft);
  }

  .publish-preview dl > div:last-child {
    border-bottom: 0;
  }

  .publish-preview dt {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6px;
    text-transform: uppercase;
  }

  .publish-preview dd {
    overflow-wrap: anywhere;
    margin: 0;
    color: var(--text-soft);
    font-size: 8.5px;
  }

  .publish-preview__archive {
    display: grid;
    width: 100%;
    gap: 6px;
  }

  .publish-preview__archive span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6px;
    text-transform: uppercase;
  }

  .publish-preview__archive code {
    overflow-wrap: anywhere;
    padding: 9px;
    border: 1px solid var(--line-ink);
    border-radius: 6px;
    background: var(--bg-ink);
    color: var(--text-muted);
    font-size: 7.5px;
    line-height: 1.5;
  }

  .publish-form__actions {
    grid-column: 1 / -1;
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    padding: 13px 20px;
    border-top: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  /* =====================================================
     MAIN LAYOUT
     ===================================================== */

  .versions-layout {
    display: grid;
    grid-template-columns:
      260px minmax(0, 1fr);
    gap: 16px;
    align-items: start;
  }

  /* =====================================================
     PACKAGE DIRECTORY
     ===================================================== */

  .package-directory {
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

  .directory-count,
  .history-count {
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

  .package-search {
    position: relative;
    display: block;
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

  .package-search input,
  .version-search input {
    width: 100%;
    min-height: 35px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    outline: 0;
    background: var(--bg-ink-soft);
    color: var(--text);
    padding: 0 10px 0 33px;
    font: inherit;
    font-size: 10.5px;
  }

  .package-search input:focus,
  .version-search input:focus {
    border-color: var(--brand);
  }

  .package-options {
    display: grid;
    max-height: calc(100vh - 270px);
    overflow-y: auto;
  }

  .package-option {
    display: grid;
    width: 100%;
    min-height: 59px;
    grid-template-columns:
      31px minmax(0, 1fr);
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

  .package-option:last-child {
    border-bottom: 0;
  }

  .package-option:hover:not(:disabled) {
    background: var(--bg-elevated);
    transform: none;
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
    border: 1px solid var(--line);
    border-radius: 7px;
    background: var(--bg-ink-soft);
    color: var(--text-muted);
    font-size: 9px;
    font-weight: 700;
  }

  .package-option.selected
    .package-option__mark {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .package-option__content {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .package-option__content strong {
    overflow: hidden;
    color: var(--text-soft);
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 600;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .package-option.selected
    .package-option__content strong {
    color: var(--brand-bright);
  }

  .package-option__content small {
    color: var(--text-muted);
    font-size: 8px;
  }

  .directory-message {
    padding: 18px 14px;
    color: var(--text-muted);
    font-size: 11px;
  }

  .directory-footer {
    display: flex;
    min-height: 43px;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
    padding: 0 13px;
    border-top: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
    color: var(--text-muted);
    font-size: 9px;
    text-decoration: none;
  }

  .directory-footer:hover {
    color: var(--brand-soft);
  }

  .directory-footer svg {
    width: 12px;
    height: 12px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  /* =====================================================
     PACKAGE OVERVIEW
     ===================================================== */

  .version-detail {
    display: grid;
    min-width: 0;
    gap: 16px;
  }

  .package-overview {
    position: relative;
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .package-overview__header {
    display: flex;
    min-height: 96px;
    align-items: center;
    justify-content: space-between;
    gap: 20px;
    padding: 17px 19px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .package-identity {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 12px;
  }

  .package-mark {
    display: grid;
    width: 45px;
    height: 45px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 10px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 14px;
    font-weight: 750;
  }

  .package-identity > div {
    display: grid;
    min-width: 0;
    gap: 5px;
  }

  .package-title-line {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 9px;
  }

  .package-title-line h2 {
    overflow-wrap: anywhere;
    color: var(--text);
    font-family: var(--font-mono);
    font-size: 17px;
    font-weight: 620;
    letter-spacing: -0.025em;
  }

  .package-identity p:not(.panel-kicker) {
    display: -webkit-box;
    max-width: 600px;
    overflow: hidden;
    color: var(--text-muted);
    font-size: 9.5px;
    line-height: 1.45;
    -webkit-box-orient: vertical;
    -webkit-line-clamp: 2;
    line-clamp: 2;
  }

  .visibility-badge {
    display: inline-flex;
    width: fit-content;
    min-height: 22px;
    align-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    background: var(--bg-elevated);
    color: var(--text-muted);
    padding: 1px 8px;
    font-size: 8.5px;
  }

  .visibility-badge.public {
    border-color: var(--info-line);
    background: var(--info-faint);
    color: var(--info);
  }

  .package-link {
    display: inline-flex;
    flex: 0 0 auto;
    align-items: center;
    gap: 5px;
    color: var(--brand-soft);
    font-size: 9.5px;
    font-weight: 600;
    text-decoration: none;
  }

  .package-link:hover {
    color: var(--brand-bright);
  }

  .package-link svg {
    width: 12px;
    height: 12px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .detail-progress {
    position: absolute;
    z-index: 3;
    top: 95px;
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

  .release-metrics {
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    border-bottom: 1px solid var(--line-soft);
  }

  .release-metrics article {
    display: grid;
    min-width: 0;
    gap: 4px;
    padding: 15px 18px;
    border-right: 1px solid var(--line-soft);
  }

  .release-metrics article:last-child {
    border-right: 0;
  }

  .release-metrics span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6.5px;
    letter-spacing: 0.06em;
    text-transform: uppercase;
  }

  .release-metrics strong {
    overflow-wrap: anywhere;
    color: var(--text);
    font-family: var(--font-mono);
    font-size: 16px;
    font-weight: 620;
  }

  .release-metrics small {
    overflow: hidden;
    color: var(--text-faint);
    font-size: 8px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .repository-row {
    display: grid;
    grid-template-columns:
      auto minmax(0, 1fr);
    align-items: center;
    gap: 12px;
    padding: 11px 15px;
    background: var(--bg-panel-strong);
  }

  .repository-row > span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6px;
    text-transform: uppercase;
  }

  .repository-row a {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 5px;
    overflow: hidden;
    color: var(--brand-soft);
    font-size: 8.5px;
    text-decoration: none;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .repository-row a:hover {
    color: var(--brand-bright);
  }

  .repository-row svg {
    width: 10px;
    height: 10px;
    flex: 0 0 auto;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  /* =====================================================
     ARCHIVED NOTICE
     ===================================================== */

  .archived-notice {
    display: grid;
    grid-template-columns:
      auto minmax(0, 1fr) auto;
    align-items: center;
    gap: 11px;
    padding: 13px;
    border: 1px solid var(--warning-line);
    border-radius: var(--radius-sm);
    background: var(--warning-faint);
  }

  .archived-notice > span {
    display: grid;
    width: 34px;
    height: 34px;
    place-items: center;
    border-radius: 8px;
    background: var(--bg-panel);
    color: var(--warning);
  }

  .archived-notice svg {
    width: 16px;
    height: 16px;
  }

  .archived-notice > div {
    display: grid;
    gap: 3px;
  }

  .archived-notice strong {
    color: var(--text);
    font-size: 10.5px;
  }

  .archived-notice p {
    color: var(--text-muted);
    font-size: 8.5px;
    line-height: 1.45;
  }

  .archived-notice a {
    color: var(--warning);
    font-size: 9px;
    font-weight: 600;
  }

  /* =====================================================
     VERSION ONBOARDING
     ===================================================== */

  .version-onboarding {
    display: grid;
    grid-template-columns:
      minmax(0, 0.82fr)
      minmax(400px, 1.18fr);
    gap: clamp(38px, 7vw, 76px);
    align-items: center;
    padding: clamp(30px, 5vw, 50px);
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-lg);
    background: var(--bg-panel);
    box-shadow: var(--shadow-panel);
  }

  .version-onboarding__copy {
    display: grid;
    align-content: center;
  }

  .version-onboarding__copy h2 {
    max-width: 550px;
    margin-top: 10px;
    color: var(--text);
    font-size: clamp(31px, 4.6vw, 50px);
    font-weight: 640;
    line-height: 1;
    letter-spacing: -0.052em;
  }

  .version-onboarding__copy h2 span {
    display: block;
    color: var(--brand-soft);
  }

  .version-onboarding__copy > p:not(.panel-kicker) {
    max-width: 540px;
    margin-top: 17px;
    color: var(--text-muted);
    font-size: 12px;
    line-height: 1.7;
  }

  .version-onboarding__actions {
    margin-top: 24px;
  }

  .publish-terminal {
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: 11px;
    background: var(--bg-ink);
    box-shadow: var(--shadow-panel);
  }

  .publish-terminal > header {
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

  .publish-terminal header > div {
    display: flex;
    gap: 5px;
  }

  .publish-terminal header > div span {
    width: 7px;
    height: 7px;
    border-radius: 50%;
    background: var(--text-faint);
  }

  .publish-terminal header > div span:first-child {
    background: var(--brand);
  }

  .publish-terminal header strong,
  .publish-terminal header small {
    font-family: var(--font-mono);
    font-size: 7px;
  }

  .publish-terminal header strong {
    overflow: hidden;
    color: var(--text-muted);
    text-align: center;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .publish-terminal header small {
    color: var(--text-faint);
  }

  .publish-terminal__body {
    display: grid;
    align-content: center;
    min-height: 225px;
    padding: 24px 20px;
  }

  .terminal-command,
  .terminal-output {
    display: grid;
    min-height: 35px;
    grid-template-columns:
      23px minmax(0, 1fr);
    align-items: center;
    font-family: var(--font-mono);
    font-size: 9px;
  }

  .terminal-command span {
    color: var(--brand);
    font-weight: 800;
  }

  .terminal-command code {
    color: var(--text);
  }

  .terminal-output span,
  .terminal-output code {
    color: var(--text-muted);
  }

  .terminal-command--primary {
    margin-top: 17px;
  }

  .terminal-output--success span,
  .terminal-output--success code {
    color: var(--green-soft);
  }

  .publish-terminal > footer {
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

  .version-onboarding__steps {
    grid-column: 1 / -1;
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    padding-top: 25px;
    border-top: 1px solid var(--line);
  }

  .version-onboarding__steps article {
    display: grid;
    grid-template-columns:
      auto minmax(0, 1fr);
    gap: 10px;
    padding: 0 18px;
    border-right: 1px solid var(--line);
  }

  .version-onboarding__steps article:first-child {
    padding-left: 0;
  }

  .version-onboarding__steps article:last-child {
    border-right: 0;
  }

  .version-onboarding__steps article > span {
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 8px;
    font-weight: 700;
  }

  .version-onboarding__steps article.active > span {
    color: var(--brand-soft);
  }

  .version-onboarding__steps article > div {
    display: grid;
    gap: 4px;
  }

  .version-onboarding__steps strong {
    color: var(--text-soft);
    font-size: 10px;
  }

  .version-onboarding__steps p {
    color: var(--text-muted);
    font-size: 8.5px;
    line-height: 1.45;
  }

  /* =====================================================
     HISTORY PANEL
     ===================================================== */

  .history-panel {
    overflow: visible;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .history-panel__header {
    display: flex;
    min-height: 79px;
    align-items: center;
    justify-content: space-between;
    gap: 20px;
    padding: 14px 17px;
    border-bottom: 1px solid var(--line-soft);
    border-radius:
      var(--radius-md)
      var(--radius-md)
      0 0;
    background: var(--bg-panel-strong);
  }

  .history-panel__header > div {
    display: grid;
    gap: 3px;
  }

  .history-panel__header h2 {
    color: var(--text);
    font-size: 14px;
    font-weight: 610;
  }

  .history-panel__header p:not(.panel-kicker) {
    color: var(--text-muted);
    font-size: 9.5px;
  }

  .history-toolbar {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 15px;
    padding: 9px 13px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .version-search {
    position: relative;
    width: min(290px, 100%);
  }

  .version-search svg {
    left: 11px;
  }

  .version-filters {
    display: flex;
    flex-wrap: wrap;
    gap: 4px;
  }

  .version-filters button {
    min-height: 31px;
    gap: 7px;
    border-color: transparent;
    background: transparent;
    color: var(--text-muted);
    padding: 0 9px;
    font-size: 9px;
  }

  .version-filters button:hover:not(:disabled) {
    border-color: var(--line);
    background: var(--bg-elevated);
    color: var(--text-soft);
    transform: none;
  }

  .version-filters button.active {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-bright);
  }

  .version-filters button span {
    display: grid;
    min-width: 18px;
    height: 18px;
    place-items: center;
    border-radius: 999px;
    background: var(--bg-panel);
    padding-inline: 5px;
    font-family: var(--font-mono);
    font-size: 7px;
  }

  .history-empty {
    display: grid;
    min-height: 190px;
    place-content: center;
    justify-items: center;
    gap: 6px;
    padding: 25px;
    text-align: center;
  }

  .history-empty__icon {
    display: grid;
    width: 42px;
    height: 42px;
    place-items: center;
    border: 1px solid var(--line);
    border-radius: 10px;
    background: var(--bg-ink-soft);
    color: var(--text-muted);
  }

  .history-empty__icon svg {
    width: 18px;
    height: 18px;
  }

  .history-empty strong {
    margin-top: 5px;
    color: var(--text-soft);
    font-size: 11px;
  }

  .history-empty p {
    color: var(--text-muted);
    font-size: 9px;
  }

  /* =====================================================
     VERSION LIST
     ===================================================== */

  .version-list {
    display: grid;
  }

  .version-list__heading,
  .version-row__main {
    display: grid;
    grid-template-columns:
      minmax(135px, 0.7fr)
      minmax(190px, 1fr)
      minmax(150px, 0.8fr)
      minmax(90px, 0.45fr)
      minmax(110px, auto);
    align-items: center;
    gap: 13px;
  }

  .version-list__heading {
    min-height: 34px;
    padding: 7px 14px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 7px;
    font-weight: 600;
    letter-spacing: 0.07em;
    text-transform: uppercase;
  }

  .version-row {
    position: relative;
    border-bottom: 1px solid var(--line-soft);
  }

  .version-row:last-child {
    border-bottom: 0;
    border-radius:
      0 0
      var(--radius-md)
      var(--radius-md);
  }

  .version-row:hover {
    background: rgba(255, 255, 255, 0.018);
  }

  .version-row__main {
    min-height: 72px;
    padding: 10px 14px;
  }

  .version-identity {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 9px;
  }

  .version-icon {
    display: grid;
    width: 31px;
    height: 31px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 7px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 700;
  }

  .version-identity > div {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .version-identity strong {
    overflow-wrap: anywhere;
    color: var(--text);
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 600;
  }

  .version-identity small {
    color: var(--text-muted);
    font-size: 7.5px;
  }

  .checksum-cell,
  .archive-cell,
  .version-state {
    display: grid;
    min-width: 0;
    gap: 4px;
  }

  .checksum-cell > span,
  .archive-cell > span,
  .version-state > span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6px;
    text-transform: uppercase;
  }

  .checksum-cell > div {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 6px;
  }

  .checksum-cell code {
    min-width: 0;
    overflow: hidden;
    color: var(--text-soft);
    font-size: 8px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .copy-button {
    display: grid;
    width: 26px;
    height: 26px;
    min-height: 26px;
    flex: 0 0 auto;
    place-items: center;
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-muted);
    padding: 0;
  }

  .copy-button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
    transform: none;
  }

  .copy-button.copied {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .copy-button svg {
    width: 12px;
    height: 12px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .archive-cell > a {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 5px;
    overflow: hidden;
    color: var(--brand-soft);
    font-size: 8px;
    text-decoration: none;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .archive-cell > a:hover {
    color: var(--brand-bright);
  }

  .archive-cell svg {
    width: 10px;
    height: 10px;
    flex: 0 0 auto;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .archive-cell strong {
    color: var(--text-faint);
    font-size: 8px;
    font-weight: 500;
  }

  .version-controls {
    display: flex;
    align-items: center;
    justify-content: flex-end;
    gap: 6px;
  }

  .download-button {
    display: inline-flex;
    min-height: 29px;
    align-items: center;
    justify-content: center;
    border: 1px solid var(--line-strong);
    border-radius: 7px;
    background: var(--bg-elevated);
    color: var(--text-soft);
    padding: 0 9px;
    font-size: 8.5px;
    font-weight: 600;
    text-decoration: none;
  }

  .download-button:hover {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .version-actions {
    position: relative;
  }

  .version-actions summary {
    display: grid;
    width: 29px;
    height: 29px;
    place-items: center;
    border: 1px solid var(--line);
    border-radius: 7px;
    background: transparent;
    color: var(--text-muted);
    cursor: pointer;
    list-style: none;
  }

  .version-actions summary::-webkit-details-marker {
    display: none;
  }

  .version-actions summary:hover {
    border-color: var(--line-strong);
    background: var(--bg-elevated);
    color: var(--text);
  }

  .version-actions summary svg {
    width: 14px;
    height: 14px;
    fill: currentColor;
  }

  .version-actions > div {
    position: absolute;
    z-index: 20;
    top: calc(100% + 6px);
    right: 0;
    display: grid;
    width: 165px;
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: 8px;
    background: var(--bg-panel-strong);
    box-shadow: var(--shadow-deep);
  }

  .version-actions button {
    min-height: 36px;
    justify-content: flex-start;
    border: 0;
    border-bottom: 1px solid var(--line-soft);
    border-radius: 0;
    background: transparent;
    color: var(--text-soft);
    padding: 0 11px;
    font-size: 9.5px;
  }

  .version-actions button:last-child {
    border-bottom: 0;
  }

  .version-actions button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
    transform: none;
  }

  .version-actions .action-danger {
    color: var(--danger);
  }

  .version-message {
    display: flex;
    align-items: flex-start;
    gap: 8px;
    padding: 9px 14px 10px 54px;
    border-top: 1px solid var(--warning-line);
    background: var(--warning-faint);
  }

  .version-message > span {
    display: grid;
    width: 20px;
    height: 20px;
    flex: 0 0 auto;
    place-items: center;
    color: var(--warning);
  }

  .version-message svg {
    width: 14px;
    height: 14px;
  }

  .version-message p {
    margin: 0;
    color: var(--warning);
    font-size: 8.5px;
    line-height: 1.5;
  }

  /* =====================================================
     MODAL
     ===================================================== */

  .modal-layer {
    position: fixed;
    inset: 0;
    z-index: 100;
    display: grid;
    place-items: center;
    padding: 20px;
  }

  .modal-backdrop {
    position: absolute;
    inset: 0;
    width: 100%;
    min-height: 100%;
    border: 0;
    border-radius: 0;
    background: rgba(0, 0, 0, 0.72);
    padding: 0;
    cursor: default;
    backdrop-filter: blur(3px);
  }

  .modal-backdrop:hover:not(:disabled) {
    background: rgba(0, 0, 0, 0.72);
    transform: none;
  }

  .confirm-modal {
    position: relative;
    z-index: 1;
    display: grid;
    width: min(100%, 450px);
    max-width: none;
    justify-items: start;
    gap: 13px;
    margin: 0;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    color: var(--text);
    padding: 20px;
    box-shadow: var(--shadow-deep);
  }

  .confirm-modal:focus {
    outline: none;
  }

  .confirm-modal::backdrop {
    background: transparent;
  }

  .confirm-modal__icon {
    display: grid;
    width: 38px;
    height: 38px;
    place-items: center;
    border: 1px solid var(--green-line);
    border-radius: 9px;
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .confirm-modal__icon--warning {
    border-color: var(--warning-line);
    background: var(--warning-faint);
    color: var(--warning);
  }

  .confirm-modal__icon--danger {
    border-color: var(--danger-line);
    background: var(--danger-faint);
    color: var(--danger);
  }

  .confirm-modal__icon svg {
    width: 18px;
    height: 18px;
  }

  .confirm-modal__content {
    display: grid;
    gap: 7px;
  }

  .confirm-modal__content h2 {
    color: var(--text);
    font-size: 16px;
    font-weight: 620;
  }

  .confirm-modal__content p {
    color: var(--text-muted);
    font-size: 11px;
    line-height: 1.6;
  }

  .confirmation-field {
    display: grid;
    width: 100%;
    gap: 7px;
  }

  .confirmation-field > span {
    color: var(--text-soft);
    font-size: 9.5px;
    line-height: 1.5;
  }

  .confirmation-field code {
    color: var(--danger);
    font-size: 9px;
  }

  .confirm-modal__actions {
    display: flex;
    width: 100%;
    justify-content: flex-end;
    gap: 8px;
    padding-top: 15px;
    border-top: 1px solid var(--line-soft);
  }

  /* =====================================================
     REDUCED MOTION
     ===================================================== */

  @media (prefers-reduced-motion: reduce) {
    .versions-page,
    .publish-panel,
    .page-loading__bar,
    .detail-progress {
      animation: none;
    }
  }

  /* =====================================================
     RESPONSIVE
     ===================================================== */

  @media (max-width: 1060px) {
    .publish-form {
      grid-template-columns: 1fr;
    }

    .publish-form__fields {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .publish-preview {
      grid-template-columns:
        auto minmax(0, 1fr);
      align-items: center;
    }

    .publish-preview > .panel-kicker,
    .publish-preview dl,
    .publish-preview__archive {
      grid-column: 1 / -1;
    }

    .version-onboarding {
      grid-template-columns: 1fr;
    }

    .version-list__heading,
    .version-row__main {
      grid-template-columns:
        minmax(130px, 0.7fr)
        minmax(170px, 1fr)
        minmax(100px, 0.55fr)
        minmax(100px, auto);
    }

    .archive-cell {
      display: none;
    }
  }

  @media (max-width: 900px) {
    .versions-layout {
      grid-template-columns: 1fr;
    }

    .package-directory {
      position: static;
    }

    .package-options {
      max-height: 260px;
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

    .publish-form__primary {
      grid-template-columns: 1fr;
    }

    .package-overview__header {
      align-items: flex-start;
      flex-direction: column;
    }

    .package-link {
      width: 100%;
      justify-content: space-between;
    }

    .release-metrics {
      grid-template-columns: 1fr;
    }

    .release-metrics article {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .release-metrics article:last-child {
      border-bottom: 0;
    }

    .history-toolbar {
      align-items: stretch;
      flex-direction: column;
    }

    .version-search {
      width: 100%;
    }

    .version-filters {
      overflow-x: auto;
      flex-wrap: nowrap;
    }

    .version-filters button {
      flex: 0 0 auto;
    }

    .version-list__heading {
      display: none;
    }

    .version-row__main {
      grid-template-columns:
        minmax(0, 1fr) auto;
      gap: 13px;
      padding: 14px;
    }

    .version-identity {
      grid-column: 1;
    }

    .version-controls {
      grid-column: 2;
      grid-row: 1;
    }

    .checksum-cell {
      grid-column: 1 / -1;
      padding-top: 10px;
      border-top: 1px solid var(--line-soft);
    }

    .version-state {
      grid-column: 1 / -1;
    }

    .version-message {
      padding-left: 14px;
    }

    .version-onboarding__steps {
      grid-template-columns: 1fr;
      gap: 15px;
    }

    .version-onboarding__steps article,
    .version-onboarding__steps article:first-child {
      padding: 0;
      border-right: 0;
    }
  }

  @media (max-width: 620px) {
    .publish-form__actions,
    .confirm-modal__actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .publish-form__actions button,
    .confirm-modal__actions button {
      width: 100%;
    }

    .version-onboarding {
      padding: 26px 19px;
    }

    .version-onboarding__copy h2 {
      font-size: clamp(31px, 10vw, 43px);
    }

    .publish-terminal > footer {
      align-items: flex-start;
      flex-direction: column;
      justify-content: center;
      padding-block: 10px;
    }

    .archived-notice {
      grid-template-columns:
        auto minmax(0, 1fr);
    }

    .archived-notice > a {
      grid-column: 2;
    }
  }

  @media (max-width: 500px) {
    .publish-form__fields,
    .publish-preview {
      padding: 16px;
    }

    .version-row__main {
      grid-template-columns: 1fr;
    }

    .version-controls {
      grid-column: 1;
      grid-row: auto;
      justify-content: flex-start;
    }

    .checksum-cell {
      grid-column: 1;
    }

    .version-state {
      grid-column: 1;
    }

    .version-actions > div {
      position: fixed;
      right: 16px;
      bottom: 16px;
      top: auto;
      width: calc(100% - 32px);
    }

    .repository-row {
      grid-template-columns: 1fr;
      gap: 5px;
    }
  }
</style>
