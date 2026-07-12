<script lang="ts">
  import { usePageRefresh } from '$lib/stores/pageRefresh';
  import { goto } from '$app/navigation';
  import { tick } from 'svelte';
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
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import { canManageTokens, canRevokeToken } from '$lib/permissions';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  let workspaces: Workspace[] = [];
  let selectedWorkspaceId = '';
  let tokens: CliToken[] = [];
  let newToken: CliToken | null = null;
  let tokenToRevoke: CliToken | null = null;
  let revokeDialog: HTMLDialogElement | null = null;

  let tokenName = 'Vix CLI';
  let loading = true;
  let saving = false;
  let initialized = false;
  let tokenRequestId = 0;
  let revoking = '';
  let showForm = false;
  let copied = false;
  let error = '';
  let success = '';

  $: selectedWorkspace =
    workspaces.find((workspace) => workspace.id === selectedWorkspaceId) ?? null;

  $: currentRole = selectedWorkspace?.current_user_role ?? 'viewer';
  $: canManage = canManageTokens(currentRole);

  $: activeTokens = tokens.filter(
    (token) => !token.revoked && !isExpired(token.expires_at)
  ).length;

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

  function roleForWorkspace(workspaceId: string) {
    return (
      workspaces.find((workspace) => workspace.id === workspaceId)
        ?.current_user_role ?? 'viewer'
    );
  }

  function expiryTimestamp(
    value: number | string | null | undefined
  ): number | null {
    if (value === null || value === undefined || value === 0 || value === '') {
      return null;
    }

    if (typeof value === 'number') {
      return value < 1_000_000_000_000 ? value * 1000 : value;
    }

    const numericValue = Number(value);

    if (Number.isFinite(numericValue) && value.trim() !== '') {
      return numericValue < 1_000_000_000_000
        ? numericValue * 1000
        : numericValue;
    }

    const parsed = new Date(value).getTime();

    return Number.isNaN(parsed) ? null : parsed;
  }

  function formatExpiry(value: number | string | null | undefined) {
    if (value === null || value === undefined || value === 0 || value === '') {
      return 'Never expires';
    }

    const timestamp = expiryTimestamp(value);

    if (timestamp === null) {
      return 'Unknown expiration';
    }

    return new Date(timestamp).toLocaleDateString(undefined, {
      year: 'numeric',
      month: 'short',
      day: 'numeric'
    });
  }

  function isExpired(value: number | string | null | undefined) {
    const timestamp = expiryTimestamp(value);

    return timestamp !== null && timestamp <= Date.now();
  }

  function tokenStatus(token: CliToken) {
    if (token.revoked) {
      return 'Revoked';
    }

    if (isExpired(token.expires_at)) {
      return 'Expired';
    }

    return 'Active';
  }

  async function copyNewToken() {
    if (!newToken?.value) {
      return;
    }

    try {
      await navigator.clipboard.writeText(newToken.value);
      copied = true;

      window.setTimeout(() => {
        copied = false;
      }, 1800);
    } catch {
      error = 'Unable to copy the token. Copy it manually.';
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
      const workspaceData = await listWorkspaces(user.id);
      workspaces = workspaceData.workspaces;

      const requestedWorkspaceId = new URLSearchParams(
        window.location.search
      ).get('workspace_id');

      selectedWorkspaceId =
        requestedWorkspaceId &&
        workspaces.some((workspace) => workspace.id === requestedWorkspaceId)
          ? requestedWorkspaceId
          : workspaces[0]?.id ?? '';

      workspaceContext.setWorkspaces(workspaces, selectedWorkspaceId);

      await loadTokens();
      initialized = true;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load tokens.';
    } finally {
      loading = false;
    }
  }

  async function loadTokens() {
    const requestId = ++tokenRequestId;
    const user = $auth.user;

    if (!user || !selectedWorkspaceId) {
      tokens = [];
      return;
    }

    workspaceContext.setSelectedWorkspace(selectedWorkspaceId);

    const workspaceRole = roleForWorkspace(selectedWorkspaceId);

    if (!canManageTokens(workspaceRole)) {
      tokens = [];
      return;
    }

    const workspaceId = selectedWorkspaceId;
    const data = await listTokens(workspaceId, user.id);

    if (
      requestId !== tokenRequestId ||
      selectedWorkspaceId !== workspaceId
    ) {
      return;
    }

    tokens = data.tokens;
  }

  async function switchWorkspace(workspaceId: string) {
    selectedWorkspaceId = workspaceId;
    tokens = [];
    newToken = null;
    tokenToRevoke = null;
    showForm = false;
    tokenName = 'Vix CLI';
    error = '';
    success = '';
    loading = true;

    const url = new URL(window.location.href);
    url.searchParams.set('workspace_id', workspaceId);
    history.replaceState(null, '', `${url.pathname}${url.search}`);

    try {
      await loadTokens();
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load tokens.';
    } finally {
      loading = false;
    }
  }

  async function submitToken() {
    const user = $auth.user;

    if (
      !user ||
      !selectedWorkspaceId ||
      !tokenName.trim() ||
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
      const created = await createToken({
        workspaceId: selectedWorkspaceId,
        userId: user.id,
        name: tokenName.trim(),
        scopes: defaultCliScopes
      });

      newToken = created.token;
      tokens = [created.token, ...tokens];

      tokenName = 'Vix CLI';
      showForm = false;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to create token.';
    } finally {
      saving = false;
    }
  }

  async function openRevokeModal(token: CliToken) {
    if (token.revoked || !canRevokeToken(currentRole)) {
      return;
    }

    tokenToRevoke = token;

    await tick();
    revokeDialog?.focus();
  }
  function closeRevokeModal() {
    if (revoking) {
      return;
    }

    tokenToRevoke = null;
  }

  async function revoke() {
    const user = $auth.user;
    const token = tokenToRevoke;

    if (
      !user ||
      !token ||
      token.revoked ||
      !canRevokeToken(currentRole)
    ) {
      return;
    }

    revoking = token.id;
    error = '';
    success = '';

    try {
      const revoked = await revokeToken({
        workspaceId: token.workspace_id,
        userId: user.id,
        tokenId: token.id
      });

      tokens = tokens.map((item) =>
        item.id === token.id ? revoked.token : item
      );

      if (newToken?.id === token.id) {
        newToken = null;
      }

      success = `Token "${token.name}" revoked.`;
      tokenToRevoke = null;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to revoke token.';
    } finally {
      revoking = '';
    }
  }

  function handleModalKeydown(event: KeyboardEvent) {
    if (
      event.key === 'Escape' &&
      tokenToRevoke &&
      !revoking
    ) {
      closeRevokeModal();
    }
  }

  function handleSubmitToken(event: SubmitEvent) {
    event.preventDefault();
    void submitToken();
  }

  usePageRefresh(load);
</script>

<svelte:head>
  <title>Tokens | Softadastra Cloud</title>
</svelte:head>

<svelte:window onkeydown={handleModalKeydown} />

<PageHeader
  eyebrow="CLI"
  title="Tokens"
  workspaceName={selectedWorkspace?.name ?? ''}
  role={currentRole}
>
  {#if canManage && !showForm}
    <button
      class="new-token-button"
      type="button"
      onclick={() => {
        showForm = true;
        error = '';
        success = '';
      }}
    >
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path d="M12 5v14M5 12h14" />
      </svg>
      New token
    </button>
  {/if}
</PageHeader>

<div class="page-introduction">
  <p>
    Tokens allow the Vix CLI to access this workspace.
  </p>

  {#if !loading && canManage}
    <div class="token-summary" aria-label="Token summary">
      <span class="summary-stat">
        <strong>{activeTokens}</strong>
        <span class="summary-label">active</span>
      </span>

      <span class="summary-stat">
        <strong>{tokens.length}</strong>
        <span class="summary-label">total</span>
      </span>
    </div>
  {/if}
</div>

<InlineError message={error} />

{#if success}
  <p class="success-message" role="status">
    {success}
  </p>
{/if}

{#if newToken?.value}
  <section class="token-reveal">
    <div class="section-header">
      <div>
        <span class="eyebrow">Save this now</span>
        <h2>New token</h2>
        <p>Copy this value now. It will not be displayed again.</p>
      </div>
    </div>

    <div class="copy-field">
      <code>{newToken.value}</code>

      <button
        class:copied
        class="copy-button"
        type="button"
        aria-label={copied ? 'Token copied' : 'Copy token'}
        title={copied ? 'Copied' : 'Copy token'}
        onclick={copyNewToken}
      >
        {#if copied}
          <svg viewBox="0 0 24 24" aria-hidden="true">
            <path d="m5 12 4 4L19 6" />
          </svg>
        {:else}
          <svg viewBox="0 0 24 24" aria-hidden="true">
            <rect x="8" y="8" width="11" height="11" rx="2" />
            <path
              d="M16 8V5a2 2 0 0 0-2-2H5a2 2 0 0 0-2 2v9a2 2 0 0 0 2 2h3"
            />
          </svg>
        {/if}
      </button>
    </div>

    <p class="token-command">
      Use this token with <code>vix login</code>.
    </p>
  </section>
{/if}

{#if showForm && canManage}
  <section class="create-section">
    <div class="section-header">
      <div>
        <span class="eyebrow">New token</span>
        <h2>Create token</h2>
        <p>Use a name that identifies the machine using this token.</p>
      </div>
    </div>

    <form onsubmit={handleSubmitToken}>
      <label>
        Token name

        <input
          bind:value={tokenName}
          placeholder="Vix CLI"
          autocomplete="off"
          required
        />
      </label>

      <div class="form-actions">
        <button
          class="cancel-button"
          type="button"
          disabled={saving}
          onclick={() => {
            showForm = false;
            tokenName = 'Vix CLI';
          }}
        >
          Cancel
        </button>

        <button
          type="submit"
          disabled={
            saving ||
            !tokenName.trim() ||
            !selectedWorkspaceId
          }
        >
          {saving ? 'Creating…' : 'Create token'}
        </button>
      </div>
    </form>
  </section>
{/if}

{#if !canManage}
  <ReadOnlyNotice
    message="Only workspace owners and admins can manage CLI tokens."
  />
{/if}

<section class="tokens-section">
  <div class="section-heading section-heading--bordered">
    <div>
      <h2>Workspace tokens</h2>
      <p>Credentials that can authenticate the Vix CLI.</p>
    </div>

    <span class="section-count">{tokens.length}</span>
  </div>

  {#if loading}
    <p class="loading-text">Loading tokens…</p>
  {:else if !canManage}
    <div class="section-notice">
      <ReadOnlyNotice
        message="Token details are restricted to workspace owners and admins."
      />
    </div>
  {:else if tokens.length === 0}
    <div class="empty-wrapper">
      <EmptyState
        title="No tokens yet"
        body="Create a token before connecting the Vix CLI."
      />
    </div>
  {:else}
    <div class="token-list">
      {#each tokens as token (token.id)}
        <div
          class="token-row"
          class:token-row--inactive={
            token.revoked || isExpired(token.expires_at)
          }
        >
          <div class="token-content">
            <div class="token-primary">
              <strong>{token.name}</strong>

              <span
                class:state-revoked={token.revoked}
                class:state-expired={!token.revoked && isExpired(token.expires_at)}
                class="token-state"
              >
                {tokenStatus(token)}
              </span>
            </div>

            <div class="token-meta">
              <span>{formatExpiry(token.expires_at)}</span>

              <span aria-hidden="true">·</span>

              <span>
                {token.scopes.length}
                {token.scopes.length === 1 ? ' permission' : ' permissions'}
              </span>
            </div>

            <div class="token-permissions">
              <span class="permission-label">Permissions</span>

              {#if token.scopes.length > 0}
                <span class="permission-values">
                  {#each token.scopes as scope}
                    <code>{scope}</code>
                  {/each}
                </span>
              {:else}
                <span class="no-permissions">None</span>
              {/if}
            </div>
          </div>

          {#if !token.revoked && canRevokeToken(currentRole)}
            <button
              class="revoke-button"
              type="button"
              disabled={revoking === token.id}
              onclick={() => openRevokeModal(token)}
            >
              {revoking === token.id ? 'Revoking…' : 'Revoke'}
            </button>
          {/if}
        </div>
      {/each}
    </div>
  {/if}
</section>

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
      <div class="confirm-modal__icon" aria-hidden="true">
        <svg viewBox="0 0 24 24">
          <path d="M12 9v4" />
          <path d="M12 17h.01" />
          <path
            d="M10.3 3.8 2.4 18a2 2 0 0 0 1.7 3h15.8a2 2 0 0 0 1.7-3L13.7 3.8a2 2 0 0 0-3.4 0Z"
          />
        </svg>
      </div>

      <div class="confirm-modal__content">
        <h2 id="revoke-modal-title">Revoke token?</h2>

        <p id="revoke-modal-description">
          The token <strong>{tokenToRevoke.name}</strong> will immediately
          stop working. Any Vix client using it will lose access.
        </p>
      </div>

      <div class="confirm-modal__actions">
        <button
          class="modal-cancel-button"
          type="button"
          disabled={revoking === tokenToRevoke.id}
          onclick={closeRevokeModal}
        >
          Cancel
        </button>

        <button
          class="modal-revoke-button"
          type="button"
          disabled={revoking === tokenToRevoke.id}
          onclick={revoke}
        >
          {revoking === tokenToRevoke.id ? 'Revoking…' : 'Revoke token'}
        </button>
      </div>
    </dialog>
  </div>
{/if}

<style>
  /* ── Intro ──────────────────────────────────────────── */

  .page-introduction {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    margin: -8px 0 18px;
  }

  .page-introduction > p {
    max-width: 680px;
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.6;
  }

  .token-summary {
    display: flex;
    align-items: stretch;
    flex: 0 0 auto;
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
    background: var(--bg-panel);
    overflow: hidden;
  }

  .summary-stat {
    display: grid;
    gap: 1px;
    padding: 7px 14px;
    border-right: 1px solid var(--line-soft);
    text-align: center;
  }

  .summary-stat:last-child {
    border-right: 0;
  }

  .summary-stat strong {
    color: var(--text);
    font-family: var(--font-mono);
    font-size: 15px;
    font-weight: 600;
    font-variant-numeric: tabular-nums;
    line-height: 1.1;
  }

  .summary-label {
    color: var(--text-muted);
    font-size: 10px;
  }

  .new-token-button svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
  }

  /* ── Reveal + create panels ─────────────────────────── */

  .token-reveal,
  .create-section {
    display: grid;
    gap: 15px;
    margin-bottom: 18px;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    padding: 17px;
    box-shadow: inset 3px 0 0 var(--brand);
  }

  .section-header {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 16px;
  }

  .section-header > div {
    display: grid;
    gap: 4px;
  }

  .section-header h2 {
    font-size: 14px;
  }

  .section-header p {
    color: var(--text-muted);
    font-size: 12.5px;
  }

  .copy-field {
    position: relative;
    display: flex;
    min-width: 0;
  }

  .copy-field code {
    display: block;
    width: 100%;
    min-height: 42px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    background: var(--bg-ink);
    color: var(--text);
    padding: 10px 48px 10px 12px;
    font-family: var(--font-mono);
    font-size: 12px;
    line-height: 1.7;
    overflow-wrap: anywhere;
    word-break: break-all;
  }

  .copy-button {
    position: absolute;
    top: 5px;
    right: 5px;
    display: grid;
    width: 32px;
    height: 32px;
    min-height: 32px;
    place-items: center;
    border: 1px solid var(--line-strong);
    background: var(--bg-elevated);
    color: var(--text-soft);
    padding: 0;
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease),
      color var(--speed) var(--ease);
  }

  .copy-button:hover:not(:disabled) {
    border-color: var(--line-strong);
    background: var(--bg-panel-strong);
    color: var(--text);
  }

  /* Copied = success → green flat */
  .copy-button.copied {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .copy-button svg {
    width: 16px;
    height: 16px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .token-command {
    color: var(--text-muted);
    font-size: 12px;
  }

  .token-command code {
    padding: 1px 6px;
    border: 1px solid var(--line-ink);
    border-radius: 4px;
    background: var(--bg-ink);
    color: var(--brand-bright);
    font-size: 11px;
  }

  .form-actions {
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    border-top: 1px solid var(--line-soft);
    padding-top: 14px;
  }

  .cancel-button {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-soft);
  }

  .cancel-button:hover:not(:disabled) {
    border-color: rgba(255, 255, 255, 0.18);
    background: var(--bg-elevated);
    color: var(--text);
  }

  /* ── Tokens list ────────────────────────────────────── */

  .tokens-section {
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    overflow: hidden;
  }

  .section-heading {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 16px;
  }

  .section-heading--bordered {
    padding: 13px 16px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-panel-strong);
  }

  .section-heading > div {
    display: grid;
    gap: 3px;
  }

  .section-heading h2 {
    font-size: 13.5px;
  }

  .section-heading p {
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

  .loading-text {
    min-height: 72px;
    padding: 20px 16px;
    color: var(--text-muted);
    font-size: 12.5px;
  }

  .empty-wrapper,
  .section-notice {
    padding: 14px;
  }

  .token-list {
    display: grid;
  }

  .token-row {
    display: grid;
    grid-template-columns: minmax(0, 1fr) auto;
    gap: 18px;
    align-items: center;
    min-width: 0;
    padding: 14px 16px;
    border-bottom: 1px solid var(--line-soft);
    transition: background var(--speed) var(--ease);
  }

  .token-row:last-child {
    border-bottom: 0;
  }

  .token-row:hover {
    background: var(--bg-elevated);
  }

  .token-row--inactive .token-content {
    opacity: 0.6;
  }

  .token-content {
    display: grid;
    min-width: 0;
    gap: 7px;
  }

  .token-primary {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 9px;
    flex-wrap: wrap;
  }

  .token-primary strong {
    color: var(--text);
    font-size: 13.5px;
    font-weight: 600;
  }

  /* Active token state = green flat (success/valid) */
  .token-state {
    display: inline-flex;
    min-height: 20px;
    align-items: center;
    border: 1px solid var(--green-line);
    border-radius: 999px;
    background: var(--green-faint);
    color: var(--green-soft);
    padding: 0 8px;
    font-size: 10.5px;
    font-weight: 550;
  }

  /* Expired = caution → warning flat */
  .token-state.state-expired {
    border-color: var(--warning-line);
    background: var(--warning-faint);
    color: var(--warning);
  }

  /* Revoked = terminal → danger flat */
  .token-state.state-revoked {
    border-color: var(--danger-line);
    background: var(--danger-faint);
    color: var(--danger);
  }

  .token-meta {
    display: flex;
    align-items: center;
    gap: 7px;
    color: var(--text-muted);
    font-size: 11.5px;
    flex-wrap: wrap;
  }

  .token-permissions {
    display: flex;
    min-width: 0;
    align-items: baseline;
    gap: 9px;
    flex-wrap: wrap;
  }

  .permission-label {
    flex: 0 0 auto;
    color: var(--text-muted);
    font-size: 11.5px;
  }

  .permission-values {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 5px;
    flex-wrap: wrap;
  }

  .permission-values code {
    padding: 1px 7px;
    border: 1px solid var(--line-ink);
    border-radius: 4px;
    background: var(--bg-ink);
    color: var(--text-soft);
    font-family: var(--font-mono);
    font-size: 10px;
    overflow-wrap: anywhere;
  }

  .no-permissions {
    color: var(--text-muted);
    font-size: 11.5px;
  }

  .revoke-button {
    min-height: 30px;
    border-color: var(--danger-line);
    background: transparent;
    color: var(--danger);
    padding: 0 11px;
    font-size: 12px;
  }

  .revoke-button:hover:not(:disabled) {
    border-color: var(--danger);
    background: var(--danger-faint);
  }

  /* ── Confirmation modal ─────────────────────────────── */

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
    background: rgba(0, 0, 0, 0.7);
    backdrop-filter: blur(3px);
    cursor: default;
  }

  .modal-backdrop:hover:not(:disabled),
  .modal-backdrop:active {
    background: rgba(0, 0, 0, 0.7);
    transform: none;
  }

  .modal-backdrop:focus,
  .modal-backdrop:focus-visible {
    outline: none;
    box-shadow: none;
  }

  .confirm-modal {
    position: relative;
    z-index: 1;
    display: grid;
    width: min(100%, 440px);
    max-width: none;
    margin: 0;
    gap: 16px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    color: var(--text);
    padding: 20px;
    box-shadow: var(--shadow-deep);
  }

  .confirm-modal:focus,
  .confirm-modal:focus-visible {
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
    border: 1px solid var(--danger-line);
    border-radius: 50%;
    background: var(--danger-faint);
    color: var(--danger);
  }

  .confirm-modal__icon svg {
    width: 19px;
    height: 19px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .confirm-modal__content {
    display: grid;
    gap: 8px;
  }

  .confirm-modal__content h2 {
    color: var(--text);
    font-size: 16px;
  }

  .confirm-modal__content p {
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.65;
  }

  .confirm-modal__content strong {
    color: var(--text);
    font-weight: 600;
  }

  .confirm-modal__actions {
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    border-top: 1px solid var(--line-soft);
    padding-top: 16px;
  }

  .modal-cancel-button {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-soft);
  }

  .modal-cancel-button:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
  }

  .modal-revoke-button {
    border-color: var(--danger);
    background: var(--danger);
    color: #240707;
  }

  .modal-revoke-button:hover:not(:disabled) {
    background: #fca5a5;
    border-color: #fca5a5;
  }

  /* ── Responsive ─────────────────────────────────────── */

  @media (max-width: 620px) {
    .page-introduction {
      align-items: flex-start;
      flex-direction: column;
    }

    .token-summary {
      width: 100%;
    }

    .summary-stat {
      flex: 1 1 0;
    }

    .token-reveal,
    .create-section {
      padding: 14px;
    }

    .token-row {
      grid-template-columns: 1fr;
      gap: 11px;
    }

    .token-permissions {
      align-items: flex-start;
      flex-direction: column;
      gap: 5px;
    }

    .revoke-button {
      width: 100%;
    }

    .form-actions {
      flex-direction: column-reverse;
    }

    .form-actions button {
      width: 100%;
    }
  }

  @media (max-width: 520px) {
    .confirm-modal__actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .confirm-modal__actions button {
      width: 100%;
    }
  }
</style>
