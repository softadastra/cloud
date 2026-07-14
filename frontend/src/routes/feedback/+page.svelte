<script lang="ts">
  import { tick } from 'svelte';

  import {
    closeFeedback,
    createFeedback,
    deleteFeedback,
    listMyFeedback,
    updateFeedback
  } from '$lib/api/feedback';

  import {
    ApiError,
    type FeedbackItem
  } from '$lib/api/types';

  import InlineError from '$lib/components/InlineError.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import WorkspaceAvatar from '$lib/components/WorkspaceAvatar.svelte';

  import { usePageRefresh } from '$lib/stores/pageRefresh';
  import { workspaceContext } from '$lib/stores/workspace';

  const MAX_TITLE_LENGTH = 160;
  const MAX_MESSAGE_LENGTH = 4000;

  const CATEGORIES = [
    {
      value: 'bug',
      label: 'Bug',
      description:
        'Something is not working as expected.'
    },
    {
      value: 'improvement',
      label: 'Improvement',
      description:
        'An existing workflow could work better.'
    },
    {
      value: 'idea',
      label: 'Idea',
      description:
        'A new capability or product direction.'
    },
    {
      value: 'question',
      label: 'Question',
      description:
        'Something about the product needs clarification.'
    },
    {
      value: 'other',
      label: 'Other',
      description:
        'Feedback that does not fit another category.'
    }
  ] as const;

  type ComposerMode =
    | 'create'
    | 'edit';

  type FeedbackFilter =
    | 'all'
    | 'active'
    | 'resolved';

  type ConfirmationAction =
    | 'close'
    | 'delete';

  type Confirmation = {
    item: FeedbackItem;
    action: ConfirmationAction;
  };

  let feedback: FeedbackItem[] = [];

  let loading = true;
  let saving = false;
  let busyFeedbackId = '';

  let error = '';
  let success = '';

  let showComposer = false;
  let composerMode: ComposerMode = 'create';
  let editingFeedbackId = '';

  let category = 'improvement';
  let title = '';
  let message = '';
  let includeWorkspace = true;

  let selectedFeedbackId = '';

  let feedbackQuery = '';
  let categoryFilter = 'all';
  let statusFilter: FeedbackFilter = 'all';

  let confirmation: Confirmation | null = null;
  let confirmationDialog: HTMLDialogElement | null = null;
  let deleteConfirmation = '';

  $: selectedWorkspace =
    $workspaceContext.selectedWorkspace ?? null;

  $: selectedCategory =
    CATEGORIES.find(
      (item) =>
        item.value === category
    ) ?? CATEGORIES[1];

  $: selectedFeedback =
    feedback.find(
      (item) =>
        item.id === selectedFeedbackId
    ) ?? null;

  $: resolvedCount =
    feedback.filter(isResolved).length;

  $: activeCount =
    feedback.length - resolvedCount;

  $: visibleFeedback =
    feedback.filter((item) => {
      const query =
        feedbackQuery
          .trim()
          .toLowerCase();

      const matchesQuery =
        !query ||
        item.title
          .toLowerCase()
          .includes(query) ||
        item.message
          .toLowerCase()
          .includes(query) ||
        item.category
          ?.toLowerCase()
          .includes(query) ||
        item.status
          ?.toLowerCase()
          .includes(query);

      const matchesCategory =
        categoryFilter === 'all' ||
        item.category === categoryFilter;

      const matchesStatus =
        statusFilter === 'all' ||
        (
          statusFilter === 'resolved'
            ? isResolved(item)
            : !isResolved(item)
        );

      return (
        matchesQuery &&
        matchesCategory &&
        matchesStatus
      );
    });

  $: confirmationNeedsTitle =
    confirmation?.action === 'delete';

  $: confirmationCanContinue =
    Boolean(confirmation) &&
    (
      !confirmationNeedsTitle ||
      deleteConfirmation ===
        confirmation?.item.title
    );

  $: if (
    !loading &&
    visibleFeedback.length > 0 &&
    !visibleFeedback.some(
      (item) =>
        item.id === selectedFeedbackId
    )
  ) {
    selectedFeedbackId =
      visibleFeedback[0].id;
  }

  $: if (
    selectedFeedbackId &&
    !feedback.some(
      (item) =>
        item.id === selectedFeedbackId
    )
  ) {
    selectedFeedbackId =
      feedback[0]?.id ?? '';
  }

  function categoryMeta(
    value?: string
  ) {
    return (
      CATEGORIES.find(
        (item) =>
          item.value === value
      ) ?? CATEGORIES[4]
    );
  }

  function categoryLabel(
    value?: string
  ) {
    return categoryMeta(value).label;
  }

  function categoryInitial(
    value?: string
  ) {
    return categoryLabel(value)
      .slice(0, 1)
      .toUpperCase();
  }

  function normalizeStatus(
    value?: string
  ) {
    return (
      value
        ?.trim()
        .toLowerCase() ||
      'open'
    );
  }

  function statusLabel(
    value?: string
  ) {
    return normalizeStatus(value)
      .replace(/[_-]+/g, ' ')
      .replace(
        /\b\w/g,
        (letter) =>
          letter.toUpperCase()
      );
  }

  function isResolved(
    item: FeedbackItem
  ) {
    return [
      'resolved',
      'closed',
      'completed'
    ].includes(
      normalizeStatus(item.status)
    );
  }

  function isInReview(
    item: FeedbackItem
  ) {
    return [
      'in_review',
      'reviewing',
      'in_progress',
      'planned'
    ].includes(
      normalizeStatus(item.status)
    );
  }

  function isEditable(
    item: FeedbackItem
  ) {
    return (
      normalizeStatus(item.status) ===
      'open'
    );
  }

  function statusStage(
    item: FeedbackItem
  ) {
    if (isResolved(item)) {
      return 3;
    }

    if (isInReview(item)) {
      return 2;
    }

    return 1;
  }

  function formatDate(
    value?: string | number
  ) {
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
        value <
        1_000_000_000_000
          ? value * 1000
          : value
      );
    } else {
      const numericValue =
        Number(value);

      date =
        Number.isFinite(numericValue) &&
        value.trim() !== ''
          ? new Date(
              numericValue <
              1_000_000_000_000
                ? numericValue * 1000
                : numericValue
            )
          : new Date(value);
    }

    if (
      Number.isNaN(
        date.getTime()
      )
    ) {
      return 'Unknown date';
    }

    return new Intl.DateTimeFormat(
      undefined,
      {
        dateStyle: 'medium',
        timeStyle: 'short'
      }
    ).format(date);
  }

  function resetForm() {
    category = 'improvement';
    title = '';
    message = '';
    includeWorkspace = true;
    editingFeedbackId = '';
    composerMode = 'create';
  }

  function resetFilters() {
    feedbackQuery = '';
    categoryFilter = 'all';
    statusFilter = 'all';
  }

  function openCreateComposer() {
    resetForm();

    showComposer = true;
    selectedFeedbackId = '';

    error = '';
    success = '';
  }

  function openEditComposer(
    item: FeedbackItem
  ) {
    if (!isEditable(item)) {
      error =
        'Only open feedback can be edited.';

      return;
    }

    composerMode = 'edit';
    editingFeedbackId = item.id;

    category =
      item.category ||
      'improvement';

    title = item.title;
    message = item.message;

    showComposer = true;
    selectedFeedbackId = item.id;

    error = '';
    success = '';
  }

  function closeComposer() {
    if (saving) {
      return;
    }

    showComposer = false;
    resetForm();
    error = '';

    if (
      !selectedFeedbackId &&
      feedback.length > 0
    ) {
      selectedFeedbackId =
        feedback[0].id;
    }
  }

  function toggleComposer() {
    if (showComposer) {
      closeComposer();
      return;
    }

    openCreateComposer();
  }

  function selectFeedback(
    item: FeedbackItem
  ) {
    selectedFeedbackId = item.id;
    showComposer = false;

    error = '';
    success = '';
  }

  async function load() {
    loading = true;
    error = '';

    try {
      const data =
        await listMyFeedback();

      feedback = data.feedback;

      if (feedback.length === 0) {
        resetForm();
        showComposer = true;
        selectedFeedbackId = '';
      } else if (
        !feedback.some(
          (item) =>
            item.id === selectedFeedbackId
        )
      ) {
        selectedFeedbackId =
          feedback[0].id;
      }
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
    const cleanTitle =
      title.trim();

    const cleanMessage =
      message.trim();

    if (
      !cleanTitle ||
      !cleanMessage
    ) {
      error =
        'Title and message are required.';

      return;
    }

    if (
      cleanTitle.length >
      MAX_TITLE_LENGTH
    ) {
      error =
        `Title cannot exceed ` +
        `${MAX_TITLE_LENGTH} characters.`;

      return;
    }

    if (
      cleanMessage.length >
      MAX_MESSAGE_LENGTH
    ) {
      error =
        `Message cannot exceed ` +
        `${MAX_MESSAGE_LENGTH} characters.`;

      return;
    }

    saving = true;
    error = '';
    success = '';

    try {
      if (
        composerMode === 'edit' &&
        editingFeedbackId
      ) {
        const current =
          feedback.find(
            (item) =>
              item.id ===
              editingFeedbackId
          );

        if (
          !current ||
          !isEditable(current)
        ) {
          error =
            'This feedback can no longer be edited.';

          return;
        }

        const updated =
          await updateFeedback({
            feedbackId:
              editingFeedbackId,

            category,

            title:
              cleanTitle,

            message:
              cleanMessage
          });

        feedback =
          feedback.map(
            (item) =>
              item.id === updated.id
                ? updated
                : item
          );

        selectedFeedbackId =
          updated.id;

        success =
          'Feedback updated successfully.';
      } else {
        const created =
          await createFeedback({
            category,

            title:
              cleanTitle,

            message:
              cleanMessage,

            workspaceId:
              includeWorkspace
                ? selectedWorkspace?.id
                : undefined
          });

        feedback = [
          created,
          ...feedback
        ];

        selectedFeedbackId =
          created.id;

        success =
          'Feedback sent. The request is now available in your history.';
      }

      showComposer = false;
      resetForm();
      statusFilter = 'all';
      categoryFilter = 'all';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : composerMode === 'edit'
            ? 'Unable to update feedback.'
            : 'Unable to send feedback.';
    } finally {
      saving = false;
    }
  }

  async function openConfirmation(
    item: FeedbackItem,
    action: ConfirmationAction
  ) {
    if (busyFeedbackId) {
      return;
    }

    confirmation = {
      item,
      action
    };

    deleteConfirmation = '';

    await tick();

    confirmationDialog?.focus();
  }

  function closeConfirmation() {
    if (busyFeedbackId) {
      return;
    }

    confirmation = null;
    deleteConfirmation = '';
  }

  async function confirmAction() {
    if (
      !confirmation ||
      !confirmationCanContinue
    ) {
      return;
    }

    const {
      item,
      action
    } = confirmation;

    busyFeedbackId = item.id;
    error = '';
    success = '';

    try {
      if (action === 'close') {
        const updated =
          await closeFeedback(item.id);

        feedback =
          feedback.map(
            (entry) =>
              entry.id === item.id
                ? updated
                : entry
          );

        selectedFeedbackId =
          updated.id;

        success =
          'Feedback closed.';
      } else {
        await deleteFeedback(item.id);

        feedback =
          feedback.filter(
            (entry) =>
              entry.id !== item.id
          );

        selectedFeedbackId =
          feedback[0]?.id ?? '';

        success =
          'Feedback deleted.';
      }

      confirmation = null;
      deleteConfirmation = '';
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : action === 'close'
            ? 'Unable to close feedback.'
            : 'Unable to delete feedback.';
    } finally {
      busyFeedbackId = '';
    }
  }

  function confirmationTitle(
    value: Confirmation
  ) {
    return value.action === 'delete'
      ? 'Delete this feedback?'
      : 'Close this feedback?';
  }

  function confirmationMessage(
    value: Confirmation
  ) {
    if (value.action === 'delete') {
      return (
        'This request will be removed from your ' +
        'feedback history. This action cannot be undone.'
      );
    }

    return (
      'The request will be marked as closed. ' +
      'Its message and history will remain visible.'
    );
  }

  function confirmationButtonLabel(
    value: Confirmation
  ) {
    return value.action === 'delete'
      ? 'Delete feedback'
      : 'Close feedback';
  }

  function handleModalKeydown(
    event: KeyboardEvent
  ) {
    if (
      event.key === 'Escape' &&
      confirmation &&
      !busyFeedbackId
    ) {
      event.preventDefault();
      closeConfirmation();
    }
  }

  usePageRefresh(load);
</script>

<svelte:head>
  <title>
    Feedback | Softadastra Cloud
  </title>

  <meta
    name="description"
    content="Report problems, suggest improvements, ask questions and follow the status of your Softadastra Cloud feedback."
  />
</svelte:head>

<svelte:window
  onkeydown={handleModalKeydown}
/>

<div class="feedback-page">
  <!-- ===================================================
       HEADER
       =================================================== -->
  <header class="page-header">
    <div class="page-heading">
      <p class="page-eyebrow">
        <span aria-hidden="true"></span>
        Product support
      </p>

      <h1>Feedback</h1>

      <p>
        Report a problem, suggest an improvement or
        ask a question about Softadastra Cloud.
      </p>
    </div>

    <div class="page-header__actions">
      {#if selectedWorkspace}
        <div class="workspace-context">
          <WorkspaceAvatar
            workspace={selectedWorkspace}
            size="sm"
          />

          <span>
            <strong>
              {selectedWorkspace.name}
            </strong>

            <small>
              Current workspace
            </small>
          </span>
        </div>
      {/if}

      {#if !loading}
        <button
          class:button-secondary={showComposer}
          class="header-action"
          type="button"
          onclick={toggleComposer}
        >
          {#if !showComposer}
            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path d="M12 5v14"></path>
              <path d="M5 12h14"></path>
            </svg>
          {/if}

          {showComposer
            ? 'Close'
            : 'Send feedback'}
        </button>
      {/if}
    </div>
  </header>

  <InlineError message={error} />

  {#if success}
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

      <p>{success}</p>
    </div>
  {/if}

  <!-- ===================================================
       LOADING
       =================================================== -->
  {#if loading}
    <section class="page-loading">
      <span class="page-loading__mark">
        F
      </span>

      <div>
        <strong>
          Loading your feedback
        </strong>

        <p>
          Preparing requests, statuses and product
          support history.
        </p>
      </div>

      <span
        class="page-loading__bar"
        aria-hidden="true"
      ></span>
    </section>
  {:else}
    <!-- =================================================
         COMPOSER
         ================================================= -->
    {#if showComposer}
      <section
        class="composer-panel"
        aria-labelledby="composer-title"
      >
        <header class="composer-header">
          <span
            class="composer-header__icon"
            aria-hidden="true"
          >
            {#if composerMode === 'edit'}
              <svg
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                stroke-width="1.8"
                stroke-linecap="round"
                stroke-linejoin="round"
              >
                <path d="M12 20h9"></path>

                <path
                  d="M16.5 3.5a2.1 2.1 0 0 1 3 3L8 18l-4 1 1-4Z"
                ></path>
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
                <path d="M21 15a4 4 0 0 1-4 4H8l-5 3V7a4 4 0 0 1 4-4h10a4 4 0 0 1 4 4Z"></path>
                <path d="M8 9h8"></path>
                <path d="M8 13h5"></path>
              </svg>
            {/if}
          </span>

          <div>
            <p class="panel-kicker">
              {composerMode === 'edit'
                ? 'Update request'
                : feedback.length === 0
                  ? 'First request'
                  : 'New request'}
            </p>

            <h2 id="composer-title">
              {composerMode === 'edit'
                ? 'Edit your feedback'
                : 'Tell us what needs attention'}
            </h2>

            <p>
              Explain what happened, what you expected
              and what would make the experience
              better.
            </p>
          </div>
        </header>

        <form
          class="composer-form"
          onsubmit={(event) => {
            event.preventDefault();
            void submit();
          }}
        >
          <div class="composer-fields">
            <fieldset>
              <legend>Category</legend>

              <div class="category-options">
                {#each CATEGORIES as item}
                  <button
                    class:selected={
                      category === item.value
                    }
                    type="button"
                    aria-pressed={
                      category === item.value
                    }
                    onclick={() => {
                      category = item.value;
                    }}
                  >
                    <span class="category-option__mark">
                      {item.label
                        .slice(0, 1)
                        .toUpperCase()}
                    </span>

                    <span>
                      <strong>
                        {item.label}
                      </strong>

                      <small>
                        {item.description}
                      </small>
                    </span>
                  </button>
                {/each}
              </div>
            </fieldset>

            <label>
              <span>Title</span>

              <input
                bind:value={title}
                maxlength={MAX_TITLE_LENGTH}
                placeholder="Summarize the request in a few words"
                autocomplete="off"
                required
              />

              <small class="field-footer">
                <span>
                  A clear title makes the request
                  easier to review.
                </span>

                <code>
                  {title.length}/{MAX_TITLE_LENGTH}
                </code>
              </small>
            </label>

            <label>
              <span>Message</span>

              <textarea
                bind:value={message}
                maxlength={MAX_MESSAGE_LENGTH}
                rows="11"
                placeholder="Describe what happened, what you expected and any useful steps to reproduce it."
                required
              ></textarea>

              <small class="field-footer">
                <span>
                  Include enough context to understand
                  the situation without sharing
                  sensitive data.
                </span>

                <code
                  class:near-limit={
                    message.length >
                    MAX_MESSAGE_LENGTH * 0.9
                  }
                >
                  {message.length}/{MAX_MESSAGE_LENGTH}
                </code>
              </small>
            </label>

            {#if
              composerMode === 'create' &&
              selectedWorkspace
            }
              <label
                class:checked={includeWorkspace}
                class="workspace-option"
              >
                <input
                  bind:checked={includeWorkspace}
                  type="checkbox"
                />

                <span
                  class="workspace-option__check"
                  aria-hidden="true"
                >
                  <svg
                    viewBox="0 0 24 24"
                  >
                    <path d="m5 12 4 4L19 6"></path>
                  </svg>
                </span>

                <span>
                  <strong>
                    Attach current workspace
                  </strong>

                  <small>
                    {selectedWorkspace.name}
                  </small>
                </span>
              </label>
            {/if}

            <div class="privacy-note">
              <span aria-hidden="true">
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="1.8"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                >
                  <rect
                    x="5"
                    y="10"
                    width="14"
                    height="11"
                    rx="2"
                  ></rect>

                  <path d="M8 10V7a4 4 0 0 1 8 0v3"></path>
                </svg>
              </span>

              <p>
                Never include passwords, access
                tokens, private keys or other
                credentials.
              </p>
            </div>
          </div>

          <aside class="composer-guide">
            <p class="panel-kicker">
              Request preview
            </p>

            <span class="composer-guide__mark">
              {categoryInitial(category)}
            </span>

            <div class="composer-guide__identity">
              <strong>
                {title.trim() ||
                  'Untitled feedback'}
              </strong>

              <span>
                {selectedCategory.label}
              </span>
            </div>

            <dl>
              <div>
                <dt>Category</dt>

                <dd>
                  {selectedCategory.label}
                </dd>
              </div>

              <div>
                <dt>Initial status</dt>

                <dd>Open</dd>
              </div>

              <div>
                <dt>Workspace</dt>

                <dd>
                  {composerMode === 'edit'
                    ? 'Existing attachment unchanged'
                    : includeWorkspace &&
                        selectedWorkspace
                      ? selectedWorkspace.name
                      : 'Not attached'}
                </dd>
              </div>
            </dl>

            <div class="quality-guide">
              <span>Useful feedback explains</span>

              <ol>
                <li>
                  What you were trying to do.
                </li>

                <li>
                  What happened or felt difficult.
                </li>

                <li>
                  What you expected instead.
                </li>
              </ol>
            </div>
          </aside>

          <div class="composer-actions">
            <button
              class="button-secondary"
              type="button"
              disabled={saving}
              onclick={closeComposer}
            >
              Cancel
            </button>

            <button
              type="submit"
              disabled={
                saving ||
                !title.trim() ||
                !message.trim()
              }
            >
              {saving
                ? composerMode === 'edit'
                  ? 'Saving changes…'
                  : 'Sending feedback…'
                : composerMode === 'edit'
                  ? 'Save changes'
                  : 'Send feedback'}
            </button>
          </div>
        </form>
      </section>
    {/if}

    <!-- =================================================
         FIRST-TIME STATE
         ================================================= -->
    {#if
      feedback.length === 0 &&
      !showComposer
    }
      <section class="first-feedback">
        <div class="first-feedback__copy">
          <p class="panel-kicker">
            Product feedback
          </p>

          <h2>
            Help shape the product
            <span>
              through real experience.
            </span>
          </h2>

          <p>
            Report a problem, suggest a better
            workflow or ask about something that is
            unclear. Every request remains available
            here so you can follow its status.
          </p>

          <button
            type="button"
            onclick={openCreateComposer}
          >
            Send your first feedback

            <svg
              viewBox="0 0 24 24"
              aria-hidden="true"
            >
              <path d="M5 12h14"></path>
              <path d="m13 6 6 6-6 6"></path>
            </svg>
          </button>
        </div>

        <div class="first-feedback__workflow">
          <article class="active">
            <span>01</span>

            <div>
              <strong>
                Submit the request
              </strong>

              <p>
                Explain the problem, question or
                improvement clearly.
              </p>
            </div>
          </article>

          <article>
            <span>02</span>

            <div>
              <strong>
                Follow the review
              </strong>

              <p>
                Return here to see when the request
                enters review.
              </p>
            </div>
          </article>

          <article>
            <span>03</span>

            <div>
              <strong>
                Keep the history
              </strong>

              <p>
                Closed and resolved requests remain
                visible to your account.
              </p>
            </div>
          </article>
        </div>
      </section>
    {/if}

    <!-- =================================================
         FEEDBACK WORKSPACE
         ================================================= -->
    {#if feedback.length > 0}
      <div class="feedback-workspace">
        <!-- =============================================
             REQUEST LIST
             ============================================= -->
        <aside
          class="feedback-list-panel"
          aria-label="Your feedback"
        >
          <header class="feedback-list-header">
            <div>
              <p class="panel-kicker">
                Your requests
              </p>

              <h2>Feedback history</h2>
            </div>

            <span class="feedback-count">
              {visibleFeedback.length}
            </span>
          </header>

          <div class="feedback-toolbar">
            <label class="feedback-search">
              <span class="visually-hidden">
                Find feedback
              </span>

              <svg
                viewBox="0 0 24 24"
                aria-hidden="true"
              >
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
              />
            </label>

            <select
              bind:value={categoryFilter}
              aria-label="Filter by category"
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
          </div>

          <nav
            class="status-filters"
            aria-label="Feedback status"
          >
            <button
              class:active={
                statusFilter === 'all'
              }
              type="button"
              onclick={() => {
                statusFilter = 'all';
              }}
            >
              All
              <span>{feedback.length}</span>
            </button>

            <button
              class:active={
                statusFilter === 'active'
              }
              type="button"
              onclick={() => {
                statusFilter = 'active';
              }}
            >
              Active
              <span>{activeCount}</span>
            </button>

            <button
              class:active={
                statusFilter === 'resolved'
              }
              type="button"
              onclick={() => {
                statusFilter = 'resolved';
              }}
            >
              Resolved
              <span>{resolvedCount}</span>
            </button>
          </nav>

          {#if visibleFeedback.length === 0}
            <div class="list-empty">
              <span
                class="list-empty__icon"
                aria-hidden="true"
              >
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="1.8"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                >
                  <circle
                    cx="11"
                    cy="11"
                    r="7"
                  ></circle>

                  <path d="m20 20-4-4"></path>
                </svg>
              </span>

              <strong>
                No matching requests
              </strong>

              <p>
                Change the search text or selected
                filters.
              </p>

              <button
                class="button-secondary"
                type="button"
                onclick={resetFilters}
              >
                Reset filters
              </button>
            </div>
          {:else}
            <div class="feedback-list">
              {#each visibleFeedback as item (item.id)}
                <button
                  class:selected={
                    selectedFeedbackId === item.id
                  }
                  class="feedback-row"
                  type="button"
                  aria-pressed={
                    selectedFeedbackId === item.id
                  }
                  onclick={() =>
                    selectFeedback(item)}
                >
                  <span class="feedback-row__mark">
                    {categoryInitial(
                      item.category
                    )}
                  </span>

                  <span class="feedback-row__content">
                    <span class="feedback-row__title">
                      <strong>
                        {item.title}
                      </strong>

                      <StatusBadge
                        status={item.status}
                      />
                    </span>

                    <span class="feedback-row__meta">
                      {categoryLabel(
                        item.category
                      )}

                      <i aria-hidden="true">
                        ·
                      </i>

                      {formatDate(
                        item.created_at
                      )}
                    </span>

                    <span class="feedback-row__preview">
                      {item.message}
                    </span>
                  </span>
                </button>
              {/each}
            </div>
          {/if}
        </aside>

        <!-- =============================================
             REQUEST DETAIL
             ============================================= -->
        <main class="feedback-detail">
          {#if selectedFeedback}
            <article
              class="feedback-inspector"
              aria-labelledby="feedback-title"
            >
              <header class="feedback-inspector__header">
                <div class="feedback-inspector__identity">
                  <span class="feedback-inspector__mark">
                    {categoryInitial(
                      selectedFeedback.category
                    )}
                  </span>

                  <div>
                    <p class="panel-kicker">
                      {categoryLabel(
                        selectedFeedback.category
                      )}
                    </p>

                    <h2 id="feedback-title">
                      {selectedFeedback.title}
                    </h2>

                    <p>
                      Submitted
                      {formatDate(
                        selectedFeedback.created_at
                      )}
                    </p>
                  </div>
                </div>

                <StatusBadge
                  status={selectedFeedback.status}
                />
              </header>

              <section class="request-progress">
                <article
                  class:active={
                    statusStage(
                      selectedFeedback
                    ) >= 1
                  }
                >
                  <span>01</span>

                  <div>
                    <strong>Submitted</strong>
                    <small>
                      Request received
                    </small>
                  </div>
                </article>

                <span
                  class:active={
                    statusStage(
                      selectedFeedback
                    ) >= 2
                  }
                  class="progress-line"
                  aria-hidden="true"
                ></span>

                <article
                  class:active={
                    statusStage(
                      selectedFeedback
                    ) >= 2
                  }
                >
                  <span>02</span>

                  <div>
                    <strong>Review</strong>
                    <small>
                      Product evaluation
                    </small>
                  </div>
                </article>

                <span
                  class:active={
                    statusStage(
                      selectedFeedback
                    ) >= 3
                  }
                  class="progress-line"
                  aria-hidden="true"
                ></span>

                <article
                  class:active={
                    statusStage(
                      selectedFeedback
                    ) >= 3
                  }
                >
                  <span>03</span>

                  <div>
                    <strong>Completed</strong>
                    <small>
                      Closed or resolved
                    </small>
                  </div>
                </article>
              </section>

              <section class="feedback-message">
                <p class="panel-kicker">
                  Full message
                </p>

                <p>
                  {selectedFeedback.message}
                </p>
              </section>

              <dl class="feedback-metadata">
                <div>
                  <dt>Category</dt>

                  <dd>
                    {categoryLabel(
                      selectedFeedback.category
                    )}
                  </dd>
                </div>

                <div>
                  <dt>Status</dt>

                  <dd>
                    {statusLabel(
                      selectedFeedback.status
                    )}
                  </dd>
                </div>

                <div>
                  <dt>Submitted</dt>

                  <dd>
                    {formatDate(
                      selectedFeedback.created_at
                    )}
                  </dd>
                </div>
              </dl>

              <footer class="feedback-inspector__footer">
                <p>
                  Edit open requests while the details
                  are still changing. Close a request
                  when you no longer need an answer.
                </p>

                <div class="feedback-actions">
                  {#if
                    isEditable(
                      selectedFeedback
                    )
                  }
                    <button
                      class="button-secondary"
                      type="button"
                      disabled={
                        Boolean(
                          busyFeedbackId
                        )
                      }
                      onclick={() =>
                        openEditComposer(
                          selectedFeedback
                        )}
                    >
                      Edit request
                    </button>
                  {/if}

                  {#if
                    !isResolved(
                      selectedFeedback
                    )
                  }
                    <button
                      class="button-secondary"
                      type="button"
                      disabled={
                        Boolean(
                          busyFeedbackId
                        )
                      }
                      onclick={() =>
                        openConfirmation(
                          selectedFeedback,
                          'close'
                        )}
                    >
                      Close request
                    </button>
                  {/if}

                  <button
                    class="delete-action"
                    type="button"
                    disabled={
                      Boolean(
                        busyFeedbackId
                      )
                    }
                    onclick={() =>
                      openConfirmation(
                        selectedFeedback,
                        'delete'
                      )}
                  >
                    Delete
                  </button>
                </div>
              </footer>
            </article>
          {:else}
            <section class="detail-empty">
              <span
                class="detail-empty__icon"
                aria-hidden="true"
              >
                <svg
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  stroke-width="1.8"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                >
                  <path d="M21 15a4 4 0 0 1-4 4H8l-5 3V7a4 4 0 0 1 4-4h10a4 4 0 0 1 4 4Z"></path>
                </svg>
              </span>

              <strong>
                Select a feedback request
              </strong>

              <p>
                Choose a request from the history to
                inspect its message and status.
              </p>
            </section>
          {/if}
        </main>
      </div>
    {/if}
  {/if}
</div>

<!-- =====================================================
     CONFIRMATION MODAL
     ===================================================== -->
{#if confirmation}
  <div class="modal-layer">
    <button
      class="modal-backdrop"
      type="button"
      aria-label="Close confirmation"
      onclick={closeConfirmation}
    ></button>

    <dialog
      bind:this={confirmationDialog}
      class="confirm-modal"
      open
      aria-modal="true"
      aria-labelledby="confirmation-title"
      aria-describedby="confirmation-description"
      tabindex="-1"
    >
      <span
        class:confirm-modal__icon--danger={
          confirmation.action === 'delete'
        }
        class="confirm-modal__icon"
        aria-hidden="true"
      >
        <svg
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="1.9"
          stroke-linecap="round"
          stroke-linejoin="round"
        >
          <path d="M12 9v4"></path>
          <path d="M12 17h.01"></path>

          <path
            d="m10.3 3.7-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.7-3.3l-8-14a2 2 0 0 0-3.4 0Z"
          ></path>
        </svg>
      </span>

      <div class="confirm-modal__content">
        <h2 id="confirmation-title">
          {confirmationTitle(
            confirmation
          )}
        </h2>

        <p id="confirmation-description">
          {confirmationMessage(
            confirmation
          )}
        </p>
      </div>

      {#if confirmationNeedsTitle}
        <label class="confirmation-field">
          <span>
            Enter
            <code>
              {confirmation.item.title}
            </code>
            to confirm
          </span>

          <input
            bind:value={deleteConfirmation}
            placeholder={confirmation.item.title}
            autocomplete="off"
          />
        </label>
      {/if}

      <div class="confirm-modal__actions">
        <button
          class="button-secondary"
          type="button"
          disabled={
            Boolean(
              busyFeedbackId
            )
          }
          onclick={closeConfirmation}
        >
          Cancel
        </button>

        <button
          class:danger-button={
            confirmation.action ===
            'delete'
          }
          type="button"
          disabled={
            Boolean(
              busyFeedbackId
            ) ||
            !confirmationCanContinue
          }
          onclick={confirmAction}
        >
          {busyFeedbackId
            ? 'Working…'
            : confirmationButtonLabel(
                confirmation
              )}
        </button>
      </div>
    </dialog>
  </div>
{/if}

<style>
  .feedback-page {
    display: grid;
    max-width: 1180px;
    gap: 18px;
    margin-inline: auto;
    padding-bottom: 44px;
    animation: page-enter 420ms var(--ease) both;
  }

  @keyframes page-enter {
    from {
      opacity: 0;
      transform: translateY(7px);
    }
  }

  .visually-hidden {
    position: absolute;
    width: 1px;
    height: 1px;
    overflow: hidden;
    clip: rect(0 0 0 0);
    white-space: nowrap;
  }

  /* =====================================================
     HEADER
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
    box-shadow: 0 0 0 4px var(--brand-faint);
  }

  .page-heading h1 {
    color: var(--text);
    font-size: clamp(30px, 4vw, 44px);
    font-weight: 650;
    line-height: 1;
    letter-spacing: -0.045em;
  }

  .page-heading > p:last-child {
    max-width: 680px;
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

  .workspace-context {
    display: flex;
    min-height: 42px;
    align-items: center;
    gap: 9px;
    padding: 5px 10px;
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
    background: var(--bg-panel);
  }

  .workspace-context > span {
    display: grid;
    min-width: 0;
    line-height: 1.3;
  }

  .workspace-context strong {
    color: var(--text-soft);
    font-size: 10px;
    font-weight: 600;
  }

  .workspace-context small {
    color: var(--text-muted);
    font-size: 9px;
  }

  .header-action svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
  }

  /* =====================================================
     SHARED CONTROLS
     ===================================================== */

  button {
    display: inline-flex;
    min-height: 39px;
    align-items: center;
    justify-content: center;
    gap: 7px;
    border: 1px solid var(--brand);
    border-radius: var(--radius-sm);
    background: var(--brand);
    color: var(--brand-ink);
    padding: 0 14px;
    font: inherit;
    font-size: 12px;
    font-weight: 650;
    cursor: pointer;
    transition:
      border-color var(--speed) var(--ease),
      background var(--speed) var(--ease),
      color var(--speed) var(--ease),
      transform var(--speed) var(--ease);
  }

  button:hover:not(:disabled) {
    border-color: var(--brand-soft);
    background: var(--brand-soft);
    transform: translateY(-1px);
  }

  button:disabled {
    cursor: not-allowed;
    opacity: 0.5;
  }

  .button-secondary {
    border-color: var(--line-strong);
    background: transparent;
    color: var(--text-soft);
  }

  .button-secondary:hover:not(:disabled) {
    border-color: rgba(255, 255, 255, 0.17);
    background: var(--bg-elevated);
    color: var(--text);
  }

  .delete-action,
  .danger-button {
    border-color: var(--danger-line);
    background: transparent;
    color: var(--danger);
  }

  .delete-action:hover:not(:disabled),
  .danger-button:hover:not(:disabled) {
    border-color: var(--danger);
    background: var(--danger-faint);
    color: var(--danger);
  }

  .panel-kicker {
    color: var(--brand-soft);
    font-family: var(--font-mono);
    font-size: 9px;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }

  /* =====================================================
     FEEDBACK STATES
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

  .page-loading {
    position: relative;
    display: flex;
    min-height: 116px;
    align-items: center;
    gap: 13px;
    overflow: hidden;
    padding: 20px;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .page-loading__mark {
    display: grid;
    width: 44px;
    height: 44px;
    place-items: center;
    border-radius: 10px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 14px;
    font-weight: 800;
  }

  .page-loading > div {
    display: grid;
    gap: 4px;
  }

  .page-loading strong {
    color: var(--text);
    font-size: 13px;
  }

  .page-loading p {
    color: var(--text-muted);
    font-size: 11px;
  }

  .page-loading__bar {
    position: absolute;
    right: 0;
    bottom: 0;
    left: 0;
    height: 2px;
    background: var(--brand);
    transform-origin: left;
    animation: loading-bar 1.4s ease-in-out infinite;
  }

  @keyframes loading-bar {
    0% {
      transform: scaleX(0);
    }

    65% {
      transform: scaleX(1);
    }

    100% {
      opacity: 0;
      transform: scaleX(1);
    }
  }

  /* =====================================================
     COMPOSER
     ===================================================== */

  .composer-panel {
    overflow: hidden;
    border: 1px solid var(--brand-line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    box-shadow: inset 3px 0 0 var(--brand);
    animation: composer-enter 300ms var(--ease) both;
  }

  @keyframes composer-enter {
    from {
      opacity: 0;
      transform: translateY(-6px);
    }
  }

  .composer-header {
    display: flex;
    align-items: flex-start;
    gap: 12px;
    padding: 18px 19px;
    border-bottom: 1px solid var(--line-soft);
  }

  .composer-header__icon {
    display: grid;
    width: 43px;
    height: 43px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 10px;
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .composer-header__icon svg {
    width: 20px;
    height: 20px;
  }

  .composer-header > div {
    display: grid;
    gap: 4px;
  }

  .composer-header h2 {
    color: var(--text);
    font-size: 17px;
    font-weight: 620;
  }

  .composer-header p:not(.panel-kicker) {
    max-width: 680px;
    color: var(--text-muted);
    font-size: 11px;
    line-height: 1.55;
  }

  .composer-form {
    display: grid;
    grid-template-columns:
      minmax(0, 1.4fr)
      minmax(280px, 0.6fr);
  }

  .composer-fields {
    display: grid;
    gap: 19px;
    padding: 21px;
    border-right: 1px solid var(--line-soft);
  }

  .composer-fields fieldset {
    display: grid;
    gap: 9px;
    margin: 0;
    border: 0;
    padding: 0;
  }

  .composer-fields legend,
  .composer-fields label > span {
    color: var(--text-soft);
    font-size: 11px;
    font-weight: 550;
  }

  .category-options {
    display: grid;
    grid-template-columns:
      repeat(5, minmax(0, 1fr));
    gap: 7px;
  }

  .category-options button {
    display: grid;
    min-height: 92px;
    grid-template-columns: 28px;
    align-content: start;
    justify-content: start;
    gap: 8px;
    border-color: var(--line);
    background: transparent;
    color: var(--text-soft);
    padding: 10px;
    text-align: left;
  }

  .category-options button:hover:not(:disabled) {
    border-color: var(--line-strong);
    background: var(--bg-elevated);
    transform: none;
  }

  .category-options button.selected {
    border-color: var(--brand-line);
    background: var(--brand-faint);
  }

  .category-option__mark {
    display: grid;
    width: 28px;
    height: 28px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 7px;
    color: var(--text-muted);
    font-size: 10px;
  }

  .category-options button.selected
    .category-option__mark {
    border-color: var(--brand-line);
    color: var(--brand-soft);
  }

  .category-options button > span:last-child {
    display: grid;
    gap: 3px;
  }

  .category-options strong {
    color: var(--text-soft);
    font-size: 10px;
  }

  .category-options small {
    color: var(--text-muted);
    font-size: 8px;
    line-height: 1.4;
  }

  .composer-fields label {
    display: grid;
    gap: 7px;
  }

  .composer-fields input,
  .composer-fields textarea,
  .feedback-toolbar input,
  .feedback-toolbar select,
  .confirmation-field input {
    width: 100%;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    outline: 0;
    background: transparent;
    color: var(--text);
    padding-inline: 11px;
    font: inherit;
    font-size: 12px;
  }

  .composer-fields input,
  .feedback-toolbar input,
  .feedback-toolbar select,
  .confirmation-field input {
    min-height: 42px;
  }

  .composer-fields textarea {
    min-height: 210px;
    resize: vertical;
    padding-block: 11px;
    line-height: 1.65;
  }

  .composer-fields input:focus,
  .composer-fields textarea:focus,
  .feedback-toolbar input:focus,
  .feedback-toolbar select:focus,
  .confirmation-field input:focus {
    border-color: var(--brand);
    box-shadow: 0 0 0 3px var(--brand-faint);
  }

  .field-footer {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 14px;
    color: var(--text-muted);
    font-size: 9px;
    font-weight: 400;
    line-height: 1.5;
  }

  .field-footer code {
    flex: 0 0 auto;
    color: var(--text-faint);
    font-size: 9px;
  }

  .field-footer code.near-limit {
    color: var(--warning);
  }

  .workspace-option {
    display: grid !important;
    grid-template-columns:
      auto auto minmax(0, 1fr);
    align-items: center;
    gap: 9px !important;
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
    padding: 11px;
    cursor: pointer;
  }

  .workspace-option.checked {
    border-color: var(--brand-line);
    background: var(--brand-faint);
  }

  .workspace-option > input {
    position: absolute;
    width: 1px;
    height: 1px;
    opacity: 0;
  }

  .workspace-option__check {
    display: grid;
    width: 22px;
    height: 22px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 6px;
    color: transparent;
  }

  .workspace-option.checked
    .workspace-option__check {
    border-color: var(--brand);
    background: var(--brand);
    color: var(--brand-ink);
  }

  .workspace-option__check svg {
    width: 12px;
    height: 12px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2.2;
    stroke-linecap: round;
  }

  .workspace-option > span:last-child {
    display: grid;
    gap: 3px;
  }

  .workspace-option strong {
    color: var(--text-soft);
    font-size: 10.5px;
  }

  .workspace-option small {
    color: var(--text-muted);
    font-size: 9px;
  }

  .privacy-note {
    display: flex;
    align-items: flex-start;
    gap: 9px;
    padding: 10px 11px;
    border: 1px solid var(--warning-line);
    border-radius: var(--radius-sm);
    background: var(--warning-faint);
  }

  .privacy-note > span {
    color: var(--warning);
  }

  .privacy-note svg {
    width: 16px;
    height: 16px;
  }

  .privacy-note p {
    color: var(--text-muted);
    font-size: 9.5px;
    line-height: 1.5;
  }

  .composer-guide {
    display: grid;
    align-content: start;
    justify-items: start;
    gap: 13px;
    padding: 21px;
  }

  .composer-guide__mark {
    display: grid;
    width: 49px;
    height: 49px;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 11px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 15px;
    font-weight: 750;
  }

  .composer-guide__identity {
    display: grid;
    gap: 4px;
  }

  .composer-guide__identity strong {
    overflow-wrap: anywhere;
    color: var(--text);
    font-size: 12px;
  }

  .composer-guide__identity span {
    color: var(--text-muted);
    font-size: 9px;
  }

  .composer-guide dl {
    display: grid;
    width: 100%;
    margin: 2px 0 0;
    border: 1px solid var(--line);
    border-radius: var(--radius-sm);
  }

  .composer-guide dl > div {
    display: grid;
    gap: 4px;
    padding: 10px;
    border-bottom: 1px solid var(--line-soft);
  }

  .composer-guide dl > div:last-child {
    border-bottom: 0;
  }

  .composer-guide dt {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 8px;
    text-transform: uppercase;
  }

  .composer-guide dd {
    overflow-wrap: anywhere;
    margin: 0;
    color: var(--text-soft);
    font-size: 9px;
  }

  .quality-guide {
    display: grid;
    width: 100%;
    gap: 8px;
    padding-top: 3px;
  }

  .quality-guide > span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 8px;
    text-transform: uppercase;
  }

  .quality-guide ol {
    display: grid;
    gap: 7px;
    margin: 0;
    padding-left: 19px;
    color: var(--text-muted);
    font-size: 9px;
    line-height: 1.5;
  }

  .quality-guide li::marker {
    color: var(--brand-soft);
  }

  .composer-actions {
    grid-column: 1 / -1;
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    padding: 13px 20px;
    border-top: 1px solid var(--line-soft);
  }

  /* =====================================================
     FIRST FEEDBACK
     ===================================================== */

  .first-feedback {
    display: grid;
    grid-template-columns:
      minmax(0, 1fr)
      minmax(340px, 0.8fr);
    gap: clamp(38px, 7vw, 80px);
    align-items: center;
    min-height: 410px;
    padding: clamp(32px, 6vw, 60px);
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-lg);
    background: var(--bg-panel);
  }

  .first-feedback__copy {
    display: grid;
    justify-items: start;
  }

  .first-feedback__copy h2 {
    max-width: 620px;
    margin-top: 10px;
    color: var(--text);
    font-size: clamp(33px, 5vw, 52px);
    font-weight: 640;
    line-height: 1;
    letter-spacing: -0.052em;
  }

  .first-feedback__copy h2 span {
    display: block;
    color: var(--brand-soft);
  }

  .first-feedback__copy > p:not(.panel-kicker) {
    max-width: 560px;
    margin-top: 18px;
    color: var(--text-muted);
    font-size: 12.5px;
    line-height: 1.7;
  }

  .first-feedback__copy button {
    margin-top: 25px;
  }

  .first-feedback__copy button svg {
    width: 14px;
    height: 14px;
    fill: none;
    stroke: currentColor;
    stroke-width: 2;
    stroke-linecap: round;
  }

  .first-feedback__workflow {
    display: grid;
    border-left: 1px solid var(--line);
  }

  .first-feedback__workflow article {
    display: grid;
    grid-template-columns:
      37px minmax(0, 1fr);
    gap: 13px;
    padding: 20px 0 20px 24px;
    border-bottom: 1px solid var(--line);
  }

  .first-feedback__workflow article:last-child {
    border-bottom: 0;
  }

  .first-feedback__workflow article > span {
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 700;
  }

  .first-feedback__workflow article.active > span {
    color: var(--brand-soft);
  }

  .first-feedback__workflow article > div {
    display: grid;
    gap: 5px;
  }

  .first-feedback__workflow strong {
    color: var(--text-soft);
    font-size: 12px;
  }

  .first-feedback__workflow p {
    color: var(--text-muted);
    font-size: 10px;
    line-height: 1.55;
  }

  /* =====================================================
     FEEDBACK WORKSPACE
     ===================================================== */

  .feedback-workspace {
    display: grid;
    grid-template-columns:
      minmax(310px, 0.74fr)
      minmax(0, 1.26fr);
    gap: 16px;
    align-items: start;
  }

  .feedback-list-panel,
  .feedback-inspector,
  .detail-empty {
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
  }

  .feedback-list-panel {
    position: sticky;
    top: 24px;
    overflow: hidden;
  }

  .feedback-list-header {
    display: flex;
    min-height: 72px;
    align-items: center;
    justify-content: space-between;
    gap: 14px;
    padding: 13px 15px;
    border-bottom: 1px solid var(--line-soft);
  }

  .feedback-list-header > div {
    display: grid;
    gap: 3px;
  }

  .feedback-list-header h2 {
    color: var(--text);
    font-size: 14px;
    font-weight: 610;
  }

  .feedback-count {
    display: grid;
    min-width: 25px;
    height: 23px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 999px;
    color: var(--text-muted);
    padding-inline: 7px;
    font-family: var(--font-mono);
    font-size: 9px;
  }

  .feedback-toolbar {
    display: grid;
    grid-template-columns:
      minmax(0, 1fr)
      minmax(115px, 0.55fr);
    gap: 8px;
    padding: 10px;
    border-bottom: 1px solid var(--line-soft);
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
    transform: translateY(-50%);
    pointer-events: none;
  }

  .feedback-toolbar input {
    padding-left: 33px;
  }

  .feedback-toolbar input,
  .feedback-toolbar select {
    min-height: 36px;
    font-size: 10px;
  }

  .status-filters {
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    gap: 4px;
    padding: 8px 10px;
    border-bottom: 1px solid var(--line-soft);
  }

  .status-filters button {
    min-height: 31px;
    gap: 6px;
    border-color: transparent;
    background: transparent;
    color: var(--text-muted);
    padding: 0 7px;
    font-size: 9px;
  }

  .status-filters button:hover:not(:disabled) {
    border-color: var(--line);
    background: var(--bg-elevated);
    color: var(--text-soft);
    transform: none;
  }

  .status-filters button.active {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-bright);
  }

  .status-filters button span {
    display: grid;
    min-width: 18px;
    height: 18px;
    place-items: center;
    border-radius: 999px;
    border: 1px solid var(--line);
    padding-inline: 4px;
    font-family: var(--font-mono);
    font-size: 7px;
  }

  .feedback-list {
    display: grid;
    max-height: calc(100vh - 300px);
    overflow-y: auto;
  }

  .feedback-row {
    display: grid;
    width: 100%;
    min-height: 94px;
    grid-template-columns:
      34px minmax(0, 1fr);
    align-items: start;
    justify-content: stretch;
    gap: 10px;
    padding: 13px;
    border: 0;
    border-bottom: 1px solid var(--line-soft);
    border-radius: 0;
    background: transparent;
    color: var(--text);
    text-align: left;
  }

  .feedback-row:last-child {
    border-bottom: 0;
  }

  .feedback-row:hover:not(:disabled) {
    background: var(--bg-elevated);
    transform: none;
  }

  .feedback-row.selected {
    background: var(--brand-faint);
    box-shadow: inset 2px 0 0 var(--brand);
  }

  .feedback-row__mark {
    display: grid;
    width: 34px;
    height: 34px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 8px;
    color: var(--text-muted);
    font-size: 11px;
  }

  .feedback-row.selected
    .feedback-row__mark {
    border-color: var(--brand-line);
    color: var(--brand-soft);
  }

  .feedback-row__content {
    display: grid;
    min-width: 0;
    gap: 5px;
  }

  .feedback-row__title {
    display: flex;
    min-width: 0;
    align-items: flex-start;
    justify-content: space-between;
    gap: 8px;
  }

  .feedback-row__title strong {
    min-width: 0;
    overflow: hidden;
    color: var(--text-soft);
    font-size: 11px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .feedback-row.selected
    .feedback-row__title strong {
    color: var(--brand-bright);
  }

  .feedback-row__meta {
    display: flex;
    flex-wrap: wrap;
    gap: 5px;
    color: var(--text-muted);
    font-size: 8.5px;
  }

  .feedback-row__meta i {
    font-style: normal;
  }

  .feedback-row__preview {
    display: -webkit-box;
    overflow: hidden;
    color: var(--text-muted);
    font-size: 9.5px;
    line-height: 1.5;
    -webkit-box-orient: vertical;
    -webkit-line-clamp: 2;
    line-clamp: 2;
  }

  .list-empty {
    display: grid;
    min-height: 260px;
    place-content: center;
    justify-items: center;
    gap: 7px;
    padding: 24px;
    text-align: center;
  }

  .list-empty__icon {
    display: grid;
    width: 42px;
    height: 42px;
    place-items: center;
    border: 1px solid var(--line);
    border-radius: 10px;
    color: var(--text-muted);
  }

  .list-empty__icon svg {
    width: 18px;
    height: 18px;
  }

  .list-empty strong {
    color: var(--text-soft);
    font-size: 11px;
  }

  .list-empty p {
    color: var(--text-muted);
    font-size: 9px;
  }

  .list-empty button {
    min-height: 31px;
    margin-top: 6px;
    font-size: 9px;
  }

  /* =====================================================
     FEEDBACK DETAIL
     ===================================================== */

  .feedback-detail {
    min-width: 0;
  }

  .feedback-inspector {
    overflow: hidden;
    animation: inspector-enter 260ms var(--ease) both;
  }

  @keyframes inspector-enter {
    from {
      opacity: 0;
      transform: translateY(-5px);
    }
  }

  .feedback-inspector__header {
    display: flex;
    min-height: 102px;
    align-items: flex-start;
    justify-content: space-between;
    gap: 18px;
    padding: 18px;
    border-bottom: 1px solid var(--line-soft);
  }

  .feedback-inspector__identity {
    display: flex;
    min-width: 0;
    align-items: flex-start;
    gap: 12px;
  }

  .feedback-inspector__mark {
    display: grid;
    width: 43px;
    height: 43px;
    flex: 0 0 auto;
    place-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 9px;
    background: var(--brand-faint);
    color: var(--brand-soft);
    font-size: 13px;
    font-weight: 750;
  }

  .feedback-inspector__identity > div {
    display: grid;
    min-width: 0;
    gap: 5px;
  }

  .feedback-inspector h2 {
    max-width: 680px;
    overflow-wrap: anywhere;
    color: var(--text);
    font-size: clamp(18px, 2.4vw, 25px);
    font-weight: 620;
    line-height: 1.15;
    letter-spacing: -0.025em;
  }

  .feedback-inspector__identity p:not(.panel-kicker) {
    color: var(--text-muted);
    font-size: 9.5px;
  }

  .request-progress {
    display: grid;
    grid-template-columns:
      minmax(0, 1fr)
      44px
      minmax(0, 1fr)
      44px
      minmax(0, 1fr);
    align-items: center;
    padding: 15px 18px;
    border-bottom: 1px solid var(--line-soft);
  }

  .request-progress article {
    display: grid;
    grid-template-columns:
      28px minmax(0, 1fr);
    align-items: center;
    gap: 8px;
    opacity: 0.52;
  }

  .request-progress article.active {
    opacity: 1;
  }

  .request-progress article > span {
    display: grid;
    width: 28px;
    height: 28px;
    place-items: center;
    border: 1px solid var(--line-strong);
    border-radius: 50%;
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 8px;
  }

  .request-progress article.active > span {
    border-color: var(--brand-line);
    background: var(--brand-faint);
    color: var(--brand-soft);
  }

  .request-progress article > div {
    display: grid;
    gap: 2px;
  }

  .request-progress strong {
    color: var(--text-soft);
    font-size: 9px;
  }

  .request-progress small {
    color: var(--text-muted);
    font-size: 7.5px;
  }

  .progress-line {
    height: 1px;
    background: var(--line-strong);
  }

  .progress-line.active {
    background: var(--brand);
  }

  .feedback-message {
    display: grid;
    gap: 10px;
    min-height: 230px;
    align-content: start;
    padding: 22px 19px;
    border-bottom: 1px solid var(--line-soft);
  }

  .feedback-message > p:last-child {
    max-width: 800px;
    color: var(--text-soft);
    font-size: 12px;
    line-height: 1.75;
    white-space: pre-wrap;
    overflow-wrap: anywhere;
  }

  .feedback-metadata {
    display: grid;
    grid-template-columns:
      repeat(3, minmax(0, 1fr));
    margin: 0;
    border-bottom: 1px solid var(--line-soft);
  }

  .feedback-metadata > div {
    display: grid;
    min-width: 0;
    gap: 5px;
    padding: 13px 16px;
    border-right: 1px solid var(--line-soft);
  }

  .feedback-metadata > div:last-child {
    border-right: 0;
  }

  .feedback-metadata dt {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 8px;
    text-transform: uppercase;
  }

  .feedback-metadata dd {
    overflow-wrap: anywhere;
    margin: 0;
    color: var(--text-soft);
    font-size: 10px;
  }

  .feedback-inspector__footer {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 20px;
    padding: 14px 16px;
  }

  .feedback-inspector__footer > p {
    max-width: 560px;
    color: var(--text-muted);
    font-size: 9px;
    line-height: 1.5;
  }

  .feedback-actions {
    display: flex;
    flex: 0 0 auto;
    flex-wrap: wrap;
    justify-content: flex-end;
    gap: 7px;
  }

  .feedback-actions button {
    min-height: 33px;
    padding: 0 10px;
    font-size: 9px;
  }

  .detail-empty {
    display: grid;
    min-height: 420px;
    place-content: center;
    justify-items: center;
    gap: 7px;
    padding: 30px;
    text-align: center;
  }

  .detail-empty__icon {
    display: grid;
    width: 45px;
    height: 45px;
    place-items: center;
    border: 1px solid var(--line);
    border-radius: 10px;
    color: var(--text-muted);
  }

  .detail-empty__icon svg {
    width: 20px;
    height: 20px;
  }

  .detail-empty strong {
    color: var(--text-soft);
    font-size: 12px;
  }

  .detail-empty p {
    max-width: 360px;
    color: var(--text-muted);
    font-size: 10px;
    line-height: 1.5;
  }

  /* =====================================================
     MODAL
     ===================================================== */

  .modal-layer {
    position: fixed;
    inset: 0;
    z-index: 100;
    display: grid;
    place-items: center;
    padding: 20px;
  }

  .modal-backdrop {
    position: absolute;
    inset: 0;
    width: 100%;
    min-height: 100%;
    border: 0;
    border-radius: 0;
    background: rgba(0, 0, 0, 0.72);
    padding: 0;
    cursor: default;
    backdrop-filter: blur(3px);
  }

  .modal-backdrop:hover:not(:disabled) {
    background: rgba(0, 0, 0, 0.72);
    transform: none;
  }

  .confirm-modal {
    position: relative;
    z-index: 1;
    display: grid;
    width: min(100%, 460px);
    max-width: none;
    justify-items: start;
    gap: 14px;
    margin: 0;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    color: var(--text);
    padding: 20px;
    box-shadow: var(--shadow-deep);
  }

  .confirm-modal:focus {
    outline: none;
  }

  .confirm-modal::backdrop {
    background: transparent;
  }

  .confirm-modal__icon {
    display: grid;
    width: 39px;
    height: 39px;
    place-items: center;
    border: 1px solid var(--warning-line);
    border-radius: 9px;
    background: var(--warning-faint);
    color: var(--warning);
  }

  .confirm-modal__icon--danger {
    border-color: var(--danger-line);
    background: var(--danger-faint);
    color: var(--danger);
  }

  .confirm-modal__icon svg {
    width: 18px;
    height: 18px;
  }

  .confirm-modal__content {
    display: grid;
    gap: 7px;
  }

  .confirm-modal__content h2 {
    color: var(--text);
    font-size: 17px;
    font-weight: 620;
  }

  .confirm-modal__content p {
    color: var(--text-muted);
    font-size: 11px;
    line-height: 1.65;
  }

  .confirmation-field {
    display: grid;
    width: 100%;
    gap: 7px;
  }

  .confirmation-field > span {
    color: var(--text-soft);
    font-size: 10px;
    line-height: 1.5;
  }

  .confirmation-field code {
    color: var(--danger);
    font-size: 9px;
  }

  .confirm-modal__actions {
    display: flex;
    width: 100%;
    justify-content: flex-end;
    gap: 8px;
    padding-top: 15px;
    border-top: 1px solid var(--line-soft);
  }

  /* =====================================================
     MOTION
     ===================================================== */

  @media (prefers-reduced-motion: reduce) {
    .feedback-page,
    .page-loading__bar,
    .composer-panel,
    .feedback-inspector {
      animation: none;
    }
  }

  /* =====================================================
     RESPONSIVE
     ===================================================== */

  @media (max-width: 1050px) {
    .category-options {
      grid-template-columns:
        repeat(3, minmax(0, 1fr));
    }

    .composer-form {
      grid-template-columns: 1fr;
    }

    .composer-fields {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .composer-guide {
      grid-template-columns:
        auto minmax(0, 1fr);
      align-items: center;
    }

    .composer-guide > .panel-kicker,
    .composer-guide dl,
    .quality-guide {
      grid-column: 1 / -1;
    }

    .feedback-workspace {
      grid-template-columns:
        minmax(280px, 0.68fr)
        minmax(0, 1.32fr);
    }
  }

  @media (max-width: 880px) {
    .feedback-workspace {
      grid-template-columns: 1fr;
    }

    .feedback-list-panel {
      position: static;
    }

    .feedback-list {
      max-height: 410px;
    }

    .first-feedback {
      grid-template-columns: 1fr;
    }

    .first-feedback__workflow {
      border-top: 1px solid var(--line);
      border-left: 0;
    }

    .first-feedback__workflow article {
      padding-left: 0;
    }
  }

  @media (max-width: 760px) {
    .page-header {
      align-items: stretch;
      flex-direction: column;
    }

    .page-header__actions {
      align-items: stretch;
      flex-direction: column;
    }

    .workspace-context,
    .header-action {
      width: 100%;
    }

    .category-options {
      grid-template-columns:
        repeat(2, minmax(0, 1fr));
    }

    .request-progress {
      grid-template-columns: 1fr;
      gap: 10px;
    }

    .progress-line {
      width: 1px;
      height: 20px;
      margin-left: 14px;
    }

    .feedback-metadata {
      grid-template-columns: 1fr;
    }

    .feedback-metadata > div {
      border-right: 0;
      border-bottom: 1px solid var(--line-soft);
    }

    .feedback-metadata > div:last-child {
      border-bottom: 0;
    }

    .feedback-inspector__footer {
      align-items: stretch;
      flex-direction: column;
    }

    .feedback-actions {
      justify-content: flex-start;
    }
  }

  @media (max-width: 620px) {
    .category-options {
      grid-template-columns: 1fr;
    }

    .category-options button {
      min-height: 68px;
      grid-template-columns:
        28px minmax(0, 1fr);
      align-items: center;
    }

    .composer-actions,
    .confirm-modal__actions {
      align-items: stretch;
      flex-direction: column-reverse;
    }

    .composer-actions button,
    .confirm-modal__actions button {
      width: 100%;
    }

    .first-feedback {
      padding: 28px 20px;
    }

    .first-feedback__copy h2 {
      font-size: clamp(33px, 10vw, 45px);
    }

    .feedback-toolbar {
      grid-template-columns: 1fr;
    }

    .feedback-inspector__header {
      align-items: flex-start;
      flex-direction: column;
    }
  }

  @media (max-width: 500px) {
    .composer-fields,
    .composer-guide {
      padding: 16px;
    }

    .status-filters {
      grid-template-columns: 1fr;
    }

    .feedback-list {
      max-height: none;
    }

    .feedback-actions {
      display: grid;
      width: 100%;
    }

    .feedback-actions button {
      width: 100%;
    }

    .field-footer {
      align-items: flex-end;
      flex-direction: column;
    }
  }
</style>
