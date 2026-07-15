<script lang="ts">
  import { goto } from '$app/navigation';

  function openDashboard() {
    void goto('/dashboard');
  }
</script>

<svelte:head>
  <title>Vix CLI connected | Softadastra Cloud</title>

  <meta
    name="description"
    content="Vix CLI is now connected to your Softadastra Cloud account."
  />

  <meta
    name="robots"
    content="noindex, nofollow"
  />
</svelte:head>

<main class="success-page">
  <section
    class="success-shell"
    aria-labelledby="success-title"
    aria-live="polite"
  >
    <header class="success-header">
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

      <div class="connection-status">
        <span aria-hidden="true"></span>

        <div>
          <strong>Connection complete</strong>
          <small>Vix CLI</small>
        </div>
      </div>
    </header>

    <div class="success-content">
      <section class="success-copy">
        <div
          class="success-mark"
          aria-hidden="true"
        >
          <svg viewBox="0 0 24 24">
            <path d="m5 12 4 4L19 6" />
          </svg>
        </div>

        <p class="eyebrow">
          Vix CLI authentication
        </p>

        <h1 id="success-title">
          Your terminal
          <span>is connected.</span>
        </h1>

        <p class="message">
          Vix CLI is now signed in to your Softadastra Cloud account. The
          temporary authorization code has been returned to the local process
          that started this sign-in flow.
        </p>

        <div class="next-action">
          <span aria-hidden="true">
            <svg viewBox="0 0 24 24">
              <path d="M4 17 10 11 4 5" />
              <path d="M12 19h8" />
            </svg>
          </span>

          <div>
            <strong>Return to your terminal</strong>

            <p>
              You can close this browser tab and continue using Vix CLI.
            </p>
          </div>
        </div>

        <div class="actions">
          <button
            type="button"
            on:click={openDashboard}
          >
            Open dashboard

            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path d="M5 12h14" />
              <path d="m13 6 6 6-6 6" />
            </svg>
          </button>

          <a href="/">
            Cloud home
          </a>
        </div>
      </section>

      <aside
        class="terminal-result"
        aria-label="Vix CLI authentication result"
      >
        <header class="terminal-result__header">
          <div aria-hidden="true">
            <span></span>
            <span></span>
            <span></span>
          </div>

          <strong>Vix CLI</strong>

          <small>local terminal</small>
        </header>

        <div class="terminal-result__body">
          <p class="terminal-command">
            <span>$</span>
            vix login
          </p>

          <div class="terminal-output">
            <p>
              <span class="terminal-success">✓</span>
              Browser authorization completed
            </p>

            <p>
              <span class="terminal-success">✓</span>
              Authorization code received
            </p>

            <p>
              <span class="terminal-success">✓</span>
              Cloud session stored
            </p>
          </div>

          <p class="terminal-ready">
            <span>→</span>
            Vix CLI is ready
          </p>
        </div>

        <dl class="connection-details">
          <div>
            <dt>Application</dt>
            <dd>Vix CLI</dd>
          </div>

          <div>
            <dt>Account</dt>
            <dd>Authenticated</dd>
          </div>

          <div>
            <dt>Callback</dt>
            <dd>Completed</dd>
          </div>

          <div>
            <dt>Status</dt>
            <dd class="is-connected">
              Connected
            </dd>
          </div>
        </dl>

        <footer class="terminal-result__footer">
          <span aria-hidden="true"></span>

          <p>
            Future supported Cloud commands can now use this authenticated CLI
            session.
          </p>
        </footer>
      </aside>
    </div>

    <footer class="success-footer">
      <span
        class="success-footer__icon"
        aria-hidden="true"
      >
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

      <div>
        <strong>The authorization handoff is finished.</strong>

        <p>
          This page no longer needs access to the local callback or the
          temporary authorization code.
        </p>
      </div>
    </footer>
  </section>
</main>

<style>
  .success-page {
    --success-bg: var(--bg, #131619);
    --success-panel: var(--bg-panel, #1a1e22);
    --success-elevated: var(--bg-elevated, #20252a);

    --success-border: var(--border, rgba(255, 255, 255, 0.1));
    --success-border-strong: rgba(255, 255, 255, 0.16);

    --success-text: var(--text, #f4f6f7);
    --success-text-soft: var(--text-soft, #b6bdc3);
    --success-text-muted: var(--text-muted, #7f8992);

    --success-brand: var(--brand, #f97316);
    --success-brand-soft: var(--brand-soft, #fb923c);
    --success-brand-faint: var(--brand-faint, rgba(249, 115, 22, 0.1));
    --success-brand-line: var(--brand-line, rgba(249, 115, 22, 0.35));

    --success-green: var(--success, #22c55e);
    --success-green-soft: rgba(34, 197, 94, 0.09);
    --success-green-line: rgba(34, 197, 94, 0.32);

    display: grid;
    min-height: 100vh;
    place-items: center;
    overflow: hidden;
    background: var(--success-bg);
    color: var(--success-text);
    padding: 32px;
  }

  .success-shell {
    width: min(100%, 1040px);
    overflow: hidden;
    border: 1px solid var(--success-border);
    border-radius: 22px;
    background: var(--success-panel);
  }

  /* Header */

  .success-header {
    display: flex;
    min-height: 68px;
    align-items: center;
    justify-content: space-between;
    gap: 20px;
    padding: 0 22px;
    border-bottom: 1px solid var(--success-border);
    background: var(--success-elevated);
  }

  .brand {
    display: inline-flex;
    align-items: center;
    gap: 10px;
    color: var(--success-text);
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

  .connection-status {
    display: inline-grid;
    min-height: 34px;
    grid-template-columns: 8px minmax(0, 1fr);
    align-items: center;
    gap: 9px;
    border: 1px solid var(--success-green-line);
    border-radius: 999px;
    background: var(--success-green-soft);
    padding: 5px 11px;
  }

  .connection-status > span {
    width: 7px;
    height: 7px;
    border-radius: 50%;
    background: var(--success-green);
  }

  .connection-status > div {
    display: flex;
    align-items: center;
    gap: 7px;
  }

  .connection-status strong {
    color: var(--success-green);
    font-size: 0.68rem;
    font-weight: 680;
  }

  .connection-status small {
    color: var(--success-text-muted);
    font-size: 0.65rem;
  }

  .connection-status small::before {
    content: '·';
    margin-right: 7px;
  }

  /* Main content */

  .success-content {
    display: grid;
    grid-template-columns:
      minmax(0, 1.05fr)
      minmax(360px, 0.95fr);
    gap: clamp(40px, 7vw, 82px);
    align-items: center;
    padding: clamp(38px, 6vw, 70px);
  }

  .success-copy {
    display: grid;
    min-width: 0;
    justify-items: start;
  }

  .success-mark {
    display: grid;
    width: 52px;
    height: 52px;
    place-items: center;
    margin-bottom: 22px;
    border: 1px solid var(--success-green-line);
    border-radius: 14px;
    background: var(--success-green-soft);
    color: var(--success-green);
  }

  .success-mark svg {
    width: 25px;
    height: 25px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2.4;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .eyebrow {
    color: var(--success-brand-soft);
    font-family: var(--font-mono, monospace);
    font-size: 0.68rem;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }

  h1 {
    max-width: 660px;
    margin: 14px 0 0;
    color: var(--success-text);
    font-size: clamp(2.8rem, 5.8vw, 5rem);
    font-weight: 690;
    line-height: 0.97;
    letter-spacing: -0.055em;
  }

  h1 span {
    display: block;
    margin-top: 5px;
    color: var(--success-green);
  }

  .message {
    max-width: 650px;
    margin-top: 22px;
    color: var(--success-text-soft);
    font-size: 0.95rem;
    line-height: 1.72;
  }

  .next-action {
    display: grid;
    width: 100%;
    grid-template-columns: 39px minmax(0, 1fr);
    align-items: start;
    gap: 12px;
    margin-top: 29px;
    padding: 15px;
    border: 1px solid var(--success-brand-line);
    border-radius: 12px;
    background: var(--success-brand-faint);
  }

  .next-action > span {
    display: grid;
    width: 38px;
    height: 38px;
    place-items: center;
    color: var(--success-brand-soft);
  }

  .next-action svg {
    width: 19px;
    height: 19px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.9;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .next-action > div {
    display: grid;
    gap: 4px;
  }

  .next-action strong {
    color: var(--success-text);
    font-size: 0.8rem;
    font-weight: 650;
  }

  .next-action p {
    color: var(--success-text-muted);
    font-size: 0.71rem;
    line-height: 1.5;
  }

  .actions {
    display: flex;
    flex-wrap: wrap;
    gap: 9px;
    margin-top: 23px;
  }

  .actions button,
  .actions a {
    display: inline-flex;
    min-height: 41px;
    align-items: center;
    justify-content: center;
    border-radius: 8px;
    padding: 0 14px;
    font: inherit;
    font-size: 0.74rem;
    font-weight: 650;
    text-decoration: none;
  }

  .actions button {
    gap: 7px;
    border: 1px solid var(--success-brand);
    background: var(--success-brand);
    color: #1a0f06;
    cursor: pointer;
  }

  .actions button:hover {
    background: var(--success-brand-soft);
  }

  .actions button svg {
    width: 13px;
    height: 13px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .actions a {
    border: 1px solid var(--success-border);
    color: var(--success-text-soft);
  }

  .actions a:hover {
    border-color: var(--success-brand-line);
    background: var(--success-brand-faint);
    color: var(--success-brand-soft);
  }

  /* Terminal result */

  .terminal-result {
    display: grid;
    min-width: 0;
    overflow: hidden;
    border: 1px solid var(--success-border-strong);
    border-radius: 16px;
    background: #101311;
  }

  .terminal-result__header {
    display: grid;
    min-height: 47px;
    grid-template-columns: 1fr auto 1fr;
    align-items: center;
    gap: 12px;
    padding: 0 13px;
    border-bottom: 1px solid #292e2a;
    background: #181c19;
  }

  .terminal-result__header > div {
    display: flex;
    gap: 5px;
  }

  .terminal-result__header div > span {
    width: 7px;
    height: 7px;
    border-radius: 50%;
    background: #454b46;
  }

  .terminal-result__header strong {
    color: #d7dbd8;
    font-family: var(--font-mono, monospace);
    font-size: 0.67rem;
    font-weight: 600;
  }

  .terminal-result__header small {
    justify-self: end;
    color: #6f7770;
    font-size: 0.62rem;
  }

  .terminal-result__body {
    display: grid;
    min-height: 252px;
    align-content: center;
    gap: 20px;
    padding: 27px 22px;
  }

  .terminal-command,
  .terminal-output p,
  .terminal-ready {
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
    color: var(--success-brand);
  }

  .terminal-output {
    display: grid;
    gap: 9px;
  }

  .terminal-output p {
    color: #aab1ab;
    font-size: 0.7rem;
    line-height: 1.45;
  }

  .terminal-success {
    color: var(--success-green);
  }

  .terminal-ready {
    padding-top: 15px;
    border-top: 1px solid #292e2a;
    color: #b8ecc2;
    font-size: 0.72rem;
    font-weight: 600;
  }

  .terminal-ready span {
    color: var(--success-green);
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

  .connection-details .is-connected {
    color: var(--success-green);
  }

  .terminal-result__footer {
    display: grid;
    grid-template-columns: 8px minmax(0, 1fr);
    align-items: start;
    gap: 9px;
    padding: 14px 16px;
    background: #181c19;
  }

  .terminal-result__footer > span {
    width: 7px;
    height: 7px;
    margin-top: 4px;
    border-radius: 50%;
    background: var(--success-green);
  }

  .terminal-result__footer p {
    color: #777f78;
    font-size: 0.65rem;
    line-height: 1.5;
  }

  /* Footer */

  .success-footer {
    display: grid;
    min-height: 68px;
    grid-template-columns: 33px minmax(0, 1fr);
    align-items: center;
    gap: 11px;
    padding: 12px 22px;
    border-top: 1px solid var(--success-border);
    background: var(--success-elevated);
  }

  .success-footer__icon {
    display: grid;
    width: 32px;
    height: 32px;
    place-items: center;
    color: var(--success-text-muted);
  }

  .success-footer__icon svg {
    width: 17px;
    height: 17px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .success-footer > div {
    display: grid;
    gap: 3px;
  }

  .success-footer strong {
    color: var(--success-text-soft);
    font-size: 0.71rem;
    font-weight: 640;
  }

  .success-footer p {
    color: var(--success-text-muted);
    font-size: 0.66rem;
    line-height: 1.45;
  }

  /* Responsive */

  @media (max-width: 840px) {
    .success-page {
      place-items: start center;
      padding: 20px;
    }

    .success-content {
      grid-template-columns: 1fr;
    }

    .terminal-result {
      width: 100%;
      max-width: 620px;
    }
  }

  @media (max-width: 560px) {
    .success-page {
      padding: 0;
    }

    .success-shell {
      min-height: 100vh;
      border: 0;
      border-radius: 0;
    }

    .success-header {
      min-height: 61px;
      padding-inline: 16px;
    }

    .connection-status small {
      display: none;
    }

    .connection-status small::before {
      content: none;
    }

    .success-content {
      gap: 34px;
      padding: 38px 18px;
    }

    h1 {
      font-size: clamp(2.55rem, 13vw, 3.8rem);
    }

    .message {
      font-size: 0.9rem;
    }

    .actions {
      width: 100%;
      flex-direction: column;
    }

    .actions button,
    .actions a {
      width: 100%;
    }

    .terminal-result__body {
      min-height: 220px;
      padding-inline: 17px;
    }

    .success-footer {
      align-items: start;
      padding-inline: 17px;
    }
  }
</style>
