<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { login } from '$lib/api/auth';
  import { ApiError } from '$lib/api/types';
  import AuthPanel from '$lib/components/AuthPanel.svelte';
  import { auth } from '$lib/stores/auth';

  let email = '';
  let password = '';
  let showPassword = false;

  let error = '';
  let loading = false;

  $: normalizedEmail = email.trim();

  $: canSubmit =
    Boolean(normalizedEmail) &&
    Boolean(password) &&
    !loading;

  function destinationAfterLogin() {
    const redirect =
      new URLSearchParams(window.location.search)
        .get('redirect')
        ?.trim() ?? '';

    if (
      redirect.startsWith('/') &&
      !redirect.startsWith('//')
    ) {
      return redirect;
    }

    return '/dashboard';
  }

  function clearError() {
    if (error) {
      error = '';
    }
  }

  function handleSubmit(event: SubmitEvent) {
    event.preventDefault();
    void submit();
  }

  async function submit() {
    if (!normalizedEmail || !password || loading) {
      return;
    }

    error = '';
    loading = true;

    try {
      const data = await login(
        normalizedEmail,
        password
      );

      auth.setSession(data);

      await goto(destinationAfterLogin());
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to sign in. Check your email and password.';
    } finally {
      loading = false;
    }
  }

  onMount(() => {
    if ($auth.session) {
      void goto('/dashboard');
    }
  });
</script>

<svelte:head>
  <title>Sign in | Softadastra Cloud</title>

  <meta
    name="description"
    content="Sign in to manage your Softadastra Cloud workspaces, C++ projects, packages and build activity."
  />
</svelte:head>

<main class="login-page">
  <section
    class="login-context"
    aria-labelledby="login-context-title"
  >
    <a
      class="brand"
      href="/"
      aria-label="Softadastra Cloud home"
    >
      <img
        src="/brand/softadastra-cloud.svg"
        alt=""
        aria-hidden="true"
      />

      <span>Softadastra Cloud</span>
    </a>

    <div class="context-content">
      <p class="eyebrow">C++ cloud workspace</p>

      <h1 id="login-context-title">
        Continue working with your projects and team.
      </h1>

      <p class="context-description">
        Access private packages, project lockfiles, build reports,
        workspace members and CLI credentials from one place.
      </p>

      <div class="context-points">
        <div>
          <svg viewBox="0 0 24 24" aria-hidden="true">
            <path d="m5 12 4 4L19 6"></path>
          </svg>

          <span>
            Manage projects and private package versions
          </span>
        </div>

        <div>
          <svg viewBox="0 0 24 24" aria-hidden="true">
            <path d="m5 12 4 4L19 6"></path>
          </svg>

          <span>
            Review lockfiles and build activity
          </span>
        </div>

        <div>
          <svg viewBox="0 0 24 24" aria-hidden="true">
            <path d="m5 12 4 4L19 6"></path>
          </svg>

          <span>
            Control workspace members and CLI access
          </span>
        </div>
      </div>
    </div>

    <p class="context-footer">
      Built for the Vix and Softadastra C++ ecosystem.
    </p>
  </section>

  <section class="login-form-column">
    <AuthPanel
      title="Sign in"
      subtitle="Enter your account details to continue."
    >
      <form
        class="login-form"
        onsubmit={handleSubmit}
      >
        <label>
          Email address

          <input
            bind:value={email}
            type="email"
            placeholder="you@example.com"
            autocomplete="email"
            inputmode="email"
            required
            disabled={loading}
            oninput={clearError}
          />
        </label>

        <label>
          Password

          <span class="password-field">
            <input
              bind:value={password}
              type={showPassword ? 'text' : 'password'}
              placeholder="Enter your password"
              autocomplete="current-password"
              required
              disabled={loading}
              oninput={clearError}
            />

            <button
              class="password-toggle"
              type="button"
              disabled={loading}
              aria-label={
                showPassword
                  ? 'Hide password'
                  : 'Show password'
              }
              aria-pressed={showPassword}
              onclick={() =>
                showPassword = !showPassword}
            >
              {#if showPassword}
                <svg viewBox="0 0 24 24" aria-hidden="true">
                  <path
                    d="M3 3l18 18M10.6 10.7a2 2 0 0 0 2.7 2.7M9.9 4.2A10.8 10.8 0 0 1 12 4c5.5 0 9 5.5 9 8a11.7 11.7 0 0 1-2.1 3.6M6.6 6.6C4.3 8 3 10.3 3 12c0 2.5 3.5 8 9 8a10.8 10.8 0 0 0 3.4-.6"
                  ></path>
                </svg>
              {:else}
                <svg viewBox="0 0 24 24" aria-hidden="true">
                  <path
                    d="M3 12s3.5-8 9-8 9 8 9 8-3.5 8-9 8-9-8-9-8Z"
                  ></path>

                  <circle cx="12" cy="12" r="3"></circle>
                </svg>
              {/if}
            </button>
          </span>
        </label>

        {#if error}
          <div
            class="login-error"
            role="alert"
            aria-live="assertive"
          >
            <svg viewBox="0 0 24 24" aria-hidden="true">
              <circle cx="12" cy="12" r="9"></circle>
              <path d="M12 8v5"></path>
              <path d="M12 17h.01"></path>
            </svg>

            <p>{error}</p>
          </div>
        {/if}

        <button
          class="sign-in-button"
          type="submit"
          disabled={!canSubmit}
        >
          {#if loading}
            <span
              class="button-spinner"
              aria-hidden="true"
            ></span>

            <span>Signing in…</span>
          {:else}
            <span>Sign in</span>

            <svg viewBox="0 0 24 24" aria-hidden="true">
              <path d="m9 18 6-6-6-6"></path>
            </svg>
          {/if}
        </button>
      </form>

      <div class="account-switch">
        <span>New to Softadastra Cloud?</span>

        <a href="/register">
          Create an account
        </a>
      </div>

      <p class="security-note">
        Your password and session credentials are transmitted
        securely and are never displayed in Softadastra Cloud.
      </p>
    </AuthPanel>
  </section>
</main>

<style>
  .login-page {
    position: relative;
    width: 100vw;
    min-height: 100dvh;
    margin-left: calc(50% - 50vw);

    display: grid;
    grid-template-columns:
      minmax(420px, 46%)
      minmax(0, 54%);

    background: var(--bg);
    overflow: hidden;
  }
  /* Context */

  .login-context {
    display: flex;
    min-width: 0;
    min-height: 100dvh;
    flex-direction: column;
    justify-content: space-between;
    gap: 48px;

    border-right: 1px solid var(--line);

    background:
      radial-gradient(
        circle at 12% 15%,
        rgba(249, 115, 22, 0.09),
        transparent 40%
      ),
      var(--bg-ink);

    padding:
      clamp(30px, 5vw, 70px)
      clamp(34px, 5.5vw, 82px);
  }
  .brand {
    display: inline-flex;
    width: fit-content;
    align-items: center;
    gap: 10px;
    color: var(--text);
    font-size: 13px;
    font-weight: 650;
  }

  .brand img {
    width: 28px;
    height: 28px;
  }

  .context-content {
    display: grid;
    max-width: 540px;
    gap: 18px;
  }

  .context-content h1 {
    max-width: 520px;
    color: var(--text);
    font-size: clamp(28px, 4vw, 44px);
    font-weight: 650;
    line-height: 1.12;
    letter-spacing: -0.035em;
  }

  .context-description {
    max-width: 510px;
    color: var(--text-muted);
    font-size: 14px;
    line-height: 1.75;
  }

  .context-points {
    display: grid;
    gap: 12px;
    margin-top: 6px;
  }

  .context-points > div {
    display: grid;
    grid-template-columns: 18px minmax(0, 1fr);
    gap: 9px;
    align-items: start;
    color: var(--text-soft);
    font-size: 12px;
    line-height: 1.55;
  }

  .context-points svg {
    width: 16px;
    height: 16px;
    margin-top: 1px;
    fill: none;
    stroke: var(--brand);
    stroke-width: 1.9;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .context-footer {
    color: var(--text-faint);
    font-size: 10.5px;
  }

  /* Form column */

  .login-form-column {
    display: grid;
    min-width: 0;
    min-height: 100dvh;
    place-items: center;

    background: var(--bg);

    padding:
      clamp(30px, 6vw, 80px)
      clamp(24px, 7vw, 100px);
  }

  .brand img {
    display: block;
    width: 24px;
    height: 24px;
    flex: 0 0 auto;
    object-fit: contain;
  }

  .login-form-column :global(.auth-panel) {
    width: min(100%, 440px);
  }

  .login-form {
    display: grid;
    gap: 16px;
  }

  .password-field {
    position: relative;
    display: block;
  }

  .password-field input {
    padding-right: 48px;
  }

  .password-toggle {
    position: absolute;
    top: 50%;
    right: 6px;
    display: grid;
    width: 32px;
    height: 30px;
    min-height: 30px;
    place-items: center;
    border: 0;
    background: transparent;
    color: var(--text-muted);
    padding: 0;
    transform: translateY(-50%);
  }

  .password-toggle:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
    transform: translateY(-50%);
  }

  .password-toggle svg {
    width: 16px;
    height: 16px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .login-error {
    display: grid;
    grid-template-columns: 17px minmax(0, 1fr);
    gap: 9px;
    align-items: start;
    border: 1px solid var(--danger-line);
    border-radius: var(--radius-sm);
    background: var(--danger-faint);
    color: var(--danger);
    padding: 10px 11px;
  }

  .login-error svg {
    width: 16px;
    height: 16px;
    margin-top: 1px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .login-error p {
    font-size: 11.5px;
    line-height: 1.5;
  }

  .sign-in-button {
    display: flex;
    width: 100%;
    min-height: 40px;
    align-items: center;
    justify-content: center;
    gap: 8px;
    margin-top: 2px;
    border-color: var(--brand);
    background: var(--brand);
    color: var(--brand-ink);
    font-weight: 650;
  }

  .sign-in-button:hover:not(:disabled) {
    border-color: var(--brand-soft);
    background: var(--brand-soft);
  }

  .sign-in-button svg {
    width: 16px;
    height: 16px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .button-spinner {
    width: 14px;
    height: 14px;
    border: 2px solid rgba(26, 15, 6, 0.3);
    border-top-color: var(--brand-ink);
    border-radius: 50%;
    animation: spin 700ms linear infinite;
  }

  .account-switch {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 6px;
    margin-top: 18px;
    color: var(--text-muted);
    font-size: 11.5px;
  }

  .account-switch a {
    color: var(--link);
    font-weight: 600;
  }

  .account-switch a:hover {
    color: var(--link-hover);
    text-decoration: underline;
    text-underline-offset: 3px;
  }

  .security-note {
    max-width: 390px;
    margin: 16px auto 0;
    color: var(--text-faint);
    font-size: 9.5px;
    line-height: 1.55;
    text-align: center;
  }

  @keyframes spin {
    to {
      transform: rotate(360deg);
    }
  }

  @media (max-width: 860px) {
    .login-page {
      width: 100vw;
      min-height: 100dvh;
      grid-template-columns: 1fr;
      overflow: visible;
    }

    .login-context {
      min-height: auto;
      gap: 28px;
      border-right: 0;
      border-bottom: 1px solid var(--line);
      padding: 28px 22px;
    }

    .context-content h1 {
      font-size: clamp(25px, 7vw, 36px);
    }

    .context-points,
    .context-footer {
      display: none;
    }

    .login-form-column {
      min-height: auto;
      place-items: start center;
      padding: 38px 18px 56px;
    }
  }

  @media (max-width: 520px) {
    .login-context {
      padding: 22px 18px;
    }

    .context-description {
      font-size: 12.5px;
    }

    .login-form-column {
      align-items: start;
      padding: 28px 14px 44px;
    }

    .account-switch {
      align-items: center;
      flex-direction: column;
    }
  }

  @media (prefers-reduced-motion: reduce) {
    .button-spinner {
      animation-duration: 1.4s;
    }
  }
</style>
