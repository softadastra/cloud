<script lang="ts">
  import { onMount } from 'svelte';
  import { ApiError, type FeedbackItem } from '$lib/api/types';
  import { createFeedback, listMyFeedback } from '$lib/api/feedback';
  import EmptyState from '$lib/components/EmptyState.svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import LoadingButton from '$lib/components/LoadingButton.svelte';
  import PageHeader from '$lib/components/PageHeader.svelte';
  import StatusBadge from '$lib/components/StatusBadge.svelte';
  import { workspaceContext } from '$lib/stores/workspace';

  let category = 'improvement';
  let title = '';
  let message = '';
  let includeWorkspace = true;
  let feedback: FeedbackItem[] = [];
  let loading = true;
  let saving = false;
  let error = '';
  let success = '';

  async function load() {
    loading = true;
    error = '';
    try {
      const data = await listMyFeedback();
      feedback = data.feedback;
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to load feedback.';
    } finally {
      loading = false;
    }
  }

  async function submit() {
    if (!title.trim() || !message.trim()) {
      error = 'Title and message are required.';
      return;
    }
    saving = true;
    error = '';
    success = '';
    try {
      const item = await createFeedback({
        category,
        title: title.trim(),
        message: message.trim(),
        workspaceId: includeWorkspace ? $workspaceContext.selectedWorkspace?.id : undefined
      });
      feedback = [item, ...feedback];
      title = '';
      message = '';
      success = 'Thanks. Your feedback was sent.';
    } catch (err) {
      error = err instanceof ApiError ? err.message : 'Unable to send feedback.';
    } finally {
      saving = false;
    }
  }

  function dateLabel(value: number) {
    return value ? new Date(value * 1000).toLocaleString() : '';
  }

  onMount(load);
</script>

<svelte:head><title>Feedback | Softadastra Cloud</title></svelte:head>

<PageHeader eyebrow="Feedback" title="Help improve Softadastra Cloud" />

<section class="dashboard-grid">
  <form class="panel" on:submit|preventDefault={submit}>
    <div class="panel-header"><h2>Send feedback</h2></div>
    <label>
      Category
      <select bind:value={category}>
        <option value="bug">Bug</option>
        <option value="idea">Idea</option>
        <option value="improvement">Improvement</option>
        <option value="question">Question</option>
        <option value="other">Other</option>
      </select>
    </label>
    <label>
      Title
      <input bind:value={title} type="text" required />
    </label>
    <label>
      Message
      <textarea bind:value={message} rows="6" required></textarea>
    </label>
    {#if $workspaceContext.selectedWorkspace}
      <label class="checkbox-row"><input bind:checked={includeWorkspace} type="checkbox" /> Attach current workspace</label>
    {/if}
    {#if error}<InlineError message={error} />{/if}
    {#if success}<p class="success-message">{success}</p>{/if}
    <LoadingButton loading={saving} loadingLabel="Sending...">Send feedback</LoadingButton>
  </form>

  <div class="panel span-2">
    <div class="panel-header"><h2>Your feedback</h2><span>{feedback.length}</span></div>
    {#if loading}
      <p class="muted">Loading feedback...</p>
    {:else if feedback.length === 0}
      <EmptyState title="No feedback sent yet." body="Your ideas and reports will appear here after submission." />
    {:else}
      <div class="table-list">
        {#each feedback as item}
          <div class="row">
            <span><strong>{item.title}</strong><small>{item.category} · {dateLabel(item.created_at)}</small><small>{item.message}</small></span>
            <span class="actions"><StatusBadge status={item.status} /></span>
          </div>
        {/each}
      </div>
    {/if}
  </div>
</section>
