<script lang="ts">
  import { onMount } from 'svelte';
  import {
    createFeedback,
    listMyFeedback
  } from '$lib/api/feedback';
  import {
    ApiError,
    type FeedbackItem
  } from '$lib/api/types';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import LoadingButton from '$lib/components/LoadingButton.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import { workspaceContext } from '$lib/stores/workspace';

  const MAX_TITLE_LENGTH = 160;
  const MAX_MESSAGE_LENGTH = 4000;

  const CATEGORIES = [
    {
      value: 'bug',
      label: 'Bug',
      description: 'Something is not working correctly.'
    },
    {
      value: 'idea',
      label: 'Idea',
      description: 'A new capability or direction.'
    },
    {
      value: 'improvement',
      label: 'Improvement',
      description: 'A better way to handle an existing feature.'
    },
    {
      value: 'question',
      label: 'Question',
      description: 'Something that needs clarification.'
    },
    {
      value: 'other',
      label: 'Other',
      description: 'Feedback that does not fit another category.'
    }
  ] as const;

  let category = 'improvement';
  let title = '';
  let message = '';
  let includeWorkspace = true;

  let feedback: FeedbackItem[] = [];

  let loading = true;
  let saving = false;

  let error = '';
  let success = '';

  let feedbackQuery = '';
  let categoryFilter = 'all';
  let statusFilter = 'all';
  let openFeedbackId = '';

  $: selectedCategory =
    CATEGORIES.find(
      (item) => item.value === category
    ) ?? CATEGORIES[2];

  $: statusOptions = Array.from(
    new Set(
      feedback
        .map((item) => item.status)
        .filter(
          (status): status is string =>
            Boolean(status)
        )
    )
  );

  $: visibleFeedback = feedback.filter((item) => {
    const query = feedbackQuery
      .trim()
      .toLowerCase();

    const matchesQuery =
      !query ||
      item.title.toLowerCase().includes(query) ||
      item.message.toLowerCase().includes(query) ||
      item.category?.toLowerCase().includes(query) ||
      item.status?.toLowerCase().includes(query);

    const matchesCategory =
      categoryFilter === 'all' ||
      item.category === categoryFilter;

    const matchesStatus =
      statusFilter === 'all' ||
      item.status === statusFilter;

    return (
      matchesQuery &&
      matchesCategory &&
      matchesStatus
    );
  });

  $: bugCount = feedback.filter(
    (item) => item.category === 'bug'
  ).length;

  $: ideaCount = feedback.filter(
    (item) => item.category === 'idea'
  ).length;

  $: resolvedCount = feedback.filter((item) =>
    ['resolved', 'closed', 'completed'].includes(
      item.status?.toLowerCase() ?? ''
    )
  ).length;

  $: activeCount =
    feedback.length - resolvedCount;

  $: messageLength = message.length;

  function categoryLabel(value?: string) {
    return (
      CATEGORIES.find(
        (item) => item.value === value
      )?.label ??
      value ??
      'Other'
    );
  }

  function statusLabel(value?: string) {
    if (!value) {
      return 'Unknown';
    }

    return value
      .replace(/[_-]+/g, ' ')
      .replace(/\b\w/g, (letter) =>
        letter.toUpperCase()
      );
  }

  function formatDate(value?: string | number) {
    if (
      value === undefined ||
      value === null ||
      value === ''
    ) {
      return 'Unknown date';
    }

    let date: Date;

    if (typeof value === 'number') {
      date = new Date(
        value < 1_000_000_000_000
          ? value * 1000
          : value
      );
    } else {
      const numericValue = Number(value);

      date =
        Number.isFinite(numericValue) &&
        value.trim() !== ''
          ? new Date(
              numericValue < 1_000_000_000_000
                ? numericValue * 1000
                : numericValue
            )
          : new Date(value);
    }

    if (Number.isNaN(date.getTime())) {
      return 'Unknown date';
    }

    return new Intl.DateTimeFormat(undefined, {
      dateStyle: 'medium',
      timeStyle: 'short'
    }).format(date);
  }

  function resetForm() {
    category = 'improvement';
    title = '';
    message = '';
    includeWorkspace = true;
  }

  function resetFilters() {
    feedbackQuery = '';
    categoryFilter = 'all';
    statusFilter = 'all';
  }

  function toggleFeedback(itemId: string) {
    openFeedbackId =
      openFeedbackId === itemId
        ? ''
        : itemId;
  }

  function handleSubmit(event: SubmitEvent) {
    event.preventDefault();
    void submit();
  }

  async function load() {
    loading = true;
    error = '';

    try {
      const data = await listMyFeedback();
      feedback = data.feedback;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load feedback.';
    } finally {
      loading = false;
    }
  }

  async function submit() {
    const cleanTitle = title.trim();
    const cleanMessage = message.trim();

    if (!cleanTitle || !cleanMessage) {
      error = 'Title and message are required.';
      return;
    }

    if (cleanTitle.length > MAX_TITLE_LENGTH) {
      error =
        `Title cannot exceed ${MAX_TITLE_LENGTH} characters.`;
      return;
    }

    if (cleanMessage.length > MAX_MESSAGE_LENGTH) {
      error =
        `Message cannot exceed ${MAX_MESSAGE_LENGTH} characters.`;
      return;
    }

    saving = true;
    error = '';
    success = '';

    try {
      const item = await createFeedback({
        category,
        title: cleanTitle,
        message: cleanMessage,
        workspaceId:
          includeWorkspace
            ? $workspaceContext.selectedWorkspace?.id
            : undefined
      });

      feedback = [
        item,
        ...feedback
      ];

      openFeedbackId = item.id;

      resetForm();

      success =
        'Feedback sent. Thank you for helping improve Softadastra Cloud.';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to send feedback.';
    } finally {
      saving = false;
    }
  }

  onMount(() => {
    void load();
  });
</script>

<svelte:head>
  <title>Feedback | Softadastra Cloud</title>
</svelte:head>

<PageHeader
  eyebrow="Support"
  title="Feedback"
/>

<p class="page-description">
  Report a problem, suggest an improvement or ask a question about
  Softadastra Cloud.
</p>

<InlineError message={error} />

{#if success}
  <p class="success-message" role="status">
    {success}
  </p>
{/if}

<section
  class="feedback-summary"
  aria-label="Feedback summary"
>
  <div class="summary-item">
    <span>Total</span>
    <strong>{feedback.length}</strong>
    <small>Submitted feedback</small>
  </div>

  <div class="summary-item">
    <span>Active</span>
    <strong>{activeCount}</strong>
    <small>Still being reviewed</small>
  </div>

  <div class="summary-item">
    <span>Bugs</span>
    <strong>{bugCount}</strong>
    <small>Reported problems</small>
  </div>

  <div class="summary-item">
    <span>Ideas</span>
    <strong>{ideaCount}</strong>
    <small>Suggested directions</small>
  </div>
</section>

<div class="feedback-layout">
  <aside class="feedback-form-panel">
    <div class="form-header">
      <div>
        <h2>Send feedback</h2>

        <p>
          Give enough context to understand the problem or suggestion.
        </p>
      </div>
    </div>

    <form onsubmit={handleSubmit}>
      <label>
        Category

        <select bind:value={category}>
          {#each CATEGORIES as item}
            <option value={item.value}>
              {item.label}
            </option>
          {/each}
        </select>

        <small class="field-description">
          {selectedCategory.description}
        </small>
      </label>

      <label>
        Title

        <input
          bind:value={title}
          maxlength={MAX_TITLE_LENGTH}
          placeholder="Describe the feedback in a few words"
          required
          autocomplete="off"
        />

        <small class="field-counter">
          {title.length}/{MAX_TITLE_LENGTH}
        </small>
      </label>

      <label>
        Message

        <textarea
          bind:value={message}
          maxlength={MAX_MESSAGE_LENGTH}
          rows="9"
          placeholder="Explain what happened, what you expected or what should be improved."
          required
        ></textarea>

        <small
          class:near-limit={
            messageLength >
            MAX_MESSAGE_LENGTH * 0.9
          }
          class="field-counter"
        >
          {messageLength}/{MAX_MESSAGE_LENGTH}
        </small>
      </label>

      {#if $workspaceContext.selectedWorkspace}
        <label class="workspace-option">
          <input
            bind:checked={includeWorkspace}
            type="checkbox"
          />

          <span>
            <strong>Attach current workspace</strong>

            <small>
              {$workspaceContext.selectedWorkspace.name}
            </small>
          </span>
        </label>
      {/if}

      <div class="privacy-note">
        <svg viewBox="0 0 24 24" aria-hidden="true">
          <rect
            x="5"
            y="10"
            width="14"
            height="10"
            rx="2"
          ></rect>

          <path d="M8 10V7a4 4 0 0 1 8 0v3"></path>
        </svg>

        <p>
          Do not include passwords, access tokens, private keys or
          other sensitive credentials.
        </p>
      </div>

      <div class="form-actions">
        <LoadingButton
          loading={saving}
          loadingLabel="Sending…"
        >
          Send feedback
        </LoadingButton>
      </div>
    </form>
  </aside>

  <main
    class="feedback-history"
    aria-labelledby="feedback-history-title"
  >
    <div class="history-header">
      <div>
        <h2 id="feedback-history-title">
          Your feedback
        </h2>

        <p>
          Feedback and support requests submitted by your account.
        </p>
      </div>

      <span class="section-count">
        {visibleFeedback.length}
      </span>
    </div>

    {#if feedback.length > 0}
      <div class="feedback-toolbar">
        <div class="feedback-search">
          <svg viewBox="0 0 24 24" aria-hidden="true">
            <circle
              cx="11"
              cy="11"
              r="7"
            ></circle>

            <path d="m20 20-4-4"></path>
          </svg>

          <input
            bind:value={feedbackQuery}
            type="search"
            placeholder="Find feedback"
            aria-label="Find feedback"
          />
        </div>

        <select
          bind:value={categoryFilter}
          aria-label="Filter feedback by category"
        >
          <option value="all">
            All categories
          </option>

          {#each CATEGORIES as item}
            <option value={item.value}>
              {item.label}
            </option>
          {/each}
        </select>

        <select
          bind:value={statusFilter}
          aria-label="Filter feedback by status"
        >
          <option value="all">
            All statuses
          </option>

          {#each statusOptions as feedbackStatus}
            <option value={feedbackStatus}>
              {statusLabel(feedbackStatus)}
            </option>
          {/each}
        </select>
      </div>
    {/if}

    {#if loading}
      <p class="loading-state">
        Loading feedback…
      </p>
    {:else if feedback.length === 0}
      <div class="empty-wrapper">
        <EmptyState
          title="Nothing sent yet"
          body="Your ideas, bug reports and questions will appear here."
        />
      </div>
    {:else if visibleFeedback.length === 0}
      <div class="no-results">
        <strong>No matching feedback</strong>

        <p>
          Change the search text or selected filters.
        </p>

        <button
          class="reset-filters-button"
          type="button"
          onclick={resetFilters}
        >
          Reset filters
        </button>
      </div>
    {:else}
      <div class="feedback-list">
        {#each visibleFeedback as item (item.id)}
          <article
            class:expanded={
              openFeedbackId === item.id
            }
            class="feedback-entry"
          >
            <button
              class="feedback-row"
              type="button"
              aria-expanded={
                openFeedbackId === item.id
              }
              onclick={() =>
                toggleFeedback(item.id)}
            >
              <span class="feedback-category">
                {categoryLabel(item.category)
                  .slice(0, 1)
                  .toUpperCase()}
              </span>

              <span class="feedback-identity">
                <strong>{item.title}</strong>

                <span class="feedback-metadata">
                  <span class="category-label">
                    {categoryLabel(item.category)}
                  </span>

                  <span aria-hidden="true">·</span>

                  <time>
                    {formatDate(item.created_at)}
                  </time>
                </span>

                <span class="feedback-preview">
                  {item.message}
                </span>
              </span>

              <span class="feedback-status">
                <StatusBadge status={item.status} />
              </span>

              <svg
                class:rotated={
                  openFeedbackId === item.id
                }
                class="row-chevron"
                viewBox="0 0 24 24"
                aria-hidden="true"
              >
                <path d="m9 18 6-6-6-6"></path>
              </svg>
            </button>

            {#if openFeedbackId === item.id}
              <div class="feedback-details">
                <div class="details-heading">
                  <div>
                    <span>Full message</span>

                    <strong>{item.title}</strong>
                  </div>

                  <StatusBadge status={item.status} />
                </div>

                <p>{item.message}</p>

                <dl class="feedback-properties">
                  <div>
                    <dt>Category</dt>

                    <dd>
                      {categoryLabel(item.category)}
                    </dd>
                  </div>

                  <div>
                    <dt>Status</dt>

                    <dd>
                      {statusLabel(item.status)}
                    </dd>
                  </div>

                  <div>
                    <dt>Submitted</dt>

                    <dd>
                      {formatDate(item.created_at)}
                    </dd>
                  </div>
                </dl>
              </div>
            {/if}
          </article>
        {/each}
      </div>
    {/if}
  </main>
</div>

<style>
  .page-description {
    max-width: 760px;
    margin: -8px 0 18px;
    color: var(--text-muted);
    font-size: 13px;
    line-height: 1.6;
  }

  /* Summary */

  .feedback-summary {
    display: grid;
    grid-template-columns: repeat(4, minmax(0, 1fr));
    margin-bottom: 16px;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    overflow: hidden;
  }

  .summary-item {
    display: grid;
    min-width: 0;
    gap: 3px;
    padding: 14px 16px;
    border-right: 1px solid var(--line-soft);
  }

  .summary-item:last-child {
    border-right: 0;
  }

  .summary-item > span {
    color: var(--text-muted);
    font-size: 11px;
  }

  .summary-item > strong {
    color: var(--text);
    font-family: var(--font-mono);
    font-size: 19px;
    font-weight: 600;
  }

  .summary-item > small {
    color: var(--text-faint);
    font-size: 10.5px;
  }

  /* Layout */

  .feedback-layout {
    display: grid;
    grid-template-columns:
      minmax(320px, 0.75fr)
      minmax(0, 1.45fr);
    gap: 16px;
    align-items: start;
  }

  .feedback-form-panel,
  .feedback-history {
    min-width: 0;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    overflow: hidden;
  }

  .feedback-form-panel {
    position: sticky;
    top: 24px;
  }

  .form-header,
  .history-header {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 14px;
    padding: 14px 16px;
    border-bottom: 1px solid var(--line-soft);
  }

  .form-header > div,
  .history-header > div {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .form-header h2,
  .history-header h2 {
    color: var(--text);
    font-size: 13.5px;
  }

  .form-header p,
  .history-header p {
    color: var(--text-muted);
    font-size: 11.5px;
    line-height: 1.5;
  }

  .section-count {
    display: inline-grid;
    min-width: 24px;
    height: 22px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    background: var(--bg-elevated);
    color: var(--text-muted);
    padding: 0 7px;
    font-family: var(--font-mono);
    font-size: 10.5px;
  }

  /* Form */

  .feedback-form-panel form {
    display: grid;
    gap: 15px;
    padding: 16px;
  }

  .feedback-form-panel textarea {
    min-height: 170px;
    resize: vertical;
    line-height: 1.55;
  }

  .field-description {
    color: var(--text-muted);
    font-size: 10.5px;
    font-weight: 400;
    line-height: 1.45;
  }

  .field-counter {
    justify-self: flex-end;
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 9.5px;
    font-weight: 400;
  }

  .field-counter.near-limit {
    color: var(--warning);
  }

  .workspace-option {
    display: flex;
    align-items: flex-start;
    gap: 9px;
    border: 1px solid var(--line-soft);
    border-radius: var(--radius-sm);
    background: var(--bg-ink-soft);
    padding: 10px;
    cursor: pointer;
  }

  .workspace-option input {
    width: 15px;
    height: 15px;
    min-height: 0;
    margin-top: 2px;
    flex: 0 0 auto;
    accent-color: var(--brand);
  }

  .workspace-option > span {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .workspace-option strong {
    color: var(--text-soft);
    font-size: 11.5px;
    font-weight: 550;
  }

  .workspace-option small {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 10.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .privacy-note {
    display: grid;
    grid-template-columns: 16px minmax(0, 1fr);
    gap: 8px;
    align-items: start;
    color: var(--text-muted);
  }

  .privacy-note svg {
    width: 15px;
    height: 15px;
    margin-top: 1px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  .privacy-note p {
    font-size: 10.5px;
    line-height: 1.5;
  }

  .form-actions {
    display: flex;
    justify-content: flex-start;
    border-top: 1px solid var(--line-soft);
    padding-top: 14px;
  }

  /* Toolbar */

  .feedback-toolbar {
    display: grid;
    grid-template-columns:
      minmax(220px, 1fr)
      minmax(135px, 180px)
      minmax(135px, 180px);
    gap: 10px;
    padding: 10px 12px;
    border-bottom: 1px solid var(--line-soft);
    background: var(--bg-ink-soft);
  }

  .feedback-search {
    position: relative;
  }

  .feedback-search svg {
    position: absolute;
    top: 50%;
    left: 11px;
    width: 14px;
    height: 14px;
    fill: none;
    stroke: var(--text-muted);
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
    transform: translateY(-50%);
    pointer-events: none;
  }

  .feedback-search input {
    min-height: 34px;
    padding-left: 33px;
    font-size: 12px;
  }

  .feedback-toolbar select {
    min-height: 34px;
    font-size: 12px;
  }

  /* Feedback entries */

  .feedback-list {
    display: grid;
  }

  .feedback-entry {
    min-width: 0;
    border-bottom: 1px solid var(--line-soft);
  }

  .feedback-entry:last-child {
    border-bottom: 0;
  }

  .feedback-entry.expanded {
    background: var(--bg-ink-soft);
  }

  .feedback-row {
    display: grid;
    grid-template-columns:
      34px
      minmax(0, 1fr)
      auto
      18px;
    gap: 12px;
    align-items: center;
    width: 100%;
    min-height: 82px;
    border: 0;
    border-radius: 0;
    background: transparent;
    padding: 12px 16px;
    color: var(--text);
    text-align: left;
  }

  .feedback-row:hover:not(:disabled) {
    background: rgba(255, 255, 255, 0.018);
    transform: none;
  }

  .feedback-category {
    display: grid;
    width: 34px;
    height: 34px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    background: var(--bg-elevated);
    color: var(--text-soft);
    font-size: 12px;
    font-weight: 650;
  }

  .feedback-identity {
    display: grid;
    min-width: 0;
    gap: 4px;
  }

  .feedback-identity > strong {
    color: var(--text);
    font-size: 12.5px;
    font-weight: 600;
    overflow-wrap: anywhere;
  }

  .feedback-metadata {
    display: flex;
    min-width: 0;
    align-items: center;
    gap: 5px;
    color: var(--text-muted);
    font-size: 10.5px;
    flex-wrap: wrap;
  }

  .category-label {
    color: var(--brand-bright);
    font-family: var(--font-mono);
    font-size: 9.5px;
    font-weight: 600;
    letter-spacing: 0.05em;
    text-transform: uppercase;
  }

  .feedback-preview {
    display: -webkit-box;
    color: var(--text-muted);
    font-size: 11px;
    line-height: 1.5;
    overflow: hidden;
    line-clamp: 2;
    -webkit-line-clamp: 2;
    -webkit-box-orient: vertical;
  }

  .feedback-status {
    display: flex;
    align-items: center;
    justify-content: flex-end;
  }

  .row-chevron {
    width: 16px;
    height: 16px;
    fill: none;
    stroke: var(--text-muted);
    stroke-width: 1.7;
    stroke-linecap: round;
    stroke-linejoin: round;
    transition:
      transform var(--speed) var(--ease),
      stroke var(--speed) var(--ease);
  }

  .row-chevron.rotated {
    transform: rotate(90deg);
    stroke: var(--text-soft);
  }

  /* Expanded details */

  .feedback-details {
    display: grid;
    gap: 16px;
    padding: 16px;
    border-top: 1px solid var(--line-soft);
    background: var(--bg-ink);
  }

  .details-heading {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 14px;
  }

  .details-heading > div {
    display: grid;
    min-width: 0;
    gap: 3px;
  }

  .details-heading span {
    color: var(--text-muted);
    font-size: 10px;
  }

  .details-heading strong {
    color: var(--text-soft);
    font-size: 12px;
    font-weight: 600;
  }

  .feedback-details > p {
    color: var(--text-soft);
    font-size: 12px;
    line-height: 1.7;
    white-space: pre-wrap;
    overflow-wrap: anywhere;
  }

  .feedback-properties {
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 1fr));
    margin: 0;
    border: 1px solid var(--line-soft);
    border-radius: var(--radius-sm);
    overflow: hidden;
  }

  .feedback-properties > div {
    display: grid;
    min-width: 0;
    gap: 3px;
    padding: 10px 12px;
    border-right: 1px solid var(--line-soft);
  }

  .feedback-properties > div:last-child {
    border-right: 0;
  }

  .feedback-properties dt {
    color: var(--text-muted);
    font-size: 9.5px;
  }

  .feedback-properties dd {
    margin: 0;
    color: var(--text-soft);
    font-size: 10.5px;
    font-weight: 550;
    overflow-wrap: anywhere;
  }

  .loading-state {
    min-height: 100px;
    padding: 24px 16px;
    color: var(--text-muted);
    font-size: 12px;
  }

  .empty-wrapper {
    padding: 14px;
  }

  .no-results {
    display: grid;
    min-height: 180px;
    place-content: center;
    justify-items: center;
    gap: 6px;
    padding: 24px;
    text-align: center;
  }

  .no-results strong {
    color: var(--text-soft);
    font-size: 13px;
  }

  .no-results p {
    color: var(--text-muted);
    font-size: 11.5px;
  }

  .reset-filters-button {
    min-height: 30px;
    margin-top: 6px;
    border-color: var(--info-line);
    background: var(--info-faint);
    color: var(--link);
    padding: 0 11px;
    font-size: 11px;
  }

  .reset-filters-button:hover:not(:disabled) {
    border-color: var(--link);
    background: rgba(107, 184, 255, 0.14);
    color: var(--link-hover);
  }

  @media (max-width: 980px) {
    .feedback-layout {
      grid-template-columns:
        minmax(290px, 0.8fr)
        minmax(0, 1.2fr);
    }

    .feedback-toolbar {
      grid-template-columns: 1fr 1fr;
    }

    .feedback-search {
      grid-column: 1 / -1;
    }
  }

  @media (max-width: 820px) {
    .feedback-summary {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }

    .summary-item:nth-child(2) {
      border-right: 0;
    }

    .summary-item:nth-child(-n + 2) {
      border-bottom: 1px solid var(--line-soft);
    }

    .feedback-layout {
      grid-template-columns: 1fr;
    }

    .feedback-form-panel {
      position: static;
    }
  }

  @media (max-width: 620px) {
    .feedback-toolbar {
      grid-template-columns: 1fr;
    }

    .feedback-search {
      grid-column: auto;
    }

    .feedback-row {
      grid-template-columns:
        32px
        minmax(0, 1fr)
        18px;
    }

    .feedback-status {
      grid-column: 2;
      justify-content: flex-start;
    }

    .row-chevron {
      grid-column: 3;
      grid-row: 1;
    }

    .feedback-properties {
      grid-template-columns: 1fr;
    }

    .feedback-properties > div {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .feedback-properties > div:last-child {
      border-bottom: 0;
    }
  }

  @media (max-width: 460px) {
    .feedback-summary {
      grid-template-columns: 1fr;
    }

    .summary-item {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .summary-item:last-child {
      border-bottom: 0;
    }

    .feedback-form-panel form {
      padding: 14px;
    }

    .feedback-row {
      padding: 12px 14px;
    }

    .details-heading {
      align-items: stretch;
      flex-direction: column;
    }

    .form-actions :global(button) {
      width: 100%;
    }
  }
</style>
