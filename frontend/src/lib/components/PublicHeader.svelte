<script lang="ts">
  import { onMount } from 'svelte';
  import { auth } from '$lib/stores/auth';

  export let platformHref = '/#platform';
  export let showPlatform = true;

  let checkingAuth = true;

  onMount(async () => {
    try {
      await auth.ensureAuthLoaded();
    } finally {
      checkingAuth = false;
    }
  });
</script>

<nav class="public-header" aria-label="Public navigation">
  <a class="brand" href="/">
    <img src="/brand/softadastra-cloud.svg" alt="" />
    <span>Softadastra Cloud</span>
  </a>

  <div class="links">
    {#if showPlatform}
      <a href={platformHref}>Platform</a>
    {/if}
    <a href="/support">Support</a>
    <a href="/supporters">Supporters</a>
    {#if checkingAuth}
      <span class="auth-placeholder" aria-hidden="true"></span>
    {:else if $auth.session}
      <a class="nav-cta" href="/dashboard">Open dashboard</a>
    {:else}
      <a href="/login">Sign in</a>
      <a class="nav-cta" href="/register">Create account</a>
    {/if}
  </div>
</nav>

<style>
  .public-header {
    display: flex;
    width: min(1120px, 100%);
    align-items: center;
    justify-content: space-between;
    gap: 16px;
    margin: 0 auto 44px;
  }

  .brand,
  .links {
    display: flex;
    align-items: center;
    gap: 14px;
    flex-wrap: wrap;
  }

  .brand img {
    width: 28px;
    height: 28px;
  }

  .brand span {
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

  .nav-cta {
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

  .auth-placeholder {
    display: inline-block;
    width: 112px;
    height: 38px;
  }

  @media (max-width: 640px) {
    .public-header {
      align-items: flex-start;
      flex-direction: column;
    }
  }
</style>
