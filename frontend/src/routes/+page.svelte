<script lang="ts">
  import { auth } from '$lib/stores/auth';

  const features = [
    {
      title: 'Workspaces',
      body: 'Create controlled spaces for projects, members and permissions.'
    },
    {
      title: 'Private packages',
      body: 'Publish and organize Vix packages without exposing private work.'
    },
    {
      title: 'Lockfiles',
      body: 'Keep a history of dependency state for Vix projects.'
    },
    {
      title: 'Build reports',
      body: 'Send local build results to the cloud so the team can see what happened.'
    },
    {
      title: 'Permissions',
      body: 'Control owner, admin, member and viewer access with workspace scopes.'
    },
    {
      title: 'Public profiles',
      body: 'Show public packages, pinned work and public contribution activity.'
    },
    {
      title: 'Feedback',
      body: 'Send product feedback directly from the platform while the project is still moving fast.'
    },
    {
      title: 'Founding Supporters',
      body: 'Support the early development and get closer access to the project.'
    }
  ];
</script>

<svelte:head>
  <title>Softadastra Cloud</title>
  <meta
    name="description"
    content="Softadastra Cloud is a C++ cloud workspace for Vix projects, private packages, lockfiles, build reports, permissions and project activity."
  />
</svelte:head>

<main class="home-page">
  <nav class="public-nav" aria-label="Public navigation">
    <a class="brand" href="/">
      <img src="/brand/softadastra-cloud.svg" alt="" />
      <span>Softadastra Cloud</span>
    </a>

    <div class="nav-links">
      <a href="#platform">Platform</a>
      <a href="/support">Support</a>
      <a href="/supporters">Supporters</a>
      {#if $auth.session}
        <a class="nav-cta" href="/dashboard">Open dashboard</a>
      {:else}
        <a href="/login">Sign in</a>
        <a class="nav-cta" href="/register">Create account</a>
      {/if}
    </div>
  </nav>

  <section class="hero">
    <p class="eyebrow">Softadastra Cloud</p>
    <h1>C++ cloud workspace for Vix projects.</h1>
    <p>
      Softadastra Cloud helps developers and teams organize private packages,
      workspaces, lockfiles, build reports, permissions and project activity around Vix.
    </p>
    <div class="hero-actions">
      {#if $auth.session}
        <a class="primary-action" href="/dashboard">Open dashboard</a>
      {:else}
        <a class="primary-action" href="/register">Create account</a>
        <a class="secondary-action" href="/login">Sign in</a>
      {/if}
      <a class="secondary-action" href="/support">Support the project</a>
    </div>
  </section>

  <section id="platform" class="split-section">
    <div>
      <p class="eyebrow">Why it exists</p>
      <h2>Vix stays local. Softadastra Cloud organizes what needs to be shared.</h2>
    </div>
    <div class="section-copy">
      <p>
        A C++ project is not only source code. A team also needs to know which packages
        exist, which versions were published, which lockfiles were used, who has access,
        and what happened during local or CI builds. Softadastra Cloud gives that shared
        project state a place to live without replacing the local Vix workflow.
      </p>
      <p>
        Developers keep building locally with Vix. The cloud records the metadata,
        private package archives, lockfiles, build reports and team activity that make
        a project easier to understand over time.
      </p>
    </div>
  </section>

  <section class="feature-grid" aria-label="Softadastra Cloud features">
    {#each features as feature}
      <article>
        <h2>{feature.title}</h2>
        <p>{feature.body}</p>
      </article>
    {/each}
  </section>

  <section class="relationship-section">
    <div>
      <p class="eyebrow">Vix and Cloud</p>
      <h2>Local-first C++ workflow with cloud visibility.</h2>
      <p>
        Softadastra Cloud is not designed to replace the local C++ workflow. It complements
        Vix by giving projects a place to record packages, versions, lockfiles, reports and
        team activity. The current MVP does not run C++ builds in the cloud; builds remain
        local or in external CI, and their reports can be sent to the dashboard.
      </p>
    </div>
  </section>

  <section class="support-callout">
    <div>
      <p class="eyebrow">Founding Supporters</p>
      <h2>Support the development of Softadastra Cloud</h2>
      <p>
        Softadastra Cloud is still in active development. Founding Supporters help make the
        project sustainable while keeping the core tooling open, serious and useful for C++ developers.
      </p>
    </div>

    <div class="support-plans">
      <article>
        <span>Founding Supporter</span>
        <strong>$10/month</strong>
      </article>
      <article>
        <span>Founding Builder</span>
        <strong>$25/month</strong>
      </article>
      <a href="/support">Become a Founding Supporter</a>
    </div>
  </section>

  <section class="status-section">
    <p class="eyebrow">Product status</p>
    <h2>Early active development, with a working foundation.</h2>
    <p>
      The current version already includes workspaces, private registry foundations,
      lockfiles, build reports, public profiles, feedback, supporters and platform
      administration. More features are still being built, including stronger package
      resolution, production deployment hardening and deeper CLI workflows.
    </p>
  </section>

  <footer class="home-footer">
    <span>Softadastra Cloud</span>
    <nav aria-label="Footer navigation">
      <a href="/support">Support</a>
      <a href="/supporters">Supporters</a>
      <a href="/login">Sign in</a>
      <a href="/dashboard">Dashboard</a>
    </nav>
  </footer>
</main>

<style>
  .home-page {
    min-height: 100vh;
    background:
      radial-gradient(circle at 80% 0%, rgba(68, 120, 255, 0.12), transparent 32rem),
      var(--bg);
    color: var(--text);
    padding: 24px;
  }

  .public-nav,
  .hero,
  .split-section,
  .feature-grid,
  .relationship-section,
  .support-callout,
  .status-section,
  .home-footer {
    width: min(1120px, 100%);
    margin: 0 auto;
  }

  .public-nav {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 16px;
    margin-bottom: 52px;
  }

  .brand,
  .nav-links,
  .home-footer nav {
    display: flex;
    align-items: center;
    gap: 14px;
    flex-wrap: wrap;
  }

  .brand img {
    width: 28px;
    height: 28px;
  }

  .brand span,
  .home-footer span {
    color: var(--text);
    font-weight: 800;
  }

  a {
    color: var(--text-soft);
    font-size: 13px;
    font-weight: 650;
  }

  a:hover {
    color: var(--link-hover);
  }

  .nav-cta,
  .primary-action,
  .secondary-action,
  .support-plans a {
    display: inline-flex;
    min-height: 38px;
    align-items: center;
    justify-content: center;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-sm);
    background: var(--brand-faint);
    color: var(--brand-bright);
    padding: 0 14px;
  }

  .hero {
    padding: 34px 0 54px;
  }

  .eyebrow {
    margin: 0;
    color: var(--brand-bright);
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 800;
    letter-spacing: 0.12em;
    text-transform: uppercase;
  }

  .hero h1 {
    max-width: 780px;
    margin: 12px 0 0;
    color: var(--text);
    font-size: clamp(38px, 7vw, 76px);
    line-height: 0.98;
  }

  .hero p {
    max-width: 760px;
    margin: 18px 0 0;
    color: var(--text-muted);
    font-size: 17px;
    line-height: 1.7;
  }

  .hero-actions {
    display: flex;
    gap: 12px;
    flex-wrap: wrap;
    margin-top: 28px;
  }

  .secondary-action {
    border-color: var(--line-strong);
    background: var(--bg-panel);
    color: var(--text-soft);
  }

  .split-section,
  .support-callout {
    display: grid;
    grid-template-columns: minmax(0, 0.85fr) minmax(0, 1.15fr);
    gap: 42px;
    border-top: 1px solid var(--line);
    padding: 34px 0;
  }

  h2 {
    margin: 10px 0 0;
    color: var(--text);
    font-size: clamp(24px, 4vw, 38px);
    line-height: 1.08;
  }

  .section-copy,
  .relationship-section p,
  .support-callout p,
  .status-section p {
    color: var(--text-muted);
    font-size: 14px;
    line-height: 1.75;
  }

  .feature-grid {
    display: grid;
    grid-template-columns: repeat(4, minmax(0, 1fr));
    gap: 14px;
    padding: 8px 0 38px;
  }

  .feature-grid article,
  .relationship-section,
  .support-plans article,
  .status-section {
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    padding: 18px;
  }

  .feature-grid h2 {
    margin: 0;
    font-size: 15px;
  }

  .feature-grid p {
    margin: 9px 0 0;
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.65;
  }

  .relationship-section {
    margin-bottom: 34px;
    padding: 26px;
  }

  .relationship-section div {
    max-width: 760px;
  }

  .support-plans {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 12px;
    align-content: start;
  }

  .support-plans article {
    display: grid;
    gap: 7px;
  }

  .support-plans span {
    color: var(--text-muted);
    font-size: 13px;
  }

  .support-plans strong {
    color: var(--text);
    font-size: 22px;
  }

  .support-plans a {
    grid-column: 1 / -1;
    width: fit-content;
  }

  .status-section {
    margin-bottom: 34px;
  }

  .home-footer {
    display: flex;
    justify-content: space-between;
    gap: 16px;
    border-top: 1px solid var(--line);
    padding: 22px 0 10px;
  }

  @media (max-width: 940px) {
    .feature-grid {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }

    .split-section,
    .support-callout {
      grid-template-columns: 1fr;
    }
  }

  @media (max-width: 640px) {
    .home-page {
      padding: 18px;
    }

    .public-nav,
    .home-footer {
      align-items: flex-start;
      flex-direction: column;
    }

    .feature-grid,
    .support-plans {
      grid-template-columns: 1fr;
    }
  }
</style>
