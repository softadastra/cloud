<script lang="ts">
  import { browser } from '$app/environment';
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';

  import { authorizeCliLogin } from '$lib/api/auth';
  import { ApiError } from '$lib/api/types';
  import { auth } from '$lib/stores/auth';

  let status = 'Validating the CLI request...';
  let error = '';
  let progressStep = 0;
  let isRunning = false;

  const steps = [
    {
      title: 'Validate request',
      text: 'Check the callback and authorization state.'
    },
    {
      title: 'Confirm session',
      text: 'Use your current Softadastra Cloud account.'
    },
    {
      title: 'Authorize Vix CLI',
      text: 'Create a temporary authorization code.'
    },
    {
      title: 'Connect terminal',
      text: 'Return the code to the local Vix process.'
    }
  ];

  function currentPathWithQuery() {
    if (!browser) {
      return '/cli/login';
    }

    return `${window.location.pathname}${window.location.search}`;
  }

  function isSafeLoopbackRedirect(value: string) {
    try {
      const url = new URL(value);

      return (
        url.protocol === 'http:' &&
        url.hostname === '127.0.0.1' &&
        url.pathname === '/callback' &&
        !url.username &&
        !url.password &&
        !url.hash
      );
    } catch {
      return false;
    }
  }

  async function run() {
    if (!browser || isRunning) {
      return;
    }

    isRunning = true;
    error = '';
    progressStep = 0;
    status = 'Validating the CLI request...';

    const params = new URLSearchParams(window.location.search);
    const state = params.get('state')?.trim() ?? '';
    const redirectUri = params.get('redirect_uri')?.trim() ?? '';

    if (!state || !redirectUri || !isSafeLoopbackRedirect(redirectUri)) {
      error = 'The CLI login request is invalid or no longer usable.';
      status = '';
      isRunning = false;
      return;
    }

    progressStep = 1;
    status = 'Checking your Cloud session...';

    const authStatus = await auth.ensureAuthLoaded();

    if (authStatus === 'missing' || authStatus === 'invalid') {
      await goto(`/login?redirect=${encodeURIComponent(currentPathWithQuery())}`, {
        replaceState: true
      });

      return;
    }

    if (authStatus === 'unreachable') {
      error =
        'Softadastra Cloud is temporarily unavailable. Return to the terminal and try again.';
      status = '';
      isRunning = false;
      return;
    }

    progressStep = 2;
    status = 'Authorizing Vix CLI...';

    try {
      const data = await authorizeCliLogin(state, redirectUri);
      const callback = new URL(redirectUri);

      callback.searchParams.set('code', data.code);
      callback.searchParams.set('state', state);

      progressStep = 3;
      status = 'Connecting your terminal...';

      try {
        await fetch(callback.toString(), {
          method: 'GET',
          mode: 'no-cors',
          cache: 'no-store',
          keepalive: true
        });
      } catch {
        const image = new Image();
        image.src = callback.toString();
      }

      window.setTimeout(() => {
        void goto('/cli/success', {
          replaceState: true
        });
      }, 900);
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to authorize the Vix CLI.';

      status = '';
      isRunning = false;
    }
  }

  onMount(() => {
    void run();
  });
</script>

<svelte:head>
  <title>Connect Vix CLI | Softadastra Cloud</title>

  <meta
    name="description"
    content="Authorize Vix CLI access to your Softadastra Cloud account."
  />

  <meta
    name="robots"
    content="noindex, nofollow"
  />
</svelte:head>

<main class="cli-login-page">
  <section
    class="cli-login-shell"
    aria-live="polite"
    aria-busy={isRunning && !error}
  >
    <header class="cli-login-header">
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

      <div class="secure-handoff">
        <span aria-hidden="true"></span>
        Secure CLI handoff
      </div>
    </header>

    <div class="cli-login-content">
      <section class="cli-login-copy">
        <p class="eyebrow">
          Vix CLI authorization
        </p>

        <h1>
          Connecting this terminal
          <span>to your Cloud account.</span>
        </h1>

        <p class="description">
          Softadastra Cloud is validating the local Vix request, confirming
          your session, and returning a temporary authorization code to the
          terminal that started the sign-in flow.
        </p>

        <ol class="authorization-steps">
          {#each steps as step, index}
            <li
              class:is-active={index === progressStep && !error}
              class:is-complete={index < progressStep && !error}
            >
              <span class="step-number">
                {#if index < progressStep && !error}
                  <svg
                    viewBox="0 0 24 24"
                    aria-hidden="true"
                  >
                    <path d="m5 12 4 4L19 6" />
                  </svg>
                {:else}
                  {String(index + 1).padStart(2, '0')}
                {/if}
              </span>

              <div>
                <strong>{step.title}</strong>
                <p>{step.text}</p>
              </div>
            </li>
          {/each}
        </ol>

        {#if status}
          <div class="status-message">
            <span
              class="status-spinner"
              aria-hidden="true"
            ></span>

            <div>
              <strong>{status}</strong>
              <p>Keep the terminal open while this page completes the connection.</p>
            </div>
          </div>
        {/if}

        {#if error}
          <div
            class="error-message"
            role="alert"
          >
            <span
              class="error-icon"
              aria-hidden="true"
            >
              <svg viewBox="0 0 24 24">
                <circle
                  cx="12"
                  cy="12"
                  r="9"
                />
                <path d="M12 8v5" />
                <path d="M12 17h.01" />
              </svg>
            </span>

            <div>
              <strong>CLI connection failed</strong>
              <p>{error}</p>
            </div>
          </div>

          <div class="error-actions">
            <button
              type="button"
              disabled={isRunning}
              on:click={() => void run()}
            >
              Try again
            </button>

            <a href="/dashboard">
              Return to Cloud
            </a>
          </div>
        {/if}
      </section>

      <aside
        class="terminal-preview"
        aria-label="Vix CLI connection preview"
      >
        <header class="terminal-preview__header">
          <div>
            <span aria-hidden="true"></span>
            <span aria-hidden="true"></span>
            <span aria-hidden="true"></span>
          </div>

          <strong>Vix CLI</strong>

          <small>local terminal</small>
        </header>

        <div class="terminal-preview__body">
          <p class="terminal-command">
            <span>$</span>
            vix login
          </p>

          <div class="terminal-output">
            <p>
              <span class="terminal-success">✓</span>
              Browser authorization started
            </p>

            <p>
              <span class="terminal-arrow">→</span>
              Waiting for Softadastra Cloud
            </p>

            <p class:is-current={progressStep === 3 && !error}>
              <span class="terminal-dot">•</span>
              Connecting local callback
            </p>
          </div>
        </div>

        <dl class="connection-details">
          <div>
            <dt>Application</dt>
            <dd>Vix CLI</dd>
          </div>

          <div>
            <dt>Callback</dt>
            <dd>127.0.0.1</dd>
          </div>

          <div>
            <dt>Authorization</dt>
            <dd>Temporary code</dd>
          </div>

          <div>
            <dt>Cloud session</dt>
            <dd class="connection-active">Authenticated</dd>
          </div>
        </dl>

        <footer class="terminal-preview__footer">
          <span aria-hidden="true"></span>

          <p>
            The application tab remains on Softadastra Cloud while the code is
            delivered to the local CLI callback.
          </p>
        </footer>
      </aside>
    </div>

    <footer class="cli-login-footer">
      <span class="footer-icon" aria-hidden="true">
        <svg viewBox="0 0 24 24">
          <path d="M7 10V7a5 5 0 0 1 10 0v3" />

          <rect
            x="5"
            y="10"
            width="14"
            height="11"
            rx="2"
          />
        </svg>
      </span>

      <p>
        This page only sends the temporary authorization result to the
        loopback callback opened by the Vix process on your computer.
      </p>
    </footer>
  </section>
</main>

<style>
  .cli-login-page {
    --cli-bg: var(--bg, #131619);
    --cli-panel: var(--bg-panel, #1a1e22);
    --cli-elevated: var(--bg-elevated, #20252a);
    --cli-border: var(--border, rgba(255, 255, 255, 0.1));
    --cli-border-strong: rgba(255, 255, 255, 0.16);

    --cli-text: var(--text, #f4f6f7);
    --cli-text-soft: var(--text-soft, #b6bdc3);
    --cli-text-muted: var(--text-muted, #7f8992);

    --cli-brand: var(--brand, #f97316);
    --cli-brand-soft: var(--brand-soft, #fb923c);
    --cli-brand-faint: var(--brand-faint, rgba(249, 115, 22, 0.1));
    --cli-brand-line: var(--brand-line, rgba(249, 115, 22, 0.35));

    --cli-success: var(--success, #22c55e);
    --cli-danger: var(--danger, #ef4444);

    display: grid;
    min-height: 100vh;
    place-items: center;
    overflow: hidden;
    background: var(--cli-bg);
    color: var(--cli-text);
    padding: 32px;
  }

  .cli-login-shell {
    width: min(100%, 1060px);
    overflow: hidden;
    border: 1px solid var(--cli-border);
    border-radius: 22px;
    background: var(--cli-panel);
  }

  .cli-login-header {
    display: flex;
    min-height: 68px;
    align-items: center;
    justify-content: space-between;
    gap: 20px;
    padding: 0 22px;
    border-bottom: 1px solid var(--cli-border);
    background: var(--cli-elevated);
  }

  .brand {
    display: inline-flex;
    align-items: center;
    gap: 10px;
    color: var(--cli-text);
    text-decoration: none;
  }

  .brand img {
    width: 31px;
    height: 31px;
    object-fit: contain;
  }

  .brand span {
    font-size: 0.84rem;
    font-weight: 700;
    letter-spacing: -0.02em;
  }

  .secure-handoff {
    display: inline-flex;
    min-height: 29px;
    align-items: center;
    gap: 7px;
    border: 1px solid var(--cli-brand-line);
    border-radius: 999px;
    background: var(--cli-brand-faint);
    color: var(--cli-brand-soft);
    padding: 0 10px;
    font-size: 0.68rem;
    font-weight: 650;
  }

  .secure-handoff > span {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: var(--cli-brand);
  }

  .cli-login-content {
    display: grid;
    grid-template-columns:
      minmax(0, 1.08fr)
      minmax(360px, 0.92fr);
    gap: clamp(38px, 7vw, 82px);
    align-items: center;
    padding: clamp(36px, 6vw, 68px);
  }

  .cli-login-copy {
    display: grid;
    min-width: 0;
    justify-items: start;
  }

  .eyebrow {
    color: var(--cli-brand-soft);
    font-family: var(--font-mono, monospace);
    font-size: 0.68rem;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }

  h1 {
    max-width: 680px;
    margin: 14px 0 0;
    color: var(--cli-text);
    font-size: clamp(2.6rem, 5.6vw, 4.9rem);
    font-weight: 690;
    line-height: 0.98;
    letter-spacing: -0.055em;
  }

  h1 span {
    display: block;
    margin-top: 5px;
    color: var(--cli-text-soft);
  }

  .description {
    max-width: 660px;
    margin-top: 21px;
    color: var(--cli-text-soft);
    font-size: 0.95rem;
    line-height: 1.72;
  }

  .authorization-steps {
    display: grid;
    width: 100%;
    margin: 30px 0 0;
    padding: 0;
    list-style: none;
  }

  .authorization-steps li {
    display: grid;
    grid-template-columns: 36px minmax(0, 1fr);
    gap: 11px;
    min-height: 62px;
    align-items: center;
    padding: 9px 0;
    border-bottom: 1px solid var(--cli-border);
    opacity: 0.52;
  }

  .authorization-steps li:first-child {
    border-top: 1px solid var(--cli-border);
  }

  .authorization-steps li.is-active,
  .authorization-steps li.is-complete {
    opacity: 1;
  }

  .step-number {
    display: grid;
    width: 34px;
    height: 34px;
    place-items: center;
    border: 1px solid var(--cli-border);
    border-radius: 9px;
    color: var(--cli-text-muted);
    font-family: var(--font-mono, monospace);
    font-size: 0.66rem;
    font-weight: 700;
  }

  .authorization-steps li.is-active .step-number {
    border-color: var(--cli-brand-line);
    background: var(--cli-brand-faint);
    color: var(--cli-brand-soft);
  }

  .authorization-steps li.is-complete .step-number {
    border-color: rgba(34, 197, 94, 0.32);
    background: rgba(34, 197, 94, 0.08);
    color: var(--cli-success);
  }

  .step-number svg {
    width: 15px;
    height: 15px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2.2;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .authorization-steps li > div {
    display: grid;
    gap: 3px;
  }

  .authorization-steps strong {
    color: var(--cli-text);
    font-size: 0.8rem;
    font-weight: 640;
  }

  .authorization-steps p {
    color: var(--cli-text-muted);
    font-size: 0.7rem;
    line-height: 1.45;
  }

  .status-message,
  .error-message {
    display: grid;
    width: 100%;
    grid-template-columns: 35px minmax(0, 1fr);
    align-items: start;
    gap: 11px;
    margin-top: 22px;
    padding: 14px;
    border-radius: 12px;
  }

  .status-message {
    border: 1px solid var(--cli-brand-line);
    background: var(--cli-brand-faint);
  }

  .status-spinner {
    width: 28px;
    height: 28px;
    border: 2px solid rgba(249, 115, 22, 0.22);
    border-top-color: var(--cli-brand);
    border-radius: 50%;
    animation: cli-spin 800ms linear infinite;
  }

  .status-message > div,
  .error-message > div {
    display: grid;
    gap: 4px;
  }

  .status-message strong {
    color: var(--cli-text);
    font-size: 0.78rem;
    font-weight: 650;
  }

  .status-message p {
    color: var(--cli-text-muted);
    font-size: 0.7rem;
    line-height: 1.5;
  }

  .error-message {
    border: 1px solid rgba(239, 68, 68, 0.32);
    background: rgba(239, 68, 68, 0.08);
  }

  .error-icon {
    display: grid;
    width: 31px;
    height: 31px;
    place-items: center;
    color: var(--cli-danger);
  }

  .error-icon svg {
    width: 19px;
    height: 19px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.9;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .error-message strong {
    color: #fecaca;
    font-size: 0.78rem;
    font-weight: 650;
  }

  .error-message p {
    color: #fca5a5;
    font-size: 0.72rem;
    line-height: 1.5;
  }

  .error-actions {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
    margin-top: 13px;
  }

  .error-actions button,
  .error-actions a {
    display: inline-flex;
    min-height: 38px;
    align-items: center;
    justify-content: center;
    border-radius: 8px;
    padding: 0 13px;
    font: inherit;
    font-size: 0.73rem;
    font-weight: 650;
    text-decoration: none;
  }

  .error-actions button {
    border: 1px solid var(--cli-brand);
    background: var(--cli-brand);
    color: #1a0f06;
  }

  .error-actions button:disabled {
    cursor: not-allowed;
    opacity: 0.55;
  }

  .error-actions a {
    border: 1px solid var(--cli-border);
    color: var(--cli-text-soft);
  }

  .terminal-preview {
    display: grid;
    min-width: 0;
    overflow: hidden;
    border: 1px solid var(--cli-border-strong);
    border-radius: 16px;
    background: #101311;
  }

  .terminal-preview__header {
    display: grid;
    min-height: 47px;
    grid-template-columns: 1fr auto 1fr;
    align-items: center;
    gap: 12px;
    padding: 0 13px;
    border-bottom: 1px solid #292e2a;
    background: #181c19;
  }

  .terminal-preview__header > div {
    display: flex;
    gap: 5px;
  }

  .terminal-preview__header div > span {
    width: 7px;
    height: 7px;
    border-radius: 50%;
    background: #454b46;
  }

  .terminal-preview__header strong {
    color: #d7dbd8;
    font-family: var(--font-mono, monospace);
    font-size: 0.67rem;
    font-weight: 600;
  }

  .terminal-preview__header small {
    justify-self: end;
    color: #6f7770;
    font-size: 0.62rem;
  }

  .terminal-preview__body {
    display: grid;
    gap: 19px;
    min-height: 216px;
    align-content: center;
    padding: 25px 22px;
  }

  .terminal-command,
  .terminal-output p {
    display: flex;
    align-items: center;
    gap: 8px;
    margin: 0;
    font-family: var(--font-mono, monospace);
  }

  .terminal-command {
    color: #f1f3f1;
    font-size: 0.83rem;
  }

  .terminal-command > span {
    color: var(--cli-brand);
  }

  .terminal-output {
    display: grid;
    gap: 9px;
  }

  .terminal-output p {
    color: #858d86;
    font-size: 0.7rem;
    line-height: 1.45;
  }

  .terminal-output p.is-current {
    color: #f3b27e;
  }

  .terminal-success {
    color: var(--cli-success);
  }

  .terminal-arrow {
    color: var(--cli-brand);
  }

  .terminal-dot {
    color: #6f7770;
  }

  .connection-details {
    display: grid;
    margin: 0;
    padding: 5px 16px;
    border-top: 1px solid #292e2a;
    border-bottom: 1px solid #292e2a;
  }

  .connection-details > div {
    display: flex;
    min-height: 41px;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    border-bottom: 1px solid #252a26;
  }

  .connection-details > div:last-child {
    border-bottom: 0;
  }

  .connection-details dt {
    color: #737b74;
    font-size: 0.67rem;
  }

  .connection-details dd {
    margin: 0;
    color: #cbd0cc;
    font-family: var(--font-mono, monospace);
    font-size: 0.67rem;
    text-align: right;
  }

  .connection-details .connection-active {
    color: var(--cli-success);
  }

  .terminal-preview__footer {
    display: grid;
    grid-template-columns: 8px minmax(0, 1fr);
    align-items: start;
    gap: 9px;
    padding: 14px 16px;
    background: #181c19;
  }

  .terminal-preview__footer > span {
    width: 7px;
    height: 7px;
    margin-top: 4px;
    border-radius: 50%;
    background: var(--cli-brand);
  }

  .terminal-preview__footer p {
    color: #777f78;
    font-size: 0.65rem;
    line-height: 1.5;
  }

  .cli-login-footer {
    display: grid;
    min-height: 61px;
    grid-template-columns: 30px minmax(0, 1fr);
    align-items: center;
    gap: 10px;
    padding: 11px 22px;
    border-top: 1px solid var(--cli-border);
    background: var(--cli-elevated);
  }

  .footer-icon {
    display: grid;
    width: 29px;
    height: 29px;
    place-items: center;
    color: var(--cli-text-muted);
  }

  .footer-icon svg {
    width: 16px;
    height: 16px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .cli-login-footer p {
    color: var(--cli-text-muted);
    font-size: 0.68rem;
    line-height: 1.5;
  }

  @keyframes cli-spin {
    to {
      transform: rotate(360deg);
    }
  }

  @media (max-width: 840px) {
    .cli-login-page {
      place-items: start center;
      padding: 20px;
    }

    .cli-login-content {
      grid-template-columns: 1fr;
    }

    .terminal-preview {
      width: 100%;
      max-width: 620px;
    }
  }

  @media (max-width: 560px) {
    .cli-login-page {
      padding: 0;
    }

    .cli-login-shell {
      min-height: 100vh;
      border: 0;
      border-radius: 0;
    }

    .cli-login-header {
      min-height: 61px;
      padding-inline: 16px;
    }

    .secure-handoff {
      font-size: 0;
      padding-inline: 10px;
    }

    .secure-handoff::after {
      content: 'Secure';
      font-size: 0.66rem;
    }

    .cli-login-content {
      gap: 34px;
      padding: 38px 18px;
    }

    h1 {
      font-size: clamp(2.45rem, 13vw, 3.7rem);
    }

    .description {
      font-size: 0.9rem;
    }

    .terminal-preview__body {
      min-height: 190px;
      padding-inline: 17px;
    }

    .cli-login-footer {
      align-items: start;
      padding-inline: 17px;
    }
  }

  @media (prefers-reduced-motion: reduce) {
    .status-spinner {
      animation-duration: 1.6s;
    }
  }
</style>
