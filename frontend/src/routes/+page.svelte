<script lang="ts">
  import { onMount } from 'svelte';
  import { auth } from '$lib/stores/auth';
  import PublicHeader from '$lib/components/PublicHeader.svelte';

  let checkingAuth = true;
  let pageReady = false;

  const capabilities = [
    {
      id: 'packages',
      number: '01',
      label: 'Private packages',
      title: 'Publish without exposing private work.',
      body:
        'Keep internal Vix packages, versions, release history and package access inside the workspace.'
    },
    {
      id: 'history',
      number: '02',
      label: 'Project history',
      title: 'Know exactly what the project used.',
      body:
        'Store lockfiles and local build reports so dependency state and build results remain understandable over time.'
    },
    {
      id: 'team',
      number: '03',
      label: 'Team access',
      title: 'Give every member the right level of access.',
      body:
        'Organize projects, members, tokens and permissions without sharing credentials or losing ownership context.'
    }
  ];

  const workflow = [
    {
      number: '01',
      owner: 'Vix.cpp',
      title: 'Build locally',
      body:
        'Create, install, build and diagnose the C++ application on the developer machine or in existing CI.'
    },
    {
      number: '02',
      owner: 'Softadastra Cloud',
      title: 'Share project context',
      body:
        'Publish private packages and send lockfiles, build reports and project metadata to the workspace.'
    },
    {
      number: '03',
      owner: 'Your team',
      title: 'Move with shared visibility',
      body:
        'Members can inspect versions, dependency state, build results and recent activity from one place.'
    }
  ];

  onMount(async () => {
    requestAnimationFrame(() => {
      pageReady = true;
    });

    try {
      await auth.ensureAuthLoaded();
    } finally {
      checkingAuth = false;
    }
  });
</script>

<svelte:head>
  <title>Softadastra Cloud | Shared workspaces for Vix projects</title>

  <meta
    name="description"
    content="Build C++ applications locally with Vix and organize private packages, lockfiles, build reports, permissions, and project activity in Softadastra Cloud."
  />

  <meta property="og:title" content="Softadastra Cloud | Shared workspaces for Vix projects" />
  <meta property="og:description" content="Build C++ applications locally with Vix and organize private packages, lockfiles, build reports, permissions, and project activity in Softadastra Cloud." />
  <meta property="og:url" content="https://business.softadastra.com/" />
  <meta property="og:image" content="https://business.softadastra.com/og-logo.png" />

  <meta name="twitter:title" content="Softadastra Cloud | Shared workspaces for Vix projects" />
  <meta name="twitter:description" content="Build C++ applications locally with Vix and organize private packages, lockfiles, build reports, permissions, and project activity in Softadastra Cloud." />
  <meta name="twitter:image" content="https://business.softadastra.com/og-logo.png" />
</svelte:head>

<main class:is-ready={pageReady} class="home-page">
  <PublicHeader />

  <!-- =========================================================
       HERO
       ========================================================= -->
  <section class="hero">
    <div class="hero__copy">
      <p class="hero__eyebrow">
        <span aria-hidden="true"></span>
        Softadastra Cloud
      </p>

      <h1>
        The shared workspace
        <span>for your Vix projects.</span>
      </h1>

      <p class="hero__intro">
        Build C++ applications locally with Vix, then keep private packages,
        lockfiles, build reports, permissions and project activity connected
        in one place.
      </p>

      <div class="hero__actions" aria-label="Get started with Softadastra Cloud">
        {#if checkingAuth}
          <span class="cta-placeholder" aria-hidden="true"></span>
        {:else if $auth.session}
          <a class="button button--primary" href="/dashboard">
            Open your dashboard

            <svg
              viewBox="0 0 24 24"
              width="14"
              height="14"
              fill="none"
              stroke="currentColor"
              stroke-width="2.2"
              stroke-linecap="round"
              stroke-linejoin="round"
              aria-hidden="true"
            >
              <path d="M5 12h14"></path>
              <path d="m13 6 6 6-6 6"></path>
            </svg>
          </a>
        {:else}
          <a class="button button--primary" href="/register">
            Create your workspace

            <svg
              viewBox="0 0 24 24"
              width="14"
              height="14"
              fill="none"
              stroke="currentColor"
              stroke-width="2.2"
              stroke-linecap="round"
              stroke-linejoin="round"
              aria-hidden="true"
            >
              <path d="M5 12h14"></path>
              <path d="m13 6 6 6-6 6"></path>
            </svg>
          </a>

          <a class="button button--secondary" href="/login">
            Sign in
          </a>
        {/if}
      </div>

      <div class="hero__facts">
        <span>
          <i aria-hidden="true">
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="2.2"
            >
              <path d="m5 12 4 4L19 6"></path>
            </svg>
          </i>
          Builds remain local
        </span>

        <span>
          <i aria-hidden="true">
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="2.2"
            >
              <path d="m5 12 4 4L19 6"></path>
            </svg>
          </i>
          Private project spaces
        </span>

        <span>
          <i aria-hidden="true">
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="2.2"
            >
              <path d="m5 12 4 4L19 6"></path>
            </svg>
          </i>
          Designed around Vix
        </span>
      </div>
    </div>

    <!-- Product preview -->
    <div
      class="cloud-preview"
      aria-label="Softadastra Cloud project workspace preview"
    >
      <div class="cloud-preview__glow" aria-hidden="true"></div>
      <div class="cloud-preview__pattern" aria-hidden="true"></div>

      <div class="cloud-window">
        <header class="cloud-window__header">
          <div class="cloud-window__brand">
            <img src="/logo.svg" alt="" aria-hidden="true" />

            <div>
              <strong>Softadastra Cloud</strong>
              <small>Workspace</small>
            </div>
          </div>

          <div class="cloud-window__header-actions">
            <span class="cloud-window__branch">
              <svg
                viewBox="0 0 24 24"
                width="11"
                height="11"
                fill="none"
                stroke="currentColor"
                stroke-width="1.8"
                aria-hidden="true"
              >
                <circle cx="6" cy="5" r="2"></circle>
                <circle cx="6" cy="19" r="2"></circle>
                <circle cx="18" cy="12" r="2"></circle>
                <path d="M6 7v10"></path>
                <path d="M8 7c3 0 3 5 8 5"></path>
              </svg>

              main
            </span>

            <span class="cloud-window__avatar">GK</span>
          </div>
        </header>

        <div class="cloud-window__body">
          <aside class="cloud-sidebar">
            <div class="cloud-sidebar__workspace">
              <span class="cloud-sidebar__workspace-mark">S</span>

              <div>
                <strong>Softadastra</strong>
                <small>Engineering</small>
              </div>
            </div>

            <nav aria-label="Workspace preview navigation">
              <span class="cloud-sidebar__item cloud-sidebar__item--active">
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="1.8"
                  aria-hidden="true"
                >
                  <rect x="3" y="3" width="7" height="7" rx="1"></rect>
                  <rect x="14" y="3" width="7" height="7" rx="1"></rect>
                  <rect x="3" y="14" width="7" height="7" rx="1"></rect>
                  <rect x="14" y="14" width="7" height="7" rx="1"></rect>
                </svg>

                Overview
              </span>

              <span class="cloud-sidebar__item">
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="1.8"
                  aria-hidden="true"
                >
                  <path d="M21 16V8l-9-5-9 5v8l9 5 9-5Z"></path>
                  <path d="m3.3 8.3 8.7 4.8 8.7-4.8"></path>
                  <path d="M12 13v8"></path>
                </svg>

                Packages
              </span>

              <span class="cloud-sidebar__item">
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="1.8"
                  aria-hidden="true"
                >
                  <rect x="4" y="10" width="16" height="10" rx="2"></rect>
                  <path d="M8 10V7a4 4 0 0 1 8 0v3"></path>
                </svg>

                Lockfiles
              </span>

              <span class="cloud-sidebar__item">
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="1.8"
                  aria-hidden="true"
                >
                  <path d="M4 19V5"></path>
                  <path d="M4 19h16"></path>
                  <path d="m7 15 3-4 3 2 4-6"></path>
                </svg>

                Build reports
              </span>

              <span class="cloud-sidebar__item">
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="1.8"
                  aria-hidden="true"
                >
                  <circle cx="8" cy="8" r="3"></circle>
                  <circle cx="17" cy="9" r="2.5"></circle>
                  <path d="M2.5 20a5.5 5.5 0 0 1 11 0"></path>
                  <path d="M13 20a4 4 0 0 1 8 0"></path>
                </svg>

                Members
              </span>
            </nav>

            <div class="cloud-sidebar__status">
              <i aria-hidden="true"></i>

              <div>
                <strong>Workspace active</strong>
                <small>All services available</small>
              </div>
            </div>
          </aside>

          <section class="cloud-content">
            <header class="cloud-content__header">
              <div>
                <p>Project overview</p>
                <h2>native-service</h2>
              </div>

              <span class="cloud-content__visibility">
                <svg
                  viewBox="0 0 24 24"
                  width="11"
                  height="11"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="1.8"
                  aria-hidden="true"
                >
                  <rect x="4" y="10" width="16" height="10" rx="2"></rect>
                  <path d="M8 10V7a4 4 0 0 1 8 0v3"></path>
                </svg>

                Private
              </span>
            </header>

            <div class="cloud-metrics">
              <article>
                <span>Private packages</span>
                <strong>12</strong>
                <small>3 updated this week</small>
              </article>

              <article>
                <span>Lockfiles</span>
                <strong>47</strong>
                <small>Latest uploaded 4m ago</small>
              </article>

              <article>
                <span>Build reports</span>
                <strong>128</strong>
                <small>96.8% successful</small>
              </article>
            </div>

            <div class="cloud-grid">
              <section class="build-panel">
                <header>
                  <div>
                    <span>Latest local build</span>
                    <strong>release · linux-x86_64</strong>
                  </div>

                  <span class="build-panel__status">
                    <i aria-hidden="true"></i>
                    Passed
                  </span>
                </header>

                <div class="build-panel__summary">
                  <div class="build-panel__score">
                    <svg viewBox="0 0 84 84" aria-hidden="true">
                      <circle cx="42" cy="42" r="33"></circle>
                      <circle
                        class="build-panel__score-progress"
                        cx="42"
                        cy="42"
                        r="33"
                      ></circle>
                    </svg>

                    <strong>100%</strong>
                  </div>

                  <dl>
                    <div>
                      <dt>Compiler</dt>
                      <dd>GCC 14.3</dd>
                    </div>

                    <div>
                      <dt>Duration</dt>
                      <dd>12.4s</dd>
                    </div>

                    <div>
                      <dt>Warnings</dt>
                      <dd>0</dd>
                    </div>
                  </dl>
                </div>

                <div class="build-panel__bar">
                  <span></span>
                </div>
              </section>

              <section class="activity-panel">
                <header>
                  <span>Recent activity</span>
                  <strong>Live</strong>
                </header>

                <div class="activity-panel__list">
                  <article style="--activity-delay: 0s">
                    <span class="activity-panel__icon activity-panel__icon--package">
                      P
                    </span>

                    <div>
                      <strong>Package published</strong>
                      <small>core-utils@1.4.2</small>
                    </div>

                    <time>2m</time>
                  </article>

                  <article style="--activity-delay: 1.2s">
                    <span class="activity-panel__icon activity-panel__icon--lock">
                      L
                    </span>

                    <div>
                      <strong>Lockfile uploaded</strong>
                      <small>47 dependencies</small>
                    </div>

                    <time>4m</time>
                  </article>

                  <article style="--activity-delay: 2.4s">
                    <span class="activity-panel__icon activity-panel__icon--build">
                      B
                    </span>

                    <div>
                      <strong>Build report received</strong>
                      <small>release · passed</small>
                    </div>

                    <time>6m</time>
                  </article>
                </div>
              </section>
            </div>
          </section>
        </div>

        <footer class="cloud-window__footer">
          <div>
            <span class="cloud-window__sync-dot" aria-hidden="true"></span>
            Project context synchronized
          </div>

          <div class="cloud-window__sync-line" aria-hidden="true">
            <span></span>
          </div>

          <strong>Local build · Cloud visibility</strong>
        </footer>
      </div>

      <div class="cloud-preview__float cloud-preview__float--package">
        <span>P</span>

        <div>
          <small>Private package</small>
          <strong>core-utils@1.4.2</strong>
        </div>

        <i>Published</i>
      </div>

      <div class="cloud-preview__float cloud-preview__float--team">
        <div class="cloud-preview__avatars" aria-hidden="true">
          <span>GK</span>
          <span>AM</span>
          <span>JD</span>
        </div>

        <div>
          <small>Workspace</small>
          <strong>3 members connected</strong>
        </div>
      </div>
    </div>
  </section>

  <!-- =========================================================
       PRIMARY VALUE
       ========================================================= -->
  <section id="platform" class="value-section">
    <header class="section-header">
      <p class="section-header__eyebrow">
        Built for the work around the build
      </p>

      <h2>
        Everything your project needs
        <span>after the code compiles.</span>
      </h2>

      <p>
        Softadastra Cloud gives the shared parts of a Vix project a durable
        place to live, while source code and compilation remain in the
        developer’s existing workflow.
      </p>
    </header>

    <div class="capabilities">
      {#each capabilities as capability}
        <article class="capability">
          <span class="capability__number">
            {capability.number}
          </span>

          <span
            class:capability__icon--packages={capability.id === 'packages'}
            class:capability__icon--history={capability.id === 'history'}
            class:capability__icon--team={capability.id === 'team'}
            class="capability__icon"
            aria-hidden="true"
          >
            {#if capability.id === 'packages'}
              <svg
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="1.8"
                stroke-linecap="round"
                stroke-linejoin="round"
              >
                <path d="M21 16V8l-9-5-9 5v8l9 5 9-5Z"></path>
                <path d="m3.3 8.3 8.7 4.8 8.7-4.8"></path>
                <path d="M12 13v8"></path>
              </svg>
            {:else if capability.id === 'history'}
              <svg
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="1.8"
                stroke-linecap="round"
                stroke-linejoin="round"
              >
                <circle cx="12" cy="12" r="9"></circle>
                <path d="M12 7v5l3 2"></path>
              </svg>
            {:else}
              <svg
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="1.8"
                stroke-linecap="round"
                stroke-linejoin="round"
              >
                <circle cx="8" cy="8" r="3"></circle>
                <circle cx="17" cy="9" r="2.5"></circle>
                <path d="M2.5 20a5.5 5.5 0 0 1 11 0"></path>
                <path d="M13 20a4 4 0 0 1 8 0"></path>
              </svg>
            {/if}
          </span>

          <div class="capability__content">
            <span>{capability.label}</span>
            <h3>{capability.title}</h3>
            <p>{capability.body}</p>
          </div>
        </article>
      {/each}
    </div>
  </section>

  <!-- =========================================================
       WORKFLOW
       ========================================================= -->
  <section class="workflow-section">
    <div class="workflow-section__heading">
      <p class="section-header__eyebrow">
        One connected path
      </p>

      <h2>
        Local development.
        <span>Shared project visibility.</span>
      </h2>

      <p>
        Cloud complements the Vix workflow rather than replacing it.
      </p>
    </div>

    <div class="workflow">
      <div class="workflow__track" aria-hidden="true">
        <span></span>
      </div>

      {#each workflow as step, index}
        <article
          class="workflow-step"
          style={`--step-delay: ${index * 1.8}s`}
        >
          <div class="workflow-step__top">
            <span class="workflow-step__number">{step.number}</span>
            <span class="workflow-step__owner">{step.owner}</span>
          </div>

          <span class="workflow-step__node" aria-hidden="true">
            {#if index === 0}
              <svg
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="1.8"
                stroke-linecap="round"
                stroke-linejoin="round"
              >
                <path d="m8 6-5 6 5 6"></path>
                <path d="m16 6 5 6-5 6"></path>
                <path d="M10 19 14 5"></path>
              </svg>
            {:else if index === 1}
              <svg
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="1.8"
                stroke-linecap="round"
                stroke-linejoin="round"
              >
                <path d="M21 16V8l-9-5-9 5v8l9 5 9-5Z"></path>
                <path d="m3.3 8.3 8.7 4.8 8.7-4.8"></path>
                <path d="M12 13v8"></path>
              </svg>
            {:else}
              <svg
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="1.8"
                stroke-linecap="round"
                stroke-linejoin="round"
              >
                <circle cx="8" cy="8" r="3"></circle>
                <circle cx="17" cy="9" r="2.5"></circle>
                <path d="M2.5 20a5.5 5.5 0 0 1 11 0"></path>
                <path d="M13 20a4 4 0 0 1 8 0"></path>
              </svg>
            {/if}
          </span>

          <h3>{step.title}</h3>
          <p>{step.body}</p>
        </article>
      {/each}
    </div>

    <div class="workflow-section__boundary">
      <span aria-hidden="true">
        <svg
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="1.9"
          stroke-linecap="round"
          stroke-linejoin="round"
        >
          <rect x="3" y="4" width="18" height="13" rx="2"></rect>
          <path d="M8 21h8M12 17v4"></path>
        </svg>
      </span>

      <p>
        Your source code and compilation remain local. Cloud receives the
        project information your team chooses to share.
      </p>
    </div>
  </section>

  <!-- =========================================================
       FINAL CTA
       ========================================================= -->
  <section class="final-cta">
    <div class="final-cta__pattern" aria-hidden="true"></div>
    <div class="final-cta__glow" aria-hidden="true"></div>

    <div class="final-cta__content">
      <p class="final-cta__eyebrow">
        Your project already has code
      </p>

      <h2>
        Now give it a shared workspace.
      </h2>

      <p>
        Create a Softadastra Cloud account and organize your first Vix project,
        private package or workspace.
      </p>

      <div class="final-cta__actions">
        {#if checkingAuth}
          <span class="cta-placeholder" aria-hidden="true"></span>
        {:else if $auth.session}
          <a class="button button--primary" href="/dashboard">
            Open your dashboard

            <svg
              viewBox="0 0 24 24"
              width="14"
              height="14"
              fill="none"
              stroke="currentColor"
              stroke-width="2.2"
              stroke-linecap="round"
              stroke-linejoin="round"
              aria-hidden="true"
            >
              <path d="M5 12h14"></path>
              <path d="m13 6 6 6-6 6"></path>
            </svg>
          </a>
        {:else}
          <a class="button button--primary" href="/register">
            Create your workspace

            <svg
              viewBox="0 0 24 24"
              width="14"
              height="14"
              fill="none"
              stroke="currentColor"
              stroke-width="2.2"
              stroke-linecap="round"
              stroke-linejoin="round"
              aria-hidden="true"
            >
              <path d="M5 12h14"></path>
              <path d="m13 6 6 6-6 6"></path>
            </svg>
          </a>

          <a class="button button--secondary" href="/login">
            Sign in
          </a>
        {/if}
      </div>
    </div>

    <div class="final-cta__project" aria-hidden="true">
      <header>
        <span class="final-cta__project-mark">S</span>

        <div>
          <strong>native-service</strong>
          <small>Workspace ready</small>
        </div>

        <span class="final-cta__project-state">
          <i></i>
          Active
        </span>
      </header>

      <div class="final-cta__project-items">
        <span>
          <i>P</i>
          Private package registry
          <strong>Ready</strong>
        </span>

        <span>
          <i>L</i>
          Lockfile history
          <strong>Ready</strong>
        </span>

        <span>
          <i>B</i>
          Local build reports
          <strong>Ready</strong>
        </span>
      </div>
    </div>
  </section>

  <footer class="home-footer">
    <div>
      <img src="/logo.svg" alt="" aria-hidden="true" />
      <span>Softadastra Cloud</span>
    </div>

    <nav aria-label="Footer navigation">
      <a href="/register">Create account</a>
      <a href="/login">Sign in</a>
      <a href="/dashboard">Dashboard</a>
    </nav>
  </footer>
</main>

<style>
  /* =========================================================
     PAGE
     ========================================================= */

  .home-page {
    position: relative;
    min-height: 100vh;
    overflow: hidden;
    background:
      radial-gradient(
        circle at 82% 2%,
        rgba(249, 115, 22, 0.1),
        transparent 32rem
      ),
      var(--bg);
    color: var(--text);
  }

  .hero,
  .value-section,
  .workflow-section,
  .final-cta,
  .home-footer {
    width: min(100% - 48px, var(--container));
    margin-inline: auto;
  }

  /* =========================================================
     SHARED
     ========================================================= */

  .button {
    display: inline-flex;
    min-height: 42px;
    align-items: center;
    justify-content: center;
    gap: 8px;
    padding: 0 16px;
    border: 1px solid transparent;
    border-radius: var(--radius-sm);
    font-size: 13px;
    font-weight: 650;
    text-decoration: none;
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease),
      color var(--speed) var(--ease),
      box-shadow var(--speed) var(--ease),
      transform var(--speed) var(--ease);
  }

  .button:hover {
    transform: translateY(-1px);
  }

  .button--primary {
    border-color: var(--brand);
    background: var(--brand);
    color: var(--brand-ink);
    box-shadow: var(--shadow-brand);
  }

  .button--primary:hover {
    border-color: var(--brand-soft);
    background: var(--brand-soft);
    color: var(--brand-ink);
  }

  .button--secondary {
    border-color: var(--line-strong);
    background: rgba(255, 255, 255, 0.025);
    color: var(--text-soft);
  }

  .button--secondary:hover {
    border-color: rgba(255, 255, 255, 0.17);
    background: var(--bg-elevated);
    color: var(--text);
  }

  .cta-placeholder {
    display: inline-block;
    width: 178px;
    height: 42px;
  }

  .section-header {
    max-width: 780px;
    margin: 0 auto;
    text-align: center;
  }

  .section-header__eyebrow,
  .final-cta__eyebrow {
    margin: 0;
    color: var(--brand-soft);
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 650;
    letter-spacing: 0.11em;
    text-transform: uppercase;
  }

  .section-header h2,
  .workflow-section__heading h2 {
    margin: 13px 0 0;
    color: var(--text);
    font-size: clamp(34px, 4.8vw, 56px);
    font-weight: 650;
    line-height: 1.02;
    letter-spacing: -0.05em;
  }

  .section-header h2 span,
  .workflow-section__heading h2 span {
    display: block;
    color: var(--text-muted);
  }

  .section-header > p:last-child,
  .workflow-section__heading > p:last-child {
    max-width: 660px;
    margin: 16px auto 0;
    color: var(--text-muted);
    font-size: 14px;
    line-height: 1.7;
  }

  /* =========================================================
     HERO
     ========================================================= */

  .hero {
    display: grid;
    grid-template-columns: minmax(0, 0.82fr) minmax(530px, 1.18fr);
    gap: clamp(48px, 7vw, 100px);
    align-items: center;
    min-height: calc(100vh - 72px);
    padding: 82px 0 96px;
  }

  .hero__copy {
    position: relative;
    z-index: 4;
    opacity: 0;
    transform: translateY(18px);
    transition:
      opacity 700ms var(--ease),
      transform 700ms var(--ease);
  }

  .is-ready .hero__copy {
    opacity: 1;
    transform: none;
  }

  .hero__eyebrow {
    display: inline-flex;
    align-items: center;
    gap: 9px;
    margin: 0;
    color: var(--brand-bright);
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 650;
    letter-spacing: 0.11em;
    text-transform: uppercase;
  }

  .hero__eyebrow > span {
    width: 7px;
    height: 7px;
    border-radius: 50%;
    background: var(--brand);
    box-shadow: 0 0 0 4px var(--brand-faint);
    animation: hero-pulse 2.2s ease-in-out infinite;
  }

  @keyframes hero-pulse {
    50% {
      box-shadow: 0 0 0 9px rgba(249, 115, 22, 0);
    }
  }

  .hero h1 {
    max-width: 680px;
    margin: 17px 0 0;
    color: var(--text);
    font-size: clamp(48px, 6.3vw, 78px);
    font-weight: 650;
    line-height: 0.95;
    letter-spacing: -0.062em;
  }

  .hero h1 span {
    display: block;
    color: var(--brand-soft);
  }

  .hero__intro {
    max-width: 610px;
    margin: 23px 0 0;
    color: var(--text-soft);
    font-size: 16px;
    line-height: 1.7;
  }

  .hero__actions {
    display: flex;
    flex-wrap: wrap;
    gap: 10px;
    margin-top: 30px;
  }

  .hero__facts {
    display: flex;
    flex-wrap: wrap;
    gap: 18px;
    margin-top: 31px;
    padding-top: 23px;
    border-top: 1px solid var(--line);
  }

  .hero__facts > span {
    display: inline-flex;
    align-items: center;
    gap: 7px;
    color: var(--text-muted);
    font-size: 9px;
    font-weight: 550;
  }

  .hero__facts i {
    display: grid;
    width: 17px;
    height: 17px;
    place-items: center;
    border-radius: 50%;
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .hero__facts svg {
    width: 9px;
    height: 9px;
  }

  /* =========================================================
     PRODUCT PREVIEW
     ========================================================= */

  .cloud-preview {
    position: relative;
    min-height: 600px;
    opacity: 0;
    transform: translateY(20px) scale(0.985);
    transition:
      opacity 850ms var(--ease) 120ms,
      transform 850ms var(--ease) 120ms;
  }

  .is-ready .cloud-preview {
    opacity: 1;
    transform: none;
  }

  .cloud-preview__glow {
    position: absolute;
    top: 8%;
    left: 50%;
    width: 84%;
    height: 72%;
    border-radius: 50%;
    background: rgba(249, 115, 22, 0.11);
    filter: blur(75px);
    transform: translateX(-50%);
  }

  .cloud-preview__pattern {
    position: absolute;
    top: 0;
    right: 3%;
    width: 76%;
    height: 47%;
    background-image: radial-gradient(
      rgba(249, 115, 22, 0.22) 0.8px,
      transparent 0.8px
    );
    background-size: 10px 10px;
    opacity: 0.45;
    mask-image: radial-gradient(circle at 50% 30%, #000, transparent 72%);
  }

  .cloud-window {
    position: absolute;
    z-index: 2;
    top: 4%;
    right: 0;
    width: 95%;
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-lg);
    background: var(--bg-panel);
    box-shadow: var(--shadow-deep);
  }

  .cloud-window__header {
    display: flex;
    min-height: 53px;
    align-items: center;
    justify-content: space-between;
    gap: 16px;
    padding: 0 14px;
    border-bottom: 1px solid var(--line);
    background: var(--bg-soft);
  }

  .cloud-window__brand,
  .cloud-window__header-actions,
  .cloud-sidebar__workspace {
    display: flex;
    align-items: center;
    gap: 9px;
  }

  .cloud-window__brand img {
    width: 23px;
    height: 23px;
  }

  .cloud-window__brand > div,
  .cloud-sidebar__workspace > div {
    display: grid;
    line-height: 1.25;
  }

  .cloud-window__brand strong,
  .cloud-sidebar__workspace strong {
    color: var(--text);
    font-size: 8px;
  }

  .cloud-window__brand small,
  .cloud-sidebar__workspace small {
    color: var(--text-muted);
    font-size: 6px;
  }

  .cloud-window__branch {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    padding: 4px 7px;
    border: 1px solid var(--line);
    border-radius: 5px;
    background: var(--bg-ink-soft);
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 6px;
  }

  .cloud-window__avatar {
    display: grid;
    width: 27px;
    height: 27px;
    place-items: center;
    border-radius: 50%;
    background: var(--brand);
    color: var(--brand-ink);
    font-size: 6px;
    font-weight: 750;
  }

  .cloud-window__body {
    display: grid;
    grid-template-columns: 142px minmax(0, 1fr);
    min-height: 445px;
  }

  .cloud-sidebar {
    display: flex;
    min-width: 0;
    flex-direction: column;
    padding: 13px 9px 10px;
    border-right: 1px solid var(--line);
    background: var(--bg-ink-soft);
  }

  .cloud-sidebar__workspace {
    padding: 0 6px 12px;
    border-bottom: 1px solid var(--line-soft);
  }

  .cloud-sidebar__workspace-mark {
    display: grid;
    width: 29px;
    height: 29px;
    place-items: center;
    border-radius: 7px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 9px;
    font-weight: 800;
  }

  .cloud-sidebar nav {
    display: grid;
    gap: 3px;
    padding-top: 11px;
  }

  .cloud-sidebar__item {
    display: flex;
    min-height: 31px;
    align-items: center;
    gap: 7px;
    padding: 0 8px;
    border-radius: 5px;
    color: var(--text-muted);
    font-size: 7px;
  }

  .cloud-sidebar__item svg {
    width: 13px;
    height: 13px;
  }

  .cloud-sidebar__item--active {
    background: var(--brand-faint);
    color: var(--brand-bright);
  }

  .cloud-sidebar__status {
    display: flex;
    align-items: center;
    gap: 7px;
    margin-top: auto;
    padding: 9px 7px 2px;
    border-top: 1px solid var(--line-soft);
  }

  .cloud-sidebar__status > i {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: var(--green);
    box-shadow: 0 0 0 3px var(--green-faint);
  }

  .cloud-sidebar__status > div {
    display: grid;
    line-height: 1.25;
  }

  .cloud-sidebar__status strong {
    color: var(--text-soft);
    font-size: 6px;
  }

  .cloud-sidebar__status small {
    color: var(--text-faint);
    font-size: 5px;
  }

  .cloud-content {
    min-width: 0;
    padding: 17px;
    background:
      radial-gradient(
        circle at 100% 0%,
        rgba(249, 115, 22, 0.04),
        transparent 40%
      ),
      var(--bg-panel);
  }

  .cloud-content__header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 14px;
  }

  .cloud-content__header p {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 5.5px;
    letter-spacing: 0.06em;
    text-transform: uppercase;
  }

  .cloud-content__header h2 {
    margin: 3px 0 0;
    color: var(--text);
    font-size: 15px;
    font-weight: 650;
    letter-spacing: -0.03em;
  }

  .cloud-content__visibility {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    padding: 4px 7px;
    border: 1px solid var(--line);
    border-radius: 5px;
    background: var(--bg-ink-soft);
    color: var(--text-muted);
    font-size: 5.5px;
  }

  .cloud-metrics {
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 1fr));
    gap: 8px;
    margin-top: 15px;
  }

  .cloud-metrics article {
    display: grid;
    gap: 3px;
    padding: 10px;
    border: 1px solid var(--line);
    border-radius: 7px;
    background: var(--bg-ink-soft);
  }

  .cloud-metrics span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 5px;
    text-transform: uppercase;
  }

  .cloud-metrics strong {
    color: var(--text);
    font-size: 14px;
    font-weight: 650;
  }

  .cloud-metrics small {
    color: var(--text-faint);
    font-size: 5px;
  }

  .cloud-grid {
    display: grid;
    grid-template-columns: minmax(0, 1.08fr) minmax(170px, 0.92fr);
    gap: 9px;
    margin-top: 9px;
  }

  .build-panel,
  .activity-panel {
    overflow: hidden;
    border: 1px solid var(--line);
    border-radius: 8px;
    background: var(--bg-ink-soft);
  }

  .build-panel > header,
  .activity-panel > header {
    display: flex;
    min-height: 42px;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
    padding: 0 10px;
    border-bottom: 1px solid var(--line-soft);
  }

  .build-panel > header > div {
    display: grid;
    line-height: 1.25;
  }

  .build-panel > header span,
  .activity-panel > header span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 5px;
    text-transform: uppercase;
  }

  .build-panel > header strong {
    color: var(--text-soft);
    font-size: 6px;
  }

  .build-panel__status {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    color: var(--green-soft) !important;
    font-family: inherit !important;
    font-size: 5.5px !important;
    font-weight: 700;
  }

  .build-panel__status i {
    width: 4px;
    height: 4px;
    border-radius: 50%;
    background: currentColor;
  }

  .build-panel__summary {
    display: grid;
    grid-template-columns: auto minmax(0, 1fr);
    align-items: center;
    gap: 12px;
    padding: 13px;
  }

  .build-panel__score {
    position: relative;
    display: grid;
    width: 70px;
    height: 70px;
    place-items: center;
  }

  .build-panel__score svg {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    transform: rotate(-90deg);
  }

  .build-panel__score circle {
    fill: none;
    stroke: var(--bg-elevated);
    stroke-width: 7;
  }

  .build-panel__score .build-panel__score-progress {
    stroke: var(--green);
    stroke-dasharray: 208;
    stroke-dashoffset: 208;
    stroke-linecap: round;
    animation: build-score 3s var(--ease) infinite alternate;
  }

  @keyframes build-score {
    to {
      stroke-dashoffset: 0;
    }
  }

  .build-panel__score > strong {
    color: var(--text);
    font-size: 10px;
  }

  .build-panel dl {
    display: grid;
    gap: 0;
    margin: 0;
  }

  .build-panel dl > div {
    display: flex;
    justify-content: space-between;
    gap: 8px;
    padding: 6px 0;
    border-bottom: 1px solid var(--line-soft);
  }

  .build-panel dt,
  .build-panel dd {
    margin: 0;
    font-family: var(--font-mono);
    font-size: 5.5px;
  }

  .build-panel dt {
    color: var(--text-muted);
  }

  .build-panel dd {
    color: var(--text-soft);
  }

  .build-panel__bar {
    height: 3px;
    margin: 0 13px 13px;
    overflow: hidden;
    border-radius: 3px;
    background: var(--bg-elevated);
  }

  .build-panel__bar span {
    display: block;
    width: 100%;
    height: 100%;
    border-radius: inherit;
    background: linear-gradient(90deg, var(--brand), var(--green));
    transform-origin: left;
    animation: build-bar 3.4s ease-in-out infinite;
  }

  @keyframes build-bar {
    0%,
    10% {
      transform: scaleX(0);
    }

    65%,
    100% {
      transform: scaleX(1);
    }
  }

  .activity-panel > header strong {
    color: var(--green-soft);
    font-size: 5.5px;
  }

  .activity-panel__list article {
    display: grid;
    grid-template-columns: auto minmax(0, 1fr) auto;
    align-items: center;
    gap: 7px;
    padding: 9px;
    border-bottom: 1px solid var(--line-soft);
    animation: activity-highlight 4.8s ease-in-out infinite;
    animation-delay: var(--activity-delay);
  }

  .activity-panel__list article:last-child {
    border-bottom: 0;
  }

  @keyframes activity-highlight {
    0%,
    22% {
      background: var(--brand-faint);
    }

    32%,
    100% {
      background: transparent;
    }
  }

  .activity-panel__icon {
    display: grid;
    width: 24px;
    height: 24px;
    place-items: center;
    border-radius: 5px;
    font-size: 6px;
    font-weight: 750;
  }

  .activity-panel__icon--package {
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .activity-panel__icon--lock {
    background: var(--warning-faint);
    color: var(--warning);
  }

  .activity-panel__icon--build {
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .activity-panel__list article > div {
    display: grid;
    min-width: 0;
    line-height: 1.25;
  }

  .activity-panel__list strong {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 5.8px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .activity-panel__list small,
  .activity-panel__list time {
    color: var(--text-faint);
    font-size: 5px;
  }

  .cloud-window__footer {
    display: grid;
    grid-template-columns: auto minmax(70px, 1fr) auto;
    min-height: 39px;
    align-items: center;
    gap: 11px;
    padding: 0 13px;
    border-top: 1px solid var(--line);
    background: var(--bg-soft);
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 5.5px;
  }

  .cloud-window__footer > div:first-child {
    display: inline-flex;
    align-items: center;
    gap: 6px;
  }

  .cloud-window__sync-dot {
    width: 5px;
    height: 5px;
    border-radius: 50%;
    background: var(--green);
    box-shadow: 0 0 0 3px var(--green-faint);
  }

  .cloud-window__sync-line {
    height: 2px;
    overflow: hidden;
    border-radius: 2px;
    background: var(--bg-elevated);
  }

  .cloud-window__sync-line span {
    display: block;
    width: 35%;
    height: 100%;
    border-radius: inherit;
    background: var(--brand);
    animation: preview-sync 3s ease-in-out infinite;
  }

  @keyframes preview-sync {
    0% {
      transform: translateX(-100%);
    }

    100% {
      transform: translateX(390%);
    }
  }

  .cloud-window__footer strong {
    color: var(--text-faint);
    font-weight: 500;
  }

  /* Floating product details */

  .cloud-preview__float {
    position: absolute;
    z-index: 4;
    display: grid;
    align-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 8px;
    background: rgba(26, 30, 34, 0.96);
    box-shadow: var(--shadow-panel);
    backdrop-filter: blur(14px);
  }

  .cloud-preview__float--package {
    bottom: 8%;
    left: -2%;
    grid-template-columns: auto minmax(0, 1fr) auto;
    gap: 9px;
    min-width: 255px;
    padding: 10px;
    animation: float-package 5s ease-in-out infinite;
  }

  .cloud-preview__float--package > span {
    display: grid;
    width: 30px;
    height: 30px;
    place-items: center;
    border-radius: 6px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 8px;
    font-weight: 800;
  }

  .cloud-preview__float > div:not(.cloud-preview__avatars) {
    display: grid;
    line-height: 1.25;
  }

  .cloud-preview__float small {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 5px;
    text-transform: uppercase;
  }

  .cloud-preview__float strong {
    color: var(--text);
    font-size: 7px;
  }

  .cloud-preview__float--package > i {
    color: var(--green-soft);
    font-size: 5.5px;
    font-style: normal;
    font-weight: 650;
  }

  .cloud-preview__float--team {
    right: -1%;
    bottom: 0;
    grid-template-columns: auto minmax(0, 1fr);
    gap: 10px;
    min-width: 205px;
    padding: 10px;
    animation: float-team 5.5s ease-in-out infinite;
  }

  .cloud-preview__avatars {
    display: flex;
  }

  .cloud-preview__avatars span {
    display: grid;
    width: 25px;
    height: 25px;
    place-items: center;
    margin-left: -6px;
    border: 2px solid var(--bg-panel);
    border-radius: 50%;
    background: var(--bg-elevated);
    color: var(--text-soft);
    font-size: 5px;
    font-weight: 700;
  }

  .cloud-preview__avatars span:first-child {
    margin-left: 0;
    background: var(--brand);
    color: var(--brand-ink);
  }

  @keyframes float-package {
    50% {
      transform: translateY(-6px);
    }
  }

  @keyframes float-team {
    50% {
      transform: translateY(5px);
    }
  }

  /* =========================================================
     CAPABILITIES
     ========================================================= */

  .value-section {
    padding: 118px 0;
    border-top: 1px solid var(--line);
  }

  .capabilities {
    display: grid;
    max-width: 980px;
    margin: 70px auto 0;
  }

  .capability {
    display: grid;
    grid-template-columns: 44px 56px minmax(0, 1fr);
    align-items: start;
    gap: 18px;
    padding: 27px 0;
    border-top: 1px solid var(--line);
    transition:
      border-color var(--speed) var(--ease),
      transform var(--speed) var(--ease);
  }

  .capability:last-child {
    border-bottom: 1px solid var(--line);
  }

  .capability:hover {
    border-color: var(--brand-line);
    transform: translateX(4px);
  }

  .capability__number {
    padding-top: 12px;
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 9px;
    font-weight: 650;
  }

  .capability__icon {
    display: grid;
    width: 50px;
    height: 50px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 11px;
    background: var(--bg-panel);
  }

  .capability__icon svg {
    width: 21px;
    height: 21px;
  }

  .capability__icon--packages {
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .capability__icon--history {
    background: var(--warning-faint);
    color: var(--warning);
  }

  .capability__icon--team {
    background: var(--info-faint);
    color: var(--info);
  }

  .capability__content > span {
    color: var(--brand-soft);
    font-family: var(--font-mono);
    font-size: 8px;
    font-weight: 650;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }

  .capability__content h3 {
    margin: 5px 0 0;
    color: var(--text);
    font-size: clamp(20px, 2.5vw, 28px);
    font-weight: 600;
    letter-spacing: -0.035em;
  }

  .capability__content p {
    max-width: 650px;
    margin: 9px 0 0;
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.65;
  }

  /* =========================================================
     WORKFLOW
     ========================================================= */

  .workflow-section {
    position: relative;
    padding: 108px 58px 54px;
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-xl);
    background:
      radial-gradient(
        circle at 10% 15%,
        rgba(249, 115, 22, 0.1),
        transparent 31%
      ),
      linear-gradient(145deg, #1a1e22, #111519);
    box-shadow: var(--shadow-panel);
  }

  .workflow-section__heading {
    max-width: 720px;
    margin: 0 auto;
    text-align: center;
  }

  .workflow {
    position: relative;
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 1fr));
    gap: 32px;
    margin-top: 75px;
  }

  .workflow__track {
    position: absolute;
    top: 70px;
    right: 15%;
    left: 15%;
    height: 2px;
    overflow: hidden;
    background: var(--line-strong);
  }

  .workflow__track::before {
    position: absolute;
    inset: 0;
    background: linear-gradient(
      90deg,
      var(--brand),
      var(--brand-soft),
      var(--green)
    );
    content: '';
    opacity: 0.5;
  }

  .workflow__track span {
    position: absolute;
    top: 50%;
    left: 0;
    width: 10px;
    height: 10px;
    border: 2px solid var(--bg-panel);
    border-radius: 50%;
    background: var(--brand);
    box-shadow: 0 0 13px var(--brand-glow);
    transform: translate(-50%, -50%);
    animation: workflow-packet 5.4s var(--ease) infinite;
  }

  @keyframes workflow-packet {
    0%,
    8% {
      left: 0;
    }

    45% {
      left: 50%;
    }

    82% {
      left: 100%;
    }

    100% {
      left: 0;
    }
  }

  .workflow-step {
    position: relative;
    z-index: 2;
    text-align: center;
  }

  .workflow-step__top {
    display: flex;
    justify-content: center;
    gap: 7px;
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 7px;
    text-transform: uppercase;
  }

  .workflow-step__owner {
    color: var(--brand-soft);
  }

  .workflow-step__node {
    display: grid;
    width: 55px;
    height: 55px;
    place-items: center;
    margin: 13px auto 17px;
    border: 1px solid var(--line-strong);
    border-radius: 14px;
    background: var(--bg-panel-strong);
    color: var(--brand-soft);
    box-shadow:
      0 8px 20px rgba(0, 0, 0, 0.22),
      0 0 0 7px rgba(26, 30, 34, 0.9);
    animation: workflow-node 5.4s ease-in-out infinite;
    animation-delay: var(--step-delay);
  }

  .workflow-step__node svg {
    width: 21px;
    height: 21px;
  }

  @keyframes workflow-node {
    0%,
    18% {
      border-color: var(--brand-line);
      color: var(--brand-bright);
      transform: translateY(-4px);
    }

    26%,
    100% {
      border-color: var(--line-strong);
      color: var(--brand-soft);
      transform: none;
    }
  }

  .workflow-step h3 {
    margin: 0;
    color: var(--text);
    font-size: 16px;
    font-weight: 600;
    letter-spacing: -0.025em;
  }

  .workflow-step p {
    max-width: 290px;
    margin: 8px auto 0;
    color: var(--text-muted);
    font-size: 11px;
    line-height: 1.6;
  }

  .workflow-section__boundary {
    display: flex;
    max-width: 700px;
    align-items: center;
    gap: 11px;
    margin: 60px auto 0;
    padding: 13px 15px;
    border: 1px solid var(--green-line);
    border-radius: 9px;
    background: var(--green-faint);
  }

  .workflow-section__boundary > span {
    display: grid;
    width: 32px;
    height: 32px;
    flex: 0 0 auto;
    place-items: center;
    border-radius: 7px;
    background: var(--bg-panel-strong);
    color: var(--green-soft);
  }

  .workflow-section__boundary svg {
    width: 16px;
    height: 16px;
  }

  .workflow-section__boundary p {
    margin: 0;
    color: rgba(148, 220, 172, 0.8);
    font-size: 10px;
    line-height: 1.55;
  }

  /* =========================================================
     FINAL CTA
     ========================================================= */

  .final-cta {
    position: relative;
    display: grid;
    grid-template-columns: minmax(0, 0.92fr) minmax(340px, 0.58fr);
    gap: clamp(44px, 8vw, 100px);
    align-items: center;
    overflow: hidden;
    margin-top: 108px;
    padding: 70px;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-xl);
    background:
      radial-gradient(
        circle at 12% 10%,
        rgba(249, 115, 22, 0.2),
        transparent 34%
      ),
      linear-gradient(145deg, #25150b, #15130f 54%, #101315);
    box-shadow: var(--shadow-lg);
  }

  .final-cta__pattern {
    position: absolute;
    inset: 0;
    background-image:
      linear-gradient(rgba(255, 255, 255, 0.025) 1px, transparent 1px),
      linear-gradient(
        90deg,
        rgba(255, 255, 255, 0.025) 1px,
        transparent 1px
      );
    background-size: 40px 40px;
    mask-image: radial-gradient(circle at 25% 20%, #000, transparent 70%);
  }

  .final-cta__glow {
    position: absolute;
    top: 8%;
    left: 7%;
    width: 340px;
    height: 340px;
    border-radius: 50%;
    background: var(--brand-glow);
    filter: blur(80px);
  }

  .final-cta__content,
  .final-cta__project {
    position: relative;
    z-index: 2;
  }

  .final-cta h2 {
    max-width: 650px;
    margin: 13px 0 0;
    color: var(--text);
    font-size: clamp(39px, 5vw, 62px);
    font-weight: 650;
    line-height: 0.98;
    letter-spacing: -0.055em;
  }

  .final-cta__content > p:not(.final-cta__eyebrow) {
    max-width: 560px;
    margin: 18px 0 0;
    color: var(--text-soft);
    font-size: 14px;
    line-height: 1.65;
  }

  .final-cta__actions {
    display: flex;
    flex-wrap: wrap;
    gap: 10px;
    margin-top: 27px;
  }

  .final-cta__project {
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: 12px;
    background: rgba(26, 30, 34, 0.9);
    box-shadow: var(--shadow-panel);
    backdrop-filter: blur(16px);
  }

  .final-cta__project > header {
    display: grid;
    grid-template-columns: auto minmax(0, 1fr) auto;
    align-items: center;
    gap: 9px;
    min-height: 56px;
    padding: 0 13px;
    border-bottom: 1px solid var(--line);
  }

  .final-cta__project-mark {
    display: grid;
    width: 32px;
    height: 32px;
    place-items: center;
    border-radius: 7px;
    background: var(--brand);
    color: var(--brand-ink);
    font-size: 9px;
    font-weight: 800;
  }

  .final-cta__project header > div {
    display: grid;
    line-height: 1.25;
  }

  .final-cta__project header strong {
    color: var(--text);
    font-size: 8px;
  }

  .final-cta__project header small {
    color: var(--text-muted);
    font-size: 6px;
  }

  .final-cta__project-state {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    color: var(--green-soft);
    font-size: 6px;
    font-weight: 650;
  }

  .final-cta__project-state i {
    width: 5px;
    height: 5px;
    border-radius: 50%;
    background: currentColor;
  }

  .final-cta__project-items {
    display: grid;
    padding: 8px;
  }

  .final-cta__project-items > span {
    display: grid;
    grid-template-columns: auto minmax(0, 1fr) auto;
    align-items: center;
    gap: 8px;
    padding: 10px 8px;
    border-bottom: 1px solid var(--line-soft);
    color: var(--text-soft);
    font-size: 7px;
  }

  .final-cta__project-items > span:last-child {
    border-bottom: 0;
  }

  .final-cta__project-items i {
    display: grid;
    width: 24px;
    height: 24px;
    place-items: center;
    border-radius: 5px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 6px;
    font-style: normal;
    font-weight: 750;
  }

  .final-cta__project-items strong {
    color: var(--green-soft);
    font-size: 5.5px;
  }

  /* =========================================================
     FOOTER
     ========================================================= */

  .home-footer {
    display: flex;
    min-height: 88px;
    align-items: center;
    justify-content: space-between;
    gap: 22px;
    margin-top: 72px;
    border-top: 1px solid var(--line);
  }

  .home-footer > div {
    display: flex;
    align-items: center;
    gap: 9px;
  }

  .home-footer img {
    width: 23px;
    height: 23px;
  }

  .home-footer span {
    color: var(--text-soft);
    font-size: 11px;
    font-weight: 650;
  }

  .home-footer nav {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 18px;
  }

  .home-footer a {
    color: var(--text-muted);
    font-size: 10px;
    text-decoration: none;
    transition: color var(--speed) var(--ease);
  }

  .home-footer a:hover {
    color: var(--brand-soft);
  }

  /* =========================================================
     REDUCED MOTION
     ========================================================= */

  @media (prefers-reduced-motion: reduce) {
    .hero__eyebrow > span,
    .build-panel__score-progress,
    .build-panel__bar span,
    .activity-panel__list article,
    .cloud-window__sync-line span,
    .cloud-preview__float,
    .workflow__track span,
    .workflow-step__node {
      animation: none;
    }

    .hero__copy,
    .cloud-preview {
      opacity: 1;
      transform: none;
    }
  }

  /* =========================================================
     RESPONSIVE
     ========================================================= */

  @media (max-width: 1040px) {
    .hero {
      grid-template-columns: 1fr;
      min-height: auto;
      padding-top: 72px;
    }

    .hero__copy {
      max-width: 760px;
    }

    .cloud-preview {
      min-height: 610px;
    }

    .cloud-window {
      width: 92%;
    }

    .final-cta {
      grid-template-columns: 1fr;
    }

    .final-cta__project {
      max-width: 520px;
    }
  }

  @media (max-width: 820px) {
    .workflow-section {
      padding-inline: 28px;
    }

    .workflow {
      grid-template-columns: 1fr;
      gap: 18px;
      margin-top: 55px;
    }

    .workflow__track {
      display: none;
    }

    .workflow-step {
      display: grid;
      grid-template-columns: auto minmax(0, 1fr);
      gap: 13px;
      padding: 14px;
      border: 1px solid var(--line);
      border-radius: 9px;
      background: rgba(255, 255, 255, 0.02);
      text-align: left;
    }

    .workflow-step__top {
      grid-column: 2;
      justify-content: flex-start;
    }

    .workflow-step__node {
      grid-row: 1 / 4;
      margin: 0;
    }

    .workflow-step h3,
    .workflow-step p {
      grid-column: 2;
      max-width: none;
      margin-inline: 0;
    }
  }

  @media (max-width: 720px) {
    .hero,
    .value-section,
    .workflow-section,
    .final-cta,
    .home-footer {
      width: min(100% - 28px, var(--container));
    }

    .hero {
      padding: 58px 0 76px;
    }

    .hero h1 {
      font-size: clamp(43px, 13vw, 62px);
    }

    .hero__actions {
      align-items: stretch;
      flex-direction: column;
    }

    .hero__actions .button {
      width: 100%;
    }

    .hero__facts {
      display: grid;
      gap: 11px;
    }

    .cloud-preview {
      min-height: 560px;
    }

    .cloud-window {
      width: 100%;
    }

    .cloud-window__body {
      grid-template-columns: 1fr;
    }

    .cloud-sidebar {
      display: none;
    }

    .cloud-content {
      padding: 13px;
    }

    .cloud-preview__float--package {
      left: -6px;
    }

    .cloud-preview__float--team {
      right: -6px;
    }

    .capability {
      grid-template-columns: 36px 47px minmax(0, 1fr);
      gap: 12px;
    }

    .capability__icon {
      width: 44px;
      height: 44px;
    }

    .workflow-section {
      padding: 78px 18px 32px;
    }

    .final-cta {
      padding: 48px 25px;
    }

    .final-cta h2 {
      font-size: clamp(38px, 11vw, 54px);
    }

    .final-cta__actions {
      align-items: stretch;
      flex-direction: column;
    }

    .final-cta__actions .button {
      width: 100%;
    }

    .home-footer {
      align-items: flex-start;
      flex-direction: column;
      justify-content: center;
      padding: 24px 0;
    }
  }

  @media (max-width: 520px) {
    .cloud-preview {
      min-height: 520px;
    }

    .cloud-window__header-actions {
      display: none;
    }

    .cloud-metrics {
      grid-template-columns: 1fr;
    }

    .cloud-metrics article:nth-child(2),
    .cloud-metrics article:nth-child(3) {
      display: none;
    }

    .cloud-grid {
      grid-template-columns: 1fr;
    }

    .activity-panel {
      display: none;
    }

    .cloud-window__footer {
      grid-template-columns: minmax(0, 1fr) auto;
    }

    .cloud-window__sync-line {
      display: none;
    }

    .cloud-preview__float {
      min-width: 0;
      max-width: calc(100% - 12px);
    }

    .cloud-preview__float--team {
      display: none;
    }

    .capability {
      grid-template-columns: 42px minmax(0, 1fr);
    }

    .capability__number {
      display: none;
    }

    .final-cta__project {
      display: none;
    }
  }
  /* ============================================================
   UNIFORM BACKGROUND OVERRIDE
   No mixed gradients or decorative background glows
   Place at the very end of the page <style>
   ============================================================ */

/* Global page */

.home-page {
  background: var(--bg);
}

/* Remove decorative background layers */

.cloud-preview__glow,
.cloud-preview__pattern,
.final-cta__pattern,
.final-cta__glow {
  display: none;
}

/* Main sections remain on the global background */

.hero,
.value-section {
  background: transparent;
}

/* Product preview */

.cloud-window {
  background: var(--bg-panel);
}

.cloud-content {
  background: var(--bg-panel);
}

/* Keep cards and panels simple */

.cloud-metrics article,
.build-panel,
.activity-panel,
.cloud-preview__float,
.final-cta__project {
  background: var(--bg-ink-soft);
}

/* Workflow gets one solid raised surface */

.workflow-section {
  background: var(--bg-panel);
  border-color: var(--line-strong);
  box-shadow: var(--shadow-panel);
}

/* Remove the mixed gradient from the workflow track */

.workflow__track::before {
  background: var(--brand);
  opacity: 0.7;
}

/* Keep the moving point orange */

.workflow__track span {
  background: var(--brand);
}

/* Final CTA uses one solid dark surface */

.final-cta {
  background: var(--bg-panel-strong);
  border-color: var(--brand-line);
  box-shadow: var(--shadow-lg);
}

/* Remove remaining decorative gradients */

.build-panel__bar span {
  background: var(--brand);
}

.cloud-window__sync-line span {
  background: var(--brand);
}

/* Keep floating elements consistent */

.cloud-preview__float {
  background: var(--bg-panel-strong);
}

/* Consistent internal surfaces */

.cloud-sidebar {
  background: var(--bg-ink-soft);
}

.cloud-window__header,
.cloud-window__footer {
  background: var(--bg-soft);
}

.cloud-content,
.workflow-step__node,
.final-cta__project {
  background-color: var(--bg-panel);
}

/* Mobile workflow cards */

@media (max-width: 820px) {
  .workflow-step {
    background: var(--bg-panel-strong);
  }
}
</style>
