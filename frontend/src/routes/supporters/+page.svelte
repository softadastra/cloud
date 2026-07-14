<script lang="ts">
  import { onMount } from 'svelte';

  import InlineError from '$lib/components/InlineError.svelte';
  import PublicHeader from '$lib/components/PublicHeader.svelte';
  import SupporterBadge from '$lib/components/SupporterBadge.svelte';

  import {
    listPublicSupporters
  } from '$lib/api/support';

  import {
    ApiError,
    type PublicSupporter
  } from '$lib/api/types';

  let supporters: PublicSupporter[] = [];
  let loading = true;
  let error = '';

  $: orderedSupporters = [
    ...supporters
  ].sort(compareSupporters);

  $: builders =
    orderedSupporters.filter(
      (supporter) =>
        supporter.tier ===
        'founding_builder'
    );

  $: foundingSupporters =
    orderedSupporters.filter(
      (supporter) =>
        supporter.tier !==
        'founding_builder'
    );

  $: earliestSupportDate =
    orderedSupporters.length > 0
      ? orderedSupporters[0]
          .started_at
      : null;

  function timestampFor(
    value:
      | number
      | string
      | null
      | undefined
  ) {
    if (
      value === null ||
      value === undefined ||
      value === ''
    ) {
      return null;
    }

    if (typeof value === 'number') {
      return value <
        1_000_000_000_000
        ? value * 1000
        : value;
    }

    const numericValue =
      Number(value);

    if (
      Number.isFinite(numericValue) &&
      value.trim() !== ''
    ) {
      return numericValue <
        1_000_000_000_000
        ? numericValue * 1000
        : numericValue;
    }

    const parsed =
      new Date(value).getTime();

    return Number.isNaN(parsed)
      ? null
      : parsed;
  }

  function compareSupporters(
    left: PublicSupporter,
    right: PublicSupporter
  ) {
    const leftDate =
      timestampFor(
        left.started_at
      ) ?? Number.MAX_SAFE_INTEGER;

    const rightDate =
      timestampFor(
        right.started_at
      ) ?? Number.MAX_SAFE_INTEGER;

    if (leftDate !== rightDate) {
      return leftDate - rightDate;
    }

    return left.display_name.localeCompare(
      right.display_name
    );
  }

  function formatDate(
    value:
      | number
      | string
      | null
      | undefined
  ) {
    const timestamp =
      timestampFor(value);

    if (timestamp === null) {
      return '';
    }

    return new Intl.DateTimeFormat(
      undefined,
      {
        year: 'numeric',
        month: 'long'
      }
    ).format(
      new Date(timestamp)
    );
  }

  function dateTimeValue(
    value:
      | number
      | string
      | null
      | undefined
  ) {
    const timestamp =
      timestampFor(value);

    return timestamp === null
      ? undefined
      : new Date(
          timestamp
        ).toISOString();
  }

  function supporterInitials(
    supporter: PublicSupporter
  ) {
    const words =
      supporter.display_name
        .trim()
        .split(/\s+/)
        .filter(Boolean);

    if (words.length === 0) {
      return 'S';
    }

    if (words.length === 1) {
      return words[0]
        .slice(0, 2)
        .toUpperCase();
    }

    return (
      words[0].slice(0, 1) +
      words[
        words.length - 1
      ].slice(0, 1)
    ).toUpperCase();
  }

  function supporterKey(
    supporter: PublicSupporter
  ) {
    return [
      supporter.tier,
      supporter.username ?? '',
      supporter.display_name,
      supporter.started_at ?? ''
    ].join(':');
  }

  function profileHref(
    supporter: PublicSupporter
  ) {
    return supporter.username
      ? `/u/${supporter.username}`
      : '';
  }

  async function loadSupporters() {
    loading = true;
    error = '';

    try {
      const data =
        await listPublicSupporters();

      supporters =
        data.supporters;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load the public supporters.';
    } finally {
      loading = false;
    }
  }

  onMount(() => {
    void loadSupporters();
  });
</script>

<svelte:head>
  <title>
    Founding Supporters | Softadastra Cloud
  </title>

  <meta
    name="description"
    content="Meet the founding supporters and builders helping Vix and Softadastra Cloud grow into sustainable C++ infrastructure."
  />

  <meta property="og:title" content="Founding Supporters | Softadastra Cloud" />
  <meta property="og:description" content="Meet the founding supporters and builders helping Vix and Softadastra Cloud grow into sustainable C++ infrastructure." />
  <meta property="og:url" content="https://business.softadastra.com/supporters" />
  <meta property="og:image" content="https://business.softadastra.com/og-logo.png" />

  <meta name="twitter:title" content="Founding Supporters | Softadastra Cloud" />
  <meta name="twitter:description" content="Meet the founding supporters and builders helping Vix and Softadastra Cloud grow into sustainable C++ infrastructure." />
</svelte:head>

<main class="supporters-page">
  <PublicHeader showPlatform={false} />

  <InlineError message={error} />

  <!-- ===================================================
       HERO
       =================================================== -->
  <header class="supporters-hero">
    <div class="hero-copy">
      <p class="eyebrow">
        Founding wall
      </p>

      <h1>
        The people helping build
        <span>the early Vix ecosystem.</span>
      </h1>

      <p>
        These developers, teams and projects chose
        to support Vix and Softadastra Cloud while
        the ecosystem is still being built.
      </p>

      <div class="hero-actions">
        <a
          class="primary-action"
          href="/support"
        >
          Join the founding supporters

          <svg
            viewBox="0 0 24 24"
            aria-hidden="true"
          >
            <path d="M5 12h14"></path>
            <path d="m13 6 6 6-6 6"></path>
          </svg>
        </a>
      </div>
    </div>

    <aside
      class="recognition-summary"
      aria-label="Public supporter summary"
    >
      <article>
        <span>
          Public supporters
        </span>

        <strong>
          {loading
            ? '—'
            : orderedSupporters.length}
        </strong>

        <small>
          Visible founding records
        </small>
      </article>

      <article>
        <span>
          Founding builders
        </span>

        <strong>
          {loading
            ? '—'
            : builders.length}
        </strong>

        <small>
          Supporters with project guidance
        </small>
      </article>

      <article>
        <span>
          Earliest public support
        </span>

        <strong class="date-value">
          {loading
            ? '—'
            : earliestSupportDate
              ? formatDate(
                  earliestSupportDate
                )
              : 'Not recorded yet'}
        </strong>

        <small>
          The beginning of the public wall
        </small>
      </article>
    </aside>
  </header>

  <!-- ===================================================
       PUBLIC VISIBILITY NOTE
       =================================================== -->
  <section class="visibility-note">
    <span
      class="visibility-note__icon"
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
          d="M2 12s3.5-6 10-6 10 6 10 6-3.5 6-10 6S2 12 2 12Z"
        ></path>

        <circle
          cx="12"
          cy="12"
          r="2.5"
        ></circle>
      </svg>
    </span>

    <div>
      <strong>
        Public recognition is optional
      </strong>

      <p>
        This page includes only supporters who
        chose to make their name or project public.
        It may not represent every active supporter.
      </p>
    </div>
  </section>

  <!-- ===================================================
       LOADING
       =================================================== -->
  {#if loading}
    <section
      class="supporter-loading"
      aria-label="Loading supporters"
    >
      {#each [1, 2, 3, 4, 5, 6] as placeholder}
        <article class="supporter-skeleton">
          <header>
            <span
              class="skeleton-badge"
              aria-hidden="true"
            ></span>

            <span
              class="skeleton-date"
              aria-hidden="true"
            ></span>
          </header>

          <span
            class="skeleton-avatar"
            aria-hidden="true"
          ></span>

          <div>
            <span
              class="skeleton-name"
              aria-hidden="true"
            ></span>

            <span
              class="skeleton-project"
              aria-hidden="true"
            ></span>
          </div>

          <span
            class="skeleton-links"
            aria-hidden="true"
          ></span>
        </article>
      {/each}
    </section>

  <!-- ===================================================
       EMPTY STATE
       =================================================== -->
  {:else if orderedSupporters.length === 0}
    <section class="empty-state">
      <span
        class="empty-state__number"
        aria-hidden="true"
      >
        01
      </span>

      <div>
        <p class="eyebrow">
          The first public place
        </p>

        <h2>
          The founding wall is ready for its first
          supporter.
        </h2>

        <p>
          The first public supporters will remain
          part of the early history of Vix and
          Softadastra Cloud as the ecosystem grows.
        </p>
      </div>

      <a
        class="primary-action"
        href="/support"
      >
        Become the first public supporter

        <svg
          viewBox="0 0 24 24"
          aria-hidden="true"
        >
          <path d="M5 12h14"></path>
          <path d="m13 6 6 6-6 6"></path>
        </svg>
      </a>
    </section>

  <!-- ===================================================
       SUPPORTER DIRECTORY
       =================================================== -->
  {:else}
    <div class="supporter-directory">
      {#if builders.length > 0}
        <section
          class="supporter-group"
          aria-labelledby="builders-title"
        >
          <header class="group-heading">
            <div>
              <p class="eyebrow">
                Closer project support
              </p>

              <h2 id="builders-title">
                Founding Builders
              </h2>

              <p>
                Supporters who also work more closely
                with the project around an active
                Vix workflow.
              </p>
            </div>

            <span class="group-count">
              {builders.length}
            </span>
          </header>

          <div class="supporter-grid">
            {#each builders as supporter (supporterKey(supporter))}
              <article class="supporter-card supporter-card--builder">
                <header class="supporter-card__header">
                  <SupporterBadge
                    tier={supporter.tier}
                  />

                  {#if supporter.started_at}
                    <time
                      datetime={dateTimeValue(
                        supporter.started_at
                      )}
                    >
                      Since
                      {formatDate(
                        supporter.started_at
                      )}
                    </time>
                  {/if}
                </header>

                <div class="supporter-identity">
                  <span
                    class="supporter-avatar"
                    aria-hidden="true"
                  >
                    {supporterInitials(
                      supporter
                    )}
                  </span>

                  <div>
                    <h3>
                      {supporter.display_name}
                    </h3>

                    {#if supporter.username}
                      <a
                        class="supporter-username"
                        href={profileHref(
                          supporter
                        )}
                      >
                        @{supporter.username}
                      </a>
                    {/if}
                  </div>
                </div>

                <div class="supporter-project">
                  <span>
                    Supported project
                  </span>

                  <strong>
                    {supporter.project_name ||
                      'Independent supporter'}
                  </strong>
                </div>

                <footer class="supporter-links">
                  {#if supporter.website_url}
                    <a
                      href={supporter.website_url}
                      rel="noreferrer"
                      target="_blank"
                    >
                      Website

                      <svg
                        viewBox="0 0 24 24"
                        aria-hidden="true"
                      >
                        <path d="M14 3h7v7"></path>
                        <path d="M10 14 21 3"></path>

                        <path
                          d="M21 14v5a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5"
                        ></path>
                      </svg>
                    </a>
                  {/if}

                  {#if supporter.github_url}
                    <a
                      href={supporter.github_url}
                      rel="noreferrer"
                      target="_blank"
                    >
                      GitHub

                      <svg
                        viewBox="0 0 24 24"
                        aria-hidden="true"
                      >
                        <path d="M14 3h7v7"></path>
                        <path d="M10 14 21 3"></path>

                        <path
                          d="M21 14v5a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5"
                        ></path>
                      </svg>
                    </a>
                  {/if}

                  {#if
                    supporter.username &&
                    !supporter.website_url &&
                    !supporter.github_url
                  }
                    <a
                      href={profileHref(
                        supporter
                      )}
                    >
                      View profile

                      <svg
                        viewBox="0 0 24 24"
                        aria-hidden="true"
                      >
                        <path d="M5 12h14"></path>
                        <path d="m13 6 6 6-6 6"></path>
                      </svg>
                    </a>
                  {/if}

                  {#if
                    !supporter.username &&
                    !supporter.website_url &&
                    !supporter.github_url
                  }
                    <span>
                      Public links not provided
                    </span>
                  {/if}
                </footer>
              </article>
            {/each}
          </div>
        </section>
      {/if}

      {#if foundingSupporters.length > 0}
        <section
          class="supporter-group"
          aria-labelledby="supporters-title"
        >
          <header class="group-heading">
            <div>
              <p class="eyebrow">
                Early ecosystem support
              </p>

              <h2 id="supporters-title">
                Founding Supporters
              </h2>

              <p>
                Developers, projects and teams helping
                the ecosystem continue growing during
                its early stage.
              </p>
            </div>

            <span class="group-count">
              {foundingSupporters.length}
            </span>
          </header>

          <div class="supporter-grid">
            {#each foundingSupporters as supporter (supporterKey(supporter))}
              <article class="supporter-card">
                <header class="supporter-card__header">
                  <SupporterBadge
                    tier={supporter.tier}
                  />

                  {#if supporter.started_at}
                    <time
                      datetime={dateTimeValue(
                        supporter.started_at
                      )}
                    >
                      Since
                      {formatDate(
                        supporter.started_at
                      )}
                    </time>
                  {/if}
                </header>

                <div class="supporter-identity">
                  <span
                    class="supporter-avatar"
                    aria-hidden="true"
                  >
                    {supporterInitials(
                      supporter
                    )}
                  </span>

                  <div>
                    <h3>
                      {supporter.display_name}
                    </h3>

                    {#if supporter.username}
                      <a
                        class="supporter-username"
                        href={profileHref(
                          supporter
                        )}
                      >
                        @{supporter.username}
                      </a>
                    {/if}
                  </div>
                </div>

                <div class="supporter-project">
                  <span>
                    Supported project
                  </span>

                  <strong>
                    {supporter.project_name ||
                      'Independent supporter'}
                  </strong>
                </div>

                <footer class="supporter-links">
                  {#if supporter.website_url}
                    <a
                      href={supporter.website_url}
                      rel="noreferrer"
                      target="_blank"
                    >
                      Website

                      <svg
                        viewBox="0 0 24 24"
                        aria-hidden="true"
                      >
                        <path d="M14 3h7v7"></path>
                        <path d="M10 14 21 3"></path>

                        <path
                          d="M21 14v5a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5"
                        ></path>
                      </svg>
                    </a>
                  {/if}

                  {#if supporter.github_url}
                    <a
                      href={supporter.github_url}
                      rel="noreferrer"
                      target="_blank"
                    >
                      GitHub

                      <svg
                        viewBox="0 0 24 24"
                        aria-hidden="true"
                      >
                        <path d="M14 3h7v7"></path>
                        <path d="M10 14 21 3"></path>

                        <path
                          d="M21 14v5a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5"
                        ></path>
                      </svg>
                    </a>
                  {/if}

                  {#if
                    supporter.username &&
                    !supporter.website_url &&
                    !supporter.github_url
                  }
                    <a
                      href={profileHref(
                        supporter
                      )}
                    >
                      View profile

                      <svg
                        viewBox="0 0 24 24"
                        aria-hidden="true"
                      >
                        <path d="M5 12h14"></path>
                        <path d="m13 6 6 6-6 6"></path>
                      </svg>
                    </a>
                  {/if}

                  {#if
                    !supporter.username &&
                    !supporter.website_url &&
                    !supporter.github_url
                  }
                    <span>
                      Public links not provided
                    </span>
                  {/if}
                </footer>
              </article>
            {/each}
          </div>
        </section>
      {/if}
    </div>
  {/if}
</main>

<style>
  .supporters-page {
    min-height: 100vh;
    background: var(--bg);
    color: var(--text);
    padding: 24px;
  }

  .supporters-hero,
  .visibility-note,
  .supporter-loading,
  .empty-state,
  .supporter-directory {
    width: min(100%, 1120px);
    margin-inline: auto;
  }

  .eyebrow {
    color: var(--brand-soft);
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 0.1em;
    text-transform: uppercase;
  }

  /* =====================================================
     HERO
     ===================================================== */

  .supporters-hero {
    display: grid;
    grid-template-columns:
      minmax(0, 1.35fr)
      minmax(320px, 0.65fr);
    gap: clamp(48px, 8vw, 96px);
    align-items: center;
    padding: 72px 0 64px;
    border-bottom: 1px solid var(--line);
  }

  .hero-copy {
    display: grid;
    justify-items: start;
  }

  .hero-copy h1 {
    max-width: 790px;
    margin-top: 13px;
    color: var(--text);
    font-size: clamp(42px, 6.3vw, 72px);
    font-weight: 650;
    line-height: 0.98;
    letter-spacing: -0.055em;
  }

  .hero-copy h1 span {
    display: block;
    color: var(--brand-soft);
  }

  .hero-copy > p:last-of-type {
    max-width: 690px;
    margin-top: 20px;
    color: var(--text-muted);
    font-size: 14px;
    line-height: 1.75;
  }

  .hero-actions {
    margin-top: 28px;
  }

  .primary-action {
    display: inline-flex;
    min-height: 43px;
    align-items: center;
    justify-content: center;
    gap: 8px;
    border: 1px solid var(--brand);
    border-radius: var(--radius-sm);
    background: var(--brand);
    color: var(--brand-ink);
    padding: 0 15px;
    font-size: 12px;
    font-weight: 650;
    text-decoration: none;
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease);
  }

  .primary-action:hover {
    border-color: var(--brand-soft);
    background: var(--brand-soft);
  }

  .primary-action svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  /* =====================================================
     SUMMARY
     ===================================================== */

  .recognition-summary {
    display: grid;
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .recognition-summary article {
    display: grid;
    grid-template-columns:
      minmax(0, 1fr) auto;
    gap: 4px 18px;
    padding: 16px 18px;
    border-bottom: 1px solid var(--line-soft);
  }

  .recognition-summary article:last-child {
    border-bottom: 0;
  }

  .recognition-summary span {
    color: var(--text-muted);
    font-size: 11px;
  }

  .recognition-summary strong {
    grid-row: 1 / span 2;
    grid-column: 2;
    align-self: center;
    color: var(--text);
    font-size: 22px;
    font-weight: 630;
    letter-spacing: -0.03em;
  }

  .recognition-summary strong.date-value {
    max-width: 145px;
    font-size: 13px;
    line-height: 1.35;
    text-align: right;
  }

  .recognition-summary small {
    color: var(--text-faint);
    font-size: 10px;
  }

  /* =====================================================
     VISIBILITY NOTE
     ===================================================== */

  .visibility-note {
    display: grid;
    grid-template-columns:
      37px minmax(0, 1fr);
    align-items: center;
    gap: 12px;
    margin-top: 22px;
    padding: 13px 15px;
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
    background: var(--bg-panel);
  }

  .visibility-note__icon {
    display: grid;
    width: 36px;
    height: 36px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 8px;
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .visibility-note__icon svg {
    width: 17px;
    height: 17px;
  }

  .visibility-note > div {
    display: grid;
    gap: 3px;
  }

  .visibility-note strong {
    color: var(--text-soft);
    font-size: 11.5px;
  }

  .visibility-note p {
    max-width: 790px;
    color: var(--text-muted);
    font-size: 11px;
    line-height: 1.55;
  }

  /* =====================================================
     LOADING
     ===================================================== */

  .supporter-loading {
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    gap: 14px;
    padding: 52px 0 60px;
  }

  .supporter-skeleton {
    display: grid;
    min-height: 285px;
    align-content: start;
    gap: 17px;
    padding: 18px;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .supporter-skeleton > header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 15px;
  }

  .skeleton-badge,
  .skeleton-date,
  .skeleton-avatar,
  .skeleton-name,
  .skeleton-project,
  .skeleton-links {
    display: block;
    overflow: hidden;
    background: var(--bg-elevated);
  }

  .skeleton-badge {
    width: 112px;
    height: 24px;
    border-radius: 999px;
  }

  .skeleton-date {
    width: 76px;
    height: 10px;
    border-radius: 4px;
  }

  .skeleton-avatar {
    width: 52px;
    height: 52px;
    border-radius: 50%;
  }

  .supporter-skeleton > div {
    display: grid;
    gap: 9px;
  }

  .skeleton-name {
    width: 62%;
    height: 14px;
    border-radius: 4px;
  }

  .skeleton-project {
    width: 80%;
    height: 11px;
    border-radius: 4px;
  }

  .skeleton-links {
    width: 45%;
    height: 28px;
    margin-top: auto;
    border-radius: 7px;
  }

  .skeleton-badge::after,
  .skeleton-date::after,
  .skeleton-avatar::after,
  .skeleton-name::after,
  .skeleton-project::after,
  .skeleton-links::after {
    content: '';
    display: block;
    width: 45%;
    height: 100%;
    background: rgba(255, 255, 255, 0.035);
    animation:
      skeleton-move
      1.2s ease-in-out infinite;
  }

  @keyframes skeleton-move {
    from {
      transform: translateX(-100%);
    }

    to {
      transform: translateX(320%);
    }
  }

  /* =====================================================
     EMPTY STATE
     ===================================================== */

  .empty-state {
    display: grid;
    min-height: 430px;
    align-content: center;
    justify-items: start;
    gap: 18px;
    padding: 56px 10px 68px;
  }

  .empty-state__number {
    display: grid;
    width: 54px;
    height: 54px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 12px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-family: var(--font-mono);
    font-size: 13px;
    font-weight: 700;
  }

  .empty-state > div {
    display: grid;
    max-width: 720px;
    gap: 9px;
  }

  .empty-state h2 {
    color: var(--text);
    font-size: clamp(30px, 4.5vw, 46px);
    font-weight: 630;
    line-height: 1.04;
    letter-spacing: -0.045em;
  }

  .empty-state div > p:last-child {
    max-width: 620px;
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.7;
  }

  /* =====================================================
     DIRECTORY
     ===================================================== */

  .supporter-directory {
    display: grid;
    padding-bottom: 64px;
  }

  .supporter-group {
    display: grid;
    padding: 62px 0;
    border-bottom: 1px solid var(--line);
  }

  .supporter-group:last-child {
    border-bottom: 0;
  }

  .group-heading {
    display: flex;
    align-items: flex-end;
    justify-content: space-between;
    gap: 24px;
  }

  .group-heading > div {
    display: grid;
    max-width: 720px;
    gap: 8px;
  }

  .group-heading h2 {
    color: var(--text);
    font-size: clamp(28px, 4vw, 42px);
    font-weight: 630;
    line-height: 1.05;
    letter-spacing: -0.04em;
  }

  .group-heading div > p:last-child {
    max-width: 650px;
    color: var(--text-muted);
    font-size: 12.5px;
    line-height: 1.65;
  }

  .group-count {
    display: grid;
    min-width: 38px;
    height: 34px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    color: var(--text-muted);
    padding-inline: 10px;
    font-family: var(--font-mono);
    font-size: 11px;
  }

  .supporter-grid {
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    gap: 14px;
    margin-top: 34px;
  }

  /* =====================================================
     SUPPORTER CARD
     ===================================================== */

  .supporter-card {
    display: grid;
    min-width: 0;
    min-height: 300px;
    align-content: start;
    overflow: hidden;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease);
  }

  .supporter-card:hover {
    border-color: var(--line-strong);
  }

  .supporter-card--builder {
    border-color: var(--brand-line);
  }

  .supporter-card__header {
    display: flex;
    min-height: 57px;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    padding: 12px 15px;
    border-bottom: 1px solid var(--line-soft);
  }

  .supporter-card__header time {
    color: var(--text-muted);
    font-size: 10px;
    white-space: nowrap;
  }

  .supporter-identity {
    display: grid;
    grid-template-columns:
      52px minmax(0, 1fr);
    align-items: center;
    gap: 12px;
    padding: 20px 16px 16px;
  }

  .supporter-avatar {
    display: grid;
    width: 52px;
    height: 52px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 50%;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 14px;
    font-weight: 700;
  }

  .supporter-identity > div {
    display: grid;
    min-width: 0;
    gap: 4px;
  }

  .supporter-identity h3 {
    overflow-wrap: anywhere;
    color: var(--text);
    font-size: 16px;
    font-weight: 620;
    line-height: 1.3;
  }

  .supporter-username {
    width: fit-content;
    color: var(--brand-soft);
    font-family: var(--font-mono);
    font-size: 10.5px;
    text-decoration: none;
  }

  .supporter-username:hover {
    color: var(--brand-bright);
  }

  .supporter-project {
    display: grid;
    gap: 5px;
    margin: 0 16px;
    padding: 14px 0;
    border-top: 1px solid var(--line-soft);
  }

  .supporter-project span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 9.5px;
    letter-spacing: 0.05em;
    text-transform: uppercase;
  }

  .supporter-project strong {
    overflow-wrap: anywhere;
    color: var(--text-soft);
    font-size: 11.5px;
    font-weight: 580;
    line-height: 1.5;
  }

  .supporter-links {
    display: flex;
    min-height: 59px;
    flex-wrap: wrap;
    align-items: center;
    gap: 8px;
    margin-top: auto;
    padding: 12px 15px;
    border-top: 1px solid var(--line-soft);
  }

  .supporter-links a {
    display: inline-flex;
    min-height: 31px;
    align-items: center;
    gap: 5px;
    border: 1px solid var(--line-strong);
    border-radius: 7px;
    color: var(--text-soft);
    padding: 0 9px;
    font-size: 10.5px;
    font-weight: 600;
    text-decoration: none;
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease),
      color var(--speed) var(--ease);
  }

  .supporter-links a:hover {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .supporter-links svg {
    width: 12px;
    height: 12px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .supporter-links > span {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  /* =====================================================
     RESPONSIVE
     ===================================================== */

  @media (prefers-reduced-motion: reduce) {
    .skeleton-badge::after,
    .skeleton-date::after,
    .skeleton-avatar::after,
    .skeleton-name::after,
    .skeleton-project::after,
    .skeleton-links::after {
      animation: none;
    }

    .supporter-card {
      transition: none;
    }
  }

  @media (max-width: 920px) {
    .supporters-hero {
      grid-template-columns: 1fr;
    }

    .recognition-summary {
      max-width: 650px;
    }

    .supporter-grid,
    .supporter-loading {
      grid-template-columns:
        repeat(2, minmax(0, 1fr));
    }
  }

  @media (max-width: 680px) {
    .supporters-page {
      padding: 18px;
    }

    .supporters-hero {
      padding: 54px 0 49px;
    }

    .group-heading {
      align-items: flex-start;
      flex-direction: column;
    }

    .supporter-grid,
    .supporter-loading {
      grid-template-columns: 1fr;
    }

    .supporter-group {
      padding: 50px 0;
    }
  }

  @media (max-width: 500px) {
    .supporters-page {
      padding: 14px;
    }

    .hero-copy h1 {
      font-size: clamp(39px, 12vw, 54px);
    }

    .hero-actions,
    .primary-action {
      width: 100%;
    }

    .recognition-summary article {
      grid-template-columns: 1fr;
    }

    .recognition-summary strong,
    .recognition-summary strong.date-value {
      grid-row: auto;
      grid-column: auto;
      max-width: none;
      margin-top: 3px;
      text-align: left;
    }

    .visibility-note {
      align-items: start;
    }

    .supporter-card__header {
      align-items: flex-start;
      flex-direction: column;
    }

    .supporter-links {
      align-items: stretch;
      flex-direction: column;
    }

    .supporter-links a {
      width: 100%;
      justify-content: center;
    }
  }
</style>
