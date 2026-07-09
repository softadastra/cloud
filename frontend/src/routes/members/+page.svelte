<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { inviteMember, listMembers } from '$lib/api/members';
  import { listWorkspaces } from '$lib/api/workspaces';
  import { ApiError, type Member, type Workspace } from '$lib/api/types';
  import { auth } from '$lib/stores/auth';

  let workspaces: Workspace[] = [];
  let members: Member[] = [];
  let selectedWorkspaceId = '';
  let email = '';
  let role = 'member';
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
      await loadMembers();
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to load members.';
    } finally {
      loading = false;
    }
  }

  async function loadMembers() {
    if (!selectedWorkspaceId) {
      members = [];
      return;
    }
    const data = await listMembers(selectedWorkspaceId);
    members = data.members;
  }

  async function submitInvite() {
    const user = $auth.user;
    if (!user || !selectedWorkspaceId || !email.trim()) return;
    saving = true;
    error = '';
    try {
      const invited = await inviteMember({ workspaceId: selectedWorkspaceId, email: email.trim(), role, invitedByUserId: user.id });
      members = [invited.member, ...members];
      email = '';
      role = 'member';
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to add member.';
    } finally {
      saving = false;
    }
  }

  onMount(load);
</script>

<svelte:head><title>Members | Softadastra Cloud</title></svelte:head>
<section class="page-header"><div><p class="eyebrow">Members</p><h1>Workspace members</h1></div></section>
{#if error}<p class="form-error">{error}</p>{/if}

<section class="dashboard-grid">
  <form class="panel" on:submit|preventDefault={submitInvite}>
    <div class="panel-header"><h2>Add member</h2></div>
    <label>Workspace<select bind:value={selectedWorkspaceId} on:change={loadMembers}>{#each workspaces as workspace}<option value={workspace.id}>{workspace.name}</option>{/each}</select></label>
    <label>Email<input bind:value={email} type="email" placeholder="teammate@example.com" required /></label>
    <label>Role<select bind:value={role}><option value="admin">admin</option><option value="member">member</option><option value="viewer">viewer</option></select></label>
    <button type="submit" disabled={saving || !selectedWorkspaceId}>{saving ? 'Adding...' : 'Add member'}</button>
  </form>

  <div class="panel span-2">
    <div class="panel-header"><h2>Members</h2><span>{members.length}</span></div>
    {#if loading}<p class="muted">Loading...</p>{:else}
      <div class="table-list">
        {#each members as member}
          <div class="row"><span><strong>{member.email}</strong><small>{member.user_id}</small></span><span class="actions"><small>{member.role}</small><small>{member.status}</small></span></div>
        {:else}<p class="muted padded">No members.</p>{/each}
      </div>
    {/if}
  </div>
</section>
