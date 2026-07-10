<script lang="ts">
  import { onMount } from 'svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import { getSupportConfig } from '$lib/api/support';
  import { ApiError } from '$lib/api/types';
  import type { SupportConfig } from '$lib/api/types';

  let config: SupportConfig = {
    supporter_payment_url: '',
    builder_payment_url: '',
    contact_email: '',
    contact_url: ''
  };
  let error = '';

  function paymentLabel(url: string, label: string) {
    return url ? label : 'Payment link coming soon';
  }

  onMount(async () => {
    try {
      config = await getSupportConfig();
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to load support configuration.';
    }
  });
</script>

<svelte:head>
  <title>Support Softadastra Cloud</title>
  <meta name="description" content="Support the early development of Softadastra Cloud through the Founding Supporters program." />
</svelte:head>

<main class="support-page">
  <nav class="public-nav" aria-label="Public navigation">
    <a class="brand" href="/"><img src="/brand/softadastra-cloud.svg" alt="" /><span>Softadastra Cloud</span></a>
    <div><a href="/supporters">Supporters</a><a href="/login">Sign in</a></div>
  </nav>

  <InlineError message={error} />

  <section class="hero">
    <p class="eyebrow">Founding Supporters</p>
    <h1>Support the development of Softadastra Cloud</h1>
    <p>Softadastra Cloud is a C++ cloud workspace for Vix projects, private packages, lockfiles, build reports, permissions and team activity.</p>
    <div class="hero-actions">
      <a class:disabled={!config.supporter_payment_url} href={config.supporter_payment_url || undefined}>{paymentLabel(config.supporter_payment_url, 'Become a Founding Supporter - $10/month')}</a>
      <a class:disabled={!config.builder_payment_url} class="builder" href={config.builder_payment_url || undefined}>{paymentLabel(config.builder_payment_url, 'Become a Founding Builder - $25/month')}</a>
    </div>
    {#if !config.supporter_payment_url || !config.builder_payment_url}
      <p class="contact-note">
        Payment links are not configured yet.
        {#if config.contact_email}<a href={`mailto:${config.contact_email}`}>Contact by email</a>{/if}
        {#if config.contact_url}<a href={config.contact_url}>Contact page</a>{/if}
      </p>
    {/if}
  </section>

  <section class="content-section">
    <h2>Why this exists</h2>
    <p>Vix keeps the C++ workflow local. Softadastra Cloud is being built around the project state that teams need to understand: packages, versions, lockfiles, build reports, permissions, notifications and activity. Founding Supporters help make that work sustainable before the full paid platform is ready.</p>
    <p>I have been building Vix and Softadastra for years without monetizing the work. This supporter program is the first step toward making the project sustainable while keeping the core tooling open, serious, and useful for C++ developers.</p>
  </section>

  <section class="plans" aria-label="Support plans">
    <article>
      <span>Founding Supporter</span>
      <h2>$10/month</h2>
      <p>For developers who want to support the early development of Softadastra Cloud.</p>
      <ul>
        <li>Founding Supporter badge</li>
        <li>Name or project on the Supporters page</li>
        <li>Monthly development notes</li>
        <li>Early access to Softadastra Cloud demos</li>
        <li>Priority feedback from Gaspard</li>
      </ul>
      <a class:disabled={!config.supporter_payment_url} href={config.supporter_payment_url || undefined}>{paymentLabel(config.supporter_payment_url, 'Become a Founding Supporter')}</a>
    </article>

    <article class="builder-plan">
      <span>Founding Builder</span>
      <h2>$25/month</h2>
      <p>For developers who want to support the project and receive closer help with their own Vix workflow.</p>
      <ul>
        <li>Everything in Founding Supporter</li>
        <li>Deeper help with one Vix project</li>
        <li>Priority review of Vix setup</li>
        <li>Guidance on package publishing</li>
        <li>Help with lockfile and build report workflow</li>
        <li>Stronger visibility on the Supporters page</li>
      </ul>
      <a class:disabled={!config.builder_payment_url} href={config.builder_payment_url || undefined}>{paymentLabel(config.builder_payment_url, 'Become a Founding Builder')}</a>
    </article>
  </section>

  <section class="content-grid">
    <article><h2>What supporters receive</h2><p>Founding Supporter means you support the project and stay close to its progress. Founding Builder means you support the project and also want practical help with your own Vix project.</p></article>
    <article><h2>Honest product status</h2><p>This is not a finished enterprise SaaS subscription. Payment records and supporter linking are confirmed manually for now, while the product continues toward a complete paid platform.</p></article>
    <article><h2>How this helps</h2><p>Support gives the project room to improve the registry, CLI integration, dashboard workflow, documentation and production deployment path without changing the local-first nature of Vix.</p></article>
    <article><h2>FAQ</h2><p>Payments use external links configured by the project. Softadastra Cloud does not store card or payment details in this MVP step.</p></article>
  </section>
</main>

<style>
  .support-page { min-height: 100vh; background: var(--bg); color: var(--text); padding: 24px; }
  .public-nav { display: flex; align-items: center; justify-content: space-between; gap: 16px; max-width: 1120px; margin: 0 auto 44px; }
  .brand, .public-nav div { display: flex; align-items: center; gap: 12px; }
  .brand img { width: 28px; height: 28px; } .brand span { color: var(--text); font-weight: 800; }
  .public-nav a { color: var(--text-soft); font-size: 13px; font-weight: 650; }
  .hero, .content-section, .plans, .content-grid { max-width: 1120px; margin: 0 auto; }
  .hero { padding: 42px 0 34px; }
  .eyebrow, .plans span { color: var(--brand-bright); font-size: 11px; font-weight: 800; text-transform: uppercase; }
  .hero h1 { max-width: 780px; margin-top: 10px; color: var(--text); font-size: clamp(34px, 6vw, 62px); line-height: 1.02; }
  .hero p { max-width: 760px; margin-top: 16px; color: var(--text-muted); font-size: 16px; line-height: 1.7; }
  .hero-actions { display: flex; flex-wrap: wrap; gap: 12px; margin-top: 26px; }
  .hero-actions a, .plans article > a { border: 1px solid var(--brand-line); border-radius: var(--radius-sm); background: var(--brand-faint); color: var(--brand-bright); padding: 10px 14px; font-size: 13px; font-weight: 800; }
  .hero-actions a.builder, .builder-plan > a { border-color: rgba(245, 158, 11, 0.42); background: rgba(245, 158, 11, 0.12); color: #fbbf24; }
  a.disabled { pointer-events: none; opacity: 0.62; }
  .contact-note a { margin-left: 10px; color: var(--link); }
  .content-section { border-top: 1px solid var(--line); padding: 28px 0; }
  .content-section h2, .content-grid h2 { color: var(--text); font-size: 18px; }
  .content-section p, .content-grid p, .plans p, .plans li { color: var(--text-muted); font-size: 13px; line-height: 1.7; }
  .plans { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 16px; margin-top: 12px; }
  .plans article, .content-grid article { border: 1px solid var(--line); border-radius: var(--radius-md); background: var(--bg-panel); padding: 20px; }
  .plans h2 { margin-top: 8px; color: var(--text); font-size: 30px; }
  .plans ul { display: grid; gap: 8px; margin: 16px 0 20px; padding-left: 18px; }
  .builder-plan { border-color: rgba(245, 158, 11, 0.34) !important; }
  .content-grid { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 16px; margin-top: 16px; padding-bottom: 48px; }
  @media (max-width: 760px) { .support-page { padding: 18px; } .plans, .content-grid { grid-template-columns: 1fr; } .public-nav { align-items: flex-start; flex-direction: column; } }
</style>
