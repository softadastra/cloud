<script lang="ts">
  import { browser } from '$app/environment';
  import { goto } from '$app/navigation';
  import { onMount, tick } from 'svelte';

  import { login } from '$lib/api/auth';
  import { ApiError } from '$lib/api/types';
  import { auth } from '$lib/stores/auth';

  const EMAIL_PATTERN =
    /^[^\s@]+@[^\s@]+\.[^\s@]+$/;

  let email = '';
  let password = '';

  let showPassword = false;
  let capsLockOn = false;

  let error = '';
  let loading = false;

  let errorBox: HTMLDivElement | null = null;

  $: normalizedEmail =
    email.trim().toLowerCase();

  $: emailValid =
    EMAIL_PATTERN.test(
      normalizedEmail
    );

  $: canSubmit =
    emailValid &&
    Boolean(password) &&
    !loading;

  $: registerHref =
    buildRegisterHref();

  $: redirectDestination =
    destinationAfterLogin();

  $: hasCustomDestination =
    redirectDestination !== '/dashboard';

  function destinationAfterLogin() {
    if (!browser) {
      return '/dashboard';
    }

    const redirect =
      new URLSearchParams(
        window.location.search
      )
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

  function buildRegisterHref() {
    if (!browser) {
      return '/register';
    }

    const params =
      new URLSearchParams();

    if (
      redirectDestination !== '/dashboard'
    ) {
      params.set(
        'redirect',
        redirectDestination
      );
    }

    if (normalizedEmail) {
      params.set(
        'email',
        normalizedEmail
      );
    }

    const query =
      params.toString();

    return query
      ? `/register?${query}`
      : '/register';
  }

  function destinationLabel() {
    if (
      redirectDestination === '/dashboard'
    ) {
      return 'Dashboard';
    }

    const cleanPath =
      redirectDestination
        .split('?')[0]
        .split('#')[0]
        .replace(/^\/+|\/+$/g, '');

    if (!cleanPath) {
      return 'Softadastra Cloud';
    }

    return cleanPath
      .split('/')
      .filter(Boolean)
      .map((part) =>
        part
          .replace(/[-_]+/g, ' ')
          .replace(
            /\b\w/g,
            (letter) =>
              letter.toUpperCase()
          )
      )
      .join(' · ');
  }

  async function showError(
    message: string
  ) {
    error = message;

    await tick();

    errorBox?.focus();
  }

  function clearError() {
    if (error) {
      error = '';
    }
  }

  function handlePasswordKeyState(
    event: KeyboardEvent
  ) {
    capsLockOn =
      event.getModifierState(
        'CapsLock'
      );
  }

  function handlePasswordBlur() {
    capsLockOn = false;
  }

  function handleSubmit(
    event: SubmitEvent
  ) {
    event.preventDefault();
    void submit();
  }

  async function submit() {
    if (loading) {
      return;
    }

    error = '';

    if (!normalizedEmail) {
      await showError(
        'Enter your email address.'
      );

      return;
    }

    if (!emailValid) {
      await showError(
        'Enter a valid email address.'
      );

      return;
    }

    if (!password) {
      await showError(
        'Enter your password.'
      );

      return;
    }

    loading = true;

    try {
      const data =
        await login(
          normalizedEmail,
          password
        );

      auth.setSession(data);

      await goto(
        destinationAfterLogin(),
        {
          replaceState: true
        }
      );
    } catch (err) {
      await showError(
        err instanceof ApiError
          ? err.message
          : 'Unable to sign in. Check your email and password.'
      );
    } finally {
      loading = false;
    }
  }

  onMount(() => {
    if ($auth.session) {
      void goto(
        destinationAfterLogin(),
        {
          replaceState: true
        }
      );

      return;
    }

    const params =
      new URLSearchParams(
        window.location.search
      );

    const suggestedEmail =
      params.get('email')?.trim();

    if (
      suggestedEmail &&
      EMAIL_PATTERN.test(
        suggestedEmail
      )
    ) {
      email =
        suggestedEmail;
    }
  });
</script>

<svelte:head>
  <title>
    Sign in | Softadastra Cloud
  </title>

  <meta
    name="description"
    content="Sign in to access your Softadastra Cloud workspaces, C++ projects, packages, lockfiles and build reports."
  />
</svelte:head>

<main class="login-page">
  <!-- ===================================================
       PRODUCT CONTEXT
       =================================================== -->
  <section
    class="login-guide"
    aria-labelledby="login-guide-title"
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

      <span>
        Softadastra Cloud
      </span>
    </a>

    <div class="guide-content">
      <p class="guide-eyebrow">
        Return to your workspace
      </p>

      <h1 id="login-guide-title">
        Continue from
        <span>
          where you left off.
        </span>
      </h1>

      <p class="guide-description">
        Sign in to access the projects, packages,
        build history and workspace activity already
        connected to your account.
      </p>

      <div class="workspace-summary">
        <article>
          <span
            class="summary-icon"
            aria-hidden="true"
          >
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="1.8"
              stroke-linecap="round"
              stroke-linejoin="round"
            >
              <rect
                x="3"
                y="3"
                width="7"
                height="7"
                rx="1"
              ></rect>

              <rect
                x="14"
                y="3"
                width="7"
                height="7"
                rx="1"
              ></rect>

              <rect
                x="3"
                y="14"
                width="7"
                height="7"
                rx="1"
              ></rect>

              <rect
                x="14"
                y="14"
                width="7"
                height="7"
                rx="1"
              ></rect>
            </svg>
          </span>

          <div>
            <strong>
              Workspaces and members
            </strong>

            <p>
              Return to the teams and project
              environments available to your account.
            </p>
          </div>
        </article>

        <article>
          <span
            class="summary-icon"
            aria-hidden="true"
          >
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="1.8"
              stroke-linecap="round"
              stroke-linejoin="round"
            >
              <path
                d="M3 7.5 12 3l9 4.5v9L12 21l-9-4.5Z"
              ></path>

              <path d="M3 7.5 12 12l9-4.5"></path>
              <path d="M12 12v9"></path>
            </svg>
          </span>

          <div>
            <strong>
              Projects and packages
            </strong>

            <p>
              Review connected C++ projects and the
              package versions shared through Cloud.
            </p>
          </div>
        </article>

        <article>
          <span
            class="summary-icon"
            aria-hidden="true"
          >
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="1.8"
              stroke-linecap="round"
              stroke-linejoin="round"
            >
              <path d="M4 19V9"></path>
              <path d="M10 19V5"></path>
              <path d="M16 19v-7"></path>
              <path d="M22 19V3"></path>
            </svg>
          </span>

          <div>
            <strong>
              Build and lockfile history
            </strong>

            <p>
              Keep useful project results available
              without moving compilation away from
              your machine.
            </p>
          </div>
        </article>
      </div>
    </div>

    <footer class="guide-footer">
      <span aria-hidden="true">
        <svg
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="1.8"
          stroke-linecap="round"
          stroke-linejoin="round"
        >
          <rect
            x="5"
            y="10"
            width="14"
            height="11"
            rx="2"
          ></rect>

          <path
            d="M8 10V7a4 4 0 0 1 8 0v3"
          ></path>
        </svg>
      </span>

      <p>
        Your private workspaces, packages and
        credentials remain restricted to authorized
        accounts.
      </p>
    </footer>
  </section>

  <!-- ===================================================
       SIGN IN
       =================================================== -->
  <section class="login-main">
    <div class="mobile-brand">
      <a
        href="/"
        aria-label="Softadastra Cloud home"
      >
        <img
          src="/brand/softadastra-cloud.svg"
          alt=""
          aria-hidden="true"
        />

        <span>
          Softadastra Cloud
        </span>
      </a>
    </div>

    <section
      class="login-card"
      aria-labelledby="login-title"
    >
      <header class="login-header">
        <p class="form-eyebrow">
          Account access
        </p>

        <h2 id="login-title">
          Sign in
        </h2>

        <p>
          Enter the email address and password
          connected to your Softadastra Cloud
          account.
        </p>
      </header>

      {#if hasCustomDestination}
        <div class="redirect-notice">
          <span
            class="redirect-notice__icon"
            aria-hidden="true"
          >
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="1.8"
              stroke-linecap="round"
              stroke-linejoin="round"
            >
              <path d="M5 12h14"></path>
              <path d="m13 6 6 6-6 6"></path>
            </svg>
          </span>

          <div>
            <span>
              After signing in
            </span>

            <strong>
              Continue to
              {destinationLabel()}
            </strong>
          </div>
        </div>
      {/if}

      {#if error}
        <div
          bind:this={errorBox}
          class="login-error"
          role="alert"
          aria-live="assertive"
          tabindex="-1"
        >
          <span aria-hidden="true">
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="1.8"
              stroke-linecap="round"
              stroke-linejoin="round"
            >
              <circle
                cx="12"
                cy="12"
                r="9"
              ></circle>

              <path d="M12 8v5"></path>
              <path d="M12 17h.01"></path>
            </svg>
          </span>

          <div>
            <strong>
              Sign-in failed
            </strong>

            <p>{error}</p>
          </div>
        </div>
      {/if}

      <form
        class="login-form"
        onsubmit={handleSubmit}
      >
        <label>
          <span>Email address</span>

          <input
            class:invalid={
              Boolean(normalizedEmail) &&
              !emailValid
            }
            bind:value={email}
            type="email"
            placeholder="you@example.com"
            autocomplete="email"
            inputmode="email"
            required
            disabled={loading}
            aria-invalid={
              Boolean(normalizedEmail) &&
              !emailValid
            }
            oninput={clearError}
          />

          {#if
            normalizedEmail &&
            !emailValid
          }
            <small class="field-error">
              Enter a valid email address.
            </small>
          {:else}
            <small>
              Use the address connected to your
              Softadastra Cloud account.
            </small>
          {/if}
        </label>

        <label>
          <span>Password</span>

          <span class="password-field">
            <input
              bind:value={password}
              type={
                showPassword
                  ? 'text'
                  : 'password'
              }
              placeholder="Enter your password"
              autocomplete="current-password"
              required
              disabled={loading}
              oninput={clearError}
              onkeydown={handlePasswordKeyState}
              onkeyup={handlePasswordKeyState}
              onblur={handlePasswordBlur}
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
              onclick={() => {
                showPassword =
                  !showPassword;
              }}
            >
              {#if showPassword}
                <svg
                  viewBox="0 0 24 24"
                  aria-hidden="true"
                >
                  <path d="M3 3l18 18"></path>

                  <path
                    d="M10.6 10.6a2 2 0 0 0 2.8 2.8"
                  ></path>

                  <path
                    d="M9.9 4.2A10.8 10.8 0 0 1 12 4c6.5 0 10 8 10 8a17.5 17.5 0 0 1-2.2 3.4"
                  ></path>

                  <path
                    d="M6.6 6.6C3.7 8.5 2 12 2 12s3.5 8 10 8a9.7 9.7 0 0 0 5.4-1.6"
                  ></path>
                </svg>
              {:else}
                <svg
                  viewBox="0 0 24 24"
                  aria-hidden="true"
                >
                  <path
                    d="M2 12s3.5-8 10-8 10 8 10 8-3.5 8-10 8S2 12 2 12Z"
                  ></path>

                  <circle
                    cx="12"
                    cy="12"
                    r="2.5"
                  ></circle>
                </svg>
              {/if}
            </button>
          </span>

          {#if capsLockOn}
            <small
              class="caps-lock-notice"
              role="status"
            >
              <svg
                viewBox="0 0 24 24"
                aria-hidden="true"
              >
                <path d="M12 19V5"></path>
                <path d="m6 11 6-6 6 6"></path>
                <path d="M6 22h12"></path>
              </svg>

              Caps Lock is active.
            </small>
          {:else}
            <small>
              Passwords are case-sensitive.
            </small>
          {/if}
        </label>

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

            <span>
              Signing in…
            </span>
          {:else}
            <span>
              Sign in
            </span>

            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path d="M5 12h14"></path>
              <path d="m13 6 6 6-6 6"></path>
            </svg>
          {/if}
        </button>
      </form>

      <div class="account-switch">
        <span>
          New to Softadastra Cloud?
        </span>

        <a href={registerHref}>
          Create an account
        </a>
      </div>

      <footer class="login-security">
        <span aria-hidden="true">
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="1.8"
            stroke-linecap="round"
            stroke-linejoin="round"
          >
            <path
              d="M12 3 4 7v5c0 5 3.4 8.7 8 10 4.6-1.3 8-5 8-10V7l-8-4Z"
            ></path>

            <path d="m9 12 2 2 4-4"></path>
          </svg>
        </span>

        <p>
          Your password and session credentials are
          never displayed in Softadastra Cloud.
        </p>
      </footer>
    </section>
  </section>
</main>

<style>
.login-page {
  display: grid;
  width: 100%;
  min-width: 0;
  min-height: 100dvh;
  grid-template-columns:
    minmax(0, 0.92fr)
    minmax(0, 1.08fr);
  overflow: clip;
  background: var(--bg);
}

.login-guide,
.login-main,
.guide-content,
.login-card {
  min-width: 0;
  max-width: 100%;
  box-sizing: border-box;
}
  /* =====================================================
     GUIDE
     ===================================================== */

  .login-guide {
    display: flex;
    min-width: 0;
    min-height: 100dvh;
    flex-direction: column;
    justify-content: space-between;
    gap: 48px;
    border-right: 1px solid var(--line);
    background: var(--bg);
    padding:
      clamp(32px, 5vw, 68px)
      clamp(34px, 5vw, 76px);
  }

  .brand {
    display: inline-flex;
    width: fit-content;
    align-items: center;
    gap: 10px;
    color: var(--text);
    font-size: 13px;
    font-weight: 650;
    text-decoration: none;
  }

  .brand img,
  .mobile-brand img {
    display: block;
    width: 25px;
    height: 25px;
    flex: 0 0 auto;
    object-fit: contain;
  }

  .guide-content {
    display: grid;
    max-width: 560px;
    align-content: center;
  }

  .guide-eyebrow,
  .form-eyebrow {
    color: var(--brand-soft);
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 0.1em;
    text-transform: uppercase;
  }

  .guide-content h1 {
    max-width: 550px;
    margin-top: 12px;
    color: var(--text);
    font-size: clamp(36px, 4.7vw, 58px);
    font-weight: 650;
    line-height: 0.98;
    letter-spacing: -0.052em;
  }

  .guide-content h1 span {
    display: block;
    color: var(--brand-soft);
  }

  .guide-description {
    max-width: 530px;
    margin-top: 20px;
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.75;
  }

  .workspace-summary {
    display: grid;
    margin-top: 34px;
    border-top: 1px solid var(--line);
  }

  .workspace-summary article {
    display: grid;
    min-height: 91px;
    grid-template-columns:
      36px minmax(0, 1fr);
    align-items: center;
    gap: 13px;
    border-bottom: 1px solid var(--line);
  }

  .summary-icon {
    display: grid;
    width: 35px;
    height: 35px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 8px;
    color: var(--text-muted);
  }

  .workspace-summary article:first-child
    .summary-icon {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .summary-icon svg {
    width: 17px;
    height: 17px;
  }

  .workspace-summary article > div {
    display: grid;
    gap: 5px;
  }

  .workspace-summary strong {
    color: var(--text-soft);
    font-size: 12px;
    font-weight: 600;
  }

  .workspace-summary p {
    max-width: 440px;
    color: var(--text-muted);
    font-size: 10.5px;
    line-height: 1.55;
  }

  .guide-footer {
    display: flex;
    max-width: 470px;
    align-items: flex-start;
    gap: 9px;
    color: var(--text-muted);
  }

  .guide-footer > span {
    color: var(--text-faint);
  }

  .guide-footer svg {
    width: 15px;
    height: 15px;
  }

  .guide-footer p {
    font-size: 9.5px;
    line-height: 1.55;
  }

  /* =====================================================
     LOGIN COLUMN
     ===================================================== */

  .login-main {
    display: grid;
    min-width: 0;
    min-height: 100dvh;
    place-items: center;
    overflow-y: auto;
    background: var(--bg);
    padding:
      clamp(34px, 5vw, 72px)
      clamp(24px, 7vw, 100px);
  }

  .mobile-brand {
    display: none;
  }

  .login-card {
    display: grid;
    width: min(100%, 470px);
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-lg);
    background: var(--bg-panel);
    box-shadow: var(--shadow-panel);
    animation:
      card-enter
      380ms var(--ease) both;
  }

  @keyframes card-enter {
    from {
      opacity: 0;
      transform: translateY(7px);
    }
  }

  .login-header {
    display: grid;
    gap: 7px;
    padding: 27px 27px 22px;
    border-bottom: 1px solid var(--line-soft);
  }

  .login-header h2 {
    color: var(--text);
    font-size: 29px;
    font-weight: 640;
    line-height: 1.08;
    letter-spacing: -0.04em;
  }

  .login-header > p:last-child {
    max-width: 390px;
    color: var(--text-muted);
    font-size: 11px;
    line-height: 1.6;
  }

  /* =====================================================
     REDIRECT CONTEXT
     ===================================================== */

  .redirect-notice {
    display: grid;
    grid-template-columns:
      34px minmax(0, 1fr);
    align-items: center;
    gap: 10px;
    margin: 18px 27px 0;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-sm);
    background: var(--brand-faint);
    padding: 10px 11px;
  }

  .redirect-notice__icon {
    display: grid;
    width: 33px;
    height: 33px;
    place-items: center;
    border-radius: 7px;
    color: var(--brand-soft);
  }

  .redirect-notice__icon svg {
    width: 16px;
    height: 16px;
  }

  .redirect-notice > div {
    display: grid;
    gap: 3px;
  }

  .redirect-notice div > span {
    color: var(--text-muted);
    font-size: 9.5px;
  }

  .redirect-notice strong {
    overflow-wrap: anywhere;
    color: var(--brand-bright);
    font-size: 10.5px;
  }

  /* =====================================================
     ERROR
     ===================================================== */

  .login-error {
    display: grid;
    grid-template-columns:
      31px minmax(0, 1fr);
    align-items: start;
    gap: 10px;
    margin: 18px 27px 0;
    border: 1px solid var(--danger-line);
    border-radius: var(--radius-sm);
    background: var(--danger-faint);
    padding: 11px;
  }

  .login-error:focus {
    outline: 2px solid var(--danger);
    outline-offset: 2px;
  }

  .login-error > span {
    display: grid;
    width: 30px;
    height: 30px;
    place-items: center;
    border-radius: 7px;
    color: var(--danger);
  }

  .login-error svg {
    width: 17px;
    height: 17px;
  }

  .login-error > div {
    display: grid;
    gap: 3px;
  }

  .login-error strong {
    color: var(--danger);
    font-size: 10.5px;
  }

  .login-error p {
    color: var(--text-soft);
    font-size: 10px;
    line-height: 1.5;
  }

  /* =====================================================
     FORM
     ===================================================== */

  .login-form {
    display: grid;
    gap: 18px;
    padding: 23px 27px 25px;
  }

  .login-form label {
    display: grid;
    gap: 7px;
  }

  .login-form label > span:first-child {
    color: var(--text-soft);
    font-size: 11px;
    font-weight: 550;
  }

  .login-form input {
    width: 100%;
    min-height: 43px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    outline: 0;
    background: transparent;
    color: var(--text);
    padding: 0 11px;
    font: inherit;
    font-size: 12px;
    transition:
      border-color var(--speed) var(--ease),
      box-shadow var(--speed) var(--ease);
  }

  .login-form input:focus {
    border-color: var(--brand);
    box-shadow:
      0 0 0 3px
      var(--brand-faint);
  }

  .login-form input.invalid {
    border-color: var(--danger);
  }

  .login-form input:disabled {
    cursor: wait;
    opacity: 0.68;
  }

  .login-form label > small {
    color: var(--text-muted);
    font-size: 9.5px;
    line-height: 1.5;
  }

  .field-error {
    color: var(--danger) !important;
  }

  /* Password */

  .password-field {
    position: relative;
    display: block;
  }

  .password-field input {
    padding-right: 46px;
  }

  .password-toggle {
    position: absolute;
    top: 50%;
    right: 6px;
    display: grid;
    width: 31px;
    height: 31px;
    min-height: 31px;
    place-items: center;
    border: 0;
    border-radius: 7px;
    background: transparent;
    color: var(--text-muted);
    padding: 0;
    cursor: pointer;
    transform: translateY(-50%);
  }

  .password-toggle:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
  }

  .password-toggle:disabled {
    cursor: wait;
    opacity: 0.5;
  }

  .password-toggle svg {
    width: 15px;
    height: 15px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .caps-lock-notice {
    display: flex;
    align-items: center;
    gap: 6px;
    color: var(--warning) !important;
  }

  .caps-lock-notice svg {
    width: 13px;
    height: 13px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  /* Submit */

  .sign-in-button {
    display: inline-flex;
    width: 100%;
    min-height: 43px;
    align-items: center;
    justify-content: center;
    gap: 8px;
    margin-top: 3px;
    border: 1px solid var(--brand);
    border-radius: var(--radius-sm);
    background: var(--brand);
    color: var(--brand-ink);
    padding: 0 15px;
    font: inherit;
    font-size: 12px;
    font-weight: 650;
    cursor: pointer;
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease),
      transform var(--speed) var(--ease);
  }

  .sign-in-button:hover:not(:disabled) {
    border-color: var(--brand-soft);
    background: var(--brand-soft);
    transform: translateY(-1px);
  }

  .sign-in-button:disabled {
    cursor: not-allowed;
    opacity: 0.48;
  }

  .sign-in-button svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .button-spinner {
    width: 14px;
    height: 14px;
    border: 2px solid
      rgba(26, 15, 6, 0.3);
    border-top-color:
      var(--brand-ink);
    border-radius: 50%;
    animation:
      spinner-rotation
      700ms linear infinite;
  }

  @keyframes spinner-rotation {
    to {
      transform: rotate(360deg);
    }
  }

  /* =====================================================
     CARD FOOTER
     ===================================================== */

  .account-switch {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 6px;
    padding: 15px 27px;
    border-top: 1px solid var(--line-soft);
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .account-switch a {
    color: var(--brand-soft);
    font-weight: 600;
    text-decoration: none;
  }

  .account-switch a:hover {
    color: var(--brand-bright);
    text-decoration: underline;
    text-underline-offset: 3px;
  }

  .login-security {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 7px;
    padding: 12px 20px;
    border-top: 1px solid var(--line-soft);
    color: var(--text-faint);
  }

  .login-security > span {
    display: grid;
    flex: 0 0 auto;
    place-items: center;
  }

  .login-security svg {
    width: 14px;
    height: 14px;
  }

  .login-security p {
    font-size: 9px;
    line-height: 1.45;
    text-align: center;
  }

  /* =====================================================
     RESPONSIVE
     ===================================================== */

  @media (prefers-reduced-motion: reduce) {
    .login-card,
    .button-spinner {
      animation: none;
    }
  }

  @media (max-width: 960px) {
    .login-page {
      grid-template-columns:
        minmax(350px, 0.8fr)
        minmax(480px, 1.2fr);
    }

    .login-guide {
      padding: 38px 32px;
    }

    .guide-content h1 {
      font-size:
        clamp(34px, 5vw, 48px);
    }
  }

  @media (max-width: 820px) {
    .login-page {
      display: block;
      min-height: 100dvh;
      overflow: visible;
    }

    .login-guide {
      display: none;
    }

    .login-main {
      display: grid;
      min-height: 100dvh;
      align-content: start;
      justify-items: center;
      gap: 25px;
      padding:
        28px 18px 52px;
    }

    .mobile-brand {
      display: block;
      width: min(100%, 470px);
    }

    .mobile-brand a {
      display: inline-flex;
      align-items: center;
      gap: 9px;
      color: var(--text);
      font-size: 12px;
      font-weight: 650;
      text-decoration: none;
    }
  }

@media (max-width: 820px) {
  .login-page {
    display: block;
    width: 100%;
    min-height: 100dvh;
    overflow: visible;
  }
}

  @media (max-width: 520px) {
    .login-main {
      padding:
        22px 13px 42px;
    }

    .login-card {
      border-radius:
        var(--radius-md);
    }

    .login-header {
      padding:
        23px 20px 19px;
    }

    .login-header h2 {
      font-size: 26px;
    }

    .login-form {
      padding:
        21px 20px 23px;
    }

    .login-error,
    .redirect-notice {
      margin:
        16px 20px 0;
    }

    .account-switch {
      align-items: center;
      flex-direction: column;
      padding-inline: 20px;
    }
  }
</style>
