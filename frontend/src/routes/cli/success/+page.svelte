<script lang="ts">
  import { goto } from '$app/navigation';

  function openDashboard() {
    void goto('/dashboard');
  }
</script>

<svelte:head>
  <title>CLI connected | Softadastra Cloud</title>
  <meta name="description" content="Vix CLI is connected to Softadastra Cloud." />
</svelte:head>

<main class="success-page">
  <section class="panel" aria-labelledby="success-title">
    <div class="topbar" aria-hidden="true">
      <span></span>
      <span></span>
      <span></span>
    </div>

    <div class="content">
      <a class="brand" href="/" aria-label="Softadastra Cloud home">
        <img src="/brand/softadastra-cloud.svg" alt="" aria-hidden="true" />
        <span>Softadastra Cloud</span>
      </a>

      <div class="status" aria-hidden="true">
        <span>✓</span>
      </div>

      <p class="eyebrow">Vix CLI authentication</p>

      <h1 id="success-title">CLI connected.</h1>

      <p class="message">
        Your terminal is signed in to Softadastra Cloud. You can close this tab
        and continue from the Vix CLI.
      </p>

      <div class="actions">
        <button type="button" on:click={openDashboard}>Open dashboard</button>
        <a href="/">Cloud home</a>
      </div>
    </div>
  </section>
</main>

<style>
  :global(body) {
    margin: 0;
    background: #08111f;
    color: #f8fafc;
  }

  .success-page {
    min-height: 100vh;
    display: grid;
    place-items: center;
    padding: 24px;
    background:
      radial-gradient(circle at 24% 22%, rgba(20, 184, 166, 0.2), transparent 30%),
      radial-gradient(circle at 80% 72%, rgba(56, 189, 248, 0.14), transparent 34%),
      linear-gradient(135deg, #08111f 0%, #111827 54%, #0b2625 100%);
  }

  .panel {
    width: min(100%, 560px);
    overflow: hidden;
    border: 1px solid rgba(148, 163, 184, 0.26);
    border-radius: 10px;
    background: rgba(15, 23, 42, 0.9);
    box-shadow: 0 26px 80px rgba(0, 0, 0, 0.42);
  }

  .topbar {
    height: 44px;
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 0 18px;
    border-bottom: 1px solid rgba(148, 163, 184, 0.16);
    background: rgba(2, 6, 23, 0.36);
  }

  .topbar span {
    width: 10px;
    height: 10px;
    border-radius: 999px;
    background: #475569;
  }

  .topbar span:nth-child(1) { background: #fb7185; }
  .topbar span:nth-child(2) { background: #fbbf24; }
  .topbar span:nth-child(3) { background: #34d399; }

  .content {
    padding: 38px 34px 34px;
  }

  .brand {
    display: inline-flex;
    align-items: center;
    gap: 12px;
    color: #e2e8f0;
    font-weight: 800;
    text-decoration: none;
  }

  .brand img {
    width: 34px;
    height: 34px;
  }

  .status {
    width: 72px;
    height: 72px;
    margin: 34px 0 22px;
    display: grid;
    place-items: center;
    border-radius: 999px;
    position: relative;
  }

  .status::before,
  .status::after {
    content: '';
    position: absolute;
    inset: 0;
    border-radius: inherit;
  }

  .status::before {
    border: 1px solid rgba(148, 163, 184, 0.28);
  }

  .status::after {
    border: 3px solid transparent;
    border-top-color: #14b8a6;
    border-right-color: #38bdf8;
    animation: spin 1.1s linear infinite;
  }

  .status span {
    z-index: 1;
    color: #14b8a6;
    font-size: 30px;
    font-weight: 900;
  }

  .eyebrow {
    margin: 0 0 10px;
    color: #14b8a6;
    font-size: 13px;
    font-weight: 800;
    text-transform: uppercase;
    letter-spacing: 0.08em;
  }

  h1 {
    margin: 0;
    font-size: clamp(2.2rem, 7vw, 3.55rem);
    line-height: 0.98;
    letter-spacing: 0;
  }

  .message {
    margin: 18px 0 0;
    color: #cbd5e1;
    line-height: 1.65;
  }

  .actions {
    display: flex;
    flex-wrap: wrap;
    gap: 14px;
    align-items: center;
    margin-top: 28px;
  }

  button,
  .actions a {
    min-height: 42px;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    border-radius: 8px;
    padding: 0 16px;
    font: inherit;
    font-weight: 800;
  }

  button {
    border: 0;
    background: #14b8a6;
    color: #031018;
    cursor: pointer;
  }

  .actions a {
    border: 1px solid rgba(148, 163, 184, 0.3);
    color: #e2e8f0;
    text-decoration: none;
  }

  @keyframes spin {
    to { transform: rotate(360deg); }
  }

  @media (max-width: 520px) {
    .content {
      padding: 30px 24px;
    }
  }
</style>
