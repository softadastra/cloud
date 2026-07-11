<script lang="ts">
  import { page } from '$app/stores';

  export let role: string | null = null;

  const navLinks = [
    { label: 'Overview', href: '/admin' },
    { label: 'Users', href: '/admin/users' },
    { label: 'Packages', href: '/admin/packages' },
    { label: 'Feedback', href: '/admin/feedback' },
    { label: 'Supporters', href: '/admin/supporters' },
    { label: 'Audit', href: '/admin/audit' }
  ];

  function humanize(value: string) {
    return value
      .replaceAll('_', ' ')
      .replace(/\b\w/g, (letter) => letter.toUpperCase());
  }

  $: current = $page.url.pathname;

  function isActive(href: string) {
    return href === '/admin'
      ? current === '/admin'
      : current.startsWith(href);
  }
</script>

<aside class="admin-rail" aria-label="Administration sections">
  <p class="rail-label">Administration</p>

  <nav>
    {#each navLinks as link (link.href)}
      <a
        href={link.href}
        class:active={isActive(link.href)}
        aria-current={isActive(link.href) ? 'page' : undefined}
      >
        {link.label}
      </a>
    {/each}
  </nav>

  {#if role}
    <div class="rail-access">
      <span>Signed in as</span>
      <strong>{humanize(role)}</strong>
    </div>
  {/if}
</aside>

<style>
  .admin-rail {
    position: sticky;
    top: 76px;
    display: grid;
    gap: 14px;
  }

  .rail-label {
    color: var(--text-faint);
    padding-left: 13px;
    font-family: var(--font-mono);
    font-size: 9px;
    letter-spacing: 0.14em;
    text-transform: uppercase;
  }

  .admin-rail nav {
    display: grid;
    gap: 1px;
  }

  .admin-rail nav a {
    display: block;
    border-left: 2px solid transparent;
    border-radius: 0 var(--radius-sm) var(--radius-sm) 0;
    color: var(--text-muted);
    padding: 7px 12px 7px 11px;
    font-size: 12.5px;
    font-weight: 550;
    transition:
      color var(--speed) var(--ease),
      background var(--speed) var(--ease);
  }

  .admin-rail nav a:hover {
    color: var(--text);
    background: var(--bg-panel);
  }

  .admin-rail nav a.active {
    border-left-color: var(--brand);
    color: var(--text);
    background: var(--bg-panel);
    font-weight: 600;
  }

  .rail-access {
    display: grid;
    gap: 3px;
    margin-left: 2px;
    border-top: 1px solid var(--line-soft);
    padding: 12px 11px 0;
  }

  .rail-access span {
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 9px;
    letter-spacing: 0.1em;
    text-transform: uppercase;
  }

  .rail-access strong {
    color: var(--text-soft);
    font-size: 12px;
    font-weight: 650;
  }

  @media (max-width: 920px) {
    .admin-rail {
      position: static;
      gap: 10px;
    }

    .admin-rail nav {
      display: flex;
      gap: 4px;
      overflow-x: auto;
      border-bottom: 1px solid var(--line);
    }

    .admin-rail nav a {
      flex: 0 0 auto;
      border-left: 0;
      border-bottom: 2px solid transparent;
      border-radius: 0;
      padding: 9px 4px 8px;
    }

    .admin-rail nav a:hover,
    .admin-rail nav a.active {
      background: transparent;
    }

    .admin-rail nav a.active {
      border-bottom-color: var(--brand);
    }

    .rail-label,
    .rail-access {
      display: none;
    }
  }
</style>
