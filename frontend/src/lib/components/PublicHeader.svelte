<script lang="ts">
  import { page } from '$app/stores';
  import { onMount } from 'svelte';

  import { auth } from '$lib/stores/auth';

  export let platformHref = '/#platform';
  export let showPlatform = true;

  let checkingAuth = true;

  let supportMenuOpen = false;
  let mobileMenuOpen = false;

  let lastLocation = '';

  $: currentPath =
    $page.url.pathname;

  $: currentLocation =
    `${$page.url.pathname}` +
    `${$page.url.search}` +
    `${$page.url.hash}`;

  $: platformActive =
    currentPath === '/';

  $: supportProgramActive =
    currentPath === '/support' ||
    currentPath.startsWith('/support/');

  $: supportersActive =
    currentPath === '/supporters' ||
    currentPath.startsWith('/supporters/');

  $: supportActive =
    supportProgramActive ||
    supportersActive;

  $: signInHref =
    buildAuthHref('/login');

  $: registerHref =
    buildAuthHref('/register');

  $: anyMenuOpen =
    supportMenuOpen ||
    mobileMenuOpen;

  $: if (
    currentLocation !== lastLocation
  ) {
    lastLocation = currentLocation;

    supportMenuOpen = false;
    mobileMenuOpen = false;
  }

  function buildAuthHref(
    destination: '/login' | '/register'
  ) {
    if (
      currentPath === '/' ||
      currentPath === '/login' ||
      currentPath === '/register'
    ) {
      return destination;
    }

    const params =
      new URLSearchParams();

    params.set(
      'redirect',
      currentLocation
    );

    return (
      `${destination}?` +
      params.toString()
    );
  }

  function toggleSupportMenu() {
    supportMenuOpen =
      !supportMenuOpen;

    mobileMenuOpen = false;
  }

  function toggleMobileMenu() {
    mobileMenuOpen =
      !mobileMenuOpen;

    supportMenuOpen = false;
  }

  function closeMenus() {
    supportMenuOpen = false;
    mobileMenuOpen = false;
  }

  function handleWindowKeydown(
    event: KeyboardEvent
  ) {
    if (
      event.key === 'Escape' &&
      anyMenuOpen
    ) {
      event.preventDefault();
      closeMenus();
    }
  }

  onMount(async () => {
    try {
      await auth.ensureAuthLoaded();
    } finally {
      checkingAuth = false;
    }
  });
</script>

<svelte:window
  onkeydown={handleWindowKeydown}
/>

<header class="public-header">
  {#if anyMenuOpen}
    <button
      class="menu-dismiss"
      type="button"
      tabindex="-1"
      aria-label="Close navigation menu"
      onclick={closeMenus}
    ></button>
  {/if}

  <div class="public-header__inner">
    <!-- =================================================
         BRAND
         ================================================= -->
    <a
      class="brand"
      href="/"
      aria-label="Softadastra Cloud home"
      onclick={closeMenus}
    >
      <img
        src="/brand/softadastra-cloud.svg"
        alt=""
        aria-hidden="true"
      />

      <span>Softadastra Cloud</span>
    </a>

    <!-- =================================================
         DESKTOP NAVIGATION
         ================================================= -->
    <nav
      class="desktop-navigation"
      aria-label="Public navigation"
    >
      {#if showPlatform}
        <a
          class:active={platformActive}
          href={platformHref}
          aria-current={
            platformActive
              ? 'page'
              : undefined
          }
        >
          Platform
        </a>
      {/if}

      <div class="support-navigation">
        <button
          class:active={supportActive}
          class:open={supportMenuOpen}
          class="support-trigger"
          type="button"
          aria-expanded={supportMenuOpen}
          aria-controls="public-support-menu"
          onclick={toggleSupportMenu}
        >
          Support

          <svg
            class:expanded={supportMenuOpen}
            viewBox="0 0 24 24"
            aria-hidden="true"
          >
            <path d="m7 9 5 5 5-5"></path>
          </svg>
        </button>

        {#if supportMenuOpen}
          <div
            id="public-support-menu"
            class="support-menu"
            aria-label="Support navigation"
          >
            <a
              class:active={supportProgramActive}
              href="/support"
              aria-current={
                supportProgramActive
                  ? 'page'
                  : undefined
              }
              onclick={closeMenus}
            >
              <span
                class="support-menu__icon"
                aria-hidden="true"
              >
                <svg viewBox="0 0 24 24">
                  <path
                    d="M12 21s-7-4.4-7-11a4 4 0 0 1 7-2.6A4 4 0 0 1 19 10c0 6.6-7 11-7 11Z"
                  ></path>
                </svg>
              </span>

              <span>
                <strong>
                  Support the project
                </strong>

                <small>
                  Join the Founding Supporters
                  program.
                </small>
              </span>
            </a>

            <a
              class:active={supportersActive}
              href="/supporters"
              aria-current={
                supportersActive
                  ? 'page'
                  : undefined
              }
              onclick={closeMenus}
            >
              <span
                class="support-menu__icon"
                aria-hidden="true"
              >
                <svg viewBox="0 0 24 24">
                  <circle
                    cx="9"
                    cy="8"
                    r="3"
                  ></circle>

                  <path
                    d="M3 20a6 6 0 0 1 12 0"
                  ></path>

                  <circle
                    cx="17"
                    cy="9"
                    r="2"
                  ></circle>

                  <path
                    d="M16 15a5 5 0 0 1 5 5"
                  ></path>
                </svg>
              </span>

              <span>
                <strong>
                  Founding wall
                </strong>

                <small>
                  Meet the public supporters and
                  builders.
                </small>
              </span>
            </a>
          </div>
        {/if}
      </div>
    </nav>

    <!-- =================================================
         DESKTOP AUTHENTICATION
         ================================================= -->
    <div class="desktop-actions">
      {#if checkingAuth}
        <div
          class="auth-placeholder"
          aria-label="Loading account status"
        >
          <span></span>
          <span></span>
        </div>
      {:else if $auth.session}
        <a
          class="primary-action"
          href="/dashboard"
        >
          Open Cloud

          <svg
            viewBox="0 0 24 24"
            aria-hidden="true"
          >
            <path d="M5 12h14"></path>
            <path d="m13 6 6 6-6 6"></path>
          </svg>
        </a>
      {:else}
        <a
          class="sign-in-action"
          href={signInHref}
        >
          Sign in
        </a>

        <a
          class="primary-action"
          href={registerHref}
        >
          Create account

          <svg
            viewBox="0 0 24 24"
            aria-hidden="true"
          >
            <path d="M5 12h14"></path>
            <path d="m13 6 6 6-6 6"></path>
          </svg>
        </a>
      {/if}
    </div>

    <!-- =================================================
         MOBILE TRIGGER
         ================================================= -->
    <button
      class:open={mobileMenuOpen}
      class="mobile-menu-trigger"
      type="button"
      aria-label={
        mobileMenuOpen
          ? 'Close navigation'
          : 'Open navigation'
      }
      aria-expanded={mobileMenuOpen}
      aria-controls="public-mobile-navigation"
      onclick={toggleMobileMenu}
    >
      {#if mobileMenuOpen}
        <svg
          viewBox="0 0 24 24"
          aria-hidden="true"
        >
          <path d="M6 6l12 12"></path>
          <path d="M18 6 6 18"></path>
        </svg>
      {:else}
        <svg
          viewBox="0 0 24 24"
          aria-hidden="true"
        >
          <path d="M4 7h16"></path>
          <path d="M4 12h16"></path>
          <path d="M4 17h16"></path>
        </svg>
      {/if}
    </button>
  </div>

  <!-- ===================================================
       MOBILE NAVIGATION
       =================================================== -->
  {#if mobileMenuOpen}
    <div
      id="public-mobile-navigation"
      class="mobile-navigation"
    >
      <nav aria-label="Mobile public navigation">
        {#if showPlatform}
          <a
            class:active={platformActive}
            href={platformHref}
            aria-current={
              platformActive
                ? 'page'
                : undefined
            }
            onclick={closeMenus}
          >
            <span
              class="mobile-navigation__icon"
              aria-hidden="true"
            >
              <svg viewBox="0 0 24 24">
                <rect
                  x="3"
                  y="3"
                  width="7"
                  height="7"
                  rx="1"
                ></rect>

                <rect
                  x="14"
                  y="3"
                  width="7"
                  height="7"
                  rx="1"
                ></rect>

                <rect
                  x="3"
                  y="14"
                  width="7"
                  height="7"
                  rx="1"
                ></rect>

                <rect
                  x="14"
                  y="14"
                  width="7"
                  height="7"
                  rx="1"
                ></rect>
              </svg>
            </span>

            <span>
              <strong>Platform</strong>

              <small>
                Discover Softadastra Cloud.
              </small>
            </span>
          </a>
        {/if}

        <section class="mobile-support-group">
          <p>Support</p>

          <a
            class:active={supportProgramActive}
            href="/support"
            aria-current={
              supportProgramActive
                ? 'page'
                : undefined
            }
            onclick={closeMenus}
          >
            <span
              class="mobile-navigation__icon"
              aria-hidden="true"
            >
              <svg viewBox="0 0 24 24">
                <path
                  d="M12 21s-7-4.4-7-11a4 4 0 0 1 7-2.6A4 4 0 0 1 19 10c0 6.6-7 11-7 11Z"
                ></path>
              </svg>
            </span>

            <span>
              <strong>
                Support the project
              </strong>

              <small>
                Founding Supporters program.
              </small>
            </span>
          </a>

          <a
            class:active={supportersActive}
            href="/supporters"
            aria-current={
              supportersActive
                ? 'page'
                : undefined
            }
            onclick={closeMenus}
          >
            <span
              class="mobile-navigation__icon"
              aria-hidden="true"
            >
              <svg viewBox="0 0 24 24">
                <circle
                  cx="9"
                  cy="8"
                  r="3"
                ></circle>

                <path
                  d="M3 20a6 6 0 0 1 12 0"
                ></path>

                <circle
                  cx="17"
                  cy="9"
                  r="2"
                ></circle>

                <path
                  d="M16 15a5 5 0 0 1 5 5"
                ></path>
              </svg>
            </span>

            <span>
              <strong>
                Founding wall
              </strong>

              <small>
                Public supporters and builders.
              </small>
            </span>
          </a>
        </section>
      </nav>

      <div class="mobile-authentication">
        {#if checkingAuth}
          <div
            class="mobile-auth-placeholder"
            aria-label="Loading account status"
          >
            <span></span>
            <span></span>
          </div>
        {:else if $auth.session}
          <a
            class="primary-action"
            href="/dashboard"
            onclick={closeMenus}
          >
            Open Cloud

            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path d="M5 12h14"></path>
              <path d="m13 6 6 6-6 6"></path>
            </svg>
          </a>
        {:else}
          <a
            class="mobile-sign-in"
            href={signInHref}
            onclick={closeMenus}
          >
            Sign in
          </a>

          <a
            class="primary-action"
            href={registerHref}
            onclick={closeMenus}
          >
            Create account

            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path d="M5 12h14"></path>
              <path d="m13 6 6 6-6 6"></path>
            </svg>
          </a>
        {/if}
      </div>
    </div>
  {/if}
</header>

<style>
  .public-header {
    position: relative;
    z-index: 30;
    width: min(100%, 1120px);
    margin-inline: auto;
    border-bottom: 1px solid var(--line);
  }

  .public-header__inner {
    display: grid;
    min-height: 64px;
    grid-template-columns:
      auto minmax(0, 1fr) auto;
    align-items: center;
    gap: 28px;
  }

  /* =====================================================
     BRAND
     ===================================================== */

  .brand {
    display: inline-flex;
    min-width: 0;
    align-items: center;
    gap: 9px;
    color: var(--text);
    text-decoration: none;
  }

  .brand img {
    display: block;
    width: 27px;
    height: 27px;
    flex: 0 0 auto;
    object-fit: contain;
  }

  .brand span {
    overflow: hidden;
    color: var(--text);
    font-size: 13px;
    font-weight: 700;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  /* =====================================================
     DESKTOP NAVIGATION
     ===================================================== */

  .desktop-navigation {
    display: flex;
    min-width: 0;
    align-items: center;
    justify-content: center;
    gap: 5px;
  }

  .desktop-navigation > a,
  .support-trigger {
    display: inline-flex;
    min-height: 36px;
    align-items: center;
    justify-content: center;
    gap: 6px;
    border: 1px solid transparent;
    border-radius: 8px;
    background: transparent;
    color: var(--text-muted);
    padding: 0 11px;
    font: inherit;
    font-size: 11.5px;
    font-weight: 550;
    text-decoration: none;
    cursor: pointer;
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease),
      color var(--speed) var(--ease);
  }

  .desktop-navigation > a:hover,
  .support-trigger:hover,
  .support-trigger.open {
    border-color: var(--line);
    background: var(--bg-elevated);
    color: var(--text);
  }

  .desktop-navigation > a.active,
  .support-trigger.active {
    color: var(--brand-bright);
  }

  .support-navigation {
    position: relative;
  }

  .support-trigger svg {
    width: 13px;
    height: 13px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
    transition:
      transform var(--speed) var(--ease);
  }

  .support-trigger svg.expanded {
    transform: rotate(180deg);
  }

  /* =====================================================
     SUPPORT MENU
     ===================================================== */

  .support-menu {
    position: absolute;
    top: calc(100% + 9px);
    left: 50%;
    z-index: 42;
    display: grid;
    width: 310px;
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: 10px;
    background: var(--bg-panel);
    padding: 6px;
    transform: translateX(-50%);
  }

  .support-menu > a {
    display: grid;
    min-height: 65px;
    grid-template-columns:
      35px minmax(0, 1fr);
    align-items: center;
    gap: 10px;
    border: 1px solid transparent;
    border-radius: 8px;
    color: var(--text);
    padding: 9px;
    text-decoration: none;
  }

  .support-menu > a:hover {
    background: var(--bg-elevated);
  }

  .support-menu > a.active {
    border-color: var(--brand-line);
    background: var(--brand-faint);
  }

  .support-menu__icon {
    display: grid;
    width: 34px;
    height: 34px;
    place-items: center;
    border: 1px solid var(--line);
    border-radius: 8px;
    color: var(--text-muted);
  }

  .support-menu > a.active
    .support-menu__icon {
    border-color: var(--brand-line);
    color: var(--brand-soft);
  }

  .support-menu__icon svg {
    width: 16px;
    height: 16px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .support-menu > a > span:last-child {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .support-menu strong {
    color: var(--text-soft);
    font-size: 11.5px;
    font-weight: 600;
  }

  .support-menu small {
    color: var(--text-muted);
    font-size: 10.5px;
    line-height: 1.45;
  }

  /* =====================================================
     DESKTOP ACTIONS
     ===================================================== */

  .desktop-actions {
    display: flex;
    min-width: 210px;
    align-items: center;
    justify-content: flex-end;
    gap: 8px;
  }

  .sign-in-action {
    display: inline-flex;
    min-height: 38px;
    align-items: center;
    justify-content: center;
    color: var(--text-soft);
    padding: 0 8px;
    font-size: 11.5px;
    font-weight: 600;
    text-decoration: none;
  }

  .sign-in-action:hover {
    color: var(--brand-soft);
  }

  .primary-action {
    display: inline-flex;
    min-height: 38px;
    align-items: center;
    justify-content: center;
    gap: 7px;
    border: 1px solid var(--brand);
    border-radius: var(--radius-sm);
    background: var(--brand);
    color: var(--brand-ink);
    padding: 0 13px;
    font-size: 11.5px;
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
    width: 13px;
    height: 13px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  /* =====================================================
     AUTH PLACEHOLDER
     ===================================================== */

  .auth-placeholder {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .auth-placeholder span {
    display: block;
    height: 36px;
    border-radius: 7px;
    background: var(--bg-elevated);
  }

  .auth-placeholder span:first-child {
    width: 58px;
  }

  .auth-placeholder span:last-child {
    width: 112px;
  }

  /* =====================================================
     MOBILE TRIGGER
     ===================================================== */

  .mobile-menu-trigger {
    display: none;
    width: 38px;
    height: 38px;
    min-height: 38px;
    place-items: center;
    border: 1px solid transparent;
    border-radius: 8px;
    background: transparent;
    color: var(--text-muted);
    padding: 0;
    font: inherit;
    cursor: pointer;
  }

  .mobile-menu-trigger:hover,
  .mobile-menu-trigger.open {
    border-color: var(--line);
    background: var(--bg-elevated);
    color: var(--text);
  }

  .mobile-menu-trigger svg {
    width: 18px;
    height: 18px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  /* =====================================================
     MOBILE NAVIGATION
     ===================================================== */

  .mobile-navigation {
    position: absolute;
    top: calc(100% + 9px);
    right: 0;
    left: 0;
    z-index: 42;
    display: none;
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: 10px;
    background: var(--bg-panel);
    padding: 7px;
  }

  .mobile-navigation nav {
    display: grid;
  }

  .mobile-navigation nav > a,
  .mobile-support-group > a {
    display: grid;
    min-height: 60px;
    grid-template-columns:
      35px minmax(0, 1fr);
    align-items: center;
    gap: 10px;
    border: 1px solid transparent;
    border-radius: 8px;
    color: var(--text);
    padding: 8px 9px;
    text-decoration: none;
  }

  .mobile-navigation nav > a:hover,
  .mobile-support-group > a:hover {
    background: var(--bg-elevated);
  }

  .mobile-navigation nav > a.active,
  .mobile-support-group > a.active {
    border-color: var(--brand-line);
    background: var(--brand-faint);
  }

  .mobile-navigation__icon {
    display: grid;
    width: 34px;
    height: 34px;
    place-items: center;
    border: 1px solid var(--line);
    border-radius: 8px;
    color: var(--text-muted);
  }

  a.active
    .mobile-navigation__icon {
    border-color: var(--brand-line);
    color: var(--brand-soft);
  }

  .mobile-navigation__icon svg {
    width: 16px;
    height: 16px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .mobile-navigation nav a > span:last-child {
    display: grid;
    gap: 3px;
  }

  .mobile-navigation strong {
    color: var(--text-soft);
    font-size: 11.5px;
    font-weight: 600;
  }

  .mobile-navigation small {
    color: var(--text-muted);
    font-size: 10.5px;
    line-height: 1.4;
  }

  .mobile-support-group {
    display: grid;
    margin-top: 5px;
    padding-top: 7px;
    border-top: 1px solid var(--line-soft);
  }

  .mobile-support-group > p {
    padding: 5px 9px 7px;
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 650;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }

  .mobile-authentication {
    display: grid;
    gap: 7px;
    margin-top: 7px;
    padding: 10px 8px 7px;
    border-top: 1px solid var(--line-soft);
  }

  .mobile-authentication
    .primary-action,
  .mobile-sign-in {
    width: 100%;
  }

  .mobile-sign-in {
    display: inline-flex;
    min-height: 38px;
    align-items: center;
    justify-content: center;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    color: var(--text-soft);
    font-size: 11.5px;
    font-weight: 600;
    text-decoration: none;
  }

  .mobile-sign-in:hover {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .mobile-auth-placeholder {
    display: grid;
    grid-template-columns:
      repeat(2, minmax(0, 1fr));
    gap: 7px;
  }

  .mobile-auth-placeholder span {
    height: 38px;
    border-radius: 7px;
    background: var(--bg-elevated);
  }

  /* =====================================================
     DISMISS LAYER
     ===================================================== */

.menu-dismiss {
  position: fixed;
  inset: 0;
  z-index: 20;
  width: 100%;
  min-height: 100%;
  border: 0;
  border-radius: 0;
  background: transparent;
  padding: 0;
  color: transparent;
  cursor: default;
  box-shadow: none;
  transform: none;
}

.menu-dismiss:hover,
.menu-dismiss:focus,
.menu-dismiss:active {
  border-color: transparent;
  background: transparent;
  color: transparent;
  box-shadow: none;
  transform: none;
  outline: none;
}

  /* =====================================================
     RESPONSIVE
     ===================================================== */

  @media (max-width: 760px) {
    .public-header__inner {
      min-height: 58px;
      grid-template-columns:
        minmax(0, 1fr) auto;
      gap: 14px;
    }

    .desktop-navigation,
    .desktop-actions {
      display: none;
    }

    .mobile-menu-trigger {
      display: grid;
    }

    .mobile-navigation {
      display: grid;
    }
  }

  @media (max-width: 420px) {
    .brand span {
      font-size: 12px;
    }

    .mobile-navigation {
      right: -2px;
      left: -2px;
    }
  }
</style>
