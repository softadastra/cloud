<script lang="ts">
  import { browser } from '$app/environment';
  import { goto } from '$app/navigation';
  import { page } from '$app/stores';
  import { onMount } from 'svelte';

  import AccessScopeBadge from '$lib/components/AccessScopeBadge.svelte';
  import RoleBadge from '$lib/components/RoleBadge.svelte';
  import WorkspaceAvatar from '$lib/components/WorkspaceAvatar.svelte';

  import { auth } from '$lib/stores/auth';
  import { notifications } from '$lib/stores/notifications';
  import { requestPageRefresh } from '$lib/stores/pageRefresh';
  import { workspaceContext } from '$lib/stores/workspace';

  const API_BASE_URL =
    import.meta.env.VITE_API_BASE_URL ??
    (
      browser
        ? `${window.location.protocol}//${window.location.hostname}:8080`
        : ''
    );

  const SIDEBAR_COLLAPSED_KEY =
    'softadastra-cloud:sidebar-collapsed';

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
          label: 'CLI tokens',
          icon:
            'M15 7a5 5 0 1 1-1.46 3.54L21 18v3h-3v-2h-3v-3h-2.17'
        }
      ]
    },
    {
      key: 'activity',
      label: 'Activity',
      items: [
        {
          href: '/notifications',
          label: 'Notifications',
          icon:
            'M18 8a6 6 0 0 0-12 0c0 7-3 7-3 9h18c0-2-3-2-3-9M10 21h4'
        },
        {
          href: '/feedback',
          label: 'Feedback',
          icon:
            'M21 15a4 4 0 0 1-4 4H8l-5 3V7a4 4 0 0 1 4-4h10a4 4 0 0 1 4 4v8ZM8 9h8M8 13h5'
        }
      ]
    },
    {
      key: 'admin',
      label: 'Internal',
      items: [
        {
          href: '/admin',
          label: 'Administration',
          icon:
            'M12 3 20 7v5c0 5-3.5 8-8 9-4.5-1-8-4-8-9V7l8-4Zm0 6v5M12 17h.01'
        }
      ]
    }
  ] as const;

  const supplementalPageLabels = [
    {
      path: '/workspaces',
      label: 'Workspaces'
    },
    {
      path: '/package-versions',
      label: 'Package versions'
    },
    {
      path: '/account',
      label: 'Account'
    }
  ] as const;

  const workspaceScopedRoutes =
    new Set([
      '/dashboard',
      '/workspaces',
      '/projects',
      '/members',
      '/packages',
      '/package-versions',
      '/lockfiles',
      '/build-reports',
      '/tokens'
    ]);

  let initializedSessionId = '';
  let verifiedSessionId = '';
  let verifyingSession = false;

  let sidebarOpen = false;
  let sidebarCollapsed = false;

  let workspaceMenuOpen = false;
  let accountMenuOpen = false;

  let lastPath = '';

  $: currentPath =
    $page.url.pathname;

  $: standalonePage =
    currentPath === '/' ||
    currentPath === '/login' ||
    currentPath === '/register' ||
    currentPath === '/support' ||
    currentPath === '/supporters' ||
    currentPath === '/404' ||
    currentPath.startsWith('/u/') ||
    currentPath.startsWith('/p/');

  $: visibleNavigationGroups =
    navigationGroups.filter(
      (group) =>
        group.key !== 'admin' ||
        Boolean(
          $auth.user?.platform_admin
        )
    );

  $: accountName =
    $auth.user?.display_name ||
    $auth.user?.name ||
    'Account';

  $: accountEmail =
    $auth.user?.email || '';

  $: initial = (
    accountName ||
    accountEmail ||
    'U'
  )
    .slice(0, 1)
    .toUpperCase();

  $: accountAvatarUrl =
    $auth.user?.avatar_url
      ? $auth.user.avatar_url.startsWith(
          'http'
        )
        ? $auth.user.avatar_url
        : `${API_BASE_URL}${$auth.user.avatar_url}`
      : '';

  $: publicProfileHref =
    $auth.user?.public_profile_enabled &&
    $auth.user?.username
      ? `/u/${$auth.user.username}`
      : '';

  $: selectedWorkspace =
    $workspaceContext.selectedWorkspace ??
    null;

  $: workspaces =
    $workspaceContext.workspaces;

  $: unread =
    $notifications.unreadCount;

  $: mobilePageTitle =
    currentNavigationLabel();

  $: menuOpen =
    workspaceMenuOpen ||
    accountMenuOpen;

  $: if (
    currentPath !== lastPath
  ) {
    lastPath = currentPath;

    sidebarOpen = false;
    workspaceMenuOpen = false;
    accountMenuOpen = false;
  }

  $: if (
    $auth.session?.id &&
    verifiedSessionId !==
      $auth.session.id &&
    !verifyingSession
  ) {
    verifyingSession = true;

    const sessionId =
      $auth.session.id;

    auth
      .refreshCurrentUser()
      .then((status) => {
        verifyingSession = false;

        if (
          status === 'ok' ||
          status === 'unreachable'
        ) {
          verifiedSessionId =
            sessionId;
        } else if (
          status === 'invalid' &&
          !standalonePage
        ) {
          void goto(
            loginHrefForCurrentPage()
          );
        }
      });
  }

  $: if (
    $auth.session?.id &&
    initializedSessionId !==
      $auth.session.id
  ) {
    initializedSessionId =
      $auth.session.id;

    void notifications.loadNotifications();

    notifications.connectRealtime(
      $auth.session.id
    );
  }

  $: if (
    !$auth.session &&
    initializedSessionId
  ) {
    initializedSessionId = '';
    verifiedSessionId = '';

    notifications.clear();
  }

  function routePath(
    href: string
  ) {
    return href.split('?', 1)[0];
  }

  function isActive(
    href: string
  ) {
    const path =
      routePath(href);

    return (
      currentPath === path ||
      currentPath.startsWith(
        `${path}/`
      )
    );
  }

  function currentNavigationLabel() {
    for (
      const group of
      navigationGroups
    ) {
      for (
        const item of
        group.items
      ) {
        if (isActive(item.href)) {
          return item.label;
        }
      }
    }

    for (
      const item of
      supplementalPageLabels
    ) {
      if (
        currentPath === item.path ||
        currentPath.startsWith(
          `${item.path}/`
        )
      ) {
        return item.label;
      }
    }

    return 'Softadastra Cloud';
  }

  function loginHrefForCurrentPage() {
    const currentHref =
      `${$page.url.pathname}` +
      `${$page.url.search}`;

    if (
      !currentHref ||
      currentHref === '/login'
    ) {
      return '/login';
    }

    return (
      '/login?redirect=' +
      encodeURIComponent(
        currentHref
      )
    );
  }

  function navigationHref(
    href: string
  ) {
    const workspaceId =
      selectedWorkspace?.id ?? '';

    const path =
      routePath(href);

    if (
      !workspaceId ||
      !workspaceScopedRoutes.has(path)
    ) {
      return href;
    }

    const queryStart =
      href.indexOf('?');

    const params =
      new URLSearchParams(
        queryStart === -1
          ? ''
          : href.slice(
              queryStart + 1
            )
      );

    params.set(
      'workspace_id',
      workspaceId
    );

    return (
      `${path}?` +
      params.toString()
    );
  }

  async function selectWorkspace(
    workspaceId: string
  ) {
    workspaceMenuOpen = false;

    workspaceContext
      .setSelectedWorkspace(
        workspaceId
      );

    if (
      !workspaceId ||
      !workspaceScopedRoutes.has(
        currentPath
      )
    ) {
      return;
    }

    const params =
      new URLSearchParams(
        $page.url.search
      );

    params.set(
      'workspace_id',
      workspaceId
    );

    const nextHref =
      `${currentPath}?` +
      params.toString();

    const currentHref =
      `${$page.url.pathname}` +
      `${$page.url.search}`;

    if (
      nextHref !== currentHref
    ) {
      await goto(
        nextHref,
        {
          keepFocus: true,
          noScroll: true
        }
      );
    }
  }

  function closeMenus() {
    workspaceMenuOpen = false;
    accountMenuOpen = false;
  }

  function closeMobileSidebar() {
    sidebarOpen = false;
    closeMenus();
  }

  function toggleWorkspaceMenu() {
    workspaceMenuOpen =
      !workspaceMenuOpen;

    accountMenuOpen = false;
  }

  function toggleAccountMenu() {
    accountMenuOpen =
      !accountMenuOpen;

    workspaceMenuOpen = false;
  }

  function toggleDesktopSidebar() {
    sidebarCollapsed =
      !sidebarCollapsed;

    closeMenus();

    localStorage.setItem(
      SIDEBAR_COLLAPSED_KEY,
      String(sidebarCollapsed)
    );
  }

  async function refreshPageData() {
    closeMenus();

    if ($auth.session?.id) {
      await auth.refreshCurrentUser();

      void notifications
        .loadNotifications();
    }

    requestPageRefresh('manual');
  }

  function logout() {
    auth.clear();
    workspaceContext.clear();
    notifications.clear();

    window.location.href =
      '/login';
  }

  function handleWindowKeydown(
    event: KeyboardEvent
  ) {
    if (event.key !== 'Escape') {
      return;
    }

    if (menuOpen) {
      closeMenus();
      return;
    }

    if (sidebarOpen) {
      closeMobileSidebar();
    }
  }

  onMount(() => {
    sidebarCollapsed =
      localStorage.getItem(
        SIDEBAR_COLLAPSED_KEY
      ) === 'true';
  });
</script>

<svelte:window
  onkeydown={handleWindowKeydown}
/>

{#if standalonePage}
  <slot />
{:else}
  <div
    class:sidebar-collapsed={
      sidebarCollapsed
    }
    class="app-shell"
  >
    {#if $auth.session}
      <!-- ===============================================
           MOBILE HEADER
           =============================================== -->
      <header class="mobile-header">
        <button
          class="icon-button"
          type="button"
          aria-label="Open navigation"
          aria-expanded={sidebarOpen}
          aria-controls="app-sidebar"
          onclick={() => {
            sidebarOpen = true;
          }}
        >
          <svg
            viewBox="0 0 24 24"
            aria-hidden="true"
          >
            <path d="M4 7h16"></path>
            <path d="M4 12h16"></path>
            <path d="M4 17h16"></path>
          </svg>
        </button>

        <div class="mobile-page-title">
          <img
            src="/brand/softadastra-cloud.svg"
            alt=""
            aria-hidden="true"
          />

          <span>
            {mobilePageTitle}
          </span>
        </div>

        <a
          class:active={
            isActive('/notifications')
          }
          class="icon-button"
          href="/notifications"
          aria-label="Notifications"
        >
          <svg
            viewBox="0 0 24 24"
            aria-hidden="true"
          >
            <path
              d="M18 8a6 6 0 0 0-12 0c0 7-3 7-3 9h18c0-2-3-2-3-9M10 21h4"
            ></path>
          </svg>

          {#if unread > 0}
            <span class="icon-button__count">
              {unread > 99
                ? '99+'
                : unread}
            </span>
          {/if}
        </a>
      </header>

      <!-- ===============================================
           MOBILE BACKDROP
           =============================================== -->
      <button
        class:visible={sidebarOpen}
        class="sidebar-backdrop"
        type="button"
        tabindex="-1"
        aria-label="Close navigation"
        onclick={closeMobileSidebar}
      ></button>

      {#if menuOpen}
        <button
          class="menu-dismiss"
          type="button"
          tabindex="-1"
          aria-label="Close menu"
          onclick={closeMenus}
        ></button>
      {/if}

      <!-- ===============================================
           SIDEBAR
           =============================================== -->
      <aside
        id="app-sidebar"
        class:open={sidebarOpen}
        class="app-sidebar"
      >
        <header class="sidebar-header">
          <a
            class="sidebar-brand"
            href={navigationHref(
              '/dashboard'
            )}
            aria-label="Softadastra Cloud dashboard"
            title="Softadastra Cloud"
            onclick={closeMobileSidebar}
          >
            <img
              src="/brand/softadastra-cloud.svg"
              alt=""
              aria-hidden="true"
            />

            <span class="sidebar-brand__text">
              <strong>
                Softadastra Cloud
              </strong>

              <small>
                C++ workspace
              </small>
            </span>
          </a>

          <button
            class="icon-button sidebar-collapse"
            type="button"
            aria-label={
              sidebarCollapsed
                ? 'Expand sidebar'
                : 'Collapse sidebar'
            }
            title={
              sidebarCollapsed
                ? 'Expand sidebar'
                : 'Collapse sidebar'
            }
            onclick={toggleDesktopSidebar}
          >
            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <rect
                x="3"
                y="4"
                width="18"
                height="16"
                rx="2"
              ></rect>

              <path d="M9 4v16"></path>

              {#if sidebarCollapsed}
                <path
                  d="m13 9 3 3-3 3"
                ></path>
              {:else}
                <path
                  d="m16 9-3 3 3 3"
                ></path>
              {/if}
            </svg>
          </button>

          <button
            class="icon-button sidebar-close"
            type="button"
            aria-label="Close navigation"
            onclick={closeMobileSidebar}
          >
            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path
                d="m6 6 12 12"
              ></path>

              <path
                d="M18 6 6 18"
              ></path>
            </svg>
          </button>
        </header>

        <!-- =============================================
             WORKSPACE CONTEXT
             ============================================= -->
        <section class="workspace-control">
          {#if
            workspaces.length > 0 &&
            selectedWorkspace
          }
            <button
              class:open={
                workspaceMenuOpen
              }
              class="workspace-trigger"
              type="button"
              title={
                selectedWorkspace.name
              }
              aria-label="Change workspace"
              aria-expanded={
                workspaceMenuOpen
              }
              onclick={toggleWorkspaceMenu}
            >
              <WorkspaceAvatar
                workspace={selectedWorkspace}
                size="sm"
              />

              <span class="workspace-trigger__identity">
                <small>
                  Current workspace
                </small>

                <strong>
                  {selectedWorkspace.name}
                </strong>
              </span>

              <svg
                class:expanded={
                  workspaceMenuOpen
                }
                class="workspace-trigger__chevron"
                viewBox="0 0 24 24"
                aria-hidden="true"
              >
                <path
                  d="m9 7 5 5-5 5"
                ></path>
              </svg>
            </button>

            {#if workspaceMenuOpen}
              <div
                class="workspace-menu"
                aria-label="Workspace switcher"
              >
                <header class="workspace-menu__header">
                  <div>
                    <span>
                      Workspaces
                    </span>

                    <small>
                      Switch active context
                    </small>
                  </div>

                  <a
                    href={navigationHref(
                      '/workspaces'
                    )}
                    onclick={closeMobileSidebar}
                  >
                    Manage
                  </a>
                </header>

                <div class="workspace-menu__list">
                  {#each workspaces as workspace (workspace.id)}
                    <button
                      class:selected={
                        workspace.id ===
                        selectedWorkspace.id
                      }
                      type="button"
                      onclick={() =>
                        void selectWorkspace(
                          workspace.id
                        )}
                    >
                      <WorkspaceAvatar
                        workspace={workspace}
                        size="sm"
                      />

                      <span>
                        <strong>
                          {workspace.name}
                        </strong>

                        <small>
                          {workspace.current_user_role ??
                            'viewer'}
                        </small>
                      </span>

                      {#if
                        workspace.id ===
                        selectedWorkspace.id
                      }
                        <svg
                          class="workspace-selected"
                          viewBox="0 0 24 24"
                          aria-hidden="true"
                        >
                          <path
                            d="m5 12 4 4L19 6"
                          ></path>
                        </svg>
                      {/if}
                    </button>
                  {/each}
                </div>

                <footer class="workspace-menu__footer">
                  <RoleBadge
                    role={selectedWorkspace.current_user_role ??
                      'viewer'}
                  />

                  <AccessScopeBadge
                    scope={selectedWorkspace.access_scope ??
                      'entire_workspace'}
                  />
                </footer>
              </div>
            {/if}
          {:else}
            <a
              class="workspace-create"
              href="/workspaces"
              title="Create workspace"
              onclick={closeMobileSidebar}
            >
              <span aria-hidden="true">
                <svg viewBox="0 0 24 24">
                  <path d="M12 5v14"></path>
                  <path d="M5 12h14"></path>
                </svg>
              </span>

              <span>
                <strong>
                  Create workspace
                </strong>

                <small>
                  Start your first environment
                </small>
              </span>
            </a>
          {/if}
        </section>

        <!-- =============================================
             PRIMARY NAVIGATION
             ============================================= -->
        <nav
          class="sidebar-nav"
          aria-label="Primary navigation"
        >
          {#each visibleNavigationGroups as group (group.key)}
            <section class="nav-group">
              <p class="nav-group__label">
                {group.label}
              </p>

              <div class="nav-group__items">
                {#each group.items as item (item.href)}
                  <a
                    class:active={
                      isActive(item.href)
                    }
                    class="nav-item"
                    href={navigationHref(
                      item.href
                    )}
                    title={item.label}
                    aria-current={
                      isActive(item.href)
                        ? 'page'
                        : undefined
                    }
                    onclick={closeMobileSidebar}
                  >
                    <span
                      class="nav-item__icon"
                      aria-hidden="true"
                    >
                      <svg
                        viewBox="0 0 24 24"
                      >
                        <path
                          d={item.icon}
                        ></path>
                      </svg>

                      {#if
                        item.href ===
                          '/notifications' &&
                        unread > 0
                      }
                        <span
                          class="nav-item__notification-dot"
                        ></span>
                      {/if}
                    </span>

                    <span class="nav-item__label">
                      {item.label}
                    </span>

                    {#if
                      item.href ===
                        '/notifications' &&
                      unread > 0
                    }
                      <span class="nav-item__count">
                        {unread > 99
                          ? '99+'
                          : unread}
                      </span>
                    {/if}
                  </a>
                {/each}
              </div>
            </section>
          {/each}
        </nav>

        <!-- =============================================
             ACCOUNT
             ============================================= -->
        <footer class="sidebar-footer">
          <div class="account-block">
            {#if accountMenuOpen}
              <div
                class="account-menu"
                aria-label="Account menu"
              >
                <header class="account-menu__identity">
                  <span
                    class="account-menu__avatar"
                    aria-hidden="true"
                  >
                    {#if accountAvatarUrl}
                      <img
                        src={accountAvatarUrl}
                        alt=""
                      />
                    {:else}
                      {initial}
                    {/if}
                  </span>

                  <span>
                    <strong>
                      {accountName}
                    </strong>

                    <small>
                      {accountEmail}
                    </small>
                  </span>
                </header>

                <a
                  class="account-menu__item"
                  href="/account"
                  onclick={closeMobileSidebar}
                >
                  <svg
                    viewBox="0 0 24 24"
                    aria-hidden="true"
                  >
                    <circle
                      cx="12"
                      cy="8"
                      r="4"
                    ></circle>

                    <path
                      d="M4 21a8 8 0 0 1 16 0"
                    ></path>
                  </svg>

                  <span>
                    Account settings
                  </span>
                </a>

                {#if publicProfileHref}
                  <a
                    class="account-menu__item"
                    href={publicProfileHref}
                    onclick={closeMobileSidebar}
                  >
                    <svg
                      viewBox="0 0 24 24"
                      aria-hidden="true"
                    >
                      <circle
                        cx="12"
                        cy="12"
                        r="9"
                      ></circle>

                      <path
                        d="M3 12h18"
                      ></path>

                      <path
                        d="M12 3a15 15 0 0 1 0 18"
                      ></path>

                      <path
                        d="M12 3a15 15 0 0 0 0 18"
                      ></path>
                    </svg>

                    <span>
                      Public profile
                    </span>
                  </a>
                {/if}

                <button
                  class="account-menu__item"
                  type="button"
                  onclick={() =>
                    void refreshPageData()}
                >
                  <svg
                    viewBox="0 0 24 24"
                    aria-hidden="true"
                  >
                    <path
                      d="M21 12a9 9 0 0 1-15.5 6.2L3 15.7"
                    ></path>

                    <path
                      d="M3 21v-5.3h5.3"
                    ></path>

                    <path
                      d="M3 12A9 9 0 0 1 18.5 5.8L21 8.3"
                    ></path>

                    <path
                      d="M21 3v5.3h-5.3"
                    ></path>
                  </svg>

                  <span>
                    Refresh data
                  </span>
                </button>

                <div
                  class="account-menu__separator"
                ></div>

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
                      d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4"
                    ></path>

                    <path
                      d="m16 17 5-5-5-5"
                    ></path>

                    <path
                      d="M21 12H9"
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
                {#if accountAvatarUrl}
                  <img
                    src={accountAvatarUrl}
                    alt=""
                  />
                {:else}
                  {initial}
                {/if}
              </span>

              <span class="account-row__identity">
                <strong>
                  {accountName}
                </strong>

                {#if accountEmail}
                  <small>
                    {accountEmail}
                  </small>
                {/if}
              </span>

              <svg
                class:expanded={
                  accountMenuOpen
                }
                class="account-row__chevron"
                viewBox="0 0 24 24"
                aria-hidden="true"
              >
                <path
                  d="m9 7 5 5-5 5"
                ></path>
              </svg>
            </button>
          </div>
        </footer>
      </aside>
    {/if}

    <!-- ===============================================
         PAGE CONTENT
         =============================================== -->
    <main
      class:authenticated={
        Boolean($auth.session)
      }
      class="app-main"
    >
      <div class="app-main__inner">
        <slot />
      </div>
    </main>
  </div>
{/if}

<style>
  .app-shell {
    min-height: 100vh;
    background: var(--bg);
  }

  /* =====================================================
     SHARED ICON BUTTON
     ===================================================== */

  .icon-button {
    position: relative;
    display: grid;
    width: 36px;
    height: 36px;
    min-height: 36px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid transparent;
    border-radius: 8px;
    background: transparent;
    color: var(--text-muted);
    padding: 0;
    font: inherit;
    cursor: pointer;
    text-decoration: none;
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease),
      color var(--speed) var(--ease);
  }

  .icon-button:hover,
  .icon-button.active {
    border-color: var(--line);
    background: var(--bg-elevated);
    color: var(--text);
  }

  .icon-button svg {
    width: 18px;
    height: 18px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .icon-button__count {
    position: absolute;
    top: -4px;
    right: -5px;
    display: grid;
    min-width: 18px;
    height: 18px;
    place-items: center;
    border: 2px solid var(--bg);
    border-radius: 999px;
    background: var(--brand);
    color: var(--brand-ink);
    padding-inline: 4px;
    font-family: var(--font-mono);
    font-size: 9px;
    font-weight: 750;
  }

  /* =====================================================
     SIDEBAR
     ===================================================== */

  .app-sidebar {
    position: fixed;
    inset: 0 auto 0 0;
    z-index: 50;
    display: flex;
    width: 280px;
    min-width: 0;
    flex-direction: column;
    overflow: visible;
    border-right: 1px solid var(--line);
    background: var(--bg-panel);
    transition:
      width var(--speed) var(--ease),
      transform var(--speed) var(--ease);
  }

  .app-main.authenticated {
    min-height: 100vh;
    margin-left: 280px;
    transition:
      margin-left var(--speed) var(--ease);
  }

  .app-main__inner {
    padding: 30px 34px 48px;
  }

  .sidebar-collapsed
    .app-sidebar {
    width: 72px;
  }

  .sidebar-collapsed
    .app-main.authenticated {
    margin-left: 72px;
  }

  /* =====================================================
     SIDEBAR HEADER
     ===================================================== */

  .sidebar-header {
    display: flex;
    min-height: 64px;
    align-items: center;
    gap: 7px;
    padding: 10px 11px;
    border-bottom: 1px solid var(--line-soft);
  }

  .sidebar-brand {
    display: flex;
    min-width: 0;
    flex: 1 1 auto;
    align-items: center;
    gap: 10px;
    overflow: hidden;
    border-radius: 8px;
    color: var(--text);
    padding: 5px 6px;
    text-decoration: none;
  }

  .sidebar-brand:hover {
    background: var(--bg-elevated);
  }

  .sidebar-brand img {
    width: 27px;
    height: 27px;
    flex: 0 0 auto;
    object-fit: contain;
  }

  .sidebar-brand__text {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .sidebar-brand__text strong {
    overflow: hidden;
    color: var(--text);
    font-size: 12.5px;
    font-weight: 650;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .sidebar-brand__text small {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 10px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .sidebar-close {
    display: none;
  }

  .sidebar-collapsed
    .sidebar-header {
    justify-content: center;
    padding-inline: 8px;
  }

  .sidebar-collapsed
    .sidebar-brand {
    flex: 0 0 auto;
    padding: 4px;
  }

  .sidebar-collapsed
    .sidebar-brand__text {
    display: none;
  }

  .sidebar-collapsed
    .sidebar-collapse {
    position: absolute;
    top: 14px;
    left: calc(100% + 9px);
    border-color: var(--line);
    background: var(--bg-panel);
  }

  /* =====================================================
     WORKSPACE CONTROL
     ===================================================== */

  .workspace-control {
    position: relative;
    padding: 10px 10px 8px;
  }

  .workspace-trigger,
  .workspace-create {
    display: grid;
    width: 100%;
    min-height: 58px;
    grid-template-columns:
      34px minmax(0, 1fr) 16px;
    align-items: center;
    gap: 9px;
    border: 1px solid var(--line);
    border-radius: 9px;
    background: var(--bg-ink-soft);
    color: var(--text);
    padding: 7px 9px;
    font: inherit;
    text-align: left;
    text-decoration: none;
    cursor: pointer;
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease);
  }

  .workspace-trigger:hover,
  .workspace-trigger.open,
  .workspace-create:hover {
    border-color: var(--brand-line);
    background: var(--brand-faint);
  }

  .workspace-trigger__identity {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .workspace-trigger__identity small {
    color: var(--text-muted);
    font-size: 10px;
  }

  .workspace-trigger__identity strong {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 11.5px;
    font-weight: 600;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .workspace-trigger__chevron {
    width: 15px;
    height: 15px;
    fill: none;
    stroke: var(--text-muted);
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
    transition:
      transform var(--speed) var(--ease);
  }

  .workspace-trigger__chevron.expanded {
    transform: rotate(90deg);
  }

  .workspace-create {
    grid-template-columns:
      34px minmax(0, 1fr);
  }

  .workspace-create > span:first-child {
    display: grid;
    width: 32px;
    height: 32px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 8px;
    color: var(--brand-soft);
  }

  .workspace-create svg {
    width: 16px;
    height: 16px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.9;
    stroke-linecap: round;
  }

  .workspace-create > span:last-child {
    display: grid;
    gap: 2px;
  }

  .workspace-create strong {
    color: var(--text-soft);
    font-size: 11.5px;
  }

  .workspace-create small {
    color: var(--text-muted);
    font-size: 10px;
  }

  .workspace-menu {
    position: absolute;
    top: calc(100% - 1px);
    right: 10px;
    left: 10px;
    z-index: 62;
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: 10px;
    background: var(--bg-panel);
  }

  .workspace-menu__header {
    display: flex;
    min-height: 58px;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    padding: 10px 12px;
    border-bottom: 1px solid var(--line-soft);
  }

  .workspace-menu__header > div {
    display: grid;
    gap: 2px;
  }

  .workspace-menu__header span {
    color: var(--text-soft);
    font-size: 11.5px;
    font-weight: 600;
  }

  .workspace-menu__header small {
    color: var(--text-muted);
    font-size: 10px;
  }

  .workspace-menu__header a {
    color: var(--brand-soft);
    font-size: 10.5px;
    font-weight: 600;
    text-decoration: none;
  }

  .workspace-menu__header a:hover {
    color: var(--brand-bright);
  }

  .workspace-menu__list {
    display: grid;
    max-height: 280px;
    overflow-y: auto;
    padding: 6px;
  }

  .workspace-menu__list button {
    display: grid;
    width: 100%;
    min-height: 48px;
    grid-template-columns:
      32px minmax(0, 1fr) 16px;
    align-items: center;
    gap: 9px;
    border: 1px solid transparent;
    border-radius: 7px;
    background: transparent;
    color: var(--text);
    padding: 6px 7px;
    font: inherit;
    text-align: left;
    cursor: pointer;
  }

  .workspace-menu__list button:hover {
    background: var(--bg-elevated);
  }

  .workspace-menu__list button.selected {
    border-color: var(--brand-line);
    background: var(--brand-faint);
  }

  .workspace-menu__list button > span {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .workspace-menu__list strong {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 11.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .workspace-menu__list small {
    color: var(--text-muted);
    font-size: 10px;
    text-transform: capitalize;
  }

  .workspace-selected {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: var(--brand-soft);
    stroke-width: 2;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .workspace-menu__footer {
    display: flex;
    flex-wrap: wrap;
    gap: 6px;
    padding: 9px 11px;
    border-top: 1px solid var(--line-soft);
  }

  .sidebar-collapsed
    .workspace-control {
    padding-inline: 9px;
  }

  .sidebar-collapsed
    .workspace-trigger {
    min-height: 52px;
    grid-template-columns: 1fr;
    justify-items: center;
    padding: 7px;
  }

  .sidebar-collapsed
    .workspace-trigger__identity,
  .sidebar-collapsed
    .workspace-trigger__chevron {
    display: none;
  }

  .sidebar-collapsed
    .workspace-create {
    min-height: 52px;
    grid-template-columns: 1fr;
    justify-items: center;
    padding: 7px;
  }

  .sidebar-collapsed
    .workspace-create >
    span:last-child {
    display: none;
  }

  .sidebar-collapsed
    .workspace-menu {
    top: 10px;
    right: auto;
    left: calc(100% + 8px);
    width: 280px;
  }

  /* =====================================================
     NAVIGATION
     ===================================================== */

  .sidebar-nav {
    display: grid;
    min-height: 0;
    flex: 1 1 auto;
    align-content: start;
    gap: 18px;
    overflow-x: hidden;
    overflow-y: auto;
    padding: 11px 10px 24px;
  }

  .nav-group {
    display: grid;
    gap: 5px;
  }

  .nav-group__label {
    padding: 0 9px;
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 650;
    letter-spacing: 0.09em;
    text-transform: uppercase;
  }

  .nav-group__items {
    display: grid;
    gap: 3px;
  }

  .nav-item {
    position: relative;
    display: grid;
    min-height: 39px;
    grid-template-columns:
      20px minmax(0, 1fr) auto;
    align-items: center;
    gap: 9px;
    border-radius: 8px;
    color: var(--text-soft);
    padding: 7px 9px;
    font-size: 12.5px;
    font-weight: 500;
    text-decoration: none;
    transition:
      background var(--speed) var(--ease),
      color var(--speed) var(--ease);
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
    content: '';
    position: absolute;
    top: 8px;
    bottom: 8px;
    left: 0;
    width: 2px;
    border-radius:
      0 2px 2px 0;
    background: var(--brand);
  }

  .nav-item__icon {
    position: relative;
    display: grid;
    width: 20px;
    height: 20px;
    place-items: center;
  }

  .nav-item__icon svg {
    width: 17px;
    height: 17px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.55;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .nav-item__notification-dot {
    position: absolute;
    top: -1px;
    right: -1px;
    width: 6px;
    height: 6px;
    border: 1px solid var(--bg-panel);
    border-radius: 50%;
    background: var(--brand);
  }

  .nav-item__label {
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .nav-item__count {
    display: grid;
    min-width: 22px;
    height: 21px;
    place-items: center;
    border-radius: 999px;
    background: var(--brand);
    color: var(--brand-ink);
    padding-inline: 5px;
    font-family: var(--font-mono);
    font-size: 9.5px;
    font-weight: 750;
  }

  .sidebar-collapsed
    .sidebar-nav {
    gap: 10px;
    padding-inline: 9px;
  }

  .sidebar-collapsed
    .nav-group__label {
    height: 1px;
    overflow: hidden;
    margin: 3px 7px;
    background: var(--line-soft);
    padding: 0;
    color: transparent;
  }

  .sidebar-collapsed
    .nav-item {
    min-height: 43px;
    grid-template-columns: 1fr;
    justify-items: center;
    padding: 8px;
  }

  .sidebar-collapsed
    .nav-item__label,
  .sidebar-collapsed
    .nav-item__count {
    display: none;
  }

  .sidebar-collapsed
    .nav-item.active::before {
    top: 9px;
    bottom: 9px;
  }

  /* =====================================================
     ACCOUNT
     ===================================================== */

  .sidebar-footer {
    position: relative;
    padding: 10px;
    border-top: 1px solid var(--line-soft);
    background: var(--bg-panel);
  }

  .account-block {
    position: relative;
  }

  .account-row {
    display: grid;
    width: 100%;
    min-height: 54px;
    grid-template-columns:
      34px minmax(0, 1fr) 16px;
    align-items: center;
    gap: 9px;
    border: 1px solid transparent;
    border-radius: 9px;
    background: transparent;
    color: var(--text);
    padding: 7px 8px;
    font: inherit;
    text-align: left;
    cursor: pointer;
  }

  .account-row:hover,
  .account-row.open {
    border-color: var(--line);
    background: var(--bg-elevated);
  }

  .account-row__avatar,
  .account-menu__avatar {
    display: grid;
    width: 33px;
    height: 33px;
    place-items: center;
    overflow: hidden;
    border-radius: 50%;
    background: var(--brand);
    color: var(--brand-ink);
    font-size: 11px;
    font-weight: 750;
  }

  .account-row__avatar img,
  .account-menu__avatar img {
    width: 100%;
    height: 100%;
    object-fit: cover;
  }

  .account-row__identity {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .account-row__identity strong {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 11.5px;
    font-weight: 600;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .account-row__identity small {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 10px;
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
    transition:
      transform var(--speed) var(--ease);
  }

  .account-row__chevron.expanded {
    transform: rotate(-90deg);
  }

  .account-menu {
    position: absolute;
    right: 0;
    bottom: calc(100% + 8px);
    left: 0;
    z-index: 62;
    display: grid;
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: 10px;
    background: var(--bg-panel);
    padding: 6px;
  }

  .account-menu__identity {
    display: grid;
    min-width: 0;
    grid-template-columns:
      34px minmax(0, 1fr);
    align-items: center;
    gap: 9px;
    padding: 7px;
    border-bottom: 1px solid var(--line-soft);
    margin-bottom: 4px;
  }

  .account-menu__identity > span:last-child {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .account-menu__identity strong {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 11.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .account-menu__identity small {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 10px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .account-menu__item {
    display: grid;
    width: 100%;
    min-height: 38px;
    grid-template-columns:
      18px minmax(0, 1fr);
    align-items: center;
    gap: 9px;
    border: 0;
    border-radius: 7px;
    background: transparent;
    color: var(--text-soft);
    padding: 7px 8px;
    font: inherit;
    font-size: 11.5px;
    font-weight: 550;
    text-align: left;
    text-decoration: none;
    cursor: pointer;
  }

  .account-menu__item:hover {
    background: var(--bg-elevated);
    color: var(--text);
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

  .account-menu__item--danger:hover {
    background: var(--danger-faint);
    color: var(--danger);
  }

  .account-menu__separator {
    height: 1px;
    margin: 3px;
    background: var(--line-soft);
  }

  .sidebar-collapsed
    .sidebar-footer {
    padding-inline: 9px;
  }

  .sidebar-collapsed
    .account-row {
    min-height: 52px;
    grid-template-columns: 1fr;
    justify-items: center;
    padding: 7px;
  }

  .sidebar-collapsed
    .account-row__identity,
  .sidebar-collapsed
    .account-row__chevron {
    display: none;
  }

  .sidebar-collapsed
    .account-menu {
    right: auto;
    bottom: 0;
    left: calc(100% + 8px);
    width: 270px;
  }

  /* =====================================================
     DISMISS LAYERS
     ===================================================== */

  .menu-dismiss {
    position: fixed;
    inset: 0;
    z-index: 40;
    width: 100%;
    min-height: 100%;
    border: 0;
    background: transparent;
    padding: 0;
    cursor: default;
  }

  .sidebar-backdrop {
    position: fixed;
    inset: 0;
    z-index: 42;
    display: none;
    width: 100%;
    min-height: 100%;
    border: 0;
    background: rgba(0, 0, 0, 0.68);
    padding: 0;
    cursor: default;
  }

  /* =====================================================
     MOBILE HEADER
     ===================================================== */

  .mobile-header {
    display: none;
  }

  /* =====================================================
     RESPONSIVE
     ===================================================== */

  @media (prefers-reduced-motion: reduce) {
    .app-sidebar,
    .app-main.authenticated,
    .workspace-trigger__chevron,
    .account-row__chevron {
      transition: none;
    }
  }

  @media (max-width: 880px) {
    .mobile-header {
      position: fixed;
      top: 0;
      right: 0;
      left: 0;
      z-index: 38;
      display: grid;
      min-height: 58px;
      grid-template-columns:
        38px minmax(0, 1fr) 38px;
      align-items: center;
      gap: 10px;
      padding: 8px 13px;
      border-bottom: 1px solid var(--line-soft);
      background: var(--bg);
    }

    .mobile-page-title {
      display: flex;
      min-width: 0;
      align-items: center;
      justify-content: center;
      gap: 8px;
    }

    .mobile-page-title img {
      width: 22px;
      height: 22px;
      flex: 0 0 auto;
      object-fit: contain;
    }

    .mobile-page-title span {
      overflow: hidden;
      color: var(--text-soft);
      font-size: 12px;
      font-weight: 600;
      text-overflow: ellipsis;
      white-space: nowrap;
    }

    .app-sidebar,
    .sidebar-collapsed
      .app-sidebar {
      width: min(88vw, 320px);
      transform: translateX(-100%);
    }

    .app-sidebar.open,
    .sidebar-collapsed
      .app-sidebar.open {
      transform: translateX(0);
    }

    .sidebar-collapsed
      .sidebar-header {
      justify-content: initial;
      padding-inline: 11px;
    }

    .sidebar-collapsed
      .sidebar-brand {
      flex: 1 1 auto;
      padding: 5px 6px;
    }

    .sidebar-collapsed
      .sidebar-brand__text {
      display: grid;
    }

    .sidebar-collapse {
      display: none;
    }

    .sidebar-close {
      display: grid;
    }

    .sidebar-collapsed
      .workspace-control {
      padding-inline: 10px;
    }

    .sidebar-collapsed
      .workspace-trigger {
      min-height: 58px;
      grid-template-columns:
        34px minmax(0, 1fr) 16px;
      justify-items: initial;
      padding: 7px 9px;
    }

    .sidebar-collapsed
      .workspace-trigger__identity {
      display: grid;
    }

    .sidebar-collapsed
      .workspace-trigger__chevron {
      display: block;
    }

    .sidebar-collapsed
      .workspace-create {
      min-height: 58px;
      grid-template-columns:
        34px minmax(0, 1fr);
      justify-items: initial;
      padding: 7px 9px;
    }

    .sidebar-collapsed
      .workspace-create >
      span:last-child {
      display: grid;
    }

    .sidebar-collapsed
      .workspace-menu {
      top: calc(100% - 1px);
      right: 10px;
      left: 10px;
      width: auto;
    }

    .sidebar-collapsed
      .sidebar-nav {
      gap: 18px;
      padding-inline: 10px;
    }

    .sidebar-collapsed
      .nav-group__label {
      height: auto;
      overflow: visible;
      margin: 0;
      background: transparent;
      padding: 0 9px;
      color: var(--text-muted);
    }

    .sidebar-collapsed
      .nav-item {
      min-height: 39px;
      grid-template-columns:
        20px minmax(0, 1fr) auto;
      justify-items: initial;
      padding: 7px 9px;
    }

    .sidebar-collapsed
      .nav-item__label,
    .sidebar-collapsed
      .nav-item__count {
      display: block;
    }

    .sidebar-collapsed
      .account-row {
      min-height: 54px;
      grid-template-columns:
        34px minmax(0, 1fr) 16px;
      justify-items: initial;
      padding: 7px 8px;
    }

    .sidebar-collapsed
      .account-row__identity {
      display: grid;
    }

    .sidebar-collapsed
      .account-row__chevron {
      display: block;
    }

    .sidebar-collapsed
      .account-menu {
      right: 0;
      bottom: calc(100% + 8px);
      left: 0;
      width: auto;
    }

    .app-main.authenticated,
    .sidebar-collapsed
      .app-main.authenticated {
      margin-left: 0;
      padding-top: 58px;
    }

    .app-main__inner {
      padding: 22px 14px 48px;
    }

    .sidebar-backdrop.visible {
      display: block;
    }
  }

  @media (max-width: 480px) {
    .app-main__inner {
      padding:
        18px 11px 42px;
    }

    .mobile-header {
      padding-inline: 10px;
    }

    .workspace-menu__footer {
      align-items: flex-start;
      flex-direction: column;
    }
  }
</style>
