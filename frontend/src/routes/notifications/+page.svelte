<script lang="ts">
  import { usePageRefresh } from '$lib/stores/pageRefresh';
  import { goto } from '$app/navigation';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import { notifications } from '$lib/stores/notifications';
  import { notificationHref } from '$lib/notifications/notificationLinks';
  import type { NotificationItem } from '$lib/api/types';

  let filter: 'all' | 'unread' | 'read' = 'all';
  let actionError = '';

  $: items = $notifications.notifications;
  $: unread = $notifications.unreadCount;

  async function load() {
    actionError = '';
    await notifications.loadNotifications(filter);
  }

  async function markRead(id: string) {
    actionError = '';
    try {
      await notifications.markRead(id);
    } catch (err) {
      actionError = err instanceof Error ? err.message : 'Unable to mark as read.';
    }
  }

  async function markAllRead() {
    actionError = '';
    try {
      await notifications.markAllRead();
    } catch (err) {
      actionError = err instanceof Error ? err.message : 'Unable to mark all as read.';
    }
  }

  async function openNotification(event: MouseEvent, item: NotificationItem) {
    const href = notificationHref(item);

    if (
      event.button !== 0 ||
      event.metaKey ||
      event.ctrlKey ||
      event.shiftKey ||
      event.altKey
    ) {
      if (!item.read_at) void markRead(item.id);
      return;
    }

    event.preventDefault();

    if (!item.read_at) {
      try {
        await markRead(item.id);
      } catch {
        // Navigation is more important than read-state sync; the list can refresh later.
      }
    }

    await goto(href);
  }

  function dateLabel(value: number) {
    if (!value) return '';
    return new Date(value * 1000).toLocaleString(undefined, {
      month: 'short',
      day: 'numeric',
      hour: '2-digit',
      minute: '2-digit'
    });
  }

  usePageRefresh(load);
</script>

<svelte:head><title>Notifications | Softadastra Cloud</title></svelte:head>

<PageHeader eyebrow="Activity" title="Notifications">
  {#if unread > 0}
    <button class="ghost" type="button" on:click={markAllRead}>
      Mark all read
    </button>
  {/if}
</PageHeader>

<InlineError message={$notifications.error || actionError} />

<section class="panel">
  <div class="panel-header">
    <h2>Notification center</h2>
    <span>{unread} unread</span>
  </div>

  <!-- Filter tabs -->
  <div class="filter-tabs" role="tablist" aria-label="Filter notifications">
    {#each (['all', 'unread', 'read'] as const) as tab}
      <button
        class="filter-tab"
        class:active={filter === tab}
        type="button"
        role="tab"
        aria-selected={filter === tab}
        on:click={() => { filter = tab; load(); }}
      >
        {tab.charAt(0).toUpperCase() + tab.slice(1)}
      </button>
    {/each}
  </div>

  {#if $notifications.loading}
    <p class="muted">Loading…</p>
  {:else if items.length === 0}
    <EmptyState
      title={filter === 'unread' ? 'No unread notifications' : 'No notifications yet'}
      body="Workspace events and important account updates appear here."
    />
  {:else}
    <div class="table-list notif-list">
      {#each items as item (item.id)}
        <a
          class="row notif-row"
          class:notif-unread={!item.read_at}
          href={notificationHref(item)}
          aria-label="{item.read_at ? '' : 'Unread: '}{item.title}"
          on:click={(event) => openNotification(event, item)}
        >
          <span class="notif-dot-col" aria-hidden="true">
            {#if !item.read_at}
              <span class="notif-dot"></span>
            {/if}
          </span>

          <span class="notif-body">
            <strong>{item.title}</strong>
            <small>{item.message}</small>
          </span>

          <span class="notif-meta">
            <small class="notif-type">{item.type}</small>
            {#if item.created_at}
              <small class="notif-date">{dateLabel(item.created_at)}</small>
            {/if}
          </span>
        </a>
      {/each}
    </div>
  {/if}
</section>

<style>
  /* Filter tabs */
  .filter-tabs {
    display: flex;
    gap: 2px;
    padding: 3px;
    border: 1px solid var(--line-soft);
    border-radius: var(--radius-sm);
    background: var(--bg-ink-soft);
    width: fit-content;
  }

  .filter-tab {
    min-height: 28px;
    padding: 3px 14px;
    border: 0;
    border-radius: 4px;
    background: transparent;
    color: var(--text-muted);
    font-size: 12.5px;
    font-weight: 500;
    transition:
      background var(--speed) var(--ease),
      color var(--speed) var(--ease);
  }

  .filter-tab:hover {
    background: var(--bg-elevated);
    color: var(--text);
  }

  .filter-tab.active {
    background: var(--bg-elevated);
    color: var(--text);
    font-weight: 600;
  }

  /* Notification list */
  .notif-list {
    border-radius: var(--radius-sm);
    overflow: hidden;
  }

  .notif-row {
    display: flex;
    align-items: flex-start;
    width: 100%;
    min-height: 60px;
    padding: 12px 14px;
    gap: 12px;
    border: 0;
    border-bottom: 1px solid var(--line-soft);
    border-radius: 0;
    background: transparent;
    color: var(--text);
    text-align: left;
    cursor: pointer;
    font-size: 13.5px;
    text-decoration: none;
    transition: background var(--speed) var(--ease);
  }

  .notif-row:last-child {
    border-bottom: 0;
  }

  .notif-row:hover {
    background: var(--bg-elevated);
  }

  .notif-unread {
    background: rgba(249, 115, 22, 0.03);
  }

  .notif-unread:hover {
    background: var(--brand-faint);
  }

  /* Unread indicator */
  .notif-dot-col {
    width: 10px;
    flex: 0 0 10px;
    display: flex;
    align-items: center;
    padding-top: 5px;
  }

  .notif-dot {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: var(--brand);
    flex: 0 0 auto;
  }

  .notif-body {
    display: grid;
    gap: 3px;
    flex: 1 1 auto;
    min-width: 0;
  }

  .notif-body strong {
    font-weight: 550;
    color: var(--text);
  }

  .notif-body small {
    color: var(--text-muted);
    font-size: 12.5px;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .notif-meta {
    display: flex;
    flex-direction: column;
    align-items: flex-end;
    gap: 4px;
    flex: 0 0 auto;
    padding-top: 2px;
  }

  .notif-type {
    font-family: var(--font-mono);
    font-size: 10.5px;
    color: var(--text-faint);
    text-transform: uppercase;
    letter-spacing: 0.06em;
  }

  .notif-date {
    font-size: 11.5px;
    color: var(--text-faint);
    white-space: nowrap;
  }

  @media (max-width: 600px) {
    .notif-meta {
      display: none;
    }
  }
</style>
