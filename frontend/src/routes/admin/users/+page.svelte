<script lang="ts">
  import { onMount } from 'svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import { disableAdminUserPublicProfile, listAdminUsers } from '$lib/api/admin';
  import { ApiError } from '$lib/api/types';
  import type { AdminUserSummary } from '$lib/api/types';
  let users: AdminUserSummary[] = []; let error=''; let search='';
  async function load() { try { users=(await listAdminUsers(search)).users; } catch(err) { error=err instanceof ApiError?err.message:'Unable to load users.'; } }
  async function disableProfile(id:string) { if(!confirm('Disable this public profile?')) return; await disableAdminUserPublicProfile(id); await load(); }
  onMount(load);
</script>
<section class="admin-page"><header class="admin-header"><div><h1>Users</h1><p>Inspect accounts, profiles and public visibility.</p></div><nav class="admin-nav"><a href="/admin">Overview</a><a href="/admin/users">Users</a><a href="/admin/packages">Packages</a><a href="/admin/feedback">Feedback</a><a href="/admin/supporters">Supporters</a><a href="/admin/audit">Audit</a></nav></header><InlineError message={error} />
<div class="panel"><input bind:value={search} placeholder="Search users" oninput={load} /></div>
<section class="panel"><table><thead><tr><th>Email</th><th>Profile</th><th>Supporter</th><th>Created</th><th></th></tr></thead><tbody>{#each users as user}<tr><td>{user.email}</td><td>{user.display_name || 'No name'}<br/>@{user.username || 'none'}<br/>Public: {user.public_profile_enabled ? 'yes':'no'}</td><td>{user.supporter_tier || 'none'}</td><td>{user.created_at}</td><td>{#if user.public_profile_enabled}<button class="danger" onclick={() => disableProfile(user.id)}>Disable public profile</button>{/if}</td></tr>{/each}</tbody></table></section></section>
