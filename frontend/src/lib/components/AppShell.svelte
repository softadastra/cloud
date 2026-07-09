<script lang="ts">
  import { auth } from '$lib/stores/auth';

  const navItems = [
    { href: '/dashboard', label: 'Dashboard' },
    { href: '/workspaces', label: 'Workspaces' },
    { href: '/projects', label: 'Projects' },
    { href: '/packages', label: 'Packages' },
    { href: '/package-versions', label: 'Versions' },
    { href: '/lockfiles', label: 'Lockfiles' },
    { href: '/build-reports', label: 'Build reports' },
    { href: '/tokens', label: 'Tokens' },
    { href: '/members', label: 'Members' }
  ];

  function logout() {
    auth.clear();
    window.location.href = '/login';
  }
</script>

<div class="shell">
  {#if $auth.session}
    <aside class="sidebar">
      <a class="brand" href="/dashboard" aria-label="Softadastra Cloud dashboard">
        <span class="mark">S</span>
        <span>Softadastra Cloud</span>
      </a>

      <nav aria-label="Primary navigation">
        {#each navItems as item}
          <a href={item.href}>{item.label}</a>
        {/each}
      </nav>

      <button class="ghost" type="button" on:click={logout}>Logout</button>
    </aside>
  {/if}

  <main class:authenticated={$auth.session}>
    <slot />
  </main>
</div>
