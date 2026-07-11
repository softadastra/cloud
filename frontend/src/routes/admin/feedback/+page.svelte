<script lang="ts">
  import { onMount } from 'svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import AdminShell from '$lib/components/admin/AdminShell.svelte';
  import {
    listAdminFeedback,
    updateAdminFeedbackStatus
  } from '$lib/api/admin';
  import { ApiError } from '$lib/api/types';
  import type { AdminFeedbackSummary } from '$lib/api/types';

  let feedback: AdminFeedbackSummary[] = [];
  let error = '';
  let loading = true;

  const statuses = ['open', 'reviewed', 'planned', 'closed'];

  async function load() {
    error = '';

    try {
      feedback = (await listAdminFeedback()).feedback;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load feedback.';
    } finally {
      loading = false;
    }
  }

  async function setStatus(id: string, status: string) {
    await updateAdminFeedbackStatus(id, status);
    await load();
  }

  function humanize(value: string) {
    return value
      .replaceAll('_', ' ')
      .replace(/\b\w/g, (letter) => letter.toUpperCase());
  }

  onMount(load);
</script>

<svelte:head>
  <title>Feedback | Softadastra Cloud</title>
</svelte:head>

<AdminShell
  title="Feedback"
  crumb="Feedback"
  description="Review early user feedback and update its status."
>
  <InlineError message={error} />

  <section class="panel">
    <header class="panel__head">
      <div>
        <h2>Submitted feedback</h2>
        <p>Set a status to move each item through the review flow.</p>
      </div>

      <span class="panel__meta">{feedback.length} items</span>
    </header>

    {#if loading}
      <div class="empty-panel"><p>Loading feedback…</p></div>
    {:else if feedback.length === 0}
      <div class="empty-panel"><p>No feedback has been submitted.</p></div>
    {:else}
      <div class="table-wrap">
        <table class="data-table">
          <thead>
            <tr>
              <th>Feedback</th>
              <th>User</th>
              <th>Status</th>
              <th class="feedback-message-col">Message</th>
              <th class="is-right">Set status</th>
            </tr>
          </thead>

          <tbody>
            {#each feedback as item (item.id)}
              <tr>
                <td class="is-primary">
                  {item.title}
                  <small>{humanize(item.category)}</small>
                </td>

                <td class="cell-muted is-mono">{item.user_email}</td>

                <td>
                  <span class={`badge status-${item.status}`}>
                    {humanize(item.status)}
                  </span>
                </td>

                <td class="cell-muted feedback-message">
                  {item.message}
                </td>

                <td class="is-right">
                  <select
                    class="field status-select"
                    value={item.status}
                    onchange={(e) =>
                      setStatus(item.id, e.currentTarget.value)}
                  >
                    {#each statuses as status}
                      <option value={status}>{humanize(status)}</option>
                    {/each}
                  </select>
                </td>
              </tr>
            {/each}
          </tbody>
        </table>
      </div>
    {/if}
  </section>
</AdminShell>

<style>
  .feedback-message-col {
    width: 34%;
  }

  .feedback-message {
    max-width: 320px;
    font-size: 11px;
    line-height: 1.55;
    white-space: normal;
  }

  .status-select {
    width: auto;
    min-width: 118px;
    padding: 5px 9px;
    font-size: 11px;
  }
</style>
