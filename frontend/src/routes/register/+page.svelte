<script lang="ts">
  import { browser } from '$app/environment';
  import { goto } from '$app/navigation';
  import { onMount, tick } from 'svelte';

  import {
    login,
    register
  } from '$lib/api/auth';

  import { ApiError } from '$lib/api/types';
  import { auth } from '$lib/stores/auth';

  const MIN_PASSWORD_LENGTH = 8;
  const MAX_NAME_LENGTH = 80;

  const EMAIL_PATTERN =
    /^[^\s@]+@[^\s@]+\.[^\s@]+$/;

  type RegistrationPhase =
    | 'idle'
    | 'creating'
    | 'signing-in';

  let name = '';
  let email = '';
  let password = '';
  let confirmPassword = '';

  let showPassword = false;
  let showConfirmPassword = false;

  let phase: RegistrationPhase = 'idle';

  let error = '';
  let errorBox: HTMLDivElement | null = null;

  let accountCreated = false;
  let createdEmail = '';

  $: normalizedName =
    name.trim();

  $: normalizedEmail =
    email.trim().toLowerCase();

  $: emailValid =
    EMAIL_PATTERN.test(
      normalizedEmail
    );

  $: passwordLongEnough =
    password.length >=
    MIN_PASSWORD_LENGTH;

  $: passwordsMatch =
    Boolean(confirmPassword) &&
    password === confirmPassword;

  $: loading =
    phase !== 'idle';

  $: canSubmit =
    Boolean(normalizedName) &&
    emailValid &&
    passwordLongEnough &&
    passwordsMatch &&
    !loading;

  $: signInHref =
    buildSignInHref();

  function destinationAfterRegistration() {
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

  function buildSignInHref() {
    if (!browser) {
      return '/login';
    }

    const destination =
      destinationAfterRegistration();

    if (
      destination === '/dashboard'
    ) {
      return '/login';
    }

    return (
      `/login?redirect=` +
      encodeURIComponent(
        destination
      )
    );
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

    if (!normalizedName) {
      await showError(
        'Enter your name to continue.'
      );

      return;
    }

    if (
      normalizedName.length >
      MAX_NAME_LENGTH
    ) {
      await showError(
        `Name cannot exceed ${MAX_NAME_LENGTH} characters.`
      );

      return;
    }

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

    if (!passwordLongEnough) {
      await showError(
        `Password must contain at least ${MIN_PASSWORD_LENGTH} characters.`
      );

      return;
    }

    if (!confirmPassword) {
      await showError(
        'Enter the password again.'
      );

      return;
    }

    if (
      password !==
      confirmPassword
    ) {
      await showError(
        'Passwords do not match.'
      );

      return;
    }

    phase = 'creating';

    try {
      await register(
        normalizedName,
        normalizedEmail,
        password
      );
    } catch (err) {
      phase = 'idle';

      await showError(
        err instanceof ApiError
          ? err.message
          : 'Unable to create your account.'
      );

      return;
    }

    phase = 'signing-in';

    try {
      const data =
        await login(
          normalizedEmail,
          password
        );

      auth.setSession(data);

      await goto(
        destinationAfterRegistration()
      );
    } catch {
      createdEmail =
        normalizedEmail;

      accountCreated = true;
      phase = 'idle';

      password = '';
      confirmPassword = '';

      showPassword = false;
      showConfirmPassword = false;
    }
  }

  onMount(() => {
    if ($auth.session) {
      void goto(
        destinationAfterRegistration()
      );

      return;
    }

    const params =
      new URLSearchParams(
        window.location.search
      );

    const suggestedEmail =
      params.get('email')?.trim();

    const suggestedName =
      params.get('name')?.trim();

    if (
      suggestedEmail &&
      EMAIL_PATTERN.test(
        suggestedEmail
      )
    ) {
      email =
        suggestedEmail;
    }

    if (suggestedName) {
      name =
        suggestedName.slice(
          0,
          MAX_NAME_LENGTH
        );
    }
  });
</script>

<svelte:head>
  <title>
    Create account | Softadastra Cloud
  </title>

  <meta
    name="description"
    content="Create a Softadastra Cloud account and start a workspace for C++ projects, packages, lockfiles and build reports."
  />
</svelte:head>

<main class="register-page">
  <!-- ===================================================
       PRODUCT ORIENTATION
       =================================================== -->
  <section
    class="register-guide"
    aria-labelledby="register-guide-title"
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
        Start with one account
      </p>

      <h1 id="register-guide-title">
        Give your C++ work
        <span>
          a shared home.
        </span>
      </h1>

      <p class="guide-description">
        Create your account, open a workspace and
        connect the first Vix project. Compilation
        remains local while project information,
        packages and build results stay organized
        for the team.
      </p>

      <div
        class="registration-journey"
        aria-label="Getting started steps"
      >
        <article class="active">
          <span class="journey-number">
            01
          </span>

          <div>
            <strong>
              Create your account
            </strong>

            <p>
              Use one personal identity across
              Softadastra Cloud.
            </p>
          </div>
        </article>

        <article>
          <span class="journey-number">
            02
          </span>

          <div>
            <strong>
              Create a workspace
            </strong>

            <p>
              Organize projects, packages and the
              people working with you.
            </p>
          </div>
        </article>

        <article>
          <span class="journey-number">
            03
          </span>

          <div>
            <strong>
              Connect a Vix project
            </strong>

            <p>
              Keep development local and share the
              useful project history.
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
        Private workspaces and credentials remain
        visible only to authorized members.
      </p>
    </footer>
  </section>

  <!-- ===================================================
       REGISTRATION
       =================================================== -->
  <section class="register-main">
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

    {#if accountCreated}
      <section
        class="completion-card"
        aria-labelledby="completion-title"
      >
        <span
          class="completion-icon"
          aria-hidden="true"
        >
          <svg
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
            stroke-linecap="round"
            stroke-linejoin="round"
          >
            <path d="m5 12 4 4L19 6"></path>
          </svg>
        </span>

        <div class="completion-heading">
          <p class="form-eyebrow">
            Account created
          </p>

          <h2 id="completion-title">
            Your account is ready.
          </h2>

          <p>
            The account for
            <strong>{createdEmail}</strong>
            was created successfully, but automatic
            sign-in could not be completed.
          </p>
        </div>

        <div class="completion-next">
          <span>Next step</span>

          <p>
            Sign in with the email address and
            password you just created.
          </p>
        </div>

        <a
          class="primary-link"
          href={signInHref}
        >
          Continue to sign in

          <svg
            viewBox="0 0 24 24"
            aria-hidden="true"
          >
            <path d="M5 12h14"></path>
            <path d="m13 6 6 6-6 6"></path>
          </svg>
        </a>

        <a
          class="home-link"
          href="/"
        >
          Return to Softadastra Cloud
        </a>
      </section>
    {:else}
      <section
        class="register-card"
        aria-labelledby="register-title"
      >
        <header class="register-header">
          <p class="form-eyebrow">
            New account
          </p>

          <h2 id="register-title">
            Create your account
          </h2>

          <p>
            Enter your personal details. You will
            create the workspace after signing in.
          </p>
        </header>

        {#if error}
          <div
            bind:this={errorBox}
            class="register-error"
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
                Account creation failed
              </strong>

              <p>{error}</p>
            </div>
          </div>
        {/if}

        <form
          class="register-form"
          onsubmit={handleSubmit}
        >
          <label>
            <span>Name</span>

            <input
              bind:value={name}
              type="text"
              placeholder="Your name"
              autocomplete="name"
              maxlength={MAX_NAME_LENGTH}
              required
              disabled={loading}
              oninput={clearError}
            />

            <small>
              Used for your account and workspace
              membership.
            </small>
          </label>

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
                Used to sign in and recover access to
                your account.
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
                placeholder={`At least ${MIN_PASSWORD_LENGTH} characters`}
                autocomplete="new-password"
                minlength={MIN_PASSWORD_LENGTH}
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
                aria-pressed={
                  showPassword
                }
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

            <div class="password-checks">
              <span
                class:complete={
                  passwordLongEnough
                }
              >
                <i aria-hidden="true">
                  {#if passwordLongEnough}
                    <svg
                      viewBox="0 0 24 24"
                    >
                      <path
                        d="m5 12 4 4L19 6"
                      ></path>
                    </svg>
                  {/if}
                </i>

                At least
                {MIN_PASSWORD_LENGTH}
                characters
              </span>

              <span>
                <i aria-hidden="true"></i>

                Do not reuse a password from another
                service
              </span>
            </div>
          </label>

          <label>
            <span>Confirm password</span>

            <span class="password-field">
              <input
                class:invalid={
                  Boolean(
                    confirmPassword
                  ) &&
                  !passwordsMatch
                }
                bind:value={confirmPassword}
                type={
                  showConfirmPassword
                    ? 'text'
                    : 'password'
                }
                placeholder="Enter the password again"
                autocomplete="new-password"
                minlength={MIN_PASSWORD_LENGTH}
                required
                disabled={loading}
                aria-invalid={
                  Boolean(
                    confirmPassword
                  ) &&
                  !passwordsMatch
                }
                oninput={clearError}
              />

              <button
                class="password-toggle"
                type="button"
                disabled={loading}
                aria-label={
                  showConfirmPassword
                    ? 'Hide password confirmation'
                    : 'Show password confirmation'
                }
                aria-pressed={
                  showConfirmPassword
                }
                onclick={() => {
                  showConfirmPassword =
                    !showConfirmPassword;
                }}
              >
                {#if showConfirmPassword}
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

            {#if confirmPassword}
              <small
                class:valid={
                  passwordsMatch
                }
                class:field-error={
                  !passwordsMatch
                }
                class="confirmation-state"
              >
                {#if passwordsMatch}
                  <svg
                    viewBox="0 0 24 24"
                    aria-hidden="true"
                  >
                    <path
                      d="m5 12 4 4L19 6"
                    ></path>
                  </svg>

                  Passwords match
                {:else}
                  Passwords do not match.
                {/if}
              </small>
            {/if}
          </label>

          <button
            class="create-account-button"
            type="submit"
            disabled={!canSubmit}
          >
            {#if phase === 'creating'}
              <span
                class="button-spinner"
                aria-hidden="true"
              ></span>

              <span>
                Creating account…
              </span>
            {:else if phase === 'signing-in'}
              <span
                class="button-spinner"
                aria-hidden="true"
              ></span>

              <span>
                Opening your account…
              </span>
            {:else}
              <span>
                Create account
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
            Already have an account?
          </span>

          <a href={signInHref}>
            Sign in
          </a>
        </div>

        <footer class="register-security">
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
            Your password is never displayed or
            stored in plain text.
          </p>
        </footer>
      </section>
    {/if}
  </section>
</main>

<style>
.register-page {
  display: grid;
  width: 100%;
  min-width: 0;
  max-width: 100%;
  min-height: 100dvh;
  grid-template-columns:
    minmax(0, 0.92fr)
    minmax(0, 1.08fr);
  overflow-x: clip;
  background: var(--bg);
}
.register-guide,
.register-main,
.guide-content,
.register-card,
.completion-card {
  min-width: 0;
  max-width: 100%;
  box-sizing: border-box;
}
  /* =====================================================
     GUIDE
     ===================================================== */

  .register-guide {
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

  .registration-journey {
    display: grid;
    margin-top: 34px;
    border-top: 1px solid var(--line);
  }

  .registration-journey article {
    display: grid;
    min-height: 82px;
    grid-template-columns:
      34px minmax(0, 1fr);
    align-items: center;
    gap: 13px;
    border-bottom: 1px solid var(--line);
    opacity: 0.68;
  }

  .registration-journey article.active {
    opacity: 1;
  }

  .journey-number {
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 700;
  }

  .registration-journey article.active
    .journey-number {
    color: var(--brand-soft);
  }

  .registration-journey article > div {
    display: grid;
    gap: 5px;
  }

  .registration-journey strong {
    color: var(--text-soft);
    font-size: 12px;
    font-weight: 600;
  }

  .registration-journey p {
    max-width: 430px;
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
     FORM COLUMN
     ===================================================== */

  .register-main {
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

  .register-card,
  .completion-card {
    display: grid;
    width: min(100%, 470px);
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-lg);
    background: var(--bg-panel);
    box-shadow: var(--shadow-panel);
    animation: card-enter 380ms var(--ease) both;
  }

  @keyframes card-enter {
    from {
      opacity: 0;
      transform: translateY(7px);
    }
  }

  .register-header {
    display: grid;
    gap: 7px;
    padding: 26px 27px 21px;
    border-bottom: 1px solid var(--line-soft);
  }

  .register-header h2 {
    color: var(--text);
    font-size: 27px;
    font-weight: 640;
    line-height: 1.08;
    letter-spacing: -0.038em;
  }

  .register-header > p:last-child {
    max-width: 390px;
    color: var(--text-muted);
    font-size: 11px;
    line-height: 1.6;
  }

  /* =====================================================
     ERROR
     ===================================================== */

  .register-error {
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

  .register-error:focus {
    outline: 2px solid var(--danger);
    outline-offset: 2px;
  }

  .register-error > span {
    display: grid;
    width: 30px;
    height: 30px;
    place-items: center;
    border-radius: 7px;
    color: var(--danger);
  }

  .register-error svg {
    width: 17px;
    height: 17px;
  }

  .register-error > div {
    display: grid;
    gap: 3px;
  }

  .register-error strong {
    color: var(--danger);
    font-size: 10.5px;
  }

  .register-error p {
    color: var(--text-soft);
    font-size: 10px;
    line-height: 1.5;
  }

  /* =====================================================
     FORM
     ===================================================== */

  .register-form {
    display: grid;
    gap: 17px;
    padding: 22px 27px 24px;
  }

  .register-form label {
    display: grid;
    gap: 7px;
  }

  .register-form label > span:first-child {
    color: var(--text-soft);
    font-size: 11px;
    font-weight: 550;
  }

  .register-form input {
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

  .register-form input:focus {
    border-color: var(--brand);
    box-shadow:
      0 0 0 3px
      var(--brand-faint);
  }

  .register-form input.invalid {
    border-color: var(--danger);
  }

  .register-form input:disabled {
    cursor: wait;
    opacity: 0.68;
  }

  .register-form label > small {
    color: var(--text-muted);
    font-size: 9px;
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

  .password-checks {
    display: grid;
    gap: 6px;
    margin-top: 1px;
  }

  .password-checks > span {
    display: flex;
    align-items: center;
    gap: 7px;
    color: var(--text-muted);
    font-size: 9px;
  }

  .password-checks > span.complete {
    color: var(--green-soft);
  }

  .password-checks i {
    display: grid;
    width: 15px;
    height: 15px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 50%;
  }

  .password-checks .complete i {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .password-checks svg {
    width: 9px;
    height: 9px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2.3;
    stroke-linecap: round;
  }

  .confirmation-state {
    display: flex;
    align-items: center;
    gap: 5px;
  }

  .confirmation-state.valid {
    color: var(--green-soft);
  }

  .confirmation-state svg {
    width: 12px;
    height: 12px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2.2;
    stroke-linecap: round;
  }

  /* Submit */

  .create-account-button,
  .primary-link {
    display: inline-flex;
    width: 100%;
    min-height: 43px;
    align-items: center;
    justify-content: center;
    gap: 8px;
    border: 1px solid var(--brand);
    border-radius: var(--radius-sm);
    background: var(--brand);
    color: var(--brand-ink);
    padding: 0 15px;
    font: inherit;
    font-size: 12px;
    font-weight: 650;
    cursor: pointer;
    text-decoration: none;
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease),
      transform var(--speed) var(--ease);
  }

  .create-account-button {
    margin-top: 3px;
  }

  .create-account-button:hover:not(:disabled),
  .primary-link:hover {
    border-color: var(--brand-soft);
    background: var(--brand-soft);
    transform: translateY(-1px);
  }

  .create-account-button:disabled {
    cursor: not-allowed;
    opacity: 0.48;
  }

  .create-account-button svg,
  .primary-link svg {
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

  /* Lower card */

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

  .account-switch a,
  .home-link {
    color: var(--brand-soft);
    font-weight: 600;
    text-decoration: none;
  }

  .account-switch a:hover,
  .home-link:hover {
    color: var(--brand-bright);
    text-decoration: underline;
    text-underline-offset: 3px;
  }

  .register-security {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 7px;
    padding: 12px 20px;
    border-top: 1px solid var(--line-soft);
    color: var(--text-faint);
  }

  .register-security > span {
    display: grid;
    flex: 0 0 auto;
    place-items: center;
  }

  .register-security svg {
    width: 14px;
    height: 14px;
  }

  .register-security p {
    font-size: 8.5px;
    line-height: 1.45;
    text-align: center;
  }

  /* =====================================================
     ACCOUNT CREATED FALLBACK
     ===================================================== */

  .completion-card {
    justify-items: start;
    gap: 0;
    padding: 28px;
  }

  .completion-icon {
    display: grid;
    width: 48px;
    height: 48px;
    place-items: center;
    border: 1px solid var(--green-line);
    border-radius: 12px;
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .completion-icon svg {
    width: 22px;
    height: 22px;
  }

  .completion-heading {
    display: grid;
    gap: 7px;
    margin-top: 20px;
  }

  .completion-heading h2 {
    color: var(--text);
    font-size: 29px;
    font-weight: 640;
    line-height: 1.08;
    letter-spacing: -0.04em;
  }

  .completion-heading > p:last-child {
    color: var(--text-muted);
    font-size: 11px;
    line-height: 1.65;
  }

  .completion-heading strong {
    color: var(--text-soft);
    font-weight: 600;
  }

  .completion-next {
    display: grid;
    width: 100%;
    gap: 5px;
    margin-top: 21px;
    padding: 13px;
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
  }

  .completion-next span {
    color: var(--brand-soft);
    font-family: var(--font-mono);
    font-size: 9px;
    font-weight: 700;
    text-transform: uppercase;
  }

  .completion-next p {
    color: var(--text-muted);
    font-size: 10px;
    line-height: 1.5;
  }

  .completion-card .primary-link {
    margin-top: 20px;
  }

  .home-link {
    justify-self: center;
    margin-top: 15px;
    font-size: 10px;
  }

  /* =====================================================
     RESPONSIVE
     ===================================================== */

  @media (prefers-reduced-motion: reduce) {
    .register-card,
    .completion-card,
    .button-spinner {
      animation: none;
    }
  }

  @media (max-width: 960px) {
    .register-page {
      grid-template-columns:
        minmax(0, 0.8fr)
        minmax(0, 1.2fr);
    }

    .register-guide {
      padding:
        38px 32px;
    }

    .guide-content h1 {
      font-size:
        clamp(34px, 5vw, 48px);
    }
  }

  @media (max-width: 820px) {
    .register-page {
      display: block;
      width: 100%;
      min-width: 0;
      max-width: 100%;
      min-height: 100dvh;
      overflow-x: clip;
      overflow-y: visible;
    }

    .register-guide {
      display: none;
    }

    .register-main {
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

  @media (max-width: 520px) {
    .register-main {
      padding:
        22px 13px 42px;
    }

    .register-card,
    .completion-card {
      border-radius: var(--radius-md);
    }

    .register-header {
      padding:
        22px 20px 18px;
    }

    .register-header h2 {
      font-size: 25px;
    }

    .register-form {
      padding:
        20px 20px 22px;
    }

    .register-error {
      margin:
        16px 20px 0;
    }

    .account-switch {
      align-items: center;
      flex-direction: column;
      padding-inline: 20px;
    }

    .completion-card {
      padding: 23px 20px;
    }
  }
</style>
