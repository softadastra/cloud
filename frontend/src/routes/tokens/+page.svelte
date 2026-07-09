<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { createToken, defaultCliScopes, listTokens, revokeToken } from '$lib/api/tokens';
  import { listWorkspaces } from '$lib/api/workspaces';
  import { ApiError, type CliToken, type Workspace } from '$lib/api/types';
  import { auth } from '$lib/stores/auth';

  let workspaces: Workspace[] = [];
  let selectedWorkspaceId = '';
  let tokens: CliToken[] = [];
  let newToken: CliToken | null = null;
  let tokenName = 'Vix CLI';
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
      const workspaceData = await listWorkspaces(user.id);
      workspaces = workspaceData.workspaces;
      selectedWorkspaceId = new URLSearchParams(window.location.search).get('workspace_id') ?? workspaces[0]?.id ?? '';
      await loadTokens();
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to load tokens.';
    } finally {
      loading = false;
    }
  }

  async function loadTokens() {
    const user = $auth.user;

    if (!user || !selectedWorkspaceId) {
      tokens = [];
      return;
    }

    const data = await listTokens(selectedWorkspaceId, user.id);
    tokens = data.tokens;
  }

  async function submitToken() {
    const user = $auth.user;

    if (!user || !selectedWorkspaceId || !tokenName.trim()) {
      return;
    }

    saving = true;
    error = '';
    newToken = null;

    try {
      const created = await createToken({ workspaceId: selectedWorkspaceId, userId: user.id, name: tokenName.trim(), scopes: defaultCliScopes });
      newToken = created.token;
      tokens = [created.token, ...tokens];
      tokenName = 'Vix CLI';
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to create token.';
    } finally {
      saving = false;
    }
  }

  async function revoke(token: CliToken) {
    const user = $auth.user;

    if (!user || token.revoked) {
      return;
    }

    try {
      const revoked = await revokeToken({ workspaceId: token.workspace_id, userId: user.id, tokenId: token.id });
      tokens = tokens.map((item) => (item.id === token.id ? revoked.token : item));
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to revoke token.';
    }
  }

  onMount(load);
</script>

<svelte:head><title>Tokens | Softadastra Cloud</title></svelte:head>
<section class="page-header"><div><p class="eyebrow">Tokens</p><h1>CLI tokens</h1></div></section>
{#if error}<p class="form-error">{error}</p>{/if}

<section class="dashboard-grid">
  <form class="panel" on:submit|preventDefault={submitToken}>
    <div class="panel-header"><h2>Create token</h2></div>
    <label>Workspace<select bind:value={selectedWorkspaceId} on:change={loadTokens}>{#each workspaces as workspace}<option value={workspace.id}>{workspace.name}</option>{/each}</select></label>
    <label>Name<input bind:value={tokenName} required /></label>
    <button type="submit" disabled={saving || !selectedWorkspaceId}>{saving ? 'Creating...' : 'Create CLI token'}</button>
    {#if newToken?.value}<p class="token-value">{newToken.value}</p>{/if}
  </form>

  <div class="panel span-2">
    <div class="panel-header"><h2>Tokens</h2><span>{tokens.length}</span></div>
    {#if loading}<p class="muted">Loading...</p>{:else}
      <div class="table-list">
        {#each tokens as token}
          <div class="row"><span><strong>{token.name}</strong><small>{token.scopes.join(', ')}</small></span><span class="actions"><small>{token.revoked ? 'revoked' : 'active'}</small><button class="small danger" type="button" disabled={token.revoked} on:click={() => revoke(token)}>Revoke</button></span></div>
        {:else}<p class="muted padded">No tokens.</p>{/each}
      </div>
    {/if}
  </div>
</section>
