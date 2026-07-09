<script lang="ts">
  import { goto } from '$app/navigation';
  import { login, register } from '$lib/api/auth';
  import { ApiError } from '$lib/api/types';
  import AuthPanel from '$lib/components/AuthPanel.svelte';
  import { auth } from '$lib/stores/auth';

  let name = '';
  let email = '';
  let password = '';
  let confirmPassword = '';
  let error = '';
  let loading = false;

  async function submit() {
    error = '';

    if (password !== confirmPassword) {
      error = 'Passwords do not match.';
      return;
    }

    loading = true;

    try {
      await register(name, email, password);
      const data = await login(email, password);
      auth.setSession(data);
      await goto('/dashboard');
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Registration failed.';
    } finally {
      loading = false;
    }
  }
</script>

<svelte:head>
  <title>Register | Softadastra Cloud</title>
</svelte:head>

<div class="auth-page">
  <AuthPanel title="Register" subtitle="Create your Softadastra Cloud account.">
    <form on:submit|preventDefault={submit}>
      <label>
        Name
        <input bind:value={name} type="text" autocomplete="name" required />
      </label>

      <label>
        Email
        <input bind:value={email} type="email" autocomplete="email" required />
      </label>

      <label>
        Password
        <input bind:value={password} type="password" autocomplete="new-password" minlength="8" required />
      </label>

      <label>
        Confirm password
        <input bind:value={confirmPassword} type="password" autocomplete="new-password" minlength="8" required />
      </label>

      {#if error}
        <p class="form-error">{error}</p>
      {/if}

      <button type="submit" disabled={loading || password !== confirmPassword}>{loading ? 'Creating...' : 'Create account'}</button>
    </form>

    <p class="switch-link"><a href="/login">Sign in</a></p>
  </AuthPanel>
</div>
