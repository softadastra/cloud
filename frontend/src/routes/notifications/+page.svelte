<script lang="ts">
  import { onMount } from 'svelte';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import { notifications } from '$lib/stores/notifications';

  let filter: 'all' | 'unread' | 'read' = 'all';
  let actionError = '';

  async function load() {
    actionError = '';
    await notifications.loadNotifications(filter);
  }

  async function markRead(id: string) {
    actionError = '';
    try {
      await notifications.markRead(id);
    } catch (error) {
      actionError = error instanceof Error ? error.message : 'Unable to mark notification as read.';
    }
  }

  async function markAllRead() {
    actionError = '';
    try {
      await notifications.markAllRead();
    } catch (error) {
      actionError = error instanceof Error ? error.message : 'Unable to mark notifications as read.';
    }
  }

  function dateLabel(value: number) {
    return value ? new Date(value * 1000).toLocaleString() : '';
  }

  onMount(load);
</script>

<svelte:head><title>Notifications | Softadastra Cloud</title></svelte:head>

<PageHeader eyebrow="Notifications" title="What changed recently" />

<section class="panel">
  <div class="panel-header">
    <h2>Notification center</h2>
    <span>{$notifications.unreadCount} unread</span>
  </div>

  <div class="actions horizontal">
    <select bind:value={filter} on:change={load}>
      <option value="all">All</option>
      <option value="unread">Unread</option>
      <option value="read">Read</option>
    </select>
    <button class="small" type="button" on:click={markAllRead} disabled={$notifications.unreadCount === 0}>Mark all as read</button>
  </div>

  {#if $notifications.error || actionError}
    <InlineError message={$notifications.error || actionError} />
  {/if}

  {#if $notifications.loading}
    <p class="muted">Loading notifications...</p>
  {:else if $notifications.notifications.length === 0}
    <EmptyState title="No notifications yet." body="Important updates about your workspace will appear here." />
  {:else}
    <div class="table-list">
      {#each $notifications.notifications as item}
        <button class:unread={!item.read_at} class="row notification-row" type="button" on:click={() => !item.read_at && markRead(item.id)}>
          <span>
            <strong>{item.title}</strong>
            <small>{item.message}</small>
            <small>{dateLabel(item.created_at)}</small>
          </span>
          <span class="actions"><StatusBadge status={item.read_at ? 'read' : 'unread'} /><small>{item.type}</small></span>
        </button>
      {/each}
    </div>
  {/if}
</section>
