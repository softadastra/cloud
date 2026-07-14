<script lang="ts">
  import { goto } from '$app/navigation';
  import { tick } from 'svelte';

  import { usePageRefresh } from '$lib/stores/pageRefresh';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

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

  import InlineError from '$lib/components/InlineError.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import WorkspaceAvatar from '$lib/components/WorkspaceAvatar.svelte';

  import { canCreatePackage } from '$lib/permissions';

  type PackageFilter =
    | 'all'
    | 'private'
    | 'public'
    | 'archived';

  type PackageAction =
    | 'archive'
    | 'reactivate'
    | 'delete'
    | 'public'
    | 'private';

  type Confirmation = {
    pkg: Package;
    action: PackageAction;
  };

  const visibilityOptions = [
    {
      value: 'private' as const,
      label: 'Private',
      description:
        'Available only to members and authorized CLI tokens in this workspace.'
    },
    {
      value: 'public' as const,
      label: 'Public',
      description:
        'Visible through public package pages and public profiles.'
    }
  ];

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

  let confirmation: Confirmation | null = null;
  let confirmationDialog: HTMLDialogElement | null = null;

  $: selectedWorkspace =
    workspaces.find(
      (workspace) =>
        workspace.id === selectedWorkspaceId
    ) ?? null;

  $: currentRole =
    selectedWorkspace?.current_user_role ??
    'viewer';

  $: canCreate =
    canCreatePackage(currentRole);

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

  $: visiblePackages = packages.filter(
    (pkg) => {
      const query =
        packageQuery.trim().toLowerCase();

      const matchesQuery =
        !query ||
        pkg.name
          .toLowerCase()
          .includes(query) ||
        pkg.description
          ?.toLowerCase()
          .includes(query) ||
        pkg.repository_url
          ?.toLowerCase()
          .includes(query);

      if (!matchesQuery) {
        return false;
      }

      if (packageFilter === 'private') {
        return pkg.visibility !== 'public';
      }

      if (packageFilter === 'public') {
        return pkg.visibility === 'public';
      }

      if (packageFilter === 'archived') {
        return pkg.active === false;
      }

      return pkg.active !== false;
    }
  );

  $: currentVisibility =
    visibilityOptions.find(
      (option) =>
        option.value === visibility
    ) ?? visibilityOptions[0];

  function packageInitial(
    pkg: Package
  ) {
    const value = pkg.name
      .split('/')
      .filter(Boolean)
      .at(-1);

    return (
      value ||
      pkg.name ||
      'P'
    )
      .slice(0, 1)
      .toUpperCase();
  }

  function packageStatus(
    pkg: Package
  ) {
    return (
      pkg.status ||
      (
        pkg.active === false
          ? 'archived'
          : 'active'
      )
    );
  }

  function visibilityLabel(
    value?: string
  ) {
    return value === 'public'
      ? 'Public'
      : 'Private';
  }

  function versionsHref(
    pkg: Package
  ) {
    return (
      `/package-versions?workspace_id=` +
      `${encodeURIComponent(pkg.workspace_id)}` +
      `&package_id=` +
      `${encodeURIComponent(pkg.id)}`
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

  function syncPackage(
    updated: Package
  ) {
    packages = packages.map(
      (pkg) =>
        pkg.id === updated.id
          ? updated
          : pkg
    );
  }

  function resetForm() {
    name = '';
    description = '';
    repositoryUrl = '';
    visibility = 'private';
  }

  function openForm() {
    showForm = true;
    error = '';
    success = '';
  }

  function closeForm() {
    if (saving) {
      return;
    }

    showForm = false;
    resetForm();
    error = '';
  }

  function toggleForm() {
    if (showForm) {
      closeForm();
      return;
    }

    openForm();
  }

  function updateWorkspaceUrl(
    workspaceId: string
  ) {
    const url =
      new URL(window.location.href);

    if (workspaceId) {
      url.searchParams.set(
        'workspace_id',
        workspaceId
      );
    } else {
      url.searchParams.delete(
        'workspace_id'
      );
    }

    history.replaceState(
      null,
      '',
      `${url.pathname}${url.search}`
    );
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
      const workspaceData =
        await listWorkspaces(user.id);

      workspaces =
        workspaceData.workspaces;

      const requestedWorkspaceId =
        new URLSearchParams(
          window.location.search
        ).get('workspace_id');

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

      updateWorkspaceUrl(
        selectedWorkspaceId
      );

      await loadPackages(
        selectedWorkspaceId
      );

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

  async function loadPackages(
    workspaceId: string
  ) {
    const requestId =
      ++packageRequestId;

    if (!workspaceId) {
      packages = [];
      return;
    }

    workspaceContext.setSelectedWorkspace(
      workspaceId
    );

    const data =
      await listPackages(workspaceId);

    if (
      requestId !== packageRequestId ||
      workspaceId !== selectedWorkspaceId
    ) {
      return;
    }

    packages = data.packages;
  }

  async function switchWorkspace(
    workspaceId: string
  ) {
    if (
      workspaceId === selectedWorkspaceId
    ) {
      return;
    }

    selectedWorkspaceId =
      workspaceId;

    packages = [];

    showForm = false;
    resetForm();

    packageQuery = '';
    packageFilter = 'all';

    confirmation = null;

    error = '';
    success = '';
    loading = true;

    workspaceContext.setSelectedWorkspace(
      workspaceId
    );

    updateWorkspaceUrl(workspaceId);

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

  async function submitPackage() {
    const user = $auth.user;

    const packageName =
      name.trim();

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
      const created =
        await createPackage({
          workspaceId:
            selectedWorkspaceId,

          ownerUserId:
            user.id,

          name:
            packageName,

          description:
            description.trim(),

          repositoryUrl:
            repositoryUrl.trim(),

          visibility
        });

      packages = [
        created.package,
        ...packages
      ];

      success =
        `${created.package.name} is ready. ` +
        'Publish its first version from the Vix CLI.';

      resetForm();
      showForm = false;
      packageFilter = 'all';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to create package.';
    } finally {
      saving = false;
    }
  }

  async function openConfirmation(
    pkg: Package,
    action: PackageAction
  ) {
    confirmation = {
      pkg,
      action
    };

    await tick();
    confirmationDialog?.focus();
  }

  function closeConfirmation() {
    if (busyPackageId) {
      return;
    }

    confirmation = null;
  }

  function confirmationTitle(
    value: Confirmation
  ) {
    if (value.action === 'delete') {
      return 'Delete package?';
    }

    if (value.action === 'archive') {
      return 'Archive package?';
    }

    if (value.action === 'reactivate') {
      return 'Reactivate package?';
    }

    if (value.action === 'public') {
      return 'Make package public?';
    }

    return 'Make package private?';
  }

  function confirmationMessage(
    value: Confirmation
  ) {
    const packageName =
      value.pkg.name;

    if (value.action === 'delete') {
      return (
        `${packageName} will be removed from ` +
        `normal registry use. Existing versions and ` +
        `archives are retained for safety.`
      );
    }

    if (value.action === 'archive') {
      return (
        `${packageName} will leave active registry ` +
        `workflows until it is reactivated.`
      );
    }

    if (value.action === 'reactivate') {
      return (
        `${packageName} will become available in ` +
        `active registry workflows again.`
      );
    }

    if (value.action === 'public') {
      return (
        `${packageName} will become visible through ` +
        `public package pages and public profiles.`
      );
    }

    return (
      `${packageName} will become private and ` +
      `disappear from public package pages.`
    );
  }

  function confirmationButtonLabel(
    value: Confirmation
  ) {
    if (value.action === 'delete') {
      return 'Delete package';
    }

    if (value.action === 'archive') {
      return 'Archive package';
    }

    if (value.action === 'reactivate') {
      return 'Reactivate package';
    }

    if (value.action === 'public') {
      return 'Make public';
    }

    return 'Make private';
  }

  function confirmationIsDanger(
    value: Confirmation
  ) {
    return (
      value.action === 'delete' ||
      value.action === 'archive'
    );
  }

  async function confirmPackageAction() {
    if (
      !confirmation ||
      !selectedWorkspaceId ||
      !canCreate
    ) {
      return;
    }

    const {
      pkg,
      action
    } = confirmation;

    busyPackageId = pkg.id;
    error = '';
    success = '';

    try {
      const updated =
        action === 'archive'
          ? await archivePackage(
              selectedWorkspaceId,
              pkg.id
            )
          : action === 'reactivate'
            ? await reactivatePackage(
                selectedWorkspaceId,
                pkg.id
              )
            : action === 'delete'
              ? await deletePackage(
                  selectedWorkspaceId,
                  pkg.id
                )
              : await changePackageVisibility(
                  selectedWorkspaceId,
                  pkg.id,
                  action === 'public'
                    ? 'public'
                    : 'private'
                );

      if (action === 'delete') {
        packages = packages.filter(
          (item) =>
            item.id !== pkg.id
        );
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
              : action === 'public'
                ? 'Package is now public.'
                : 'Package is now private.';

      confirmation = null;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to update package.';
    } finally {
      busyPackageId = '';
    }
  }

  function handleModalKeydown(
    event: KeyboardEvent
  ) {
    if (
      event.key === 'Escape' &&
      confirmation &&
      !busyPackageId
    ) {
      event.preventDefault();
      closeConfirmation();
    }
  }

  usePageRefresh(load);
</script>

<svelte:head>
  <title>
    Packages | Softadastra Cloud
  </title>

  <meta
    name="description"
    content="Create, publish and manage private or public Vix packages in the Softadastra Cloud registry."
  />
</svelte:head>

<svelte:window
  onkeydown={handleModalKeydown}
/>

<div class="packages-page">
  <!-- ===================================================
       HEADER
       =================================================== -->
  <header class="page-header">
    <div class="page-heading">
      <p class="page-eyebrow">
        <span aria-hidden="true"></span>
        Registry
      </p>

      <h1>Packages</h1>

      <p>
        {#if loading}
          Loading the package registry…
        {:else if selectedWorkspace}
          Packages owned by
          <strong>
            {selectedWorkspace.name}
          </strong>.
        {:else}
          Create a workspace before registering
          packages.
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
        selectedWorkspace &&
        canCreate
      }
        <button
          class:button-secondary={showForm}
          class="header-action"
          type="button"
          onclick={toggleForm}
        >
          {#if !showForm}
            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path d="M12 5v14"></path>
              <path d="M5 12h14"></path>
            </svg>
          {/if}

          {showForm
            ? 'Close'
            : packages.length === 0
              ? 'Create first package'
              : 'New package'}
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
        P
      </span>

      <div>
        <strong>
          Loading package registry
        </strong>

        <p>
          Preparing package access and registry
          visibility.
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
          Packages need a workspace first.
        </h2>

        <p>
          The workspace owns package records,
          versions, visibility and registry access.
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

  {:else}
    <!-- =================================================
         PACKAGE FORM
         ================================================= -->
    {#if showForm && canCreate}
      <section
        class="package-form-panel"
        aria-labelledby="package-form-title"
      >
        <header class="package-form-panel__header">
          <span
            class="package-form-panel__icon"
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
              Package record
            </p>

            <h2 id="package-form-title">
              Create a package
            </h2>

            <p>
              Register the package before publishing
              its first version from the Vix CLI.
            </p>
          </div>
        </header>

        <form
          class="package-form"
          onsubmit={(event) => {
            event.preventDefault();
            void submitPackage();
          }}
        >
          <div class="package-form__fields">
            <label>
              <span>Package name</span>

              <input
                bind:value={name}
                placeholder="vix/http"
                required
                autocomplete="off"
              />

              <small>
                Use a stable namespace and package
                name.
              </small>
            </label>

            <label>
              <span>Description</span>

              <textarea
                bind:value={description}
                rows="3"
                placeholder="HTTP utilities for Vix applications"
              ></textarea>
            </label>

            <label>
              <span>Repository URL</span>

              <input
                bind:value={repositoryUrl}
                type="url"
                placeholder="https://github.com/example/package"
                autocomplete="off"
              />

              <small>
                Optional source repository for
                maintainers and users.
              </small>
            </label>

            <fieldset>
              <legend>Visibility</legend>

              <div class="visibility-options">
                {#each visibilityOptions as option}
                  <button
                    class:selected={
                      visibility === option.value
                    }
                    class="visibility-option"
                    type="button"
                    aria-pressed={
                      visibility === option.value
                    }
                    onclick={() => {
                      visibility = option.value;
                    }}
                  >
                    <span
                      class="visibility-option__indicator"
                      aria-hidden="true"
                    ></span>

                    <span>
                      <strong>
                        {option.label}
                      </strong>

                      <small>
                        {option.description}
                      </small>
                    </span>
                  </button>
                {/each}
              </div>
            </fieldset>
          </div>

          <aside class="package-preview">
            <p class="panel-kicker">
              Package preview
            </p>

            <span class="package-preview__mark">
              {name.trim()
                ? name
                    .split('/')
                    .filter(Boolean)
                    .at(-1)
                    ?.charAt(0)
                    .toUpperCase() || 'P'
                : 'P'}
            </span>

            <div class="package-preview__identity">
              <strong>
                {name.trim() ||
                  'namespace/package'}
              </strong>

              <span>
                No versions published
              </span>
            </div>

            <dl>
              <div>
                <dt>Visibility</dt>

                <dd>
                  {currentVisibility.label}
                </dd>
              </div>

              <div>
                <dt>Workspace</dt>

                <dd>
                  {selectedWorkspace.name}
                </dd>
              </div>

              <div>
                <dt>Repository</dt>

                <dd>
                  {repositoryUrl.trim()
                    ? repositoryLabel(
                        repositoryUrl.trim()
                      )
                    : 'Not connected'}
                </dd>
              </div>
            </dl>

            <div class="package-preview__next">
              <span>Next command</span>

              <code>
                <i>$</i>
                vix cloud publish
              </code>
            </div>
          </aside>

          <div class="package-form__actions">
            <button
              class="button-secondary"
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
                !name.trim()
              }
            >
              {saving
                ? 'Creating package…'
                : 'Create package'}
            </button>
          </div>
        </form>
      </section>
    {/if}

    {#if !canCreate}
      <ReadOnlyNotice
        message="Your role can inspect packages and versions, but cannot create or manage registry packages."
      />
    {/if}

    <!-- =================================================
         EMPTY REGISTRY
         ================================================= -->
    {#if
      packages.length === 0 &&
      !showForm
    }
      <section class="package-onboarding">
        <div class="package-onboarding__copy">
          <p class="panel-kicker">
            First package
          </p>

          <h2>
            Create the package record.
            <span>
              Publish versions from Vix.
            </span>
          </h2>

          <p>
            The package record defines its identity,
            visibility and repository. Version archives
            are then published from the local Vix
            workflow.
          </p>

          {#if canCreate}
            <button
              type="button"
              onclick={openForm}
            >
              Create the first package

              <svg
                viewBox="0 0 24 24"
                aria-hidden="true"
              >
                <path d="M5 12h14"></path>
                <path d="m13 6 6 6-6 6"></path>
              </svg>
            </button>
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
              <code>cd my-package</code>
            </div>

            <div class="terminal-output">
              <span>→</span>
              <code>
                Local package selected
              </code>
            </div>

            <div class="terminal-command terminal-command--primary">
              <span>$</span>
              <code>vix cloud publish</code>
            </div>

            <div class="terminal-output terminal-output--success">
              <span>✓</span>
              <code>
                Version published to
                {selectedWorkspace.slug}
              </code>
            </div>
          </div>

          <footer>
            <span>
              Package source remains local
            </span>

            <span>
              Version history in Cloud
            </span>
          </footer>
        </div>

        <div class="package-onboarding__steps">
          <article class="active">
            <span>01</span>

            <div>
              <strong>
                Register the package
              </strong>

              <p>
                Choose its name, visibility and
                source repository.
              </p>
            </div>
          </article>

          <article>
            <span>02</span>

            <div>
              <strong>
                Publish a version
              </strong>

              <p>
                Send the package archive through the
                Vix CLI.
              </p>
            </div>
          </article>

          <article>
            <span>03</span>

            <div>
              <strong>
                Open version history
              </strong>

              <p>
                Inspect versions, checksums and
                release state.
              </p>
            </div>
          </article>
        </div>
      </section>

    <!-- =================================================
         ACTIVE REGISTRY
         ================================================= -->
    {:else if packages.length > 0}
      <section class="registry-overview">
        <div class="registry-overview__identity">
          <span
            class="registry-overview__icon"
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
              Workspace registry
            </p>

            <h2>
              {selectedWorkspace.name}
            </h2>

            <code>
              {selectedWorkspace.slug}
            </code>
          </div>
        </div>

        <div class="registry-metrics">
          <article>
            <span>Active packages</span>

            <strong>
              {activePackages.length}
            </strong>

            <small>
              Available in registry workflows
            </small>
          </article>

          <article>
            <span>Private</span>

            <strong>
              {privatePackages.length}
            </strong>

            <small>
              Limited to workspace access
            </small>
          </article>

          <article>
            <span>Public</span>

            <strong>
              {publicPackages.length}
            </strong>

            <small>
              Visible through public pages
            </small>
          </article>
        </div>
      </section>

      <section class="registry-panel">
        <header class="registry-panel__header">
          <div>
            <p class="panel-kicker">
              Package directory
            </p>

            <h2>
              Registry packages
            </h2>

            <p>
              Open a package to inspect and publish
              its versions.
            </p>
          </div>

          <span class="registry-count">
            {visiblePackages.length}
          </span>
        </header>

        <div class="registry-toolbar">
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

          <nav
            class="registry-filters"
            aria-label="Package filters"
          >
            <button
              class:active={
                packageFilter === 'all'
              }
              type="button"
              onclick={() => {
                packageFilter = 'all';
              }}
            >
              Active

              <span>
                {activePackages.length}
              </span>
            </button>

            <button
              class:active={
                packageFilter === 'private'
              }
              type="button"
              onclick={() => {
                packageFilter = 'private';
              }}
            >
              Private

              <span>
                {privatePackages.filter(
                  (pkg) =>
                    pkg.active !== false
                ).length}
              </span>
            </button>

            <button
              class:active={
                packageFilter === 'public'
              }
              type="button"
              onclick={() => {
                packageFilter = 'public';
              }}
            >
              Public

              <span>
                {publicPackages.filter(
                  (pkg) =>
                    pkg.active !== false
                ).length}
              </span>
            </button>

            {#if archivedPackages.length > 0}
              <button
                class:active={
                  packageFilter ===
                  'archived'
                }
                type="button"
                onclick={() => {
                  packageFilter =
                    'archived';
                }}
              >
                Archived

                <span>
                  {archivedPackages.length}
                </span>
              </button>
            {/if}
          </nav>
        </div>

        {#if visiblePackages.length === 0}
          <div class="registry-empty">
            <span
              class="registry-empty__icon"
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
              No matching packages
            </strong>

            <p>
              Change the search text or selected
              registry filter.
            </p>
          </div>
        {:else}
          <div class="package-list">
            {#each visiblePackages as pkg (pkg.id)}
              <article class="package-row">
                <a
                  class="package-main"
                  href={versionsHref(pkg)}
                >
                  <span
                    class="package-mark"
                    aria-hidden="true"
                  >
                    {packageInitial(pkg)}
                  </span>

                  <span class="package-identity">
                    <strong>
                      {pkg.name}
                    </strong>

                    <small>
                      {pkg.description ||
                        'No description provided.'}
                    </small>
                  </span>
                </a>

                <div class="package-repository">
                  <span>Repository</span>

                  {#if pkg.repository_url}
                    <a
                      href={pkg.repository_url}
                      target="_blank"
                      rel="noopener noreferrer"
                    >
                      {repositoryLabel(
                        pkg.repository_url
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
                      Not connected
                    </strong>
                  {/if}
                </div>

                <div class="package-visibility">
                  <span>Visibility</span>

                  <span
                    class:public={
                      pkg.visibility === 'public'
                    }
                    class="visibility-badge"
                  >
                    {visibilityLabel(
                      pkg.visibility
                    )}
                  </span>
                </div>

                <div class="package-state">
                  <span>Status</span>

                  <StatusBadge
                    status={packageStatus(pkg)}
                  />
                </div>

                <a
                  class="versions-link"
                  href={versionsHref(pkg)}
                >
                  Open versions

                  <svg
                    viewBox="0 0 24 24"
                    aria-hidden="true"
                  >
                    <path d="m9 18 6-6-6-6"></path>
                  </svg>
                </a>

                {#if canCreate}
                  <details class="package-actions">
                    <summary
                      aria-label={`Manage ${pkg.name}`}
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
                          busyPackageId ===
                          pkg.id
                        }
                        onclick={() =>
                          openConfirmation(
                            pkg,
                            pkg.visibility ===
                              'public'
                              ? 'private'
                              : 'public'
                          )}
                      >
                        {pkg.visibility === 'public'
                          ? 'Make private'
                          : 'Make public'}
                      </button>

                      <button
                        type="button"
                        disabled={
                          busyPackageId ===
                          pkg.id
                        }
                        onclick={() =>
                          openConfirmation(
                            pkg,
                            packageStatus(pkg) ===
                              'archived'
                              ? 'reactivate'
                              : 'archive'
                          )}
                      >
                        {packageStatus(pkg) ===
                        'archived'
                          ? 'Reactivate'
                          : 'Archive'}
                      </button>

                      <button
                        class="action-danger"
                        type="button"
                        disabled={
                          busyPackageId ===
                          pkg.id
                        }
                        onclick={() =>
                          openConfirmation(
                            pkg,
                            'delete'
                          )}
                      >
                        Delete package
                      </button>
                    </div>
                  </details>
                {/if}
              </article>
            {/each}
          </div>
        {/if}
      </section>
    {/if}
  {/if}
</div>

<!-- =====================================================
     CONFIRMATION
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
        class="confirm-modal__icon"
        aria-hidden="true"
      >
        {#if
          confirmationIsDanger(
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
            <path d="M21 16V8l-9-5-9 5v8l9 5 9-5Z"></path>
            <path d="m3.3 8.3 8.7 4.8 8.7-4.8"></path>
            <path d="M12 13v8"></path>
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

      <div class="confirm-modal__actions">
        <button
          class="button-secondary"
          type="button"
          disabled={Boolean(
            busyPackageId
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
          type="button"
          disabled={Boolean(
            busyPackageId
          )}
          onclick={confirmPackageAction}
        >
          {busyPackageId
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

  .packages-page {
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

  .primary-link svg,
  .package-onboarding__copy button svg {
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
     PACKAGE FORM
     ===================================================== */

  .package-form-panel {
    overflow: hidden;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    box-shadow: inset 3px 0 0 var(--brand);
    animation: form-enter 300ms var(--ease) both;
  }

  @keyframes form-enter {
    from {
      opacity: 0;
      transform: translateY(-6px);
    }
  }

  .package-form-panel__header {
    display: flex;
    align-items: flex-start;
    gap: 12px;
    padding: 17px 19px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .package-form-panel__icon {
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

  .package-form-panel__icon svg {
    width: 20px;
    height: 20px;
  }

  .package-form-panel__header > div {
    display: grid;
    gap: 4px;
  }

  .package-form-panel h2 {
    color: var(--text);
    font-size: 16px;
    font-weight: 620;
    letter-spacing: -0.025em;
  }

  .package-form-panel__header p:not(.panel-kicker) {
    color: var(--text-muted);
    font-size: 10px;
  }

  .package-form {
    display: grid;
    grid-template-columns:
      minmax(0, 1.35fr)
      minmax(260px, 0.65fr);
  }

  .package-form__fields {
    display: grid;
    gap: 16px;
    padding: 20px;
    border-right: 1px solid var(--line-soft);
  }

  .package-form label {
    display: grid;
    gap: 7px;
  }

  .package-form label > span,
  .package-form legend {
    color: var(--text-soft);
    font-size: 10.5px;
    font-weight: 550;
  }

  .package-form label > small {
    color: var(--text-muted);
    font-size: 8.5px;
    line-height: 1.45;
  }

  .package-form input,
  .package-form textarea {
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

  .package-form input {
    min-height: 41px;
  }

  .package-form textarea {
    min-height: 82px;
    resize: vertical;
    padding-block: 10px;
    line-height: 1.5;
  }

  .package-form input:focus,
  .package-form textarea:focus {
    border-color: var(--brand);
    box-shadow: 0 0 0 3px var(--brand-faint);
  }

  .package-form fieldset {
    display: grid;
    gap: 9px;
    margin: 0;
    border: 0;
    padding: 0;
  }

  .visibility-options {
    display: grid;
    grid-template-columns:
      repeat(2, minmax(0, 1fr));
    gap: 8px;
  }

  .visibility-option {
    display: grid;
    min-height: 88px;
    grid-template-columns:
      auto minmax(0, 1fr);
    align-items: start;
    gap: 9px;
    border-color: var(--line);
    background: var(--bg-ink-soft);
    color: var(--text);
    padding: 11px;
    text-align: left;
  }

  .visibility-option:hover:not(:disabled) {
    border-color: var(--line-strong);
    background: var(--bg-elevated);
    transform: none;
  }

  .visibility-option.selected {
    border-color: var(--brand-line);
    background: var(--brand-faint);
  }

  .visibility-option__indicator {
    width: 11px;
    height: 11px;
    margin-top: 2px;
    border: 1px solid var(--line-strong);
    border-radius: 50%;
  }

  .visibility-option.selected
    .visibility-option__indicator {
    border: 3px solid var(--bg-panel);
    background: var(--brand);
    box-shadow: 0 0 0 1px var(--brand);
  }

  .visibility-option > span:last-child {
    display: grid;
    gap: 4px;
  }

  .visibility-option strong {
    color: var(--text-soft);
    font-size: 10px;
    font-weight: 600;
  }

  .visibility-option.selected strong {
    color: var(--brand-bright);
  }

  .visibility-option small {
    color: var(--text-muted);
    font-size: 8px;
    line-height: 1.45;
  }

  .package-preview {
    display: grid;
    align-content: start;
    justify-items: start;
    gap: 12px;
    padding: 20px;
    background: var(--bg-ink-soft);
  }

  .package-preview__mark {
    display: grid;
    width: 48px;
    height: 48px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 12px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 15px;
    font-weight: 750;
  }

  .package-preview__identity {
    display: grid;
    max-width: 100%;
    gap: 3px;
  }

  .package-preview__identity strong {
    max-width: 100%;
    overflow-wrap: anywhere;
    color: var(--text);
    font-family: var(--font-mono);
    font-size: 11px;
  }

  .package-preview__identity span {
    color: var(--text-muted);
    font-size: 8.5px;
  }

  .package-preview dl {
    display: grid;
    width: 100%;
    margin: 3px 0 0;
    border: 1px solid var(--line-soft);
    border-radius: 8px;
  }

  .package-preview dl > div {
    display: grid;
    gap: 4px;
    padding: 10px;
    border-bottom: 1px solid var(--line-soft);
  }

  .package-preview dl > div:last-child {
    border-bottom: 0;
  }

  .package-preview dt {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6px;
    text-transform: uppercase;
  }

  .package-preview dd {
    overflow-wrap: anywhere;
    margin: 0;
    color: var(--text-soft);
    font-size: 8.5px;
    line-height: 1.4;
  }

  .package-preview__next {
    display: grid;
    width: 100%;
    gap: 7px;
    margin-top: 2px;
  }

  .package-preview__next > span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6px;
    text-transform: uppercase;
  }

  .package-preview__next code {
    display: flex;
    min-height: 38px;
    align-items: center;
    gap: 8px;
    padding: 0 10px;
    border: 1px solid var(--line-ink);
    border-radius: 6px;
    background: var(--bg-ink);
    color: var(--text-soft);
    font-size: 8.5px;
  }

  .package-preview__next i {
    color: var(--brand);
    font-style: normal;
    font-weight: 800;
  }

  .package-form__actions {
    grid-column: 1 / -1;
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    padding: 13px 20px;
    border-top: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  /* =====================================================
     ONBOARDING
     ===================================================== */

  .package-onboarding {
    display: grid;
    grid-template-columns:
      minmax(0, 0.82fr)
      minmax(420px, 1.18fr);
    gap: clamp(38px, 7vw, 80px);
    align-items: center;
    padding: clamp(30px, 5vw, 54px);
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-lg);
    background: var(--bg-panel);
    box-shadow: var(--shadow-panel);
  }

  .package-onboarding__copy {
    display: grid;
    align-content: center;
  }

  .package-onboarding__copy h2 {
    max-width: 560px;
    margin-top: 10px;
    color: var(--text);
    font-size: clamp(32px, 4.8vw, 52px);
    font-weight: 640;
    line-height: 1;
    letter-spacing: -0.052em;
  }

  .package-onboarding__copy h2 span {
    display: block;
    color: var(--brand-soft);
  }

  .package-onboarding__copy > p:not(.panel-kicker) {
    max-width: 550px;
    margin-top: 17px;
    color: var(--text-muted);
    font-size: 12.5px;
    line-height: 1.7;
  }

  .package-onboarding__copy > button {
    width: fit-content;
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
    min-height: 235px;
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

  .package-onboarding__steps {
    grid-column: 1 / -1;
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    padding-top: 25px;
    border-top: 1px solid var(--line);
  }

  .package-onboarding__steps article {
    display: grid;
    grid-template-columns:
      auto minmax(0, 1fr);
    gap: 10px;
    padding: 0 18px;
    border-right: 1px solid var(--line);
  }

  .package-onboarding__steps article:first-child {
    padding-left: 0;
  }

  .package-onboarding__steps article:last-child {
    border-right: 0;
  }

  .package-onboarding__steps article > span {
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 8px;
    font-weight: 700;
  }

  .package-onboarding__steps article.active > span {
    color: var(--brand-soft);
  }

  .package-onboarding__steps article > div {
    display: grid;
    gap: 4px;
  }

  .package-onboarding__steps strong {
    color: var(--text-soft);
    font-size: 10px;
  }

  .package-onboarding__steps p {
    color: var(--text-muted);
    font-size: 8.5px;
    line-height: 1.45;
  }

  /* =====================================================
     REGISTRY OVERVIEW
     ===================================================== */

  .registry-overview {
    display: grid;
    grid-template-columns:
      minmax(250px, 0.72fr)
      minmax(0, 1.28fr);
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .registry-overview__identity {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 18px 19px;
    border-right: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .registry-overview__icon {
    display: grid;
    width: 44px;
    height: 44px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 10px;
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .registry-overview__icon svg {
    width: 20px;
    height: 20px;
  }

  .registry-overview__identity > div {
    display: grid;
    min-width: 0;
    gap: 5px;
  }

  .registry-overview__identity h2 {
    color: var(--text);
    font-size: 18px;
    font-weight: 620;
    letter-spacing: -0.03em;
  }

  .registry-overview__identity code {
    color: var(--text-muted);
    font-size: 9px;
  }

  .registry-metrics {
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
  }

  .registry-metrics article {
    display: grid;
    min-width: 0;
    align-content: center;
    gap: 4px;
    padding: 15px 18px;
    border-right: 1px solid var(--line-soft);
  }

  .registry-metrics article:last-child {
    border-right: 0;
  }

  .registry-metrics span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6.5px;
    letter-spacing: 0.06em;
    text-transform: uppercase;
  }

  .registry-metrics strong {
    color: var(--text);
    font-size: 18px;
    font-weight: 620;
  }

  .registry-metrics small {
    overflow: hidden;
    color: var(--text-faint);
    font-size: 8px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  /* =====================================================
     REGISTRY PANEL
     ===================================================== */

  .registry-panel {
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .registry-panel__header {
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

  .registry-panel__header > div {
    display: grid;
    gap: 3px;
  }

  .registry-panel__header h2 {
    color: var(--text);
    font-size: 14px;
    font-weight: 610;
  }

  .registry-panel__header p:not(.panel-kicker) {
    color: var(--text-muted);
    font-size: 9.5px;
  }

  .registry-count {
    display: grid;
    min-width: 25px;
    height: 23px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    background: var(--bg-elevated);
    color: var(--text-muted);
    padding-inline: 7px;
    font-family: var(--font-mono);
    font-size: 9px;
  }

  .registry-toolbar {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 15px;
    padding: 9px 13px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .package-search {
    position: relative;
    width: min(280px, 100%);
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
    width: 100%;
    min-height: 35px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    outline: 0;
    background: var(--bg-panel);
    color: var(--text);
    padding: 0 10px 0 33px;
    font: inherit;
    font-size: 10.5px;
  }

  .package-search input:focus {
    border-color: var(--brand);
  }

  .registry-filters {
    display: flex;
    flex-wrap: wrap;
    gap: 4px;
  }

  .registry-filters button {
    min-height: 31px;
    gap: 7px;
    border-color: transparent;
    background: transparent;
    color: var(--text-muted);
    padding: 0 9px;
    font-size: 9px;
  }

  .registry-filters button:hover:not(:disabled) {
    border-color: var(--line);
    background: var(--bg-elevated);
    color: var(--text-soft);
    transform: none;
  }

  .registry-filters button.active {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-bright);
  }

  .registry-filters button span {
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

  .registry-empty {
    display: grid;
    min-height: 190px;
    place-content: center;
    justify-items: center;
    gap: 6px;
    padding: 25px;
    text-align: center;
  }

  .registry-empty__icon {
    display: grid;
    width: 42px;
    height: 42px;
    place-items: center;
    border: 1px solid var(--line);
    border-radius: 10px;
    background: var(--bg-ink-soft);
    color: var(--text-muted);
  }

  .registry-empty__icon svg {
    width: 18px;
    height: 18px;
  }

  .registry-empty strong {
    margin-top: 5px;
    color: var(--text-soft);
    font-size: 11px;
  }

  .registry-empty p {
    color: var(--text-muted);
    font-size: 9px;
  }

  /* =====================================================
     PACKAGE ROWS
     ===================================================== */

  .package-list {
    display: grid;
  }

  .package-row {
    position: relative;
    display: grid;
    min-height: 77px;
    grid-template-columns:
      minmax(240px, 1.35fr)
      minmax(170px, 0.85fr)
      minmax(95px, 0.42fr)
      minmax(90px, 0.38fr)
      auto
      34px;
    align-items: center;
    gap: 13px;
    padding: 11px 14px;
    border-bottom: 1px solid var(--line-soft);
    transition: background var(--speed) var(--ease);
  }

  .package-row:last-child {
    border-bottom: 0;
    border-radius:
      0 0
      var(--radius-md)
      var(--radius-md);
  }

  .package-row:hover {
    background: rgba(255, 255, 255, 0.018);
  }

  .package-main {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 11px;
    color: inherit;
    text-decoration: none;
  }

  .package-mark {
    display: grid;
    width: 36px;
    height: 36px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 8px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 11px;
    font-weight: 750;
  }

  .package-identity {
    display: grid;
    min-width: 0;
    gap: 4px;
  }

  .package-identity strong {
    overflow-wrap: anywhere;
    color: var(--brand-bright);
    font-family: var(--font-mono);
    font-size: 10.5px;
    font-weight: 600;
  }

  .package-identity small {
    display: -webkit-box;
    overflow: hidden;
    color: var(--text-muted);
    font-size: 8.5px;
    line-height: 1.4;
    -webkit-box-orient: vertical;
    -webkit-line-clamp: 2;
    line-clamp: 2;
  }

  .package-repository,
  .package-visibility,
  .package-state {
    display: grid;
    min-width: 0;
    gap: 4px;
  }

  .package-repository > span,
  .package-visibility > span:first-child,
  .package-state > span:first-child {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6px;
    text-transform: uppercase;
  }

  .package-repository > a {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 5px;
    overflow: hidden;
    color: var(--text-soft);
    font-size: 8.5px;
    text-decoration: none;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .package-repository > a:hover {
    color: var(--brand-soft);
  }

  .package-repository svg {
    width: 10px;
    height: 10px;
    flex: 0 0 auto;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .package-repository strong {
    color: var(--text-faint);
    font-size: 8.5px;
    font-weight: 500;
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

  .versions-link {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    color: var(--brand-soft);
    font-size: 9px;
    font-weight: 600;
    text-decoration: none;
    white-space: nowrap;
  }

  .versions-link:hover {
    color: var(--brand-bright);
  }

  .versions-link svg {
    width: 12px;
    height: 12px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  /* Package action menu */

  .package-actions {
    position: relative;
  }

  .package-actions summary {
    display: grid;
    width: 31px;
    height: 31px;
    place-items: center;
    border: 1px solid var(--line);
    border-radius: 7px;
    background: transparent;
    color: var(--text-muted);
    cursor: pointer;
    list-style: none;
  }

  .package-actions summary::-webkit-details-marker {
    display: none;
  }

  .package-actions summary:hover {
    border-color: var(--line-strong);
    background: var(--bg-elevated);
    color: var(--text);
  }

  .package-actions summary svg {
    width: 15px;
    height: 15px;
    fill: currentColor;
  }

  .package-actions > div {
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

  .package-actions button {
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

  .package-actions button:last-child {
    border-bottom: 0;
  }

  .package-actions button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
    transform: none;
  }

  .package-actions .action-danger {
    color: var(--danger);
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
    width: min(100%, 430px);
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
    border: 1px solid var(--brand-line);
    border-radius: 9px;
    background: var(--brand-faint);
    color: var(--brand-soft);
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

  .confirm-modal__actions {
    display: flex;
    width: 100%;
    justify-content: flex-end;
    gap: 8px;
    padding-top: 15px;
    border-top: 1px solid var(--line-soft);
  }

  /* =====================================================
     RESPONSIVE
     ===================================================== */

  @media (prefers-reduced-motion: reduce) {
    .packages-page,
    .package-form-panel,
    .page-loading__bar {
      animation: none;
    }
  }

  @media (max-width: 1050px) {
    .package-form {
      grid-template-columns: 1fr;
    }

    .package-form__fields {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .package-preview {
      grid-template-columns:
        auto minmax(0, 1fr);
      align-items: center;
    }

    .package-preview > .panel-kicker,
    .package-preview dl,
    .package-preview__next {
      grid-column: 1 / -1;
    }

    .package-onboarding {
      grid-template-columns: 1fr;
    }

    .package-row {
      grid-template-columns:
        minmax(220px, 1.3fr)
        minmax(150px, 0.8fr)
        minmax(90px, 0.4fr)
        auto
        34px;
    }

    .package-repository {
      display: none;
    }
  }

  @media (max-width: 820px) {
    .registry-overview {
      grid-template-columns: 1fr;
    }

    .registry-overview__identity {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .registry-toolbar {
      align-items: stretch;
      flex-direction: column;
    }

    .package-search {
      width: 100%;
    }

    .registry-filters {
      overflow-x: auto;
      flex-wrap: nowrap;
    }

    .registry-filters button {
      flex: 0 0 auto;
    }

    .package-row {
      grid-template-columns:
        minmax(0, 1fr)
        auto
        34px;
      gap: 10px;
    }

    .package-main {
      grid-column: 1;
    }

    .package-visibility,
    .package-state {
      display: none;
    }

    .versions-link {
      grid-column: 2;
    }

    .package-actions {
      grid-column: 3;
    }
  }

  @media (max-width: 720px) {
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

    .registry-metrics {
      grid-template-columns: 1fr;
    }

    .registry-metrics article {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .registry-metrics article:last-child {
      border-bottom: 0;
    }

    .package-onboarding__steps {
      grid-template-columns: 1fr;
      gap: 15px;
    }

    .package-onboarding__steps article,
    .package-onboarding__steps article:first-child {
      padding: 0;
      border-right: 0;
    }
  }

  @media (max-width: 620px) {
    .visibility-options {
      grid-template-columns: 1fr;
    }

    .package-form__actions,
    .confirm-modal__actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .package-form__actions button,
    .confirm-modal__actions button {
      width: 100%;
    }

    .package-onboarding {
      padding: 26px 19px;
    }

    .package-onboarding__copy h2 {
      font-size: clamp(31px, 10vw, 43px);
    }

    .publish-terminal > footer {
      align-items: flex-start;
      flex-direction: column;
      justify-content: center;
      padding-block: 10px;
    }
  }

  @media (max-width: 500px) {
    .package-form__fields,
    .package-preview {
      padding: 16px;
    }

    .package-row {
      grid-template-columns:
        minmax(0, 1fr)
        34px;
      padding: 13px;
    }

    .versions-link {
      grid-column: 1;
      width: fit-content;
      margin-left: 47px;
    }

    .package-actions {
      grid-column: 2;
      grid-row: 1;
    }

    .package-actions > div {
      position: fixed;
      right: 16px;
      bottom: 16px;
      top: auto;
      width: calc(100% - 32px);
    }
  }
</style>
