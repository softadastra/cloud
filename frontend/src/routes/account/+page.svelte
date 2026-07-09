<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { changePassword as changePasswordApi, updateProfile } from '$lib/api/auth';
  import { ApiError } from '$lib/api/types';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import { auth } from '$lib/stores/auth';

  let displayName = '';
  let currentPassword = '';
  let newPassword = '';
  let confirmNewPassword = '';
  let profileMessage = '';
  let passwordMessage = '';
  let error = '';
  let savingProfile = false;
  let savingPassword = false;

  $: user = $auth.user;
  $: session = $auth.session;

  onMount(async () => {
    if (!session || !user) {
      await goto('/login');
      return;
    }
    displayName = user.display_name || user.name || '';
  });

  async function saveProfile() {
    error = '';
    profileMessage = '';
    savingProfile = true;
    try {
      if (!session) return;
      const updated = await updateProfile(session.id, displayName.trim());
      auth.setUser(updated.user);
      displayName = updated.user.display_name || updated.user.name || '';
      profileMessage = 'Profile updated.';
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to update profile.';
    } finally {
      savingProfile = false;
    }
  }

  async function changePassword() {
    error = '';
    passwordMessage = '';

    if (!newPassword) {
      error = 'New password is required.';
      return;
    }

    if (newPassword !== confirmNewPassword) {
      error = 'New passwords do not match.';
      return;
    }

    savingPassword = true;
    try {
      if (!session) return;
      await changePasswordApi({
        sessionId: session.id,
        currentPassword,
        newPassword,
        confirmNewPassword
      });
      passwordMessage = 'Password changed successfully.';
      currentPassword = '';
      newPassword = '';
      confirmNewPassword = '';
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to change password.';
    } finally {
      savingPassword = false;
    }
  }
</script>

<svelte:head><title>Account | Softadastra Cloud</title></svelte:head>

<PageHeader eyebrow="Account" title="Account settings" />
<InlineError message={error} />

<section class="dashboard-grid">
  <div class="panel">
    <div class="panel-header"><h2>Profile</h2></div>
    <form on:submit|preventDefault={saveProfile}>
      <label>Email<input value={user?.email ?? ''} readonly /></label>
      <p class="muted">Email changes are not available yet.</p>
      <label>Display name<input bind:value={displayName} /></label>
      {#if profileMessage}<p class="success-message">{profileMessage}</p>{/if}
      <button type="submit" disabled={savingProfile}>{savingProfile ? 'Saving...' : 'Save profile'}</button>
    </form>
  </div>

  <div class="panel">
    <div class="panel-header"><h2>Change password</h2></div>
    <form on:submit|preventDefault={changePassword}>
      <label>Current password<input bind:value={currentPassword} type="password" autocomplete="current-password" /></label>
      <label>New password<input bind:value={newPassword} type="password" autocomplete="new-password" /></label>
      <label>Confirm new password<input bind:value={confirmNewPassword} type="password" autocomplete="new-password" /></label>
      {#if passwordMessage}<p class="success-message">{passwordMessage}</p>{/if}
      <button type="submit" disabled={savingPassword || !newPassword || newPassword !== confirmNewPassword}>{savingPassword ? 'Changing...' : 'Change password'}</button>
    </form>
  </div>
</section>

<section class="panel">
  <div class="panel-header"><h2>Session</h2></div>
  <div class="meta-grid">
    <span><strong>Email</strong>{user?.email ?? 'Unknown'}</span>
    <span><strong>Name</strong>{user?.display_name || user?.name || 'Not set'}</span>
    <span><strong>Session</strong>{session?.id ? 'active' : 'none'}</span>
    <span><strong>User ID</strong>{user?.id ?? 'Unknown'}</span>
    <span><strong>Expires at</strong>{session?.expires_at ?? 'Unknown'}</span>
  </div>
</section>
