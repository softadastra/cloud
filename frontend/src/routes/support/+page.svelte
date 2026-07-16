<script lang="ts">
  import { onMount } from 'svelte';

  import InlineError from '$lib/components/InlineError.svelte';
  import PublicHeader from '$lib/components/PublicHeader.svelte';

  import { getSupportConfig } from '$lib/api/support';

  import {
    ApiError,
    type SupportConfig
  } from '$lib/api/types';

  type SupportTier =
    | 'supporter'
    | 'builder';

  const plans = [
    {
      key: 'supporter',
      eyebrow: 'Founding Supporter',
      price: '$10',
      cadence: 'per month',
      title: 'Support the product itself.',
      description:
        'For developers who believe Vix and Softadastra Cloud should continue growing into serious, sustainable C++ infrastructure.',
      benefits: [
        'Founding Supporter badge on your Cloud account',
        'Name or project on the public Supporters page',
        'Monthly development notes',
        'Early access to selected Cloud demos',
        'A direct channel for product feedback'
      ]
    },
    {
      key: 'builder',
      eyebrow: 'Founding Builder',
      price: '$25',
      cadence: 'per month',
      title: 'Support the work and get closer help.',
      description:
        'For developers actively using Vix who also want practical guidance around one project and its Cloud workflow.',
      benefits: [
        'Everything included with Founding Supporter',
        'Review of one active Vix project setup',
        'Guidance on package publishing',
        'Help understanding lockfiles and build reports',
        'Closer technical feedback on the Vix workflow'
      ]
    }
  ] as const;

  let config: SupportConfig = {
    supporter_payment_url: '',
    builder_payment_url: '',
    contact_email: '',
    contact_url: ''
  };

  let loading = true;
  let error = '';

  $: contactHref =
    config.contact_url ||
    (
      config.contact_email
        ? `mailto:${config.contact_email}?subject=${encodeURIComponent(
            'Founding Supporters'
          )}`
        : ''
    );

  $: supporterHref =
    config.supporter_payment_url ||
    contactHref;

  function paymentUrl(
    tier: SupportTier
  ) {
    return tier === 'builder'
      ? config.builder_payment_url
      : config.supporter_payment_url;
  }

  function actionHref(
    tier: SupportTier
  ) {
    return (
      paymentUrl(tier) ||
      contactHref
    );
  }

  function actionLabel(
    tier: SupportTier
  ) {
    if (paymentUrl(tier)) {
      return tier === 'builder'
        ? 'Become a Founding Builder'
        : 'Become a Founding Supporter';
    }

    if (contactHref) {
      return 'Contact us to join';
    }

    return 'Support access coming soon';
  }

  function actionDescription(
    tier: SupportTier
  ) {
    if (paymentUrl(tier)) {
      return 'External payment';
    }

    if (contactHref) {
      return 'Manual registration';
    }

    return 'Not available yet';
  }

  function isExternalHref(
    href: string
  ) {
    return /^https?:\/\//i.test(href);
  }

  async function loadConfig() {
    loading = true;
    error = '';

    try {
      config =
        await getSupportConfig();
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load the support options.';
    } finally {
      loading = false;
    }
  }

  onMount(() => {
    void loadConfig();
  });
</script>

<svelte:head>
  <title>
    Support Vix and Softadastra Cloud
  </title>

  <meta
    name="description"
    content="Support the continued development of Vix and Softadastra Cloud through the Founding Supporters program."
  />

  <meta property="og:title" content="Support Vix and Softadastra Cloud" />
  <meta property="og:description" content="Support the continued development of Vix and Softadastra Cloud through the Founding Supporters program." />
  <meta property="og:url" content="https://cloud.softadastra.com/support" />
  <meta property="og:image" content="https://cloud.softadastra.com/og-logo.png" />
  <meta property="og:image:secure_url" content="https://cloud.softadastra.com/og-logo.png" />

  <meta name="twitter:title" content="Support Vix and Softadastra Cloud" />
  <meta name="twitter:description" content="Support the continued development of Vix and Softadastra Cloud through the Founding Supporters program." />
  <meta name="twitter:image" content="https://cloud.softadastra.com/og-logo.png" />
</svelte:head>

<main class="support-page">
  <PublicHeader showPlatform={false} />

  <InlineError message={error} />

  <!-- ===================================================
       HERO
       =================================================== -->
  <section class="support-hero">
    <div class="hero-copy">
      <p class="eyebrow">
        Founding Supporters
      </p>

      <h1>
        Help make the Vix ecosystem
        <span>sustainable.</span>
      </h1>

      <p class="hero-description">
        Vix keeps C++ development local.
        Softadastra Cloud is being built around the
        project information teams need to share:
        packages, lockfiles, build reports,
        permissions and activity.
      </p>

      <p class="hero-description">
        Your support gives this work more time,
        stability and independence while the full
        commercial platform is still being built.
      </p>

      <div class="hero-actions">
        {#if loading}
          <span
            class="primary-action disabled"
            aria-disabled="true"
          >
            Loading support options…
          </span>
        {:else if supporterHref}
          <a
            class="primary-action"
            href={supporterHref}
            target={
              isExternalHref(
                supporterHref
              )
                ? '_blank'
                : undefined
            }
            rel={
              isExternalHref(
                supporterHref
              )
                ? 'noreferrer'
                : undefined
            }
          >
            {config.supporter_payment_url
              ? 'Become a Founding Supporter'
              : 'Contact us to become a supporter'}

            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path d="M5 12h14"></path>
              <path d="m13 6 6 6-6 6"></path>
            </svg>
          </a>
        {:else}
          <span
            class="primary-action disabled"
            aria-disabled="true"
          >
            Support access coming soon
          </span>
        {/if}

        <a
          class="text-action"
          href="#support-options"
        >
          Compare support options
        </a>
      </div>
    </div>

    <aside
      class="status-panel"
      aria-label="Current program status"
    >
      <header>
        <p class="panel-kicker">
          Current stage
        </p>

        <h2>
          An early supporter program
        </h2>
      </header>

      <dl>
        <div>
          <dt>Product status</dt>
          <dd>
            Active development
          </dd>
        </div>

        <div>
          <dt>Payment</dt>
          <dd>
            External provider
          </dd>
        </div>

        <div>
          <dt>Account linking</dt>
          <dd>
            Confirmed manually
          </dd>
        </div>

        <div>
          <dt>Card information</dt>
          <dd>
            Never stored by Cloud
          </dd>
        </div>
      </dl>

      <p>
        This is not being presented as a finished
        enterprise subscription. It is a direct way
        to support the work while staying close to
        its progress.
      </p>
    </aside>
  </section>

  <!-- ===================================================
       WHAT SUPPORT ENABLES
       =================================================== -->
  <section
    class="funding-section"
    id="where-support-goes"
  >
    <header class="section-heading">
      <p class="eyebrow">
        Where the support goes
      </p>

      <h2>
        Funding focused development,
        not artificial benefits.
      </h2>

      <p>
        The program exists to give the project more
        room to improve the foundations developers
        actually use.
      </p>
    </header>

    <div class="funding-grid">
      <article>
        <span class="funding-number">
          01
        </span>

        <div>
          <h3>
            Vix and the registry
          </h3>

          <p>
            Improve package installation, SDK
            composition, registry reliability and
            the workflows developers depend on
            locally.
          </p>
        </div>
      </article>

      <article>
        <span class="funding-number">
          02
        </span>

        <div>
          <h3>
            Softadastra Cloud
          </h3>

          <p>
            Continue building shared project state,
            permissions, private packages,
            lockfiles, diagnostics and team
            activity.
          </p>
        </div>
      </article>

      <article>
        <span class="funding-number">
          03
        </span>

        <div>
          <h3>
            Documentation and delivery
          </h3>

          <p>
            Create clearer documentation, improve
            deployment paths and make the ecosystem
            easier to adopt without weakening its
            local-first design.
          </p>
        </div>
      </article>
    </div>
  </section>

  <!-- ===================================================
       FOUNDER NOTE
       =================================================== -->
  <section class="founder-section">
    <div class="founder-heading">
      <p class="eyebrow">
        A note from the founder
      </p>

      <h2>
        This program starts with honesty.
      </h2>
    </div>

    <div class="founder-note">
      <p>
        I have spent years building Vix and the
        wider Softadastra ecosystem before finding
        a reliable way to monetize the work. The
        project has reached a point where continued
        development needs more than personal time
        and occasional resources.
      </p>

      <p>
        Founding Supporters are not buying promises
        of a finished enterprise platform. They are
        helping serious C++ tooling become
        sustainable while receiving a closer view
        of how the work progresses.
      </p>

      <span>
        Gaspard
        <small>
          Founder of Softadastra
        </small>
      </span>
    </div>
  </section>

  <!-- ===================================================
       PLANS
       =================================================== -->
  <section
    class="support-options"
    id="support-options"
    aria-labelledby="support-options-title"
  >
    <header class="section-heading">
      <p class="eyebrow">
        Support options
      </p>

      <h2 id="support-options-title">
        Choose how close you want to be
        to the work.
      </h2>

      <p>
        Both options support the same long-term
        mission. The Builder level adds practical
        help around an active Vix project.
      </p>
    </header>

    <div class="plans">
      {#each plans as plan, index (plan.key)}
        <article
          class:primary-plan={
            plan.key === 'supporter'
          }
          class="plan"
        >
          <header class="plan-header">
            <div>
              <p class="plan-eyebrow">
                {plan.eyebrow}
              </p>

              <h3>
                {plan.title}
              </h3>
            </div>

            {#if index === 0}
              <span class="starting-option">
                Start here
              </span>
            {/if}
          </header>

          <div class="plan-price">
            <strong>{plan.price}</strong>
            <span>{plan.cadence}</span>
          </div>

          <p class="plan-description">
            {plan.description}
          </p>
          <ul>
            {#each plan.benefits as benefit}
              <li>
                <span aria-hidden="true">
                  <svg viewBox="0 0 24 24">
                    <path
                      d="m5 12 4 4L19 6"
                    ></path>
                  </svg>
                </span>

                {benefit}
              </li>
            {/each}
          </ul>

          <footer class="plan-footer">
            <span>
              {actionDescription(
                plan.key
              )}
            </span>

            {#if actionHref(plan.key)}
              <a
                class:contact-action={
                  !paymentUrl(plan.key)
                }
                class="plan-action"
                href={actionHref(plan.key)}
                target={
                  isExternalHref(actionHref(plan.key))
                    ? '_blank'
                    : undefined
                }
                rel={
                  isExternalHref(actionHref(plan.key))
                    ? 'noreferrer'
                    : undefined
                }
              >
                {actionLabel(plan.key)}

                <svg
                  viewBox="0 0 24 24"
                  aria-hidden="true"
                >
                  <path d="M5 12h14"></path>
                  <path
                    d="m13 6 6 6-6 6"
                  ></path>
                </svg>
              </a>
            {:else}
              <span
                class="plan-action disabled"
                aria-disabled="true"
              >
                {actionLabel(plan.key)}
              </span>
            {/if}
          </footer>
        </article>
      {/each}
    </div>
  </section>

  <!-- ===================================================
       PROCESS
       =================================================== -->
  <section class="process-section">
    <header class="section-heading">
      <p class="eyebrow">
        How it works today
      </p>

      <h2>
        A simple process while the platform
        is still early.
      </h2>
    </header>

    <div class="process-list">
      <article>
        <span>01</span>

        <div>
          <h3>
            Choose a support level
          </h3>

          <p>
            Use the configured external payment
            link. Softadastra Cloud does not receive
            or store your card details.
          </p>
        </div>
      </article>

      <article>
        <span>02</span>

        <div>
          <h3>
            The payment is confirmed
          </h3>

          <p>
            The supporter record is reviewed and
            connected manually to the correct Cloud
            account.
          </p>
        </div>
      </article>

      <article>
        <span>03</span>

        <div>
          <h3>
            Your supporter access begins
          </h3>

          <p>
            The appropriate badge, visibility and
            communication access are enabled for
            the active support period.
          </p>
        </div>
      </article>
    </div>
  </section>

  <!-- ===================================================
       FAQ
       =================================================== -->
  <section class="faq-section">
    <header class="section-heading">
      <p class="eyebrow">
        Important details
      </p>

      <h2>
        Questions before supporting
      </h2>
    </header>

    <div class="faq-list">
      <details>
        <summary>
          Is this a complete Cloud subscription?
        </summary>

        <p>
          No. Softadastra Cloud is still under active
          development. The Founding Supporters
          program supports that development and
          provides the benefits described above.
        </p>
      </details>

      <details>
        <summary>
          Where is the payment processed?
        </summary>

        <p>
          Payments use an external provider
          configured by Softadastra. Cloud does not
          store card numbers or payment credentials.
        </p>
      </details>

      <details>
        <summary>
          How is the badge connected to my account?
        </summary>

        <p>
          During this early stage, payments and
          supporter accounts are matched manually.
          Contact information may be requested when
          the payment cannot be matched
          automatically.
        </p>
      </details>

      <details>
        <summary>
          What is the difference between Supporter
          and Builder?
        </summary>

        <p>
          Supporter is primarily for helping the
          ecosystem continue. Builder includes the
          same support benefits plus closer,
          practical guidance around one Vix project.
        </p>
      </details>
    </div>
  </section>

  <!-- ===================================================
       CONTACT
       =================================================== -->
  <section class="contact-section">
    <div>
      <p class="eyebrow">
        Questions or payment problems
      </p>

      <h2>
        Contact Softadastra directly.
      </h2>

      <p>
        Use the contact option when a payment link
        is unavailable, your supporter status has
        not appeared or you need clarification
        before joining.
      </p>
    </div>

    {#if contactHref}
      <a
        href={contactHref}
        target={
          isExternalHref(contactHref)
            ? '_blank'
            : undefined
        }
        rel={
          isExternalHref(contactHref)
            ? 'noreferrer'
            : undefined
        }
      >
        Contact Softadastra

        <svg
          viewBox="0 0 24 24"
          aria-hidden="true"
        >
          <path d="M5 12h14"></path>
          <path d="m13 6 6 6-6 6"></path>
        </svg>
      </a>
    {:else}
      <span
        class="contact-disabled"
        aria-disabled="true"
      >
        Contact information coming soon
      </span>
    {/if}
  </section>
</main>

<style>
  .support-page {
    min-height: 100vh;
    background: var(--bg);
    color: var(--text);
    padding: 24px;
  }

  .support-hero,
  .funding-section,
  .founder-section,
  .support-options,
  .process-section,
  .faq-section,
  .contact-section {
    width: min(100%, 1120px);
    margin-inline: auto;
  }

  .eyebrow,
  .panel-kicker,
  .plan-eyebrow {
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

  .support-hero {
    display: grid;
    grid-template-columns:
      minmax(0, 1.35fr)
      minmax(300px, 0.65fr);
    gap: clamp(46px, 8vw, 96px);
    align-items: center;
    padding: 76px 0 68px;
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
    font-size:
      clamp(42px, 6.5vw, 74px);
    font-weight: 650;
    line-height: 0.98;
    letter-spacing: -0.055em;
  }

  .hero-copy h1 span {
    display: block;
    color: var(--brand-soft);
  }

  .hero-description {
    max-width: 720px;
    margin-top: 20px;
    color: var(--text-muted);
    font-size: 15px;
    line-height: 1.75;
  }

  .hero-description +
    .hero-description {
    margin-top: 9px;
  }

  .hero-actions {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 17px;
    margin-top: 29px;
  }

  .primary-action,
  .plan-action,
  .contact-section > a {
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
      background var(--speed) var(--ease),
      color var(--speed) var(--ease);
  }

  .primary-action:hover,
  .plan-action:hover,
  .contact-section > a:hover {
    border-color: var(--brand-soft);
    background: var(--brand-soft);
  }

  .primary-action svg,
  .plan-action svg,
  .contact-section > a svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .primary-action.disabled,
  .plan-action.disabled {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-muted);
    cursor: not-allowed;
  }

  .text-action {
    color: var(--text-soft);
    font-size: 12px;
    font-weight: 600;
    text-decoration: none;
  }

  .text-action:hover {
    color: var(--brand-soft);
  }

  /* =====================================================
     STATUS PANEL
     ===================================================== */

  .status-panel {
    display: grid;
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .status-panel > header {
    display: grid;
    gap: 5px;
    padding: 18px;
    border-bottom: 1px solid var(--line-soft);
  }

  .status-panel h2 {
    color: var(--text);
    font-size: 17px;
    font-weight: 620;
    letter-spacing: -0.02em;
  }

  .status-panel dl {
    display: grid;
    margin: 0;
  }

  .status-panel dl > div {
    display: flex;
    min-height: 49px;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    padding: 10px 18px;
    border-bottom: 1px solid var(--line-soft);
  }

  .status-panel dt {
    color: var(--text-muted);
    font-size: 11px;
  }

  .status-panel dd {
    margin: 0;
    color: var(--text-soft);
    font-size: 11px;
    font-weight: 600;
    text-align: right;
  }

  .status-panel > p {
    padding: 16px 18px;
    color: var(--text-muted);
    font-size: 11.5px;
    line-height: 1.65;
  }

  /* =====================================================
     SHARED SECTION HEADINGS
     ===================================================== */

  .funding-section,
  .founder-section,
  .support-options,
  .process-section,
  .faq-section {
    padding: 68px 0;
    border-bottom: 1px solid var(--line);
  }

  .section-heading {
    display: grid;
    max-width: 740px;
    gap: 9px;
  }

  .section-heading h2,
  .founder-heading h2 {
    color: var(--text);
    font-size:
      clamp(29px, 4vw, 44px);
    font-weight: 630;
    line-height: 1.05;
    letter-spacing: -0.045em;
  }

  .section-heading > p:last-child {
    max-width: 670px;
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.7;
  }

  /* =====================================================
     FUNDING
     ===================================================== */

  .funding-grid {
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    margin-top: 39px;
    border-top: 1px solid var(--line);
    border-bottom: 1px solid var(--line);
  }

  .funding-grid article {
    display: grid;
    min-width: 0;
    grid-template-columns:
      33px minmax(0, 1fr);
    gap: 13px;
    padding: 27px 25px;
    border-right: 1px solid var(--line);
  }

  .funding-grid article:first-child {
    padding-left: 0;
  }

  .funding-grid article:last-child {
    border-right: 0;
    padding-right: 0;
  }

  .funding-number {
    color: var(--brand-soft);
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 700;
  }

  .funding-grid article > div {
    display: grid;
    gap: 8px;
  }

  .funding-grid h3 {
    color: var(--text-soft);
    font-size: 14px;
    font-weight: 620;
  }

  .funding-grid p {
    color: var(--text-muted);
    font-size: 12px;
    line-height: 1.7;
  }

  /* =====================================================
     FOUNDER NOTE
     ===================================================== */

  .founder-section {
    display: grid;
    grid-template-columns:
      minmax(270px, 0.65fr)
      minmax(0, 1.35fr);
    gap: clamp(42px, 8vw, 90px);
    align-items: start;
  }

  .founder-heading {
    display: grid;
    gap: 9px;
  }

  .founder-note {
    display: grid;
    gap: 17px;
    border-left: 2px solid var(--brand);
    padding-left: 24px;
  }

  .founder-note > p {
    color: var(--text-soft);
    font-size: 14px;
    line-height: 1.8;
  }

  .founder-note > span {
    display: grid;
    gap: 3px;
    color: var(--text);
    font-size: 12px;
    font-weight: 650;
  }

  .founder-note small {
    color: var(--text-muted);
    font-size: 11px;
    font-weight: 450;
  }

  /* =====================================================
     PLANS
     ===================================================== */

  .plans {
    display: grid;
    grid-template-columns:
      repeat(2, minmax(0, 1fr));
    gap: 16px;
    margin-top: 38px;
  }

  .plan {
    display: grid;
    align-content: start;
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .plan.primary-plan {
    border-color: var(--brand-line);
  }

  .plan-header {
    display: flex;
    min-height: 102px;
    align-items: flex-start;
    justify-content: space-between;
    gap: 18px;
    padding: 19px 20px;
    border-bottom: 1px solid var(--line-soft);
  }

  .plan-header > div {
    display: grid;
    gap: 6px;
  }

  .plan-header h3 {
    color: var(--text);
    font-size: 17px;
    font-weight: 620;
    line-height: 1.3;
  }

  .starting-option {
    display: inline-flex;
    min-height: 25px;
    flex: 0 0 auto;
    align-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 999px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    padding: 0 9px;
    font-size: 10px;
    font-weight: 650;
  }

  .plan-price {
    display: flex;
    align-items: flex-end;
    gap: 8px;
    padding: 20px 20px 0;
  }

  .plan-price strong {
    color: var(--text);
    font-size: 35px;
    font-weight: 650;
    line-height: 1;
    letter-spacing: -0.045em;
  }

  .plan-price span {
    color: var(--text-muted);
    font-size: 11px;
  }

  .plan-description {
    min-height: 80px;
    padding: 13px 20px 0;
    color: var(--text-muted);
    font-size: 12px;
    line-height: 1.65;
  }

  .plan ul {
    display: grid;
    gap: 11px;
    margin: 19px 20px 22px;
    padding: 19px 0 0;
    border-top: 1px solid var(--line-soft);
    list-style: none;
  }

  .plan li {
    display: grid;
    grid-template-columns:
      20px minmax(0, 1fr);
    align-items: start;
    gap: 9px;
    color: var(--text-soft);
    font-size: 11.5px;
    line-height: 1.55;
  }

  .plan li > span {
    display: grid;
    width: 19px;
    height: 19px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 50%;
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .plan li svg {
    width: 10px;
    height: 10px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2.2;
    stroke-linecap: round;
  }

  .plan-footer {
    display: flex;
    min-height: 72px;
    align-items: center;
    justify-content: space-between;
    gap: 14px;
    margin-top: auto;
    padding: 14px 20px;
    border-top: 1px solid var(--line-soft);
  }

  .plan-footer > span:first-child {
    color: var(--text-muted);
    font-size: 10.5px;
  }

  .plan-action {
    min-height: 39px;
    padding-inline: 12px;
    font-size: 11.5px;
  }

  .plan-action.contact-action {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-bright);
  }

  .plan-action.contact-action:hover {
    background: rgba(249, 115, 22, 0.16);
  }

  /* =====================================================
     PROCESS
     ===================================================== */

  .process-list {
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    margin-top: 37px;
    border-top: 1px solid var(--line);
  }

  .process-list article {
    display: grid;
    min-width: 0;
    grid-template-columns:
      34px minmax(0, 1fr);
    gap: 12px;
    padding: 25px 23px;
    border-right: 1px solid var(--line);
  }

  .process-list article:first-child {
    padding-left: 0;
  }

  .process-list article:last-child {
    border-right: 0;
    padding-right: 0;
  }

  .process-list article > span {
    color: var(--brand-soft);
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 700;
  }

  .process-list article > div {
    display: grid;
    gap: 8px;
  }

  .process-list h3 {
    color: var(--text-soft);
    font-size: 13px;
    font-weight: 620;
  }

  .process-list p {
    color: var(--text-muted);
    font-size: 11.5px;
    line-height: 1.65;
  }

  /* =====================================================
     FAQ
     ===================================================== */

  .faq-list {
    display: grid;
    margin-top: 35px;
    border-top: 1px solid var(--line);
  }

  .faq-list details {
    border-bottom: 1px solid var(--line);
  }

  .faq-list summary {
    position: relative;
    display: flex;
    min-height: 65px;
    align-items: center;
    justify-content: space-between;
    gap: 20px;
    color: var(--text-soft);
    padding: 14px 38px 14px 0;
    font-size: 12.5px;
    font-weight: 600;
    cursor: pointer;
    list-style: none;
  }

  .faq-list summary::-webkit-details-marker {
    display: none;
  }

  .faq-list summary::after {
    content: '+';
    position: absolute;
    right: 4px;
    color: var(--brand-soft);
    font-family: var(--font-mono);
    font-size: 18px;
    font-weight: 450;
  }

  .faq-list details[open]
    summary::after {
    content: '−';
  }

  .faq-list details > p {
    max-width: 760px;
    padding: 0 38px 20px 0;
    color: var(--text-muted);
    font-size: 12px;
    line-height: 1.7;
  }

  /* =====================================================
     CONTACT
     ===================================================== */

  .contact-section {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 38px;
    padding: 45px 0 54px;
  }

  .contact-section > div {
    display: grid;
    max-width: 690px;
    gap: 7px;
  }

  .contact-section h2 {
    color: var(--text);
    font-size:
      clamp(24px, 3vw, 34px);
    font-weight: 630;
    letter-spacing: -0.035em;
  }

  .contact-section div > p:last-child {
    color: var(--text-muted);
    font-size: 12px;
    line-height: 1.65;
  }

  .contact-section > a {
    flex: 0 0 auto;
  }

  .contact-disabled {
    display: inline-flex;
    min-height: 43px;
    flex: 0 0 auto;
    align-items: center;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    color: var(--text-muted);
    padding: 0 14px;
    font-size: 11.5px;
  }

  /* =====================================================
     RESPONSIVE
     ===================================================== */

  @media (max-width: 900px) {
    .support-hero {
      grid-template-columns: 1fr;
    }

    .status-panel {
      max-width: 620px;
    }

    .founder-section {
      grid-template-columns: 1fr;
    }
  }

  @media (max-width: 760px) {
    .support-page {
      padding: 18px;
    }

    .support-hero {
      padding: 54px 0 50px;
    }

    .funding-section,
    .founder-section,
    .support-options,
    .process-section,
    .faq-section {
      padding: 52px 0;
    }

    .funding-grid,
    .process-list {
      grid-template-columns: 1fr;
    }

    .funding-grid article,
    .funding-grid article:first-child,
    .funding-grid article:last-child,
    .process-list article,
    .process-list article:first-child,
    .process-list article:last-child {
      padding: 22px 0;
      border-right: 0;
      border-bottom: 1px solid var(--line);
    }

    .funding-grid article:last-child,
    .process-list article:last-child {
      border-bottom: 0;
    }

    .plans {
      grid-template-columns: 1fr;
    }

    .plan-description {
      min-height: 0;
    }

    .contact-section {
      align-items: flex-start;
      flex-direction: column;
    }
  }

  @media (max-width: 520px) {
    .support-page {
      padding: 14px;
    }

    .hero-copy h1 {
      font-size:
        clamp(39px, 12vw, 54px);
    }

    .hero-actions {
      align-items: stretch;
      flex-direction: column;
      width: 100%;
    }

    .primary-action,
    .text-action {
      width: 100%;
      justify-content: center;
      text-align: center;
    }

    .plan-header {
      align-items: flex-start;
      flex-direction: column;
    }

    .plan-footer {
      align-items: stretch;
      flex-direction: column;
    }

    .plan-action {
      width: 100%;
    }

    .contact-section > a,
    .contact-disabled {
      width: 100%;
      justify-content: center;
      text-align: center;
    }
  }
</style>
