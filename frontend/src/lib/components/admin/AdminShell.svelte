<script lang="ts">
  import AdminRail from './AdminRail.svelte';

  export let title: string;
  export let description: string = '';
  export let crumb: string = 'Overview';
  export let role: string | null = null;
</script>

<div class="admin-shell">
  <AdminRail {role} />

  <div class="admin-main">
    <header class="page-head">
      <p class="page-head__crumb">
        <span>Platform</span>
        <span aria-hidden="true">/</span>
        <span>{crumb}</span>
      </p>

      <h1>{title}</h1>

      {#if description}
        <p class="page-head__desc">{description}</p>
      {/if}
    </header>

    <slot />
  </div>
</div>

<style>
  .admin-shell {
    display: grid;
    grid-template-columns: 190px minmax(0, 1fr);
    gap: 32px;
    width: 100%;
    max-width: 1280px;
    margin: 0 auto;
    align-items: start;
  }

  .admin-main {
    display: grid;
    gap: 18px;
    min-width: 0;
  }

  .page-head {
    display: grid;
    gap: 6px;
    border-bottom: 1px solid var(--line);
    padding-bottom: 16px;
  }

  .page-head__crumb {
    display: flex;
    gap: 7px;
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 10px;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }

  .page-head__crumb span:last-child {
    color: var(--text-muted);
  }

  .page-head h1 {
    font-size: 1.55rem;
    letter-spacing: -0.01em;
  }

  .page-head__desc {
    max-width: 640px;
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.65;
  }

  @media (max-width: 920px) {
    .admin-shell {
      grid-template-columns: 1fr;
      gap: 16px;
    }
  }

  /* ------------------------------------------------------------------ */
  /* Shared building blocks — scoped to the shell, available to slotted  */
  /* page content via :global.                                           */
  /* ------------------------------------------------------------------ */

  /* Panels */

  .admin-main :global(.panel) {
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    overflow: hidden;
  }

  .admin-main :global(.panel__head) {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 14px;
    border-bottom: 1px solid var(--line-soft);
    padding: 13px 16px;
  }

  .admin-main :global(.panel__head h2) {
    color: var(--text);
    font-size: 13px;
    font-weight: 650;
  }

  .admin-main :global(.panel__head p) {
    margin-top: 2px;
    color: var(--text-muted);
    font-size: 11px;
  }

  .admin-main :global(.panel__meta) {
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 10px;
  }

  /* Data table */

  .admin-main :global(.table-wrap) {
    overflow-x: auto;
  }

  .admin-main :global(.data-table) {
    width: 100%;
    border-collapse: collapse;
  }

  .admin-main :global(.data-table th) {
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
    color: var(--text-faint);
    padding: 8px 16px;
    font-family: var(--font-mono);
    font-size: 9px;
    font-weight: 600;
    letter-spacing: 0.11em;
    text-align: left;
    text-transform: uppercase;
    white-space: nowrap;
  }

  .admin-main :global(.data-table td) {
    border-bottom: 1px solid var(--line-soft);
    color: var(--text-muted);
    padding: 12px 16px;
    font-size: 11.5px;
    vertical-align: middle;
  }

  .admin-main :global(.data-table tbody tr:last-child td) {
    border-bottom: 0;
  }

  .admin-main :global(.data-table tbody tr:hover td) {
    background: var(--bg-elevated);
  }

  .admin-main :global(.data-table .is-primary) {
    color: var(--text-soft);
    font-weight: 600;
  }

  .admin-main :global(.data-table .is-primary small) {
    display: block;
    margin-top: 2px;
    color: var(--text-muted);
    font-size: 10px;
    font-weight: 400;
  }

  .admin-main :global(.is-right) {
    text-align: right;
  }

  .admin-main :global(.is-mono) {
    font-family: var(--font-mono);
    font-size: 10.5px;
    color: var(--text-faint);
  }

  .admin-main :global(.cell-muted) {
    color: var(--text-muted);
  }

  /* Badges */

  .admin-main :global(.badge) {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    background: var(--bg-elevated);
    color: var(--text-muted);
    padding: 2px 7px;
    font-family: var(--font-mono);
    font-size: 9.5px;
    font-weight: 600;
    white-space: nowrap;
  }

  .admin-main :global(.badge::before) {
    content: '';
    width: 5px;
    height: 5px;
    border-radius: 50%;
    background: var(--text-faint);
  }

  .admin-main :global(.badge.no-dot::before) {
    display: none;
  }

  .admin-main :global(.status-open) {
    border-color: var(--warning-line);
    background: var(--warning-faint);
    color: var(--warning);
  }
  .admin-main :global(.status-open::before) {
    background: var(--warning);
  }

  .admin-main :global(.status-active),
  .admin-main :global(.status-reviewed),
  .admin-main :global(.status-public),
  .admin-main :global(.is-yes) {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }
  .admin-main :global(.status-active::before),
  .admin-main :global(.status-reviewed::before),
  .admin-main :global(.status-public::before),
  .admin-main :global(.is-yes::before) {
    background: var(--green-soft);
  }

  .admin-main :global(.status-planned) {
    border-color: var(--info-line);
    background: var(--info-faint);
    color: var(--info);
  }
  .admin-main :global(.status-planned::before) {
    background: var(--info);
  }

  .admin-main :global(.status-private),
  .admin-main :global(.is-no) {
    color: var(--text-muted);
  }

  /* Action buttons */

  .admin-main :global(.btn) {
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    background: var(--bg-elevated);
    color: var(--text-soft);
    padding: 4px 10px;
    font-size: 10.5px;
    font-weight: 600;
    cursor: pointer;
    transition:
      color var(--speed) var(--ease),
      border-color var(--speed) var(--ease);
  }

  .admin-main :global(.btn:hover) {
    border-color: var(--brand);
    color: var(--text);
  }

  .admin-main :global(.btn-danger:hover) {
    border-color: var(--danger-line, var(--warning-line));
    color: var(--danger, var(--warning));
  }

  .admin-main :global(.btn:disabled) {
    opacity: 0.45;
    cursor: not-allowed;
  }

  .admin-main :global(.btn-primary) {
    border-color: var(--brand);
    background: var(--brand-faint, var(--bg-elevated));
    color: var(--brand);
  }

  /* Inputs & selects */

  .admin-main :global(.field) {
    width: 100%;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    background: var(--bg-ink-soft);
    color: var(--text);
    padding: 7px 10px;
    font-size: 12px;
    font-family: inherit;
    transition: border-color var(--speed) var(--ease);
  }

  .admin-main :global(.field:focus) {
    outline: none;
    border-color: var(--brand);
  }

  .admin-main :global(select.field) {
    cursor: pointer;
  }

  /* Empty & feedback states */

  .admin-main :global(.empty-panel) {
    display: grid;
    min-height: 120px;
    place-items: center;
    padding: 18px;
    color: var(--text-muted);
    font-size: 11px;
    text-align: center;
  }

  .admin-main :global(.notice) {
    border: 1px solid var(--green-line);
    border-radius: var(--radius-sm);
    background: var(--green-faint);
    color: var(--green-soft);
    padding: 9px 13px;
    font-size: 11.5px;
  }
</style>
