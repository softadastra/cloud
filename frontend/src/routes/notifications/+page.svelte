<script lang="ts">
  import { goto } from '$app/navigation';

  import InlineError from '$lib/components/InlineError.svelte';

  import { notifications } from '$lib/stores/notifications';
  import { usePageRefresh } from '$lib/stores/pageRefresh';

  import {
    notificationHref
  } from '$lib/notifications/notificationLinks';

  import type {
    NotificationItem
  } from '$lib/api/types';

  type NotificationFilter =
    | 'all'
    | 'unread'
    | 'read';

  type NotificationKind =
    | 'workspace'
    | 'project'
    | 'package'
    | 'build'
    | 'security'
    | 'message'
    | 'general';

  type NotificationGroup = {
    key: string;
    label: string;
    items: NotificationItem[];
  };

  let filter: NotificationFilter =
    'all';

  let actionError = '';
  let actionSuccess = '';

  let markingAllRead = false;

  $: items =
    $notifications.notifications;

  $: unread =
    $notifications.unreadCount;

  $: groups =
    groupNotifications(items);

  $: visibleCount =
    items.length;

  async function load() {
    actionError = '';
    actionSuccess = '';

    await notifications.loadNotifications(
      filter
    );
  }

  async function selectFilter(
    nextFilter: NotificationFilter
  ) {
    if (
      nextFilter === filter &&
      !$notifications.error
    ) {
      return;
    }

    filter = nextFilter;

    actionError = '';
    actionSuccess = '';

    await notifications.loadNotifications(
      nextFilter
    );
  }

  async function markAllRead() {
    if (
      markingAllRead ||
      unread === 0
    ) {
      return;
    }

    markingAllRead = true;

    actionError = '';
    actionSuccess = '';

    try {
      await notifications.markAllRead();

      await notifications.loadNotifications(
        filter
      );

      actionSuccess =
        'All notifications marked as read.';
    } catch (err) {
      actionError =
        err instanceof Error
          ? err.message
          : 'Unable to mark all notifications as read.';
    } finally {
      markingAllRead = false;
    }
  }

  async function markNotificationRead(
    item: NotificationItem,
    silent = false
  ) {
    if (item.read_at) {
      return;
    }

    try {
      await notifications.markRead(
        item.id
      );
    } catch (err) {
      if (!silent) {
        actionError =
          err instanceof Error
            ? err.message
            : 'Unable to mark the notification as read.';
      }

      throw err;
    }
  }

  async function openNotification(
    event: MouseEvent,
    item: NotificationItem
  ) {
    const href =
      notificationHref(item);

    const modifiedNavigation =
      event.button !== 0 ||
      event.metaKey ||
      event.ctrlKey ||
      event.shiftKey ||
      event.altKey;

    if (modifiedNavigation) {
      if (!item.read_at) {
        void markNotificationRead(
          item,
          true
        ).catch(() => {
          // The destination still opens through the
          // browser's native modified-click behavior.
        });
      }

      return;
    }

    event.preventDefault();

    if (!item.read_at) {
      try {
        await markNotificationRead(
          item,
          true
        );
      } catch {
        // Navigation remains more important than
        // immediate read-state synchronization.
      }
    }

    await goto(href);
  }

  function timestampFor(
    value:
      | number
      | string
      | null
      | undefined
  ) {
    if (
      value === undefined ||
      value === null ||
      value === ''
    ) {
      return null;
    }

    if (typeof value === 'number') {
      return value <
        1_000_000_000_000
        ? value * 1000
        : value;
    }

    const numericValue =
      Number(value);

    if (
      Number.isFinite(numericValue) &&
      value.trim() !== ''
    ) {
      return numericValue <
        1_000_000_000_000
        ? numericValue * 1000
        : numericValue;
    }

    const parsed =
      new Date(value).getTime();

    return Number.isNaN(parsed)
      ? null
      : parsed;
  }

  function startOfDay(
    timestamp: number
  ) {
    const date =
      new Date(timestamp);

    return new Date(
      date.getFullYear(),
      date.getMonth(),
      date.getDate()
    ).getTime();
  }

  function notificationGroupKey(
    value:
      | number
      | string
      | null
      | undefined
  ) {
    const timestamp =
      timestampFor(value);

    if (timestamp === null) {
      return 'earlier';
    }

    const today =
      startOfDay(Date.now());

    const notificationDay =
      startOfDay(timestamp);

    const difference =
      today - notificationDay;

    if (difference <= 0) {
      return 'today';
    }

    if (
      difference <=
      24 * 60 * 60 * 1000
    ) {
      return 'yesterday';
    }

    return 'earlier';
  }

  function groupLabel(
    key: string
  ) {
    if (key === 'today') {
      return 'Today';
    }

    if (key === 'yesterday') {
      return 'Yesterday';
    }

    return 'Earlier';
  }

  function groupNotifications(
    source: NotificationItem[]
  ): NotificationGroup[] {
    const grouped =
      new Map<
        string,
        NotificationItem[]
      >();

    for (const item of source) {
      const key =
        notificationGroupKey(
          item.created_at
        );

      const current =
        grouped.get(key) ?? [];

      current.push(item);
      grouped.set(key, current);
    }

    return [
      'today',
      'yesterday',
      'earlier'
    ]
      .filter((key) =>
        grouped.has(key)
      )
      .map((key) => ({
        key,
        label: groupLabel(key),
        items: grouped.get(key) ?? []
      }));
  }

  function formatNotificationDate(
    value:
      | number
      | string
      | null
      | undefined
  ) {
    const timestamp =
      timestampFor(value);

    if (timestamp === null) {
      return 'Unknown date';
    }

    const today =
      startOfDay(Date.now());

    const notificationDay =
      startOfDay(timestamp);

    const sameDay =
      today === notificationDay;

    return new Intl.DateTimeFormat(
      undefined,
      sameDay
        ? {
            hour: '2-digit',
            minute: '2-digit'
          }
        : {
            month: 'short',
            day: 'numeric',
            hour: '2-digit',
            minute: '2-digit'
          }
    ).format(
      new Date(timestamp)
    );
  }

  function fullNotificationDate(
    value:
      | number
      | string
      | null
      | undefined
  ) {
    const timestamp =
      timestampFor(value);

    if (timestamp === null) {
      return 'Unknown date';
    }

    return new Intl.DateTimeFormat(
      undefined,
      {
        dateStyle: 'full',
        timeStyle: 'short'
      }
    ).format(
      new Date(timestamp)
    );
  }

  function notificationTypeLabel(
    value?: string
  ) {
    const cleanValue =
      value
        ?.trim()
        .replace(/[._-]+/g, ' ')
        .replace(/\s+/g, ' ') ||
      'Activity';

    return cleanValue.replace(
      /\b\w/g,
      (letter) =>
        letter.toUpperCase()
    );
  }

  function notificationKind(
    value?: string
  ): NotificationKind {
    const type =
      value?.toLowerCase() ?? '';

    if (
      type.includes('workspace') ||
      type.includes('member') ||
      type.includes('invite')
    ) {
      return 'workspace';
    }

    if (
      type.includes('project') ||
      type.includes('repository')
    ) {
      return 'project';
    }

    if (
      type.includes('package') ||
      type.includes('version') ||
      type.includes('registry')
    ) {
      return 'package';
    }

    if (
      type.includes('build') ||
      type.includes('lockfile') ||
      type.includes('report')
    ) {
      return 'build';
    }

    if (
      type.includes('token') ||
      type.includes('security') ||
      type.includes('password') ||
      type.includes('session') ||
      type.includes('account')
    ) {
      return 'security';
    }

    if (
      type.includes('feedback') ||
      type.includes('message') ||
      type.includes('comment') ||
      type.includes('reply')
    ) {
      return 'message';
    }

    return 'general';
  }

  usePageRefresh(load);
</script>

<svelte:head>
  <title>
    Notifications | Softadastra Cloud
  </title>

  <meta
    name="description"
    content="Review workspace activity, project updates, package events, build results and important account notifications in Softadastra Cloud."
  />
</svelte:head>

<div class="notifications-page">
  <!-- ===================================================
       PAGE HEADER
       =================================================== -->
  <header class="page-header">
    <div class="page-heading">
      <p class="page-eyebrow">
        <span aria-hidden="true"></span>
        Account activity
      </p>

      <h1>Notifications</h1>

      <p>
        Follow important workspace, project,
        package and account activity from one
        place.
      </p>
    </div>

    <div class="page-header__actions">
      <div
        class:has-unread={unread > 0}
        class="unread-summary"
      >
        <span aria-hidden="true">
          {unread}
        </span>

        <div>
          <strong>
            {unread === 1
              ? 'Unread notification'
              : 'Unread notifications'}
          </strong>

          <small>
            {unread > 0
              ? 'New activity needs attention'
              : 'You are all caught up'}
          </small>
        </div>
      </div>

      {#if unread > 0}
        <button
          class="mark-all-button"
          type="button"
          disabled={markingAllRead}
          onclick={() =>
            void markAllRead()}
        >
          {#if markingAllRead}
            <span
              class="button-spinner"
              aria-hidden="true"
            ></span>

            Marking as read…
          {:else}
            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path d="m3 12 4 4L17 6"></path>
              <path d="m11 16 2 2 8-8"></path>
            </svg>

            Mark all read
          {/if}
        </button>
      {/if}
    </div>
  </header>

  <InlineError
    message={$notifications.error ||
      actionError}
  />

  {#if actionSuccess}
    <div
      class="success-message"
      role="status"
    >
      <span aria-hidden="true">
        <svg
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="2.2"
          stroke-linecap="round"
          stroke-linejoin="round"
        >
          <path d="m5 12 4 4L19 6"></path>
        </svg>
      </span>

      <p>{actionSuccess}</p>
    </div>
  {/if}

  <!-- ===================================================
       NOTIFICATION CENTER
       =================================================== -->
  <section
    class="notifications-panel"
    aria-labelledby="notification-center-title"
  >
    <header class="panel-header">
      <div>
        <p class="panel-kicker">
          Activity inbox
        </p>

        <h2 id="notification-center-title">
          Notification center
        </h2>

        <p>
          Select an activity to open the related
          workspace resource.
        </p>
      </div>

      <span class="visible-count">
        {visibleCount}
      </span>
    </header>

    <div class="notification-toolbar">
      <nav
        class="filter-tabs"
        aria-label="Notification filters"
      >
        {#each (
          [
            {
              value: 'all',
              label: 'All'
            },
            {
              value: 'unread',
              label: 'Unread'
            },
            {
              value: 'read',
              label: 'Read'
            }
          ] as const
        ) as tab}
          <button
            class:active={
              filter === tab.value
            }
            type="button"
            aria-pressed={
              filter === tab.value
            }
            onclick={() =>
              void selectFilter(
                tab.value
              )}
          >
            {tab.label}

            {#if tab.value === 'unread'}
              <span>{unread}</span>
            {/if}
          </button>
        {/each}
      </nav>

      <p class="toolbar-description">
        {#if filter === 'unread'}
          Showing activity that has not been opened.
        {:else if filter === 'read'}
          Showing activity already reviewed.
        {:else}
          Showing all recent account activity.
        {/if}
      </p>
    </div>

    <!-- =================================================
         LOADING
         ================================================= -->
    {#if $notifications.loading}
      <div
        class="notification-loading"
        aria-label="Loading notifications"
      >
        {#each [1, 2, 3, 4] as placeholder}
          <article class="notification-skeleton">
            <span
              class="skeleton-icon"
              aria-hidden="true"
            ></span>

            <div>
              <span
                class="skeleton-title"
                aria-hidden="true"
              ></span>

              <span
                class="skeleton-message"
                aria-hidden="true"
              ></span>
            </div>

            <span
              class="skeleton-date"
              aria-hidden="true"
            ></span>
          </article>
        {/each}
      </div>

    <!-- =================================================
         EMPTY STATES
         ================================================= -->
    {:else if items.length === 0}
      <section class="empty-state">
        <span
          class:empty-state__icon--complete={
            filter === 'unread'
          }
          class="empty-state__icon"
          aria-hidden="true"
        >
          {#if filter === 'unread'}
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="1.9"
              stroke-linecap="round"
              stroke-linejoin="round"
            >
              <path d="m5 12 4 4L19 6"></path>
            </svg>
          {:else}
            <svg
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="1.8"
              stroke-linecap="round"
              stroke-linejoin="round"
            >
              <path
                d="M18 8a6 6 0 0 0-12 0c0 7-3 7-3 9h18c0-2-3-2-3-9"
              ></path>

              <path
                d="M13.7 21a2 2 0 0 1-3.4 0"
              ></path>
            </svg>
          {/if}
        </span>

        <div>
          <p class="panel-kicker">
            {#if filter === 'unread'}
              Inbox clear
            {:else if filter === 'read'}
              No read activity
            {:else}
              Activity will appear here
            {/if}
          </p>

          <h2>
            {#if filter === 'unread'}
              You are all caught up.
            {:else if filter === 'read'}
              No notifications have been read yet.
            {:else}
              No notifications yet.
            {/if}
          </h2>

          <p>
            {#if filter === 'unread'}
              There is no new workspace or account
              activity waiting for your attention.
            {:else if filter === 'read'}
              Open a notification and it will become
              part of your reviewed activity.
            {:else}
              Workspace events, project updates,
              package activity and important account
              changes will appear here.
            {/if}
          </p>
        </div>

        {#if filter !== 'all'}
          <button
            class="secondary-button"
            type="button"
            onclick={() =>
              void selectFilter('all')}
          >
            Show all notifications
          </button>
        {/if}
      </section>

    <!-- =================================================
         NOTIFICATION GROUPS
         ================================================= -->
    {:else}
      <div class="notification-groups">
        {#each groups as group (group.key)}
          <section
            class="notification-group"
            aria-labelledby={`group-${group.key}`}
          >
            <header class="group-header">
              <h3 id={`group-${group.key}`}>
                {group.label}
              </h3>

              <span>
                {group.items.length}
              </span>
            </header>

            <div class="notification-list">
              {#each group.items as item (item.id)}
                {@const kind =
                  notificationKind(
                    item.type
                  )}

                <a
                  class:notification-row--unread={
                    !item.read_at
                  }
                  class="notification-row"
                  href={notificationHref(item)}
                  aria-label={`${item.read_at
                    ? ''
                    : 'Unread notification: '}${item.title}`}
                  onclick={(event) =>
                    void openNotification(
                      event,
                      item
                    )}
                >
                  <span
                    class={`notification-icon notification-icon--${kind}`}
                    aria-hidden="true"
                  >
                    {#if kind === 'workspace'}
                      <svg
                        viewBox="0 0 24 24"
                      >
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
                    {:else if kind === 'project'}
                      <svg
                        viewBox="0 0 24 24"
                      >
                        <path
                          d="M3 6h6l2 2h10v11H3Z"
                        ></path>

                        <path d="M7 12h10"></path>
                      </svg>
                    {:else if kind === 'package'}
                      <svg
                        viewBox="0 0 24 24"
                      >
                        <path
                          d="M3 7.5 12 3l9 4.5v9L12 21l-9-4.5Z"
                        ></path>

                        <path
                          d="M3 7.5 12 12l9-4.5"
                        ></path>

                        <path d="M12 12v9"></path>
                      </svg>
                    {:else if kind === 'build'}
                      <svg
                        viewBox="0 0 24 24"
                      >
                        <path d="M5 4h14v16H5Z"></path>
                        <path d="m8 9 2 2-2 2"></path>
                        <path d="M12 13h4"></path>
                      </svg>
                    {:else if kind === 'security'}
                      <svg
                        viewBox="0 0 24 24"
                      >
                        <rect
                          x="5"
                          y="10"
                          width="14"
                          height="11"
                          rx="2"
                        ></rect>

                        <path
                          d="M8 10V7a4 4 0 0 1 8 0v3"
                        ></path>
                      </svg>
                    {:else if kind === 'message'}
                      <svg
                        viewBox="0 0 24 24"
                      >
                        <path
                          d="M21 15a4 4 0 0 1-4 4H8l-5 3V7a4 4 0 0 1 4-4h10a4 4 0 0 1 4 4Z"
                        ></path>

                        <path d="M8 9h8"></path>
                        <path d="M8 13h5"></path>
                      </svg>
                    {:else}
                      <svg
                        viewBox="0 0 24 24"
                      >
                        <path
                          d="M18 8a6 6 0 0 0-12 0c0 7-3 7-3 9h18c0-2-3-2-3-9"
                        ></path>

                        <path
                          d="M13.7 21a2 2 0 0 1-3.4 0"
                        ></path>
                      </svg>
                    {/if}
                  </span>

                  <span class="notification-content">
                    <span class="notification-title-line">
                      <strong>
                        {item.title}
                      </strong>

                      {#if !item.read_at}
                        <span class="unread-label">
                          Unread
                        </span>
                      {/if}
                    </span>

                    <span class="notification-message">
                      {item.message}
                    </span>

                    <span class="notification-mobile-meta">
                      {notificationTypeLabel(
                        item.type
                      )}

                      <i aria-hidden="true">·</i>

                      {formatNotificationDate(
                        item.created_at
                      )}
                    </span>
                  </span>

                  <span class="notification-meta">
                    <span class="notification-type">
                      {notificationTypeLabel(
                        item.type
                      )}
                    </span>

                    <time
                      datetime={
                        timestampFor(
                          item.created_at
                        )
                          ? new Date(
                              timestampFor(
                                item.created_at
                              ) as number
                            ).toISOString()
                          : undefined
                      }
                      title={fullNotificationDate(
                        item.created_at
                      )}
                    >
                      {formatNotificationDate(
                        item.created_at
                      )}
                    </time>
                  </span>

                  <span
                    class="notification-arrow"
                    aria-hidden="true"
                  >
                    <svg
                      viewBox="0 0 24 24"
                    >
                      <path d="M5 12h14"></path>
                      <path d="m13 6 6 6-6 6"></path>
                    </svg>
                  </span>
                </a>
              {/each}
            </div>
          </section>
        {/each}
      </div>
    {/if}
  </section>
</div>

<style>
  .notifications-page {
    display: grid;
    max-width: 1120px;
    gap: 18px;
    margin-inline: auto;
    padding-bottom: 44px;
    animation:
      page-enter
      420ms var(--ease) both;
  }

  @keyframes page-enter {
    from {
      opacity: 0;
      transform: translateY(7px);
    }
  }

  /* =====================================================
     PAGE HEADER
     ===================================================== */

  .page-header {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 24px;
    padding-bottom: 22px;
    border-bottom: 1px solid var(--line);
  }

  .page-heading {
    display: grid;
    gap: 7px;
  }

  .page-eyebrow {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    color: var(--brand-bright);
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 0.1em;
    text-transform: uppercase;
  }

  .page-eyebrow > span {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: var(--brand);
    box-shadow:
      0 0 0 4px
      var(--brand-faint);
  }

  .page-heading h1 {
    color: var(--text);
    font-size:
      clamp(30px, 4vw, 44px);
    font-weight: 650;
    line-height: 1;
    letter-spacing: -0.045em;
  }

  .page-heading > p:last-child {
    max-width: 650px;
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.6;
  }

  .page-header__actions {
    display: flex;
    flex: 0 0 auto;
    align-items: center;
    gap: 9px;
  }

  .unread-summary {
    display: grid;
    min-width: 205px;
    min-height: 44px;
    grid-template-columns:
      32px minmax(0, 1fr);
    align-items: center;
    gap: 9px;
    padding: 5px 10px;
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
    background: var(--bg-panel);
  }

  .unread-summary > span {
    display: grid;
    width: 31px;
    height: 31px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 8px;
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 11px;
    font-weight: 700;
  }

  .unread-summary.has-unread {
    border-color: var(--brand-line);
  }

  .unread-summary.has-unread > span {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .unread-summary > div {
    display: grid;
    gap: 2px;
  }

  .unread-summary strong {
    color: var(--text-soft);
    font-size: 11.5px;
    font-weight: 600;
  }

  .unread-summary small {
    color: var(--text-muted);
    font-size: 10px;
  }

  .mark-all-button {
    display: inline-flex;
    min-height: 42px;
    align-items: center;
    justify-content: center;
    gap: 7px;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    background: transparent;
    color: var(--text-soft);
    padding: 0 13px;
    font: inherit;
    font-size: 11.5px;
    font-weight: 600;
    cursor: pointer;
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease),
      color var(--speed) var(--ease);
  }

  .mark-all-button:hover:not(:disabled) {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .mark-all-button:disabled {
    cursor: wait;
    opacity: 0.55;
  }

  .mark-all-button svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.9;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .button-spinner {
    width: 14px;
    height: 14px;
    border: 2px solid
      var(--line-strong);
    border-top-color: var(--brand);
    border-radius: 50%;
    animation:
      spinner-rotation
      700ms linear infinite;
  }

  @keyframes spinner-rotation {
    to {
      transform: rotate(360deg);
    }
  }

  /* =====================================================
     FEEDBACK
     ===================================================== */

  .success-message {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 11px 13px;
    border: 1px solid var(--green-line);
    border-radius: var(--radius-sm);
    background: var(--green-faint);
  }

  .success-message > span {
    display: grid;
    width: 27px;
    height: 27px;
    flex: 0 0 auto;
    place-items: center;
    border-radius: 7px;
    color: var(--green-soft);
  }

  .success-message svg {
    width: 14px;
    height: 14px;
  }

  .success-message p {
    color: var(--green-soft);
    font-size: 12px;
  }

  /* =====================================================
     PANEL
     ===================================================== */

  .notifications-panel {
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .panel-header {
    display: flex;
    min-height: 88px;
    align-items: center;
    justify-content: space-between;
    gap: 20px;
    padding: 16px 18px;
    border-bottom: 1px solid var(--line-soft);
  }

  .panel-header > div {
    display: grid;
    gap: 4px;
  }

  .panel-kicker {
    color: var(--brand-soft);
    font-family: var(--font-mono);
    font-size: 9.5px;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }

  .panel-header h2 {
    color: var(--text);
    font-size: 17px;
    font-weight: 620;
    letter-spacing: -0.02em;
  }

  .panel-header p:not(.panel-kicker) {
    color: var(--text-muted);
    font-size: 12px;
    line-height: 1.5;
  }

  .visible-count {
    display: grid;
    min-width: 29px;
    height: 27px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    color: var(--text-muted);
    padding-inline: 8px;
    font-family: var(--font-mono);
    font-size: 11px;
  }

  /* =====================================================
     TOOLBAR
     ===================================================== */

  .notification-toolbar {
    display: flex;
    min-height: 58px;
    align-items: center;
    justify-content: space-between;
    gap: 18px;
    padding: 10px 14px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .filter-tabs {
    display: flex;
    flex-wrap: wrap;
    gap: 4px;
  }

  .filter-tabs button {
    display: inline-flex;
    min-height: 34px;
    align-items: center;
    justify-content: center;
    gap: 7px;
    border: 1px solid transparent;
    border-radius: 7px;
    background: transparent;
    color: var(--text-muted);
    padding: 0 11px;
    font: inherit;
    font-size: 11.5px;
    font-weight: 550;
    cursor: pointer;
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease),
      color var(--speed) var(--ease);
  }

  .filter-tabs button:hover {
    border-color: var(--line);
    background: var(--bg-elevated);
    color: var(--text-soft);
  }

  .filter-tabs button.active {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-bright);
  }

  .filter-tabs button span {
    display: grid;
    min-width: 19px;
    height: 19px;
    place-items: center;
    border: 1px solid var(--line);
    border-radius: 999px;
    padding-inline: 5px;
    font-family: var(--font-mono);
    font-size: 10px;
  }

  .filter-tabs button.active span {
    border-color: var(--brand-line);
    color: var(--brand-soft);
  }

  .toolbar-description {
    color: var(--text-muted);
    font-size: 11px;
    line-height: 1.5;
    text-align: right;
  }

  /* =====================================================
     LOADING
     ===================================================== */

  .notification-loading {
    display: grid;
  }

  .notification-skeleton {
    display: grid;
    min-height: 86px;
    grid-template-columns:
      42px minmax(0, 1fr) 110px;
    align-items: center;
    gap: 13px;
    padding: 14px 17px;
    border-bottom: 1px solid var(--line-soft);
  }

  .notification-skeleton:last-child {
    border-bottom: 0;
  }

  .skeleton-icon,
  .skeleton-title,
  .skeleton-message,
  .skeleton-date {
    display: block;
    overflow: hidden;
    background: var(--bg-elevated);
  }

  .skeleton-icon {
    width: 40px;
    height: 40px;
    border-radius: 9px;
  }

  .notification-skeleton > div {
    display: grid;
    gap: 9px;
  }

  .skeleton-title {
    width: min(55%, 320px);
    height: 12px;
    border-radius: 4px;
  }

  .skeleton-message {
    width: min(85%, 560px);
    height: 10px;
    border-radius: 4px;
  }

  .skeleton-date {
    width: 90px;
    height: 10px;
    justify-self: end;
    border-radius: 4px;
  }

  .skeleton-icon::after,
  .skeleton-title::after,
  .skeleton-message::after,
  .skeleton-date::after {
    content: '';
    display: block;
    width: 45%;
    height: 100%;
    background: rgba(
      255,
      255,
      255,
      0.035
    );
    animation:
      skeleton-move
      1.2s ease-in-out infinite;
  }

  @keyframes skeleton-move {
    from {
      transform: translateX(-100%);
    }

    to {
      transform: translateX(320%);
    }
  }

  /* =====================================================
     EMPTY STATE
     ===================================================== */

  .empty-state {
    display: grid;
    min-height: 390px;
    align-content: center;
    justify-items: start;
    gap: 16px;
    padding:
      clamp(30px, 6vw, 58px);
  }

  .empty-state__icon {
    display: grid;
    width: 50px;
    height: 50px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 11px;
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .empty-state__icon--complete {
    border-color: var(--green-line);
    background: var(--green-faint);
    color: var(--green-soft);
  }

  .empty-state__icon svg {
    width: 23px;
    height: 23px;
  }

  .empty-state > div {
    display: grid;
    gap: 8px;
  }

  .empty-state h2 {
    max-width: 650px;
    color: var(--text);
    font-size:
      clamp(28px, 4vw, 42px);
    font-weight: 630;
    line-height: 1.04;
    letter-spacing: -0.045em;
  }

  .empty-state div > p:not(.panel-kicker) {
    max-width: 600px;
    color: var(--text-muted);
    font-size: 12.5px;
    line-height: 1.65;
  }

  .secondary-button {
    display: inline-flex;
    min-height: 38px;
    align-items: center;
    justify-content: center;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    background: transparent;
    color: var(--text-soft);
    padding: 0 13px;
    font: inherit;
    font-size: 11.5px;
    font-weight: 600;
    cursor: pointer;
  }

  .secondary-button:hover {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  /* =====================================================
     GROUPS
     ===================================================== */

  .notification-groups {
    display: grid;
  }

  .notification-group {
    display: grid;
  }

  .notification-group +
    .notification-group {
    border-top: 1px solid var(--line);
  }

  .group-header {
    display: flex;
    min-height: 42px;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    padding: 0 17px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .group-header h3 {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 0.07em;
    text-transform: uppercase;
  }

  .group-header > span {
    display: grid;
    min-width: 21px;
    height: 21px;
    place-items: center;
    border: 1px solid var(--line);
    border-radius: 999px;
    color: var(--text-muted);
    padding-inline: 5px;
    font-family: var(--font-mono);
    font-size: 10px;
  }

  /* =====================================================
     NOTIFICATION ROWS
     ===================================================== */

  .notification-list {
    display: grid;
  }

  .notification-row {
    position: relative;
    display: grid;
    min-height: 88px;
    grid-template-columns:
      42px minmax(0, 1fr) auto 27px;
    align-items: center;
    gap: 13px;
    padding: 14px 16px;
    border-bottom: 1px solid var(--line-soft);
    background: transparent;
    color: var(--text);
    text-decoration: none;
    transition:
      background var(--speed) var(--ease),
      box-shadow var(--speed) var(--ease);
  }

  .notification-row:last-child {
    border-bottom: 0;
  }

  .notification-row:hover {
    background: var(--bg-elevated);
  }

  .notification-row--unread {
    background: var(--brand-faint);
    box-shadow:
      inset 3px 0 0
      var(--brand);
  }

  .notification-row--unread:hover {
    background:
      rgba(249, 115, 22, 0.14);
  }

  .notification-icon {
    display: grid;
    width: 41px;
    height: 41px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 9px;
    background: var(--bg-ink-soft);
    color: var(--text-muted);
  }

  .notification-row--unread
    .notification-icon {
    border-color: var(--brand-line);
    color: var(--brand-soft);
  }

  .notification-icon svg {
    width: 19px;
    height: 19px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.75;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .notification-content {
    display: grid;
    min-width: 0;
    gap: 5px;
  }

  .notification-title-line {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 8px;
  }

  .notification-title-line strong {
    min-width: 0;
    overflow: hidden;
    color: var(--text-soft);
    font-size: 12px;
    font-weight: 580;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .notification-row--unread
    .notification-title-line strong {
    color: var(--text);
    font-weight: 650;
  }

  .unread-label {
    display: inline-flex;
    min-height: 21px;
    flex: 0 0 auto;
    align-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 999px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    padding: 0 7px;
    font-size: 9.5px;
    font-weight: 650;
  }

  .notification-message {
    display: -webkit-box;
    overflow: hidden;
    color: var(--text-muted);
    font-size: 11.5px;
    line-height: 1.5;
    -webkit-box-orient: vertical;
    -webkit-line-clamp: 2;
    line-clamp: 2;
  }

  .notification-meta {
    display: grid;
    min-width: 130px;
    justify-items: end;
    gap: 5px;
  }

  .notification-type {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 650;
    letter-spacing: 0.05em;
    text-transform: uppercase;
  }

  .notification-meta time {
    color: var(--text-faint);
    font-size: 11px;
    white-space: nowrap;
  }

  .notification-arrow {
    display: grid;
    width: 27px;
    height: 27px;
    place-items: center;
    color: var(--text-faint);
    transition:
      color var(--speed) var(--ease),
      transform var(--speed) var(--ease);
  }

  .notification-row:hover
    .notification-arrow {
    color: var(--brand-soft);
    transform: translateX(2px);
  }

  .notification-arrow svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.9;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .notification-mobile-meta {
    display: none;
    align-items: center;
    gap: 5px;
    color: var(--text-faint);
    font-size: 10.5px;
  }

  .notification-mobile-meta i {
    font-style: normal;
  }

  /* =====================================================
     RESPONSIVE
     ===================================================== */

  @media (prefers-reduced-motion: reduce) {
    .notifications-page,
    .button-spinner,
    .skeleton-icon::after,
    .skeleton-title::after,
    .skeleton-message::after,
    .skeleton-date::after {
      animation: none;
    }

    .notification-arrow {
      transition: none;
    }
  }

  @media (max-width: 820px) {
    .page-header {
      align-items: stretch;
      flex-direction: column;
    }

    .page-header__actions {
      align-items: stretch;
      flex-direction: column;
    }

    .unread-summary,
    .mark-all-button {
      width: 100%;
    }

    .notification-toolbar {
      align-items: stretch;
      flex-direction: column;
    }

    .toolbar-description {
      text-align: left;
    }
  }

  @media (max-width: 660px) {
    .panel-header {
      align-items: flex-start;
    }

    .notification-row {
      grid-template-columns:
        40px minmax(0, 1fr) 24px;
      gap: 11px;
      padding: 14px;
    }

    .notification-meta {
      display: none;
    }

    .notification-mobile-meta {
      display: flex;
    }

    .notification-message {
      -webkit-line-clamp: 3;
      line-clamp: 3;
    }

    .notification-skeleton {
      grid-template-columns:
        40px minmax(0, 1fr);
    }

    .skeleton-date {
      display: none;
    }
  }

  @media (max-width: 500px) {
    .filter-tabs {
      display: grid;
      width: 100%;
      grid-template-columns:
        repeat(3, minmax(0, 1fr));
    }

    .filter-tabs button {
      padding-inline: 7px;
    }

    .notification-row {
      grid-template-columns:
        36px minmax(0, 1fr);
    }

    .notification-icon {
      width: 35px;
      height: 35px;
    }

    .notification-icon svg {
      width: 17px;
      height: 17px;
    }

    .notification-arrow {
      display: none;
    }

    .notification-title-line {
      align-items: flex-start;
      flex-direction: column;
      gap: 5px;
    }

    .notification-title-line strong {
      white-space: normal;
    }

    .empty-state {
      min-height: 360px;
      padding: 28px 20px;
    }
  }
</style>
