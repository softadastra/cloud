<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { createToken, defaultCliScopes, listTokens, revokeToken } from '$lib/api/tokens';
  import { listWorkspaces } from '$lib/api/workspaces';
  import { ApiError, type CliToken, type Workspace } from '$lib/api/types';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import ReadOnlyNotice from '$lib/components/ReadOnlyNotice.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import { canManageTokens, canRevokeToken } from '$lib/permissions';
  import { auth } from '$lib/stores/auth';
  import { workspaceContext } from '$lib/stores/workspace';

  let workspaces: Workspace[] = [];
  let selectedWorkspaceId = '';
  let tokens: CliToken[] = [];
  let newToken: CliToken | null = null;
  let tokenName = 'Vix CLI';
  let loading = true;
  let saving = false;
  let error = '';
  let success = '';

  $: selectedWorkspace = workspaces.find((workspace) => workspace.id === selectedWorkspaceId) ?? null;
  $: currentRole = selectedWorkspace?.current_user_role ?? 'viewer';

  async function load() {
    const user = $auth.user;
    if (!$auth.session || !user) { await goto('/login'); return; }
    loading = true; error = '';
    try {
      const workspaceData = await listWorkspaces(user.id);
      workspaces = workspaceData.workspaces;
      selectedWorkspaceId = new URLSearchParams(window.location.search).get('workspace_id') ?? workspaces[0]?.id ?? '';
      workspaceContext.setWorkspaces(workspaces, selectedWorkspaceId);
      await loadTokens();
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to load tokens.';
    } finally { loading = false; }
  }

  async function loadTokens() {
    const user = $auth.user;
    workspaceContext.setSelectedWorkspace(selectedWorkspaceId);
    if (!user || !selectedWorkspaceId || !canManageTokens(currentRole)) { tokens = []; return; }
    const data = await listTokens(selectedWorkspaceId, user.id);
    tokens = data.tokens;
  }

  async function submitToken() {
    const user = $auth.user;
    if (!user || !selectedWorkspaceId || !tokenName.trim() || !canManageTokens(currentRole)) return;
    saving = true; error = ''; success = ''; newToken = null;
    try {
      const created = await createToken({ workspaceId: selectedWorkspaceId, userId: user.id, name: tokenName.trim(), scopes: defaultCliScopes });
      newToken = created.token;
      tokens = [created.token, ...tokens];
      tokenName = 'Vix CLI';
      success = 'CLI token created. Copy it now; it will only be shown once.';
    } catch (err) { error = err instanceof ApiError ? err.message : 'Unable to create token.'; }
    finally { saving = false; }
  }

  async function revoke(token: CliToken) {
    const user = $auth.user;
    if (!user || token.revoked || !canRevokeToken(currentRole)) return;
    if (!confirm(`Revoke token ${token.name}? Vix clients using it will stop working.`)) return;
    try {
      const revoked = await revokeToken({ workspaceId: token.workspace_id, userId: user.id, tokenId: token.id });
      tokens = tokens.map((item) => (item.id === token.id ? revoked.token : item));
      success = 'Token revoked.';
    } catch (err) { error = err instanceof ApiError ? err.message : 'Unable to revoke token.'; }
  }

  onMount(load);
</script>

<svelte:head><title>Tokens | Softadastra Cloud</title></svelte:head>
<PageHeader eyebrow="Tokens" title="CLI tokens" workspaceName={selectedWorkspace?.name ?? ''} role={currentRole} />
<InlineError message={error} />
{#if success}<p class="success-message">{success}</p>{/if}

<section class="dashboard-grid">
  <div class="panel">
    <div class="panel-header"><h2>Workspace</h2></div>
    <label>Workspace<select bind:value={selectedWorkspaceId} on:change={loadTokens}>{#each workspaces as workspace}<option value={workspace.id}>{workspace.name}</option>{/each}</select></label>
    {#if canManageTokens(currentRole)}
      <form on:submit|preventDefault={submitToken}>
        <div class="panel-header"><h2>Create token</h2></div>
        <p class="muted">This token will only be shown once.</p>
        <label>Name<input bind:value={tokenName} required /></label>
        <button type="submit" disabled={saving || !selectedWorkspaceId}>{saving ? 'Creating...' : 'Create CLI token'}</button>
        {#if newToken?.value}<p class="token-value">{newToken.value}</p>{/if}
      </form>
    {:else}
      <ReadOnlyNotice message="Only workspace owners and admins can manage CLI tokens." />
    {/if}
  </div>

  <div class="panel span-2">
    <div class="panel-header"><h2>Tokens</h2><span>{tokens.length}</span></div>
    {#if loading}<p class="muted">Loading...</p>
    {:else if !canManageTokens(currentRole)}<ReadOnlyNotice message="Token details are restricted to workspace owners and admins." />
    {:else}
      <div class="table-list">
        {#each tokens as token}
          <div class="row"><span><strong>{token.name}</strong><small>{token.scopes.join(', ')}</small></span><span class="actions"><StatusBadge status={token.revoked ? 'revoked' : 'active'} /><small>expires {token.expires_at || 'never'}</small><button class="small danger" type="button" disabled={token.revoked} on:click={() => revoke(token)}>Revoke</button></span></div>
        {:else}<EmptyState title="No CLI tokens yet." body="Create one before connecting Vix locally." />{/each}
      </div>
    {/if}
  </div>
</section>
