<script lang="ts">
  import { goto } from '$app/navigation';
  import AccessScopeBadge from '$lib/components/AccessScopeBadge.svelte';
  import RoleBadge from '$lib/components/RoleBadge.svelte';
  import { auth } from '$lib/stores/auth';
  import { notifications } from '$lib/stores/notifications';
  import { workspaceContext } from '$lib/stores/workspace';

  const navItems = [
    { href: '/dashboard', label: 'Dashboard' },
    { href: '/workspaces', label: 'Workspaces' },
    { href: '/projects', label: 'Projects' },
    { href: '/packages', label: 'Packages' },
    { href: '/package-versions', label: 'Versions' },
    { href: '/lockfiles', label: 'Lockfiles' },
    { href: '/build-reports', label: 'Build reports' },
    { href: '/tokens', label: 'Tokens' },
    { href: '/members', label: 'Members' },
    { href: '/feedback', label: 'Feedback' }
  ];

  let initializedSessionId = '';

  $: initial = ($auth.user?.name || $auth.user?.email || 'U').slice(0, 1).toUpperCase();
  $: if ($auth.session?.id && initializedSessionId !== $auth.session.id) {
    initializedSessionId = $auth.session.id;
    notifications.loadNotifications();
    notifications.connectRealtime($auth.session.id);
  }
  $: if (!$auth.session && initializedSessionId) {
    initializedSessionId = '';
    notifications.clear();
  }

  function logout() {
    auth.clear();
    workspaceContext.clear();
    notifications.clear();
    window.location.href = '/login';
  }

  async function openAccount() {
    await goto('/account');
  }
</script>

<div class="shell">
  {#if $auth.session}
    <aside class="sidebar">
      <a class="brand" href="/dashboard" aria-label="Softadastra Cloud dashboard">
        <span class="mark">S</span>
        <span>Softadastra Cloud</span>
      </a>

      {#if $workspaceContext.workspaces.length > 0}
        <label class="workspace-switcher">
          Workspace
          <select
            value={$workspaceContext.selectedWorkspace?.id ?? ''}
            on:change={(event) => workspaceContext.setSelectedWorkspace(event.currentTarget.value)}
          >
            {#each $workspaceContext.workspaces as workspace}
              <option value={workspace.id}>{workspace.name}</option>
            {/each}
          </select>
        </label>
        {#if $workspaceContext.selectedWorkspace}
          <p class="context-line compact">Role: <RoleBadge role={$workspaceContext.selectedWorkspace.current_user_role} /> Access: <AccessScopeBadge scope={$workspaceContext.selectedWorkspace.access_scope} /></p>
        {/if}
      {/if}

      <nav aria-label="Primary navigation">
        {#each navItems as item}
          <a href={item.href}>{item.label}</a>
        {/each}
      </nav>

      <div class="account-menu">
        <a class="notification-button" href="/notifications" aria-label="Open notifications">
          <span>Notifications</span>
          {#if $notifications.unreadCount > 0}
            <strong>{$notifications.unreadCount}</strong>
          {/if}
        </a>
        <button class="avatar-button" type="button" on:click={openAccount} aria-label="Open account">
          <span class="avatar">{initial}</span>
          <span>{$auth.user?.name || $auth.user?.email}</span>
        </button>
        <button class="ghost" type="button" on:click={logout}>Logout</button>
      </div>
    </aside>
  {/if}

  <main class:authenticated={$auth.session}>
    <slot />
  </main>
</div>
