<script lang="ts">
  import { goto } from '$app/navigation';
  import { page } from '$app/stores';
  import { onMount } from 'svelte';
  import AccessScopeBadge from '$lib/components/AccessScopeBadge.svelte';
  import RoleBadge from '$lib/components/RoleBadge.svelte';
  import { auth } from '$lib/stores/auth';
  import { notifications } from '$lib/stores/notifications';
  import { workspaceContext } from '$lib/stores/workspace';

  const SIDEBAR_MIN_WIDTH = 232;
  const SIDEBAR_MAX_WIDTH = 360;
  const SIDEBAR_DEFAULT_WIDTH = 264;

  const SIDEBAR_WIDTH_KEY =
    'softadastra-cloud:sidebar-width';

  const SIDEBAR_COLLAPSED_KEY =
    'softadastra-cloud:sidebar-collapsed';

  const SIDEBAR_GROUPS_KEY =
    'softadastra-cloud:sidebar-groups';

  const navigationGroups = [
    {
      key: 'workspace',
      label: 'Workspace',
      items: [
        {
          href: '/dashboard',
          label: 'Dashboard',
          icon:
            'M4 4h6v7H4V4Zm10 0h6v4h-6V4Zm0 8h6v8h-6v-8ZM4 15h6v5H4v-5Z'
        },
        {
          href: '/workspaces',
          label: 'Workspaces',
          icon:
            'M4 20V7l8-4 8 4v13M8 20v-4h8v4M8 9h1M11.5 9h1M15 9h1M8 12h1M11.5 12h1M15 12h1'
        },
        {
          href: '/projects',
          label: 'Projects',
          icon:
            'M3 7a2 2 0 0 1 2-2h5l2 2h7a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V7Z'
        },
        {
          href: '/members',
          label: 'Members',
          icon:
            'M16 21v-2a4 4 0 0 0-4-4H6a4 4 0 0 0-4 4v2M9 11a4 4 0 1 0 0-8 4 4 0 0 0 0 8ZM22 21v-2a4 4 0 0 0-3-3.87M16 3.13a4 4 0 0 1 0 7.75'
        }
      ]
    },
    {
      key: 'registry',
      label: 'Registry',
      items: [
        {
          href: '/packages',
          label: 'Packages',
          icon:
            'm12 3 8 4.5v9L12 21l-8-4.5v-9L12 3Zm0 9 8-4.5M12 12 4 7.5M12 12v9'
        },
        {
          href: '/package-versions',
          label: 'Versions',
          icon:
            'M20 12v7a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2h7M16 3h5v5M10 14 21 3'
        },
        {
          href: '/lockfiles',
          label: 'Lockfiles',
          icon:
            'M7 10V7a5 5 0 0 1 10 0v3M5 10h14v11H5V10Zm7 4v3'
        }
      ]
    },
    {
      key: 'operations',
      label: 'Operations',
      items: [
        {
          href: '/build-reports',
          label: 'Build reports',
          icon:
            'M4 19V9M10 19V5M16 19v-7M22 19V3M2 19h22'
        },
        {
          href: '/tokens',
          label: 'Tokens',
          icon:
            'M15 7a5 5 0 1 1-1.46 3.54L21 18v3h-3v-2h-3v-3h-2.17'
        }
      ]
    },
    {
      key: 'support',
      label: 'Support',
      items: [
        {
          href: '/feedback',
          label: 'Feedback',
          icon:
            'M21 15a4 4 0 0 1-4 4H8l-5 3V7a4 4 0 0 1 4-4h10a4 4 0 0 1 4 4v8ZM8 9h8M8 13h5'
        }
      ]
    }
  ] as const;

  let initializedSessionId = '';

  let sidebarOpen = false;
  let sidebarCollapsed = false;
  let sidebarWidth = SIDEBAR_DEFAULT_WIDTH;
  let resizingSidebar = false;

  let accountMenuOpen = false;

  let expandedGroups: Record<string, boolean> = {
    workspace: true,
    registry: true,
    operations: true,
    support: true
  };

  $: currentPath = $page.url.pathname;

  $: accountName =
    $auth.user?.display_name ||
    $auth.user?.name ||
    'Account';

  $: accountEmail =
    $auth.user?.email ||
    '';

  $: initial = (
    accountName ||
    accountEmail ||
    'U'
  )
    .slice(0, 1)
    .toUpperCase();

  $: unread = $notifications.unreadCount;

  $: if (
    $auth.session?.id &&
    initializedSessionId !== $auth.session.id
  ) {
    initializedSessionId = $auth.session.id;

    notifications.loadNotifications();
    notifications.connectRealtime($auth.session.id);
  }

  $: if (!$auth.session && initializedSessionId) {
    initializedSessionId = '';
    notifications.clear();
  }

  function clampSidebarWidth(value: number) {
    return Math.min(
      SIDEBAR_MAX_WIDTH,
      Math.max(SIDEBAR_MIN_WIDTH, value)
    );
  }

  function saveSidebarWidth() {
    localStorage.setItem(
      SIDEBAR_WIDTH_KEY,
      String(sidebarWidth)
    );
  }

  function saveSidebarCollapsed() {
    localStorage.setItem(
      SIDEBAR_COLLAPSED_KEY,
      String(sidebarCollapsed)
    );
  }

  function saveExpandedGroups() {
    localStorage.setItem(
      SIDEBAR_GROUPS_KEY,
      JSON.stringify(expandedGroups)
    );
  }

  function isActive(href: string) {
    return (
      currentPath === href ||
      currentPath.startsWith(`${href}/`)
    );
  }

  function groupContainsActiveRoute(
    group: (typeof navigationGroups)[number]
  ) {
    return group.items.some((item) =>
      isActive(item.href)
    );
  }

  function toggleNavigationGroup(key: string) {
    expandedGroups = {
      ...expandedGroups,
      [key]: !expandedGroups[key]
    };

    saveExpandedGroups();
  }

  function closeMobileSidebar() {
    sidebarOpen = false;
    accountMenuOpen = false;
  }

  function openMobileSidebar() {
    sidebarOpen = true;
  }

  function toggleDesktopSidebar() {
    sidebarCollapsed = !sidebarCollapsed;
    accountMenuOpen = false;

    saveSidebarCollapsed();

    if (sidebarCollapsed) {
      stopSidebarResize();
    }
  }

  function openDesktopSidebar() {
    sidebarCollapsed = false;
    saveSidebarCollapsed();
  }

  function toggleAccountMenu() {
    accountMenuOpen = !accountMenuOpen;
  }

  function startSidebarResize(event: PointerEvent) {
    if (
      event.button !== 0 ||
      sidebarCollapsed ||
      window.innerWidth <= 880
    ) {
      return;
    }

    event.preventDefault();

    resizingSidebar = true;

    document.body.style.cursor = 'col-resize';
    document.body.style.userSelect = 'none';
  }

  function resizeSidebar(event: PointerEvent) {
    if (!resizingSidebar) {
      return;
    }

    sidebarWidth = clampSidebarWidth(
      event.clientX
    );
  }

  function stopSidebarResize() {
    if (!resizingSidebar) {
      return;
    }

    resizingSidebar = false;

    document.body.style.cursor = '';
    document.body.style.userSelect = '';

    saveSidebarWidth();
  }

  function resetSidebarWidth() {
    sidebarWidth = SIDEBAR_DEFAULT_WIDTH;
    saveSidebarWidth();
  }

  function handleResizerKeydown(
    event: KeyboardEvent
  ) {
    const step = event.shiftKey ? 32 : 12;

    if (event.key === 'ArrowLeft') {
      event.preventDefault();

      sidebarWidth = clampSidebarWidth(
        sidebarWidth - step
      );

      saveSidebarWidth();
      return;
    }

    if (event.key === 'ArrowRight') {
      event.preventDefault();

      sidebarWidth = clampSidebarWidth(
        sidebarWidth + step
      );

      saveSidebarWidth();
      return;
    }

    if (event.key === 'Home') {
      event.preventDefault();

      sidebarWidth = SIDEBAR_MIN_WIDTH;
      saveSidebarWidth();
      return;
    }

    if (event.key === 'End') {
      event.preventDefault();

      sidebarWidth = SIDEBAR_MAX_WIDTH;
      saveSidebarWidth();
    }
  }

  function handleWindowKeydown(
    event: KeyboardEvent
  ) {
    if (event.key !== 'Escape') {
      return;
    }

    if (accountMenuOpen) {
      accountMenuOpen = false;
      return;
    }

    if (resizingSidebar) {
      stopSidebarResize();
      return;
    }

    if (sidebarOpen) {
      closeMobileSidebar();
    }
  }

  function logout() {
    auth.clear();
    workspaceContext.clear();
    notifications.clear();

    window.location.href = '/login';
  }

  async function openAccount() {
    accountMenuOpen = false;
    closeMobileSidebar();

    await goto('/account');
  }

  onMount(() => {
    const storedWidth = Number(
      localStorage.getItem(SIDEBAR_WIDTH_KEY)
    );

    if (Number.isFinite(storedWidth)) {
      sidebarWidth = clampSidebarWidth(
        storedWidth
      );
    }

    sidebarCollapsed =
      localStorage.getItem(
        SIDEBAR_COLLAPSED_KEY
      ) === 'true';

    const storedGroups = localStorage.getItem(
      SIDEBAR_GROUPS_KEY
    );

    if (storedGroups) {
      try {
        expandedGroups = {
          ...expandedGroups,
          ...JSON.parse(storedGroups)
        };
      } catch {
        localStorage.removeItem(
          SIDEBAR_GROUPS_KEY
        );
      }
    }

    for (const group of navigationGroups) {
      if (groupContainsActiveRoute(group)) {
        expandedGroups = {
          ...expandedGroups,
          [group.key]: true
        };
      }
    }

    return () => {
      document.body.style.cursor = '';
      document.body.style.userSelect = '';
    };
  });
</script>

<svelte:window
  onkeydown={handleWindowKeydown}
  onpointermove={resizeSidebar}
  onpointerup={stopSidebarResize}
/>

<div
  class="app-shell"
  class:sidebar-collapsed={sidebarCollapsed}
  class:sidebar-resizing={resizingSidebar}
  style={`--sidebar-current-width: ${sidebarWidth}px;`}
>
  {#if $auth.session}
    <header class="mobile-header">
      <button
        class="icon-button"
        type="button"
        aria-label="Open navigation"
        aria-expanded={sidebarOpen}
        aria-controls="app-sidebar"
        onclick={openMobileSidebar}
      >
        <svg viewBox="0 0 24 24" aria-hidden="true">
          <path d="M4 7h16M4 12h16M4 17h16"></path>
        </svg>
      </button>

      <a
        class="mobile-brand"
        href="/dashboard"
      >
        <img
          src="/brand/softadastra-cloud.svg"
          alt=""
          aria-hidden="true"
        />

        <span>Softadastra Cloud</span>
      </a>

      <a
        class="icon-button"
        href="/notifications"
        aria-label="Notifications"
      >
        <svg viewBox="0 0 24 24" aria-hidden="true">
          <path
            d="M18 8a6 6 0 0 0-12 0c0 7-3 7-3 9h18c0-2-3-2-3-9M10 21h4"
          ></path>
        </svg>

        {#if unread > 0}
          <span class="icon-button__count">
            {unread > 99 ? '99+' : unread}
          </span>
        {/if}
      </a>
    </header>

    <button
      class:visible={sidebarOpen}
      class="sidebar-backdrop"
      type="button"
      tabindex="-1"
      aria-label="Close navigation"
      onclick={closeMobileSidebar}
    ></button>

    {#if sidebarCollapsed}
      <button
        class="sidebar-reopen"
        type="button"
        aria-label="Open sidebar"
        title="Open sidebar"
        aria-controls="app-sidebar"
        onclick={openDesktopSidebar}
      >
        <svg viewBox="0 0 24 24" aria-hidden="true">
          <rect
            x="3"
            y="4"
            width="18"
            height="16"
            rx="2"
          ></rect>

          <path d="M9 4v16"></path>
          <path d="m13 9 3 3-3 3"></path>
        </svg>
      </button>
    {/if}

    <aside
      id="app-sidebar"
      class:open={sidebarOpen}
      class="app-sidebar"
    >
      <header class="sidebar-header">
        <a
          class="sidebar-brand"
          href="/dashboard"
          aria-label="Softadastra Cloud dashboard"
          onclick={closeMobileSidebar}
        >
          <img
            src="/brand/softadastra-cloud.svg"
            alt=""
            aria-hidden="true"
          />

          <span class="sidebar-brand__text">
            <strong>Softadastra Cloud</strong>
            <small>C++ workspace</small>
          </span>
        </a>

        <a
          class:active={isActive('/notifications')}
          class="icon-button header-action"
          href="/notifications"
          aria-label="Notifications"
          title="Notifications"
          onclick={closeMobileSidebar}
        >
          <svg viewBox="0 0 24 24" aria-hidden="true">
            <path
              d="M18 8a6 6 0 0 0-12 0c0 7-3 7-3 9h18c0-2-3-2-3-9M10 21h4"
            ></path>
          </svg>

          {#if unread > 0}
            <span class="icon-button__count">
              {unread > 99 ? '99+' : unread}
            </span>
          {/if}
        </a>

        <button
          class="icon-button sidebar-collapse"
          type="button"
          aria-label="Close sidebar"
          title="Close sidebar"
          aria-controls="app-sidebar"
          aria-expanded={!sidebarCollapsed}
          onclick={toggleDesktopSidebar}
        >
          <svg viewBox="0 0 24 24" aria-hidden="true">
            <rect
              x="3"
              y="4"
              width="18"
              height="16"
              rx="2"
            ></rect>

            <path d="M9 4v16"></path>
            <path d="m16 9-3 3 3 3"></path>
          </svg>
        </button>

        <button
          class="icon-button sidebar-close"
          type="button"
          aria-label="Close navigation"
          onclick={closeMobileSidebar}
        >
          <svg viewBox="0 0 24 24" aria-hidden="true">
            <path d="m6 6 12 12M18 6 6 18"></path>
          </svg>
        </button>
      </header>

      {#if $workspaceContext.workspaces.length > 0}
        <section class="workspace-picker">
          <p class="workspace-picker__label">
            Current workspace
          </p>

          <select
            aria-label="Current workspace"
            value={$workspaceContext.selectedWorkspace?.id ?? ''}
            onchange={(event) =>
              workspaceContext.setSelectedWorkspace(
                event.currentTarget.value
              )}
          >
            {#each $workspaceContext.workspaces as workspace (workspace.id)}
              <option value={workspace.id}>
                {workspace.name}
              </option>
            {/each}
          </select>

          {#if $workspaceContext.selectedWorkspace}
            <div class="workspace-picker__meta">
              <RoleBadge
                role={$workspaceContext.selectedWorkspace.current_user_role ?? 'viewer'}
              />

              <AccessScopeBadge
                scope={$workspaceContext.selectedWorkspace.access_scope ?? 'entire_workspace'}
              />
            </div>
          {/if}
        </section>
      {/if}

      <nav
        class="sidebar-nav"
        aria-label="Primary navigation"
      >
        {#each navigationGroups as group (group.key)}
          <section
            class:contains-active={groupContainsActiveRoute(group)}
            class="nav-group"
          >
            <button
              class="nav-group__toggle"
              type="button"
              aria-expanded={expandedGroups[group.key]}
              aria-controls={`sidebar-group-${group.key}`}
              onclick={() =>
                toggleNavigationGroup(group.key)}
            >
              <span>{group.label}</span>

              <svg
                class:expanded={expandedGroups[group.key]}
                viewBox="0 0 24 24"
                aria-hidden="true"
              >
                <path d="m9 7 5 5-5 5"></path>
              </svg>
            </button>

            {#if expandedGroups[group.key]}
              <div
                id={`sidebar-group-${group.key}`}
                class="nav-group__items"
              >
                {#each group.items as item (item.href)}
                  <a
                    class:active={isActive(item.href)}
                    class="nav-item"
                    href={item.href}
                    aria-current={
                      isActive(item.href)
                        ? 'page'
                        : undefined
                    }
                    onclick={closeMobileSidebar}
                  >
                    <svg
                      viewBox="0 0 24 24"
                      aria-hidden="true"
                    >
                      <path d={item.icon}></path>
                    </svg>

                    <span>{item.label}</span>
                  </a>
                {/each}
              </div>
            {/if}
          </section>
        {/each}
      </nav>

      <footer class="sidebar-footer">
        <div class="account-block">
          {#if accountMenuOpen}
            <div
              class="account-menu"
              aria-label="Account menu"
            >
              <button
                class="account-menu__item"
                type="button"
                onclick={openAccount}
              >
                <svg
                  viewBox="0 0 24 24"
                  aria-hidden="true"
                >
                  <circle cx="12" cy="8" r="4"></circle>
                  <path
                    d="M4 21a8 8 0 0 1 16 0"
                  ></path>
                </svg>

                <span>Account settings</span>
              </button>

              <div class="account-menu__separator"></div>

              <button
                class="account-menu__item account-menu__item--danger"
                type="button"
                onclick={logout}
              >
                <svg
                  viewBox="0 0 24 24"
                  aria-hidden="true"
                >
                  <path
                    d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4M16 17l5-5-5-5M21 12H9"
                  ></path>
                </svg>

                <span>Sign out</span>
              </button>
            </div>
          {/if}

          <button
            class:open={accountMenuOpen}
            class="account-row"
            type="button"
            title={accountEmail}
            aria-label="Open account menu"
            aria-expanded={accountMenuOpen}
            onclick={toggleAccountMenu}
          >
            <span
              class="account-row__avatar"
              aria-hidden="true"
            >
              {initial}
            </span>

            <span class="account-row__identity">
              <strong>{accountName}</strong>

              {#if accountEmail}
                <small>{accountEmail}</small>
              {/if}
            </span>

            <svg
              class:expanded={accountMenuOpen}
              class="account-row__chevron"
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path d="m9 7 5 5-5 5"></path>
            </svg>
          </button>
        </div>
      </footer>

      <button
        class="sidebar-resizer"
        type="button"
        aria-label="Resize sidebar"
        title="Drag to resize. Double-click to reset."
        onpointerdown={startSidebarResize}
        onkeydown={handleResizerKeydown}
        ondblclick={resetSidebarWidth}
      ></button>
    </aside>
  {/if}

  <main
    class:authenticated={$auth.session}
    class="app-main"
  >
    <div class="app-main__inner">
      <slot />
    </div>
  </main>
</div>

<style>
  .app-shell {
    min-height: 100vh;
  }

  .app-sidebar {
    width: var(
      --sidebar-current-width,
      var(--sidebar-w)
    );
    overflow-x: hidden;
    background: var(--bg-soft);
    transition:
      width var(--speed) var(--ease),
      transform var(--speed) var(--ease);
  }

  .app-main.authenticated {
    margin-left: var(
      --sidebar-current-width,
      var(--sidebar-w)
    );
    transition:
      margin-left var(--speed) var(--ease),
      padding-left var(--speed) var(--ease);
  }

  .sidebar-resizing .app-sidebar,
  .sidebar-resizing .app-main.authenticated {
    transition: none;
  }

  .sidebar-collapsed .app-sidebar {
    transform: translateX(calc(-100% - 8px));
    pointer-events: none;
  }

  .sidebar-collapsed .app-main.authenticated {
    margin-left: 0;
    padding-left: 72px;
  }

  /* Header */

  .sidebar-header {
    min-height: 62px;
    padding: 10px 10px 9px 12px;
    border-bottom: 1px solid var(--line-soft);
  }

  .sidebar-brand {
    display: flex;
    min-width: 0;
    flex: 1 1 auto;
    align-items: center;
    gap: 10px;
    padding: 5px 6px;
  }

  .sidebar-brand img {
    width: 26px;
    height: 26px;
    flex: 0 0 auto;
    object-fit: contain;
  }

  .sidebar-brand__text {
    display: grid;
    min-width: 0;
    gap: 1px;
  }

  .sidebar-brand__text strong {
    overflow: hidden;
    color: var(--text);
    font-size: 12.5px;
    font-weight: 650;
    letter-spacing: -0.01em;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .sidebar-brand__text small {
    overflow: hidden;
    color: var(--text-faint);
    font-size: 9.5px;
    font-weight: 450;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .header-action,
  .sidebar-collapse {
    flex: 0 0 auto;
  }

  /* Workspace */

  .workspace-picker {
    display: grid;
    gap: 8px;
    margin: 10px 10px 4px;
    padding: 11px;
    border: 1px solid var(--line-soft);
    border-radius: var(--radius-sm);
    background: var(--bg-ink-soft);
  }

  .workspace-picker__label {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 9px;
    font-weight: 550;
    letter-spacing: 0.1em;
    text-transform: uppercase;
  }

  .workspace-picker select {
    width: 100%;
    min-height: 34px;
    border-color: var(--line-strong);
    background: var(--bg-panel-strong);
    font-size: 12px;
    font-weight: 550;
  }

  .workspace-picker__meta {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 5px;
    flex-wrap: wrap;
  }

  /* Navigation */

  .sidebar-nav {
    display: grid;
    min-height: 0;
    flex: 1 1 auto;
    gap: 8px;
    align-content: start;
    overflow-x: hidden;
    overflow-y: auto;
    padding: 12px 10px 22px;
  }

  .nav-group {
    display: grid;
    gap: 3px;
  }

  .nav-group__toggle {
    display: flex;
    width: 100%;
    min-height: 28px;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
    border: 0;
    border-radius: var(--radius-sm);
    background: transparent;
    color: var(--text-muted);
    padding: 3px 8px;
    font-family: var(--font-mono);
    font-size: 9.5px;
    font-weight: 550;
    letter-spacing: 0.11em;
    text-align: left;
    text-transform: uppercase;
  }

  .nav-group__toggle:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text-soft);
    transform: none;
  }

  .nav-group.contains-active
    .nav-group__toggle {
    color: var(--text-soft);
  }

  .nav-group__toggle svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
    transition: transform var(--speed) var(--ease);
  }

  .nav-group__toggle svg.expanded {
    transform: rotate(90deg);
  }

  .nav-group__items {
    display: grid;
    gap: 2px;
  }

  .nav-item {
    position: relative;
    display: grid;
    grid-template-columns: 18px minmax(0, 1fr);
    gap: 9px;
    align-items: center;
    min-height: 36px;
    padding: 6px 9px;
    border-radius: var(--radius-sm);
    color: var(--text-soft);
    font-size: 12.5px;
    font-weight: 500;
  }

  .nav-item:hover {
    background: var(--bg-elevated);
    color: var(--text);
  }

  .nav-item.active {
    background: var(--brand-faint);
    color: var(--brand-bright);
    font-weight: 600;
  }

  .nav-item.active::before {
    content: "";
    position: absolute;
    top: 7px;
    bottom: 7px;
    left: 0;
    width: 2px;
    border-radius: 0 2px 2px 0;
    background: var(--brand);
  }

  .nav-item svg {
    width: 16px;
    height: 16px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.55;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .nav-item span {
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  /* Account */

  .sidebar-footer {
    position: relative;
    display: block;
    padding: 10px 11px 12px;
    border-top: 1px solid var(--line-soft);
    background: var(--bg-soft);
  }

  .account-block {
    position: relative;
    width: 100%;
  }

  .account-row {
    display: grid;
    grid-template-columns:
      32px
      minmax(0, 1fr)
      16px;
    gap: 10px;
    align-items: center;
    width: 100%;
    min-height: 52px;
    border: 1px solid transparent;
    border-radius: var(--radius-sm);
    background: transparent;
    color: var(--text);
    padding: 7px 8px;
    text-align: left;
  }

  .account-row:hover:not(:disabled),
  .account-row.open {
    border-color: var(--line-soft);
    background: var(--bg-elevated);
    transform: none;
  }

  .account-row__avatar {
    display: grid;
    width: 32px;
    height: 32px;
    place-items: center;
    border-radius: 50%;
    background: var(--brand);
    color: var(--brand-ink);
    font-size: 11px;
    font-weight: 700;
  }

  .account-row__identity {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .account-row__identity strong {
    overflow: hidden;
    color: var(--text);
    font-size: 11.5px;
    font-weight: 600;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .account-row__identity small {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 9.5px;
    font-weight: 400;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .account-row__chevron {
    width: 15px;
    height: 15px;
    fill: none;
    stroke: var(--text-muted);
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
    transition: transform var(--speed) var(--ease);
  }

  .account-row__chevron.expanded {
    transform: rotate(-90deg);
  }

  .account-menu {
    position: absolute;
    right: 0;
    bottom: calc(100% + 8px);
    left: 0;
    z-index: 8;
    display: grid;
    gap: 3px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel-strong);
    padding: 6px;
    box-shadow: var(--shadow-panel);
  }

  .account-menu__item {
    display: grid;
    grid-template-columns: 18px minmax(0, 1fr);
    gap: 9px;
    align-items: center;
    width: 100%;
    min-height: 34px;
    border: 0;
    background: transparent;
    color: var(--text-soft);
    padding: 6px 8px;
    font-size: 11.5px;
    font-weight: 550;
    text-align: left;
  }

  .account-menu__item:hover:not(:disabled) {
    background: var(--bg-elevated);
    color: var(--text);
    transform: none;
  }

  .account-menu__item svg {
    width: 16px;
    height: 16px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.6;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .account-menu__item--danger {
    color: var(--danger);
  }

  .account-menu__item--danger:hover:not(:disabled) {
    background: var(--danger-faint);
    color: var(--danger);
  }

  .account-menu__separator {
    height: 1px;
    margin: 2px 3px;
    background: var(--line-soft);
  }

  /* Collapse and resize */

  .sidebar-reopen {
    position: fixed;
    top: 16px;
    left: 16px;
    z-index: 38;
    display: grid;
    width: 36px;
    height: 36px;
    min-height: 36px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    background: var(--bg-panel);
    color: var(--text-soft);
    padding: 0;
    box-shadow: var(--shadow-soft);
  }

  .sidebar-reopen:hover:not(:disabled) {
    border-color: rgba(255, 255, 255, 0.18);
    background: var(--bg-elevated);
    color: var(--text);
  }

  .sidebar-reopen svg {
    width: 18px;
    height: 18px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.6;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .sidebar-resizer {
    position: absolute;
    top: 0;
    right: -3px;
    bottom: 0;
    z-index: 3;
    width: 7px;
    height: 100%;
    min-height: 0;
    border: 0;
    border-radius: 0;
    background: transparent;
    padding: 0;
    cursor: col-resize;
    touch-action: none;
  }

  .sidebar-resizer::before {
    content: "";
    position: absolute;
    top: 0;
    right: 3px;
    bottom: 0;
    width: 1px;
    background: transparent;
    transition: background var(--speed) var(--ease);
  }

  .sidebar-resizer:hover:not(:disabled),
  .sidebar-resizer:active,
  .sidebar-resizer:focus-visible {
    background: transparent;
    transform: none;
  }

  .sidebar-resizer:hover::before,
  .sidebar-resizer:focus-visible::before,
  .sidebar-resizing
    .sidebar-resizer::before {
    background: rgba(255, 255, 255, 0.24);
  }

  .sidebar-resizer:focus-visible {
    outline: none;
    box-shadow: none;
  }

  @media (max-width: 880px) {
    .sidebar-reopen,
    .sidebar-collapse,
    .sidebar-resizer {
      display: none;
    }

    .app-sidebar {
      width: min(88vw, 320px);
    }

    .sidebar-collapsed .app-sidebar {
      transform: translateX(-100%);
      pointer-events: auto;
    }

    .app-sidebar.open,
    .sidebar-collapsed .app-sidebar.open {
      transform: translateX(0);
    }

    .app-main.authenticated,
    .sidebar-collapsed
      .app-main.authenticated {
      margin-left: 0;
      padding: 20px 14px 48px;
    }

    .sidebar-header {
      min-height: 58px;
    }

    .workspace-picker {
      margin-top: 8px;
    }
  }
</style>
