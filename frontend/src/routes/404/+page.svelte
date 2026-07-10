<svelte:head>
  <title>Not found - Softadastra Cloud</title>
</svelte:head>

<section class="not-found-page">
  <div>
    <p>404</p>
    <h1>Page not found</h1>
    <span>The page you are looking for does not exist.</span>
    <a href="/dashboard">Go to dashboard</a>
  </div>
</section>

<style>
  .not-found-page {
    display: grid;
    min-height: 100vh;
    place-items: center;
    background: var(--bg);
    color: var(--text);
    padding: 24px;
  }

  .not-found-page > div {
    display: grid;
    width: min(440px, 100%);
    gap: 12px;
    text-align: center;
  }

  p {
    margin: 0;
    color: var(--brand);
    font-family: var(--font-mono);
    font-size: 12px;
    letter-spacing: 0.14em;
  }

  h1 {
    margin: 0;
    color: var(--text);
    font-size: 28px;
  }

  span {
    color: var(--text-muted);
    font-size: 14px;
  }

  a {
    justify-self: center;
    color: var(--link);
    font-weight: 600;
  }
</style>
