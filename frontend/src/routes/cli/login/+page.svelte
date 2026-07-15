<script lang="ts">
  import { browser } from '$app/environment';
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';

  import { authorizeCliLogin } from '$lib/api/auth';
  import { ApiError } from '$lib/api/types';
  import { auth } from '$lib/stores/auth';

  let status = 'Preparing CLI sign in...';
  let error = '';

  function currentPathWithQuery() {
    if (!browser) {
      return '/cli/login';
    }

    return `${window.location.pathname}${window.location.search}`;
  }

  function isSafeLoopbackRedirect(value: string) {
    try {
      const url = new URL(value);
      return url.protocol === 'http:' && url.hostname === '127.0.0.1' && url.pathname === '/callback';
    } catch {
      return false;
    }
  }

  async function run() {
    if (!browser) {
      return;
    }

    const params = new URLSearchParams(window.location.search);
    const state = params.get('state')?.trim() ?? '';
    const redirectUri = params.get('redirect_uri')?.trim() ?? '';

    if (!state || !redirectUri || !isSafeLoopbackRedirect(redirectUri)) {
      error = 'The CLI login request is invalid.';
      status = '';
      return;
    }

    const authStatus = await auth.ensureAuthLoaded();

    if (authStatus === 'missing' || authStatus === 'invalid') {
      await goto(`/login?redirect=${encodeURIComponent(currentPathWithQuery())}`, { replaceState: true });
      return;
    }

    if (authStatus === 'unreachable') {
      error = 'Softadastra Cloud is temporarily unavailable. Return to the terminal and try again.';
      status = '';
      return;
    }

    status = 'Authorizing Vix CLI...';

    try {
      const data = await authorizeCliLogin(state, redirectUri);
      const callback = new URL(redirectUri);
      callback.searchParams.set('code', data.code);
      callback.searchParams.set('state', state);
      window.location.replace(callback.toString());
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to authorize the Vix CLI.';
      status = '';
    }
  }

  onMount(() => {
    void run();
  });
</script>

<svelte:head>
  <title>CLI sign in | Softadastra Cloud</title>
  <meta name="description" content="Authorize Vix CLI access to Softadastra Cloud." />
</svelte:head>

<main class="cli-login-page">
  <section class="cli-login-panel" aria-live="polite">
    <a class="brand" href="/" aria-label="Softadastra Cloud home">
      <img src="/brand/softadastra-cloud.svg" alt="" aria-hidden="true" />
      <span>Softadastra Cloud</span>
    </a>

    <div class="terminal-mark" aria-hidden="true">$</div>

    <h1>Connect Vix CLI</h1>

    {#if status}
      <p>{status}</p>
      <div class="progress" aria-hidden="true"><span></span></div>
    {/if}

    {#if error}
      <p class="error">{error}</p>
    {/if}
  </section>
</main>

<style>
  :global(body) {
    margin: 0;
    background: #0b1120;
    color: #f8fafc;
  }

  .cli-login-page {
    min-height: 100vh;
    display: grid;
    place-items: center;
    padding: 24px;
    background:
      radial-gradient(circle at 20% 20%, rgba(20, 184, 166, 0.18), transparent 30%),
      linear-gradient(135deg, #0b1120 0%, #111827 55%, #10201f 100%);
  }

  .cli-login-panel {
    width: min(100%, 460px);
    border: 1px solid rgba(148, 163, 184, 0.24);
    border-radius: 8px;
    background: rgba(15, 23, 42, 0.88);
    box-shadow: 0 24px 70px rgba(0, 0, 0, 0.32);
    padding: 32px;
  }

  .brand {
    display: inline-flex;
    align-items: center;
    gap: 10px;
    color: #f8fafc;
    text-decoration: none;
    font-weight: 700;
  }

  .brand img {
    width: 30px;
    height: 30px;
  }

  .terminal-mark {
    width: 44px;
    height: 44px;
    margin-top: 34px;
    display: grid;
    place-items: center;
    border-radius: 8px;
    background: #14b8a6;
    color: #04111a;
    font: 800 24px/1 ui-monospace, SFMono-Regular, Menlo, monospace;
  }

  h1 {
    margin: 18px 0 10px;
    font-size: clamp(2rem, 6vw, 3rem);
    line-height: 1;
    letter-spacing: 0;
  }

  p {
    margin: 0;
    color: #cbd5e1;
    line-height: 1.6;
  }

  .error {
    margin-top: 18px;
    color: #fecaca;
  }

  .progress {
    margin-top: 24px;
    height: 6px;
    overflow: hidden;
    border-radius: 999px;
    background: rgba(148, 163, 184, 0.22);
  }

  .progress span {
    display: block;
    width: 42%;
    height: 100%;
    border-radius: inherit;
    background: #14b8a6;
    animation: move 1.2s ease-in-out infinite alternate;
  }

  @keyframes move {
    from { transform: translateX(0); }
    to { transform: translateX(140%); }
  }
</style>
