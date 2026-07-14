<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount, tick } from 'svelte';

  import {
    createToken,
    defaultCliScopes,
    listTokens,
    revokeToken
  } from '$lib/api/tokens';

  import { listWorkspaces } from '$lib/api/workspaces';

  import {
    ApiError,
    type CliToken,
    type Workspace
  } from '$lib/api/types';

  import InlineError from '$lib/components/InlineError.svelte';
  import WorkspaceAvatar from '$lib/components/WorkspaceAvatar.svelte';

  import {
    canManageTokens,
    canRevokeToken
  } from '$lib/permissions';

  import { auth } from '$lib/stores/auth';
  import { usePageRefresh } from '$lib/stores/pageRefresh';
  import { workspaceContext } from '$lib/stores/workspace';

  type TokenFilter =
    | 'all'
    | 'active'
    | 'expired'
    | 'revoked';

  let workspaces: Workspace[] = [];
  let tokens: CliToken[] = [];

  let selectedWorkspaceId = '';
  let selectedTokenId = '';

  let newToken: CliToken | null = null;

  let loading = true;
  let saving = false;
  let initialized = false;
  let tokenRequestId = 0;

  let showForm = false;
  let tokenName = 'Vix CLI';

  let tokenQuery = '';
  let tokenFilter: TokenFilter = 'all';

  let copied = false;
  let copyResetTimer:
    | ReturnType<typeof setTimeout>
    | undefined;

  let error = '';
  let success = '';

  let tokenToRevoke: CliToken | null = null;
  let revokeDialog: HTMLDialogElement | null = null;
  let revokeConfirmation = '';
  let revoking = '';

  $: selectedWorkspace =
    workspaces.find(
      (workspace) =>
        workspace.id === selectedWorkspaceId
    ) ?? null;

  $: selectedToken =
    tokens.find(
      (token) =>
        token.id === selectedTokenId
    ) ?? null;

  $: currentRole =
    selectedWorkspace?.current_user_role ??
    'viewer';

  $: canManage =
    canManageTokens(currentRole);

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

  $: activeTokens =
    tokens.filter(
      (token) =>
        tokenState(token) === 'active'
    ).length;

  $: expiredTokens =
    tokens.filter(
      (token) =>
        tokenState(token) === 'expired'
    ).length;

  $: revokedTokens =
    tokens.filter(
      (token) =>
        tokenState(token) === 'revoked'
    ).length;

  $: visibleTokens =
    tokens.filter((token) => {
      const query =
        tokenQuery.trim().toLowerCase();

      const scopes =
        token.scopes ?? [];

      const matchesQuery =
        !query ||
        token.name
          .toLowerCase()
          .includes(query) ||
        scopes.some(
          (scope) =>
            scope
              .toLowerCase()
              .includes(query)
        );

      const state =
        tokenState(token);

      const matchesFilter =
        tokenFilter === 'all' ||
        state === tokenFilter;

      return matchesQuery && matchesFilter;
    });

  $: revokeConfirmationValid =
    Boolean(tokenToRevoke) &&
    revokeConfirmation ===
      tokenToRevoke?.name;

  $: if (
    selectedTokenId &&
    !tokens.some(
      (token) =>
        token.id === selectedTokenId
    )
  ) {
    selectedTokenId = '';
  }

  function roleForWorkspace(
    workspaceId: string
  ) {
    return (
      workspaces.find(
        (workspace) =>
          workspace.id === workspaceId
      )?.current_user_role ??
      'viewer'
    );
  }

  function expiryTimestamp(
    value:
      | number
      | string
      | null
      | undefined
  ): number | null {
    if (
      value === null ||
      value === undefined ||
      value === 0 ||
      value === ''
    ) {
      return null;
    }

    if (typeof value === 'number') {
      return value <
        1_000_000_000_000
        ? value * 1000
        : value;
    }

    const numericValue =
      Number(value);

    if (
      Number.isFinite(numericValue) &&
      value.trim() !== ''
    ) {
      return numericValue <
        1_000_000_000_000
        ? numericValue * 1000
        : numericValue;
    }

    const parsed =
      new Date(value).getTime();

    return Number.isNaN(parsed)
      ? null
      : parsed;
  }

  function isExpired(
    value:
      | number
      | string
      | null
      | undefined
  ) {
    const timestamp =
      expiryTimestamp(value);

    return (
      timestamp !== null &&
      timestamp <= Date.now()
    );
  }

  function tokenState(
    token: CliToken
  ): Exclude<TokenFilter, 'all'> {
    if (token.revoked) {
      return 'revoked';
    }

    if (isExpired(token.expires_at)) {
      return 'expired';
    }

    return 'active';
  }

  function tokenStateLabel(
    token: CliToken
  ) {
    const state =
      tokenState(token);

    if (state === 'active') {
      return 'Active';
    }

    if (state === 'expired') {
      return 'Expired';
    }

    return 'Revoked';
  }

  function tokenStateClass(
    token: CliToken
  ) {
    return (
      `token-state ` +
      `token-state--${tokenState(token)}`
    );
  }

  function formatExpiry(
    value:
      | number
      | string
      | null
      | undefined
  ) {
    if (
      value === null ||
      value === undefined ||
      value === 0 ||
      value === ''
    ) {
      return 'Never expires';
    }

    const timestamp =
      expiryTimestamp(value);

    if (timestamp === null) {
      return 'Unknown expiration';
    }

    const date =
      new Intl.DateTimeFormat(
        undefined,
        {
          year: 'numeric',
          month: 'short',
          day: 'numeric'
        }
      ).format(
        new Date(timestamp)
      );

    return isExpired(value)
      ? `Expired ${date}`
      : `Expires ${date}`;
  }

  function tokenInitial(
    token: CliToken
  ) {
    return (
      token.name
        .trim()
        .slice(0, 1)
        .toUpperCase() ||
      'T'
    );
  }

  function openCreateForm() {
    showForm = true;
    selectedTokenId = '';
    error = '';
    success = '';
  }

  function closeCreateForm() {
    if (saving) {
      return;
    }

    showForm = false;
    tokenName = 'Vix CLI';
    error = '';
  }

  function toggleCreateForm() {
    if (showForm) {
      closeCreateForm();
      return;
    }

    openCreateForm();
  }

  function inspectToken(
    token: CliToken
  ) {
    selectedTokenId =
      selectedTokenId === token.id
        ? ''
        : token.id;
  }

  function dismissTokenReveal() {
    newToken = null;
    copied = false;

    success =
      'The token is ready for Vix CLI authentication.';
  }

  function updatePageUrl(
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

  async function copyNewToken() {
    if (!newToken?.value) {
      return;
    }

    try {
      await navigator.clipboard.writeText(
        newToken.value
      );

      copied = true;

      if (copyResetTimer) {
        clearTimeout(
          copyResetTimer
        );
      }

      copyResetTimer = setTimeout(
        () => {
          copied = false;
        },
        1800
      );
    } catch {
      error =
        'Unable to copy the token. Copy it manually before leaving this page.';
    }
  }

  async function load() {
    const user = $auth.user;

    if (
      !$auth.session ||
      !user
    ) {
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

      updatePageUrl(
        selectedWorkspaceId
      );

      await loadTokens();

      initialized = true;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load CLI tokens.';
    } finally {
      loading = false;
    }
  }

  async function loadTokens() {
    const requestId =
      ++tokenRequestId;

    const user = $auth.user;
    const workspaceId =
      selectedWorkspaceId;

    if (
      !user ||
      !workspaceId
    ) {
      tokens = [];
      selectedTokenId = '';
      return;
    }

    workspaceContext.setSelectedWorkspace(
      workspaceId
    );

    const workspaceRole =
      roleForWorkspace(workspaceId);

    if (
      !canManageTokens(workspaceRole)
    ) {
      tokens = [];
      selectedTokenId = '';
      return;
    }

    const data =
      await listTokens(
        workspaceId,
        user.id
      );

    if (
      requestId !== tokenRequestId ||
      selectedWorkspaceId !== workspaceId
    ) {
      return;
    }

    tokens = data.tokens;
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

    tokens = [];
    selectedTokenId = '';

    newToken = null;
    tokenToRevoke = null;

    showForm = false;
    tokenName = 'Vix CLI';

    tokenQuery = '';
    tokenFilter = 'all';

    error = '';
    success = '';
    loading = true;

    workspaceContext.setSelectedWorkspace(
      workspaceId
    );

    updatePageUrl(workspaceId);

    try {
      await loadTokens();
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load CLI tokens.';
    } finally {
      loading = false;
    }
  }

  async function submitToken() {
    const user = $auth.user;

    const cleanName =
      tokenName.trim();

    if (
      !user ||
      !selectedWorkspaceId ||
      !cleanName ||
      !canManage
    ) {
      return;
    }

    saving = true;
    error = '';
    success = '';

    newToken = null;
    copied = false;

    try {
      const created =
        await createToken({
          workspaceId:
            selectedWorkspaceId,

          userId:
            user.id,

          name:
            cleanName,

          scopes:
            defaultCliScopes
        });

      newToken =
        created.token;

      tokens = [
        created.token,
        ...tokens
      ];

      selectedTokenId = '';
      tokenName = 'Vix CLI';
      showForm = false;
      tokenFilter = 'all';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to create CLI token.';
    } finally {
      saving = false;
    }
  }

  async function openRevokeModal(
    token: CliToken
  ) {
    if (
      token.revoked ||
      !canRevokeToken(currentRole)
    ) {
      return;
    }

    tokenToRevoke = token;
    revokeConfirmation = '';

    await tick();

    revokeDialog?.focus();
  }

  function closeRevokeModal() {
    if (revoking) {
      return;
    }

    tokenToRevoke = null;
    revokeConfirmation = '';
  }

  async function revoke() {
    const user = $auth.user;
    const token =
      tokenToRevoke;

    if (
      !user ||
      !token ||
      token.revoked ||
      !canRevokeToken(currentRole) ||
      !revokeConfirmationValid
    ) {
      return;
    }

    revoking = token.id;
    error = '';
    success = '';

    try {
      const revoked =
        await revokeToken({
          workspaceId:
            token.workspace_id,

          userId:
            user.id,

          tokenId:
            token.id
        });

      tokens =
        tokens.map(
          (item) =>
            item.id === token.id
              ? revoked.token
              : item
        );

      if (
        newToken?.id === token.id
      ) {
        newToken = null;
      }

      success =
        `Token "${token.name}" revoked.`;

      tokenToRevoke = null;
      revokeConfirmation = '';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to revoke token.';
    } finally {
      revoking = '';
    }
  }

  function handleModalKeydown(
    event: KeyboardEvent
  ) {
    if (
      event.key === 'Escape' &&
      tokenToRevoke &&
      !revoking
    ) {
      event.preventDefault();
      closeRevokeModal();
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
    CLI tokens | Softadastra Cloud
  </title>

  <meta
    name="description"
    content="Create, review and revoke secure workspace credentials used by the Vix CLI to access Softadastra Cloud."
  />
</svelte:head>

<svelte:window
  onkeydown={handleModalKeydown}
/>

<div class="tokens-page">
  <!-- ===================================================
       HEADER
       =================================================== -->
  <header class="page-header">
    <div class="page-heading">
      <p class="page-eyebrow">
        <span aria-hidden="true"></span>
        CLI access
      </p>

      <h1>Tokens</h1>

      <p>
        {#if loading}
          Loading workspace credentials…
        {:else if selectedWorkspace && canManage}
          Secure credentials for connecting Vix to
          <strong>
            {selectedWorkspace.name}
          </strong>.
        {:else if selectedWorkspace}
          CLI credentials are restricted to workspace
          owners and administrators.
        {:else}
          Create a workspace before connecting the
          Vix CLI.
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
        canManage &&
        !newToken
      }
        <button
          class:button-secondary={showForm}
          class="header-action"
          type="button"
          onclick={toggleCreateForm}
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
            : tokens.length === 0
              ? 'Create first token'
              : 'New token'}
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
        K
      </span>

      <div>
        <strong>
          Loading CLI credentials
        </strong>

        <p>
          Checking workspace permissions and active
          tokens.
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
          CLI access belongs to a workspace.
        </h2>

        <p>
          The workspace defines which packages,
          projects and reports the authenticated Vix
          client can access.
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
       RESTRICTED ACCESS
       =================================================== -->
  {:else if !canManage}
    <section class="restricted-state">
      <span
        class="restricted-state__icon"
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
            x="5"
            y="10"
            width="14"
            height="11"
            rx="2"
          ></rect>

          <path d="M8 10V7a4 4 0 0 1 8 0v3"></path>
          <path d="M12 14v3"></path>
        </svg>
      </span>

      <div>
        <p class="panel-kicker">
          Restricted credentials
        </p>

        <h2>
          Token details are limited to trusted
          workspace roles.
        </h2>

        <p>
          Owners and administrators can create,
          inspect and revoke CLI credentials. Token
          values and metadata remain hidden from
          members and viewers.
        </p>
      </div>

      <div class="restricted-state__role">
        <span>Your current role</span>
        <strong>{currentRole}</strong>
      </div>
    </section>

  {:else}
    <!-- =================================================
         ONE-TIME TOKEN REVEAL
         ================================================= -->
    {#if newToken?.value}
      <section
        class="token-reveal"
        aria-labelledby="token-reveal-title"
      >
        <div class="token-reveal__warning">
          <span
            class="token-reveal__icon"
            aria-hidden="true"
          >
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="1.9"
              stroke-linecap="round"
              stroke-linejoin="round"
            >
              <path d="M15 7a4 4 0 1 0-7.8 1.2L3 12.4V16h3v3h3v2h4l4.8-4.8A4 4 0 0 0 15 7Z"></path>
              <circle
                cx="15"
                cy="7"
                r="1"
              ></circle>
            </svg>
          </span>

          <div>
            <p class="panel-kicker">
              One-time secret
            </p>

            <h2 id="token-reveal-title">
              Save this token before leaving.
            </h2>

            <p>
              Softadastra Cloud will not display this
              secret again. Losing it means creating
              a replacement token.
            </p>
          </div>
        </div>

        <div class="token-reveal__value">
          <code>{newToken.value}</code>

          <button
            class:copied
            class="copy-secret-button"
            type="button"
            aria-label={
              copied
                ? 'Token copied'
                : 'Copy token'
            }
            onclick={copyNewToken}
          >
            {#if copied}
              <svg
                viewBox="0 0 24 24"
                aria-hidden="true"
              >
                <path d="m5 12 4 4L19 6"></path>
              </svg>

              Copied
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

              Copy token
            {/if}
          </button>
        </div>

        <div class="token-reveal__usage">
          <div class="login-command">
            <span>$</span>
            <code>vix login</code>
          </div>

          <p>
            Run the login command locally, then enter
            this token when Vix asks for the
            workspace credential.
          </p>
        </div>

        <div class="token-reveal__footer">
          <div>
            <span>Token name</span>
            <strong>{newToken.name}</strong>
          </div>

          <div>
            <span>Permissions</span>
            <strong>
              {newToken.scopes?.length ?? 0}
              scopes
            </strong>
          </div>

          <button
            class="saved-button"
            type="button"
            onclick={dismissTokenReveal}
          >
            I have saved the token

            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path d="M5 12h14"></path>
              <path d="m13 6 6 6-6 6"></path>
            </svg>
          </button>
        </div>
      </section>
    {/if}

    <!-- =================================================
         CREATE TOKEN
         ================================================= -->
    {#if showForm && !newToken}
      <section
        class="create-panel"
        aria-labelledby="create-token-title"
      >
        <header class="create-panel__header">
          <span
            class="create-panel__icon"
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
              <path d="M15 7a4 4 0 1 0-7.8 1.2L3 12.4V16h3v3h3v2h4l4.8-4.8A4 4 0 0 0 15 7Z"></path>
              <circle
                cx="15"
                cy="7"
                r="1"
              ></circle>
            </svg>
          </span>

          <div>
            <p class="panel-kicker">
              New machine access
            </p>

            <h2 id="create-token-title">
              Create a CLI token
            </h2>

            <p>
              Use a name that identifies the machine
              or environment that will hold this
              credential.
            </p>
          </div>
        </header>

        <form
          class="create-form"
          onsubmit={(event) => {
            event.preventDefault();
            void submitToken();
          }}
        >
          <div class="create-form__fields">
            <label>
              <span>Token name</span>

              <input
                bind:value={tokenName}
                placeholder="Development laptop"
                autocomplete="off"
                required
              />

              <small>
                Examples: Development laptop,
                workstation, release CI or office
                desktop.
              </small>
            </label>

            <fieldset>
              <legend>Assigned permissions</legend>

              <div class="scope-list">
                {#each defaultCliScopes as scope}
                  <div class="scope-item">
                    <span
                      aria-hidden="true"
                    >
                      <svg
                        viewBox="0 0 24 24"
                      >
                        <path d="m5 12 4 4L19 6"></path>
                      </svg>
                    </span>

                    <code>{scope}</code>
                  </div>
                {/each}
              </div>

              <p>
                These scopes are assigned by the
                default Vix CLI access policy.
              </p>
            </fieldset>
          </div>

          <aside class="create-preview">
            <p class="panel-kicker">
              Credential preview
            </p>

            <span class="create-preview__mark">
              {tokenName.trim()
                .slice(0, 1)
                .toUpperCase() || 'T'}
            </span>

            <div class="create-preview__identity">
              <strong>
                {tokenName.trim() ||
                  'Unnamed machine'}
              </strong>

              <span>
                Vix CLI credential
              </span>
            </div>

            <dl>
              <div>
                <dt>Workspace</dt>
                <dd>
                  {selectedWorkspace.name}
                </dd>
              </div>

              <div>
                <dt>Permissions</dt>
                <dd>
                  {defaultCliScopes.length}
                  default scopes
                </dd>
              </div>

              <div>
                <dt>Secret visibility</dt>
                <dd>
                  Displayed once after creation
                </dd>
              </div>
            </dl>

            <div class="create-preview__security">
              <span aria-hidden="true">
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="1.8"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                >
                  <rect
                    x="5"
                    y="10"
                    width="14"
                    height="11"
                    rx="2"
                  ></rect>

                  <path d="M8 10V7a4 4 0 0 1 8 0v3"></path>
                </svg>
              </span>

              <p>
                The generated secret cannot be
                recovered from this page later.
              </p>
            </div>
          </aside>

          <div class="create-form__actions">
            <button
              class="button-secondary"
              type="button"
              disabled={saving}
              onclick={closeCreateForm}
            >
              Cancel
            </button>

            <button
              type="submit"
              disabled={
                saving ||
                !tokenName.trim()
              }
            >
              {saving
                ? 'Creating token…'
                : 'Create token'}
            </button>
          </div>
        </form>
      </section>
    {/if}

    <!-- =================================================
         FIRST TOKEN ONBOARDING
         ================================================= -->
    {#if
      tokens.length === 0 &&
      !showForm &&
      !newToken
    }
      <section class="token-onboarding">
        <div class="token-onboarding__copy">
          <p class="panel-kicker">
            First CLI connection
          </p>

          <h2>
            Connect Vix to
            <span>
              this workspace securely.
            </span>
          </h2>

          <p>
            A token gives one machine permission to
            access workspace packages, projects and
            project metadata through the Vix CLI.
            Create a separate token for each machine
            or automation environment.
          </p>

          <button
            type="button"
            onclick={openCreateForm}
          >
            Create the first token

            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path d="M5 12h14"></path>
              <path d="m13 6 6 6-6 6"></path>
            </svg>
          </button>
        </div>

        <div class="login-terminal">
          <header>
            <div aria-hidden="true">
              <span></span>
              <span></span>
              <span></span>
            </div>

            <strong>
              vix terminal
            </strong>

            <small>local</small>
          </header>

          <div class="login-terminal__body">
            <div class="terminal-command">
              <span>$</span>
              <code>vix login</code>
            </div>

            <div class="terminal-output">
              <span>→</span>
              <code>
                Enter workspace token
              </code>
            </div>

            <div class="terminal-secret">
              <span>••••••••••••••••••••••••</span>
              <i aria-hidden="true"></i>
            </div>

            <div class="terminal-output terminal-output--success">
              <span>✓</span>
              <code>
                Connected to
                {selectedWorkspace.slug}
              </code>
            </div>
          </div>

          <footer>
            <span>
              Machine-specific access
            </span>

            <span>
              Revocable at any time
            </span>
          </footer>
        </div>

        <div class="token-onboarding__steps">
          <article class="active">
            <span>01</span>

            <div>
              <strong>
                Name the machine
              </strong>

              <p>
                Create a credential that is easy to
                recognize later.
              </p>
            </div>
          </article>

          <article>
            <span>02</span>

            <div>
              <strong>
                Save the secret
              </strong>

              <p>
                Copy the generated value while it is
                visible.
              </p>
            </div>
          </article>

          <article>
            <span>03</span>

            <div>
              <strong>
                Authenticate Vix
              </strong>

              <p>
                Use the token during the local login
                workflow.
              </p>
            </div>
          </article>
        </div>
      </section>

    <!-- =================================================
         ACTIVE TOKEN MANAGEMENT
         ================================================= -->
    {:else if tokens.length > 0}
      <section class="access-overview">
        <div class="access-overview__identity">
          <span
            class="access-overview__icon"
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
              <path d="M15 7a4 4 0 1 0-7.8 1.2L3 12.4V16h3v3h3v2h4l4.8-4.8A4 4 0 0 0 15 7Z"></path>
              <circle
                cx="15"
                cy="7"
                r="1"
              ></circle>
            </svg>
          </span>

          <div>
            <p class="panel-kicker">
              Workspace access
            </p>

            <h2>
              {selectedWorkspace.name}
            </h2>

            <code>
              {selectedWorkspace.slug}
            </code>
          </div>
        </div>

        <div class="access-metrics">
          <article>
            <span>Active tokens</span>
            <strong>{activeTokens}</strong>
            <small>
              Machines with valid access
            </small>
          </article>

          <article>
            <span>Expired</span>
            <strong>{expiredTokens}</strong>
            <small>
              Credentials no longer valid
            </small>
          </article>

          <article>
            <span>Revoked</span>
            <strong>{revokedTokens}</strong>
            <small>
              Access disabled manually
            </small>
          </article>
        </div>
      </section>

      <section class="tokens-panel">
        <header class="tokens-panel__header">
          <div>
            <p class="panel-kicker">
              Authorized machines
            </p>

            <h2>CLI credentials</h2>

            <p>
              Review which machines can authenticate
              against this workspace.
            </p>
          </div>

          <span class="tokens-count">
            {visibleTokens.length}
          </span>
        </header>

        <div class="tokens-toolbar">
          <label class="token-search">
            <span class="visually-hidden">
              Find a token
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
              bind:value={tokenQuery}
              type="search"
              placeholder="Find a machine or permission"
            />
          </label>

          <nav
            class="token-filters"
            aria-label="Token status filters"
          >
            <button
              class:active={
                tokenFilter === 'all'
              }
              type="button"
              onclick={() => {
                tokenFilter = 'all';
              }}
            >
              All
              <span>{tokens.length}</span>
            </button>

            <button
              class:active={
                tokenFilter === 'active'
              }
              type="button"
              onclick={() => {
                tokenFilter = 'active';
              }}
            >
              Active
              <span>{activeTokens}</span>
            </button>

            {#if expiredTokens > 0}
              <button
                class:active={
                  tokenFilter === 'expired'
                }
                type="button"
                onclick={() => {
                  tokenFilter = 'expired';
                }}
              >
                Expired
                <span>{expiredTokens}</span>
              </button>
            {/if}

            {#if revokedTokens > 0}
              <button
                class:active={
                  tokenFilter === 'revoked'
                }
                type="button"
                onclick={() => {
                  tokenFilter = 'revoked';
                }}
              >
                Revoked
                <span>{revokedTokens}</span>
              </button>
            {/if}
          </nav>
        </div>

        {#if visibleTokens.length === 0}
          <div class="tokens-empty">
            <span
              class="tokens-empty__icon"
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
              No matching credentials
            </strong>

            <p>
              Change the search text or selected
              token status.
            </p>
          </div>
        {:else}
          <div class="token-list">
            <div class="token-list__heading">
              <span>Machine</span>
              <span>Expiration</span>
              <span>Permissions</span>
              <span>Status</span>
              <span></span>
            </div>

            {#each visibleTokens as token (token.id)}
              <article
                class:selected={
                  selectedTokenId === token.id
                }
                class:inactive={
                  tokenState(token) !== 'active'
                }
                class="token-row"
              >
                <div class="token-identity">
                  <span class="token-mark">
                    {tokenInitial(token)}
                  </span>

                  <div>
                    <strong>{token.name}</strong>

                    <small>
                      Workspace CLI credential
                    </small>
                  </div>
                </div>

                <div class="token-expiration">
                  <span>Expiration</span>

                  <strong>
                    {formatExpiry(
                      token.expires_at
                    )}
                  </strong>
                </div>

                <div class="token-scopes">
                  <span>Permissions</span>

                  <strong>
                    {token.scopes?.length ?? 0}
                    scopes
                  </strong>
                </div>

                <div class="token-status-cell">
                  <span>Status</span>

                  <span class={tokenStateClass(token)}>
                    {tokenStateLabel(token)}
                  </span>
                </div>

                <div class="token-controls">
                  <button
                    class="inspect-button"
                    type="button"
                    aria-expanded={
                      selectedTokenId === token.id
                    }
                    onclick={() =>
                      inspectToken(token)}
                  >
                    {selectedTokenId === token.id
                      ? 'Close'
                      : 'Inspect'}
                  </button>
                </div>
              </article>
            {/each}
          </div>
        {/if}
      </section>

      {#if selectedToken}
        <section
          class="token-inspector"
          aria-labelledby="token-inspector-title"
        >
          <header class="token-inspector__header">
            <div class="token-inspector__identity">
              <span class="token-inspector__mark">
                {tokenInitial(selectedToken)}
              </span>

              <div>
                <p class="panel-kicker">
                  Selected credential
                </p>

                <h2 id="token-inspector-title">
                  {selectedToken.name}
                </h2>

                <p>
                  Safe metadata only. The original
                  token value cannot be displayed
                  again.
                </p>
              </div>
            </div>

            <button
              class="icon-close-button"
              type="button"
              aria-label="Close token inspector"
              onclick={() => {
                selectedTokenId = '';
              }}
            >
              <svg
                viewBox="0 0 24 24"
                aria-hidden="true"
              >
                <path d="M6 6l12 12"></path>
                <path d="M18 6 6 18"></path>
              </svg>
            </button>
          </header>

          <div class="token-inspector__metrics">
            <article>
              <span>Status</span>
              <strong>
                {tokenStateLabel(selectedToken)}
              </strong>
            </article>

            <article>
              <span>Expiration</span>
              <strong>
                {formatExpiry(
                  selectedToken.expires_at
                )}
              </strong>
            </article>

            <article>
              <span>Permissions</span>
              <strong>
                {selectedToken.scopes?.length ?? 0}
              </strong>
            </article>
          </div>

          <section class="token-permissions">
            <header>
              <div>
                <p class="panel-kicker">
                  Access policy
                </p>

                <h3>Assigned permissions</h3>

                <p>
                  These scopes define which workspace
                  operations the Vix client may
                  perform.
                </p>
              </div>
            </header>

            {#if
              (selectedToken.scopes?.length ?? 0) > 0
            }
              <div class="permission-grid">
                {#each selectedToken.scopes as scope}
                  <article>
                    <span aria-hidden="true">
                      <svg
                        viewBox="0 0 24 24"
                      >
                        <path d="m5 12 4 4L19 6"></path>
                      </svg>
                    </span>

                    <code>{scope}</code>
                  </article>
                {/each}
              </div>
            {:else}
              <p class="no-scope-message">
                No permissions are attached to this
                credential.
              </p>
            {/if}
          </section>

          <footer class="token-inspector__footer">
            <div>
              <span
                class="security-note__icon"
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
                    x="5"
                    y="10"
                    width="14"
                    height="11"
                    rx="2"
                  ></rect>

                  <path d="M8 10V7a4 4 0 0 1 8 0v3"></path>
                </svg>
              </span>

              <p>
                Revoke this credential when the
                machine is lost, replaced or no longer
                needs workspace access.
              </p>
            </div>

            {#if
              !selectedToken.revoked &&
              canRevokeToken(currentRole)
            }
              <button
                class="revoke-action"
                type="button"
                onclick={() =>
                  openRevokeModal(
                    selectedToken
                  )}
              >
                Revoke token
              </button>
            {/if}
          </footer>
        </section>
      {/if}
    {/if}
  {/if}
</div>

<!-- =====================================================
     REVOKE CONFIRMATION
     ===================================================== -->
{#if tokenToRevoke}
  <div class="modal-layer">
    <button
      class="modal-backdrop"
      type="button"
      aria-label="Close confirmation"
      onclick={closeRevokeModal}
    ></button>

    <dialog
      bind:this={revokeDialog}
      class="confirm-modal"
      open
      aria-modal="true"
      aria-labelledby="revoke-modal-title"
      aria-describedby="revoke-modal-description"
      tabindex="-1"
    >
      <span
        class="confirm-modal__icon"
        aria-hidden="true"
      >
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
      </span>

      <div class="confirm-modal__content">
        <h2 id="revoke-modal-title">
          Revoke this CLI token?
        </h2>

        <p id="revoke-modal-description">
          <strong>{tokenToRevoke.name}</strong>
          will stop working immediately. Any Vix
          client using this credential will lose
          workspace access.
        </p>
      </div>

      <label class="confirmation-field">
        <span>
          Enter
          <code>{tokenToRevoke.name}</code>
          to confirm
        </span>

        <input
          bind:value={revokeConfirmation}
          placeholder={tokenToRevoke.name}
          autocomplete="off"
        />
      </label>

      <div class="confirm-modal__actions">
        <button
          class="button-secondary"
          type="button"
          disabled={
            revoking === tokenToRevoke.id
          }
          onclick={closeRevokeModal}
        >
          Cancel
        </button>

        <button
          class="danger-button"
          type="button"
          disabled={
            revoking === tokenToRevoke.id ||
            !revokeConfirmationValid
          }
          onclick={revoke}
        >
          {revoking === tokenToRevoke.id
            ? 'Revoking token…'
            : 'Revoke token'}
        </button>
      </div>
    </dialog>
  </div>
{/if}

<style>
  .tokens-page {
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

  /* Header */

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
    font-size: 10px;
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
    font-size: clamp(30px, 4vw, 44px);
    font-weight: 650;
    line-height: 1;
    letter-spacing: -0.045em;
  }

  .page-heading > p:last-child {
    color: var(--text-muted);
    font-size: 13px;
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
    min-height: 42px;
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
    line-height: 1.3;
  }

  .workspace-context strong {
    color: var(--text-soft);
    font-size: 10px;
    font-weight: 600;
  }

  .workspace-context small {
    color: var(--text-muted);
    font-size: 9px;
    text-transform: capitalize;
  }

  .header-action svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
  }

  /* Shared controls */

  button,
  .primary-link {
    display: inline-flex;
    min-height: 39px;
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

  .danger-button,
  .revoke-action {
    border-color: var(--danger-line);
    background: var(--danger-faint);
    color: var(--danger);
  }

  .danger-button:hover:not(:disabled),
  .revoke-action:hover:not(:disabled) {
    border-color: var(--danger);
    background: rgba(248, 113, 113, 0.15);
    color: var(--danger);
  }

  .primary-link svg,
  .token-onboarding__copy button svg,
  .saved-button svg {
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
    font-size: 9px;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }

  /* Feedback */

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
    width: 27px;
    height: 27px;
    flex: 0 0 auto;
    place-items: center;
    border-radius: 7px;
    background: rgba(34, 197, 94, 0.1);
    color: var(--green-soft);
  }

  .success-message svg {
    width: 14px;
    height: 14px;
  }

  .success-message p {
    color: var(--green-soft);
    font-size: 12px;
  }

  /* Loading */

  .page-loading {
    position: relative;
    display: flex;
    min-height: 116px;
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
    width: 44px;
    height: 44px;
    place-items: center;
    border-radius: 10px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 14px;
    font-weight: 800;
  }

  .page-loading > div {
    display: grid;
    gap: 4px;
  }

  .page-loading strong {
    color: var(--text);
    font-size: 13px;
  }

  .page-loading p {
    color: var(--text-muted);
    font-size: 11px;
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

  /* Required and restricted states */

  .required-state,
  .restricted-state {
    display: grid;
    max-width: 790px;
    min-height: 330px;
    align-content: center;
    justify-items: start;
    gap: 16px;
    padding: clamp(30px, 6vw, 58px);
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-lg);
    background: var(--bg-panel);
    box-shadow: var(--shadow-panel);
  }

  .required-state__icon,
  .restricted-state__icon {
    display: grid;
    width: 50px;
    height: 50px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 11px;
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .restricted-state__icon {
    border-color: var(--warning-line);
    background: var(--warning-faint);
    color: var(--warning);
  }

  .required-state__icon svg,
  .restricted-state__icon svg {
    width: 23px;
    height: 23px;
  }

  .required-state > div,
  .restricted-state > div {
    display: grid;
    gap: 8px;
  }

  .required-state h2,
  .restricted-state h2 {
    max-width: 670px;
    color: var(--text);
    font-size: clamp(28px, 4vw, 43px);
    font-weight: 630;
    line-height: 1.04;
    letter-spacing: -0.045em;
  }

  .required-state div > p:not(.panel-kicker),
  .restricted-state div > p:not(.panel-kicker) {
    max-width: 610px;
    color: var(--text-muted);
    font-size: 12.5px;
    line-height: 1.65;
  }

  .restricted-state__role {
    display: flex !important;
    min-height: 42px;
    align-items: center;
    gap: 10px !important;
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
    background: var(--bg-ink-soft);
    padding: 0 12px;
  }

  .restricted-state__role span {
    color: var(--text-muted);
    font-size: 11px;
  }

  .restricted-state__role strong {
    color: var(--warning);
    font-size: 11px;
    text-transform: capitalize;
  }

  /* Token reveal */

  .token-reveal {
    overflow: hidden;
    border: 1px solid var(--warning-line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    box-shadow:
      inset 3px 0 0 var(--warning),
      var(--shadow-panel);
    animation: reveal-enter 320ms var(--ease) both;
  }

  @keyframes reveal-enter {
    from {
      opacity: 0;
      transform: translateY(-6px);
    }
  }

  .token-reveal__warning {
    display: flex;
    align-items: flex-start;
    gap: 13px;
    padding: 19px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--warning-faint);
  }

  .token-reveal__icon {
    display: grid;
    width: 44px;
    height: 44px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--warning-line);
    border-radius: 10px;
    background: var(--bg-panel);
    color: var(--warning);
  }

  .token-reveal__icon svg {
    width: 21px;
    height: 21px;
  }

  .token-reveal__warning > div {
    display: grid;
    gap: 5px;
  }

  .token-reveal h2 {
    color: var(--text);
    font-size: 18px;
    font-weight: 620;
    letter-spacing: -0.025em;
  }

  .token-reveal__warning p:not(.panel-kicker) {
    max-width: 720px;
    color: var(--text-muted);
    font-size: 11px;
    line-height: 1.55;
  }

  .token-reveal__value {
    display: grid;
    grid-template-columns:
      minmax(0, 1fr) auto;
    gap: 8px;
    padding: 18px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink);
  }

  .token-reveal__value code {
    min-width: 0;
    overflow-wrap: anywhere;
    padding: 12px;
    border: 1px solid var(--line-ink);
    border-radius: var(--radius-sm);
    background: var(--bg-ink-soft);
    color: var(--text);
    font-size: 12px;
    line-height: 1.65;
    word-break: break-all;
  }

  .copy-secret-button {
    min-width: 120px;
    border-color: var(--line-strong);
    background: var(--bg-elevated);
    color: var(--text-soft);
  }

  .copy-secret-button:hover:not(:disabled) {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-soft);
    transform: none;
  }

  .copy-secret-button.copied {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .copy-secret-button svg {
    width: 15px;
    height: 15px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .token-reveal__usage {
    display: grid;
    grid-template-columns:
      minmax(190px, 0.4fr)
      minmax(0, 1fr);
    align-items: center;
    gap: 18px;
    padding: 15px 18px;
    border-bottom: 1px solid var(--line-soft);
  }

  .login-command {
    display: flex;
    min-height: 40px;
    align-items: center;
    gap: 9px;
    padding: 0 11px;
    border: 1px solid var(--line-ink);
    border-radius: 7px;
    background: var(--bg-ink);
  }

  .login-command span {
    color: var(--brand);
    font-family: var(--font-mono);
    font-weight: 800;
  }

  .login-command code {
    color: var(--text-soft);
    font-size: 10px;
  }

  .token-reveal__usage p {
    color: var(--text-muted);
    font-size: 10px;
    line-height: 1.55;
  }

  .token-reveal__footer {
    display: grid;
    grid-template-columns:
      minmax(150px, 0.4fr)
      minmax(150px, 0.4fr)
      auto;
    align-items: center;
    gap: 16px;
    padding: 14px 18px;
    background: var(--bg-panel-strong);
  }

  .token-reveal__footer > div {
    display: grid;
    gap: 3px;
  }

  .token-reveal__footer span {
    color: var(--text-muted);
    font-size: 9px;
  }

  .token-reveal__footer strong {
    color: var(--text-soft);
    font-size: 11px;
  }

  .saved-button {
    justify-self: end;
  }

  /* Create panel */

  .create-panel {
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

  .create-panel__header {
    display: flex;
    align-items: flex-start;
    gap: 12px;
    padding: 18px 19px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .create-panel__icon {
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

  .create-panel__icon svg {
    width: 20px;
    height: 20px;
  }

  .create-panel__header > div {
    display: grid;
    gap: 4px;
  }

  .create-panel h2 {
    color: var(--text);
    font-size: 16px;
    font-weight: 620;
  }

  .create-panel__header p:not(.panel-kicker) {
    max-width: 650px;
    color: var(--text-muted);
    font-size: 10.5px;
    line-height: 1.5;
  }

  .create-form {
    display: grid;
    grid-template-columns:
      minmax(0, 1.35fr)
      minmax(280px, 0.65fr);
  }

  .create-form__fields {
    display: grid;
    align-content: start;
    gap: 19px;
    padding: 21px;
    border-right: 1px solid var(--line-soft);
  }

  .create-form label {
    display: grid;
    gap: 7px;
  }

  .create-form label > span,
  .create-form legend {
    color: var(--text-soft);
    font-size: 11px;
    font-weight: 550;
  }

  .create-form label > small {
    color: var(--text-muted);
    font-size: 9px;
    line-height: 1.5;
  }

  .create-form input,
  .confirmation-field input {
    width: 100%;
    min-height: 42px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    outline: 0;
    background: var(--bg-ink-soft);
    color: var(--text);
    padding: 0 11px;
    font: inherit;
    font-size: 12px;
  }

  .create-form input:focus,
  .confirmation-field input:focus {
    border-color: var(--brand);
    box-shadow: 0 0 0 3px var(--brand-faint);
  }

  .create-form fieldset {
    display: grid;
    gap: 9px;
    margin: 0;
    border: 0;
    padding: 0;
  }

  .scope-list {
    display: grid;
    grid-template-columns:
      repeat(2, minmax(0, 1fr));
    gap: 8px;
  }

  .scope-item {
    display: grid;
    min-width: 0;
    min-height: 42px;
    grid-template-columns:
      auto minmax(0, 1fr);
    align-items: center;
    gap: 8px;
    padding: 8px 10px;
    border: 1px solid var(--line);
    border-radius: 7px;
    background: var(--bg-ink-soft);
  }

  .scope-item > span {
    display: grid;
    width: 20px;
    height: 20px;
    place-items: center;
    border-radius: 5px;
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .scope-item svg {
    width: 11px;
    height: 11px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
  }

  .scope-item code {
    min-width: 0;
    overflow-wrap: anywhere;
    color: var(--text-soft);
    font-size: 9px;
  }

  .create-form fieldset > p {
    color: var(--text-muted);
    font-size: 9px;
    line-height: 1.5;
  }

  .create-preview {
    display: grid;
    align-content: start;
    justify-items: start;
    gap: 12px;
    padding: 21px;
    background: var(--bg-ink-soft);
  }

  .create-preview__mark {
    display: grid;
    width: 49px;
    height: 49px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 12px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 15px;
    font-weight: 750;
  }

  .create-preview__identity {
    display: grid;
    gap: 3px;
  }

  .create-preview__identity strong {
    color: var(--text);
    font-size: 11px;
  }

  .create-preview__identity span {
    color: var(--text-muted);
    font-size: 9px;
  }

  .create-preview dl {
    display: grid;
    width: 100%;
    margin: 3px 0 0;
    border: 1px solid var(--line-soft);
    border-radius: 8px;
  }

  .create-preview dl > div {
    display: grid;
    gap: 4px;
    padding: 10px;
    border-bottom: 1px solid var(--line-soft);
  }

  .create-preview dl > div:last-child {
    border-bottom: 0;
  }

  .create-preview dt {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 8px;
    text-transform: uppercase;
  }

  .create-preview dd {
    overflow-wrap: anywhere;
    margin: 0;
    color: var(--text-soft);
    font-size: 9px;
    line-height: 1.45;
  }

  .create-preview__security {
    display: flex;
    width: 100%;
    align-items: flex-start;
    gap: 9px;
    padding: 10px;
    border: 1px solid var(--warning-line);
    border-radius: 7px;
    background: var(--warning-faint);
  }

  .create-preview__security > span {
    color: var(--warning);
  }

  .create-preview__security svg {
    width: 15px;
    height: 15px;
  }

  .create-preview__security p {
    color: var(--text-muted);
    font-size: 9px;
    line-height: 1.5;
  }

  .create-form__actions {
    grid-column: 1 / -1;
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    padding: 13px 20px;
    border-top: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  /* Onboarding */

  .token-onboarding {
    display: grid;
    grid-template-columns:
      minmax(0, 0.82fr)
      minmax(410px, 1.18fr);
    gap: clamp(38px, 7vw, 78px);
    align-items: center;
    padding: clamp(32px, 5vw, 52px);
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-lg);
    background: var(--bg-panel);
    box-shadow: var(--shadow-panel);
  }

  .token-onboarding__copy {
    display: grid;
    align-content: center;
  }

  .token-onboarding__copy h2 {
    max-width: 550px;
    margin-top: 10px;
    color: var(--text);
    font-size: clamp(32px, 4.7vw, 51px);
    font-weight: 640;
    line-height: 1;
    letter-spacing: -0.052em;
  }

  .token-onboarding__copy h2 span {
    display: block;
    color: var(--brand-soft);
  }

  .token-onboarding__copy > p:not(.panel-kicker) {
    max-width: 550px;
    margin-top: 18px;
    color: var(--text-muted);
    font-size: 12.5px;
    line-height: 1.7;
  }

  .token-onboarding__copy > button {
    width: fit-content;
    margin-top: 25px;
  }

  .login-terminal {
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: 11px;
    background: var(--bg-ink);
    box-shadow: var(--shadow-panel);
  }

  .login-terminal > header {
    display: grid;
    min-height: 44px;
    grid-template-columns:
      auto minmax(0, 1fr) auto;
    align-items: center;
    gap: 10px;
    padding: 0 12px;
    border-bottom: 1px solid var(--line-ink);
    background: var(--bg-ink-soft);
  }

  .login-terminal header > div {
    display: flex;
    gap: 5px;
  }

  .login-terminal header > div span {
    width: 7px;
    height: 7px;
    border-radius: 50%;
    background: var(--text-faint);
  }

  .login-terminal header > div span:first-child {
    background: var(--brand);
  }

  .login-terminal header strong,
  .login-terminal header small {
    font-family: var(--font-mono);
    font-size: 8px;
  }

  .login-terminal header strong {
    overflow: hidden;
    color: var(--text-muted);
    text-align: center;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .login-terminal header small {
    color: var(--text-faint);
  }

  .login-terminal__body {
    display: grid;
    align-content: center;
    min-height: 235px;
    padding: 25px 21px;
  }

  .terminal-command,
  .terminal-output {
    display: grid;
    min-height: 36px;
    grid-template-columns:
      23px minmax(0, 1fr);
    align-items: center;
    font-family: var(--font-mono);
    font-size: 9.5px;
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

  .terminal-output--success span,
  .terminal-output--success code {
    color: var(--green-soft);
  }

  .terminal-secret {
    display: flex;
    min-height: 39px;
    align-items: center;
    gap: 8px;
    margin: 5px 0 12px 23px;
    padding: 0 10px;
    border: 1px solid var(--line-ink);
    border-radius: 6px;
    background: var(--bg-ink-soft);
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 9px;
  }

  .terminal-secret i {
    width: 6px;
    height: 14px;
    background: var(--brand);
    animation: cursor-blink 1s step-end infinite;
  }

  @keyframes cursor-blink {
    50% {
      opacity: 0;
    }
  }

  .login-terminal > footer {
    display: flex;
    min-height: 40px;
    align-items: center;
    justify-content: space-between;
    gap: 15px;
    padding: 0 12px;
    border-top: 1px solid var(--line-ink);
    background: var(--bg-ink-soft);
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 7px;
  }

  .token-onboarding__steps {
    grid-column: 1 / -1;
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    padding-top: 26px;
    border-top: 1px solid var(--line);
  }

  .token-onboarding__steps article {
    display: grid;
    grid-template-columns:
      auto minmax(0, 1fr);
    gap: 10px;
    padding: 0 18px;
    border-right: 1px solid var(--line);
  }

  .token-onboarding__steps article:first-child {
    padding-left: 0;
  }

  .token-onboarding__steps article:last-child {
    border-right: 0;
  }

  .token-onboarding__steps article > span {
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 9px;
    font-weight: 700;
  }

  .token-onboarding__steps article.active > span {
    color: var(--brand-soft);
  }

  .token-onboarding__steps article > div {
    display: grid;
    gap: 4px;
  }

  .token-onboarding__steps strong {
    color: var(--text-soft);
    font-size: 10.5px;
  }

  .token-onboarding__steps p {
    color: var(--text-muted);
    font-size: 9px;
    line-height: 1.45;
  }

  /* Access overview */

  .access-overview {
    display: grid;
    grid-template-columns:
      minmax(250px, 0.72fr)
      minmax(0, 1.28fr);
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .access-overview__identity {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 19px;
    border-right: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .access-overview__icon {
    display: grid;
    width: 45px;
    height: 45px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 10px;
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .access-overview__icon svg {
    width: 21px;
    height: 21px;
  }

  .access-overview__identity > div {
    display: grid;
    min-width: 0;
    gap: 5px;
  }

  .access-overview__identity h2 {
    color: var(--text);
    font-size: 18px;
    font-weight: 620;
    letter-spacing: -0.03em;
  }

  .access-overview__identity code {
    color: var(--text-muted);
    font-size: 9px;
  }

  .access-metrics {
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
  }

  .access-metrics article {
    display: grid;
    align-content: center;
    gap: 4px;
    padding: 16px 18px;
    border-right: 1px solid var(--line-soft);
  }

  .access-metrics article:last-child {
    border-right: 0;
  }

  .access-metrics span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 8px;
    letter-spacing: 0.06em;
    text-transform: uppercase;
  }

  .access-metrics strong {
    color: var(--text);
    font-size: 19px;
    font-weight: 620;
  }

  .access-metrics small {
    color: var(--text-faint);
    font-size: 8.5px;
  }

  /* Tokens panel */

  .tokens-panel {
    overflow: visible;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .tokens-panel__header {
    display: flex;
    min-height: 80px;
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

  .tokens-panel__header > div {
    display: grid;
    gap: 3px;
  }

  .tokens-panel__header h2 {
    color: var(--text);
    font-size: 14px;
    font-weight: 610;
  }

  .tokens-panel__header p:not(.panel-kicker) {
    color: var(--text-muted);
    font-size: 10px;
  }

  .tokens-count {
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

  .tokens-toolbar {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 15px;
    padding: 10px 13px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .token-search {
    position: relative;
    width: min(310px, 100%);
  }

  .token-search svg {
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

  .token-search input {
    width: 100%;
    min-height: 36px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    outline: 0;
    background: var(--bg-panel);
    color: var(--text);
    padding: 0 10px 0 33px;
    font: inherit;
    font-size: 11px;
  }

  .token-search input:focus {
    border-color: var(--brand);
  }

  .token-filters {
    display: flex;
    flex-wrap: wrap;
    gap: 4px;
  }

  .token-filters button {
    min-height: 31px;
    gap: 7px;
    border-color: transparent;
    background: transparent;
    color: var(--text-muted);
    padding: 0 9px;
    font-size: 9px;
  }

  .token-filters button:hover:not(:disabled) {
    border-color: var(--line);
    background: var(--bg-elevated);
    color: var(--text-soft);
    transform: none;
  }

  .token-filters button.active {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-bright);
  }

  .token-filters button span {
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

  .tokens-empty {
    display: grid;
    min-height: 190px;
    place-content: center;
    justify-items: center;
    gap: 6px;
    padding: 25px;
    text-align: center;
  }

  .tokens-empty__icon {
    display: grid;
    width: 42px;
    height: 42px;
    place-items: center;
    border: 1px solid var(--line);
    border-radius: 10px;
    background: var(--bg-ink-soft);
    color: var(--text-muted);
  }

  .tokens-empty__icon svg {
    width: 18px;
    height: 18px;
  }

  .tokens-empty strong {
    color: var(--text-soft);
    font-size: 11px;
  }

  .tokens-empty p {
    color: var(--text-muted);
    font-size: 9px;
  }

  /* Token rows */

  .token-list {
    display: grid;
  }

  .token-list__heading,
  .token-row {
    display: grid;
    grid-template-columns:
      minmax(230px, 1.2fr)
      minmax(165px, 0.75fr)
      minmax(110px, 0.45fr)
      minmax(100px, 0.45fr)
      minmax(80px, auto);
    align-items: center;
    gap: 14px;
  }

  .token-list__heading {
    min-height: 35px;
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

  .token-row {
    min-height: 76px;
    padding: 11px 14px;
    border-bottom: 1px solid var(--line-soft);
    transition:
      background var(--speed) var(--ease),
      box-shadow var(--speed) var(--ease);
  }

  .token-row:last-child {
    border-bottom: 0;
  }

  .token-row:hover {
    background: rgba(255, 255, 255, 0.018);
  }

  .token-row.selected {
    background: var(--brand-faint);
    box-shadow: inset 2px 0 0 var(--brand);
  }

  .token-row.inactive
    .token-identity,
  .token-row.inactive
    .token-expiration,
  .token-row.inactive
    .token-scopes {
    opacity: 0.62;
  }

  .token-identity {
    display: grid;
    min-width: 0;
    grid-template-columns:
      auto minmax(0, 1fr);
    align-items: center;
    gap: 10px;
  }

  .token-mark {
    display: grid;
    width: 36px;
    height: 36px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 8px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 11px;
    font-weight: 750;
  }

  .token-identity > div {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .token-identity strong {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 11px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .token-identity small {
    color: var(--text-muted);
    font-size: 8px;
  }

  .token-expiration,
  .token-scopes,
  .token-status-cell {
    display: grid;
    min-width: 0;
    gap: 4px;
  }

  .token-expiration > span,
  .token-scopes > span,
  .token-status-cell > span:first-child {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 7px;
    text-transform: uppercase;
  }

  .token-expiration strong,
  .token-scopes strong {
    color: var(--text-soft);
    font-size: 9px;
    font-weight: 550;
  }

  .token-state {
    display: inline-flex;
    width: fit-content;
    min-height: 23px;
    align-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    padding: 1px 8px;
    font-size: 8.5px;
  }

  .token-state--active {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .token-state--expired {
    border-color: var(--warning-line);
    background: var(--warning-faint);
    color: var(--warning);
  }

  .token-state--revoked {
    border-color: var(--danger-line);
    background: var(--danger-faint);
    color: var(--danger);
  }

  .token-controls {
    display: flex;
    justify-content: flex-end;
  }

  .inspect-button {
    min-height: 30px;
    border-color: var(--line-strong);
    background: var(--bg-elevated);
    color: var(--text-soft);
    padding: 0 9px;
    font-size: 8.5px;
  }

  .inspect-button:hover:not(:disabled) {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-soft);
    transform: none;
  }

  /* Token inspector */

  .token-inspector {
    overflow: hidden;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    animation: inspector-enter 260ms var(--ease) both;
  }

  @keyframes inspector-enter {
    from {
      opacity: 0;
      transform: translateY(-5px);
    }
  }

  .token-inspector__header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    padding: 15px 16px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .token-inspector__identity {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 11px;
  }

  .token-inspector__mark {
    display: grid;
    width: 41px;
    height: 41px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 9px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 13px;
    font-weight: 750;
  }

  .token-inspector__identity > div {
    display: grid;
    min-width: 0;
    gap: 4px;
  }

  .token-inspector h2 {
    overflow-wrap: anywhere;
    color: var(--text);
    font-size: 15px;
    font-weight: 610;
  }

  .token-inspector__identity p:not(.panel-kicker) {
    color: var(--text-muted);
    font-size: 9px;
  }

  .icon-close-button {
    display: grid;
    width: 30px;
    height: 30px;
    min-height: 30px;
    place-items: center;
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-muted);
    padding: 0;
  }

  .icon-close-button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
    transform: none;
  }

  .icon-close-button svg {
    width: 13px;
    height: 13px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
  }

  .token-inspector__metrics {
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    border-bottom: 1px solid var(--line-soft);
  }

  .token-inspector__metrics article {
    display: grid;
    gap: 4px;
    padding: 14px 16px;
    border-right: 1px solid var(--line-soft);
  }

  .token-inspector__metrics article:last-child {
    border-right: 0;
  }

  .token-inspector__metrics span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 7px;
    text-transform: uppercase;
  }

  .token-inspector__metrics strong {
    color: var(--text-soft);
    font-size: 11px;
    font-weight: 600;
  }

  .token-permissions {
    display: grid;
  }

  .token-permissions > header {
    padding: 14px 16px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .token-permissions header > div {
    display: grid;
    gap: 3px;
  }

  .token-permissions h3 {
    color: var(--text);
    font-size: 12px;
    font-weight: 600;
  }

  .token-permissions header p:not(.panel-kicker) {
    color: var(--text-muted);
    font-size: 9px;
  }

  .permission-grid {
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    gap: 8px;
    padding: 15px;
  }

  .permission-grid article {
    display: grid;
    min-width: 0;
    min-height: 43px;
    grid-template-columns:
      auto minmax(0, 1fr);
    align-items: center;
    gap: 8px;
    padding: 8px 10px;
    border: 1px solid var(--line);
    border-radius: 7px;
    background: var(--bg-ink-soft);
  }

  .permission-grid article > span {
    display: grid;
    width: 20px;
    height: 20px;
    place-items: center;
    border-radius: 5px;
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .permission-grid svg {
    width: 11px;
    height: 11px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
  }

  .permission-grid code {
    min-width: 0;
    overflow-wrap: anywhere;
    color: var(--text-soft);
    font-size: 9px;
  }

  .no-scope-message {
    padding: 18px 16px;
    color: var(--text-muted);
    font-size: 10px;
  }

  .token-inspector__footer {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    padding: 13px 15px;
    border-top: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .token-inspector__footer > div {
    display: flex;
    max-width: 680px;
    align-items: flex-start;
    gap: 9px;
  }

  .security-note__icon {
    color: var(--warning);
  }

  .security-note__icon svg {
    width: 16px;
    height: 16px;
  }

  .token-inspector__footer p {
    color: var(--text-muted);
    font-size: 9px;
    line-height: 1.5;
  }

  .revoke-action {
    flex: 0 0 auto;
    min-height: 34px;
    padding: 0 11px;
    font-size: 9px;
  }

  /* Modal */

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
    width: min(100%, 460px);
    max-width: none;
    justify-items: start;
    gap: 14px;
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
    width: 39px;
    height: 39px;
    place-items: center;
    border: 1px solid var(--danger-line);
    border-radius: 9px;
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
    font-size: 17px;
    font-weight: 620;
  }

  .confirm-modal__content p {
    color: var(--text-muted);
    font-size: 11px;
    line-height: 1.65;
  }

  .confirm-modal__content strong {
    color: var(--text);
    font-weight: 600;
  }

  .confirmation-field {
    display: grid;
    width: 100%;
    gap: 7px;
  }

  .confirmation-field > span {
    color: var(--text-soft);
    font-size: 10px;
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

  @media (prefers-reduced-motion: reduce) {
    .tokens-page,
    .page-loading__bar,
    .token-reveal,
    .create-panel,
    .token-inspector,
    .terminal-secret i {
      animation: none;
    }
  }

  @media (max-width: 1040px) {
    .create-form {
      grid-template-columns: 1fr;
    }

    .create-form__fields {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .create-preview {
      grid-template-columns:
        auto minmax(0, 1fr);
      align-items: center;
    }

    .create-preview > .panel-kicker,
    .create-preview dl,
    .create-preview__security {
      grid-column: 1 / -1;
    }

    .token-onboarding {
      grid-template-columns: 1fr;
    }

    .permission-grid {
      grid-template-columns:
        repeat(2, minmax(0, 1fr));
    }
  }

  @media (max-width: 860px) {
    .access-overview {
      grid-template-columns: 1fr;
    }

    .access-overview__identity {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .tokens-toolbar {
      align-items: stretch;
      flex-direction: column;
    }

    .token-search {
      width: 100%;
    }

    .token-filters {
      overflow-x: auto;
      flex-wrap: nowrap;
    }

    .token-filters button {
      flex: 0 0 auto;
    }

    .token-list__heading {
      display: none;
    }

    .token-row {
      grid-template-columns:
        minmax(0, 1fr) auto;
      gap: 12px;
      padding: 14px;
    }

    .token-identity {
      grid-column: 1;
    }

    .token-controls {
      grid-column: 2;
      grid-row: 1;
    }

    .token-expiration,
    .token-scopes,
    .token-status-cell {
      grid-column: 1 / -1;
      padding-top: 9px;
      border-top: 1px solid var(--line-soft);
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

    .token-reveal__usage {
      grid-template-columns: 1fr;
    }

    .token-reveal__footer {
      grid-template-columns:
        repeat(2, minmax(0, 1fr));
    }

    .saved-button {
      grid-column: 1 / -1;
      width: 100%;
    }

    .access-metrics {
      grid-template-columns: 1fr;
    }

    .access-metrics article {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .access-metrics article:last-child {
      border-bottom: 0;
    }

    .token-onboarding__steps {
      grid-template-columns: 1fr;
      gap: 15px;
    }

    .token-onboarding__steps article,
    .token-onboarding__steps article:first-child {
      padding: 0;
      border-right: 0;
    }

    .token-inspector__metrics {
      grid-template-columns: 1fr;
    }

    .token-inspector__metrics article {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .token-inspector__metrics article:last-child {
      border-bottom: 0;
    }

    .token-inspector__footer {
      align-items: stretch;
      flex-direction: column;
    }

    .revoke-action {
      width: 100%;
    }
  }

  @media (max-width: 620px) {
    .token-reveal__value {
      grid-template-columns: 1fr;
    }

    .copy-secret-button {
      width: 100%;
    }

    .scope-list,
    .permission-grid {
      grid-template-columns: 1fr;
    }

    .create-form__actions,
    .confirm-modal__actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .create-form__actions button,
    .confirm-modal__actions button {
      width: 100%;
    }

    .token-onboarding {
      padding: 27px 19px;
    }

    .token-onboarding__copy h2 {
      font-size: clamp(32px, 10vw, 44px);
    }

    .login-terminal > footer {
      align-items: flex-start;
      flex-direction: column;
      justify-content: center;
      padding-block: 10px;
    }
  }

  @media (max-width: 500px) {
    .create-form__fields,
    .create-preview {
      padding: 16px;
    }

    .token-reveal__footer {
      grid-template-columns: 1fr;
    }

    .saved-button {
      grid-column: 1;
    }

    .token-row {
      grid-template-columns: 1fr;
    }

    .token-controls {
      grid-column: 1;
      grid-row: auto;
      justify-content: flex-start;
    }

    .token-expiration,
    .token-scopes,
    .token-status-cell {
      grid-column: 1;
    }

    .token-inspector__header {
      align-items: flex-start;
    }

    .token-inspector__identity {
      align-items: flex-start;
    }
  }
</style>
