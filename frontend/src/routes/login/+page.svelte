<script lang="ts">
  import { goto } from '$app/navigation';
  import { login } from '$lib/api/auth';
  import { ApiError } from '$lib/api/types';
  import AuthPanel from '$lib/components/AuthPanel.svelte';
  import { auth } from '$lib/stores/auth';

  let email = '';
  let password = '';
  let error = '';
  let loading = false;

  async function submit() {
    error = '';
    loading = true;

    try {
      const data = await login(email, password);
      auth.setSession(data);
      await goto('/dashboard');
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Login failed.';
    } finally {
      loading = false;
    }
  }
</script>

<svelte:head>
  <title>Login | Softadastra Cloud</title>
</svelte:head>

<div class="auth-page">
  <AuthPanel title="Login" subtitle="Access your Vix cloud workspace.">
    <form on:submit|preventDefault={submit}>
      <label>
        Email
        <input bind:value={email} type="email" autocomplete="email" required />
      </label>

      <label>
        Password
        <input bind:value={password} type="password" autocomplete="current-password" required />
      </label>

      {#if error}
        <p class="form-error">{error}</p>
      {/if}

      <button type="submit" disabled={loading}>{loading ? 'Signing in...' : 'Sign in'}</button>
    </form>

    <p class="switch-link"><a href="/register">Create an account</a></p>
  </AuthPanel>
</div>
