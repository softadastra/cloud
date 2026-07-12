<script lang="ts">
  import { usePageRefresh } from '$lib/stores/pageRefresh';
  import InlineError from '$lib/components/InlineError.svelte';
  import AdminShell from '$lib/components/admin/AdminShell.svelte';
  import SupporterBadge from '$lib/components/SupporterBadge.svelte';
  import {
    createAdminSupporter,
    deactivateAdminSupporter,
    listAdminSupporters
  } from '$lib/api/admin';
  import { ApiError } from '$lib/api/types';
  import type { AdminSupporterSummary } from '$lib/api/types';

  let supporters: AdminSupporterSummary[] = [];
  let error = '';
  let message = '';
  let loading = true;

  let displayName = '';
  let userId = '';
  let tier = 'founding_supporter';
  let username = '';
  let projectName = '';
  let websiteUrl = '';
  let githubUrl = '';
  let publicVisible = true;

  async function load() {
    try {
      supporters = (await listAdminSupporters()).supporters;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load supporters.';
    } finally {
      loading = false;
    }
  }

  async function createSupporter() {
    error = '';
    message = '';

    try {
      await createAdminSupporter({
        display_name: displayName,
        user_id: userId,
        tier,
        username,
        project_name: projectName,
        website_url: websiteUrl,
        github_url: githubUrl,
        public_visible: publicVisible
      });

      displayName = '';
      userId = '';
      username = '';
      projectName = '';
      websiteUrl = '';
      githubUrl = '';
      publicVisible = true;
      tier = 'founding_supporter';
      message = 'Supporter created.';

      await load();
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to create supporter.';
    }
  }

  async function deactivate(id: string) {
    if (!confirm('Deactivate this supporter record?')) return;

    await deactivateAdminSupporter(id);
    await load();
  }

  function humanize(value: string) {
    return value
      .replaceAll('_', ' ')
      .replace(/\b\w/g, (letter) => letter.toUpperCase());
  }

  usePageRefresh(load);
</script>

<svelte:head>
  <title>Founding Supporters | Softadastra Cloud</title>
</svelte:head>

<AdminShell
  title="Founding Supporters"
  crumb="Supporters"
  description="Manually confirm supporters after external payment confirmation. Never store payment details here."
>
  <InlineError message={error} />

  {#if message}
    <p class="notice">{message}</p>
  {/if}

  <section class="panel">
    <header class="panel__head">
      <div>
        <h2>Create supporter</h2>
        <p>Add a confirmed supporter record to the platform.</p>
      </div>
    </header>

    <div class="form-body">
      <div class="form-grid">
        <label>
          <span>Display name</span>
          <input class="field" bind:value={displayName} />
        </label>

        <label>
          <span>User ID <i>optional</i></span>
          <input class="field" bind:value={userId} />
        </label>

        <label>
          <span>Tier</span>
          <select class="field" bind:value={tier}>
            <option value="founding_supporter">Founding Supporter</option>
            <option value="founding_builder">Founding Builder</option>
          </select>
        </label>

        <label>
          <span>Username</span>
          <input class="field" bind:value={username} />
        </label>

        <label>
          <span>Project name</span>
          <input class="field" bind:value={projectName} />
        </label>

        <label>
          <span>Website URL</span>
          <input class="field" bind:value={websiteUrl} />
        </label>

        <label>
          <span>GitHub URL</span>
          <input class="field" bind:value={githubUrl} />
        </label>

        <label class="form-check">
          <input type="checkbox" bind:checked={publicVisible} />
          <span>Public visible</span>
        </label>
      </div>

      <div class="form-actions">
        <button
          type="button"
          class="btn btn-primary"
          onclick={createSupporter}
          disabled={!displayName}
        >
          Create supporter
        </button>
      </div>
    </div>
  </section>

  <section class="panel">
    <header class="panel__head">
      <div>
        <h2>Supporter records</h2>
        <p>Active and deactivated supporter entries.</p>
      </div>

      <span class="panel__meta">{supporters.length} records</span>
    </header>

    {#if loading}
      <div class="empty-panel"><p>Loading supporters…</p></div>
    {:else if supporters.length === 0}
      <div class="empty-panel"><p>No supporter records yet.</p></div>
    {:else}
      <div class="table-wrap">
        <table class="data-table">
          <thead>
            <tr>
              <th>Supporter</th>
              <th>Tier</th>
              <th>Status</th>
              <th>Public</th>
              <th>User</th>
              <th class="is-right">Action</th>
            </tr>
          </thead>

          <tbody>
            {#each supporters as supporter (supporter.id)}
              <tr>
                <td class="is-primary">
                  {supporter.display_name}
                  {#if supporter.project_name}
                    <small>{supporter.project_name}</small>
                  {/if}
                </td>

                <td><SupporterBadge tier={supporter.tier} /></td>

                <td>
                  <span class={`badge status-${supporter.status}`}>
                    {humanize(supporter.status)}
                  </span>
                </td>

                <td>
                  <span
                    class={`badge ${supporter.public_visible ? 'is-yes' : 'is-no'}`}
                  >
                    {supporter.public_visible ? 'Yes' : 'No'}
                  </span>
                </td>

                <td class="cell-muted is-mono">
                  {supporter.user_id || 'Not linked'}
                </td>

                <td class="is-right">
                  {#if supporter.status === 'active'}
                    <button
                      class="btn btn-danger"
                      onclick={() => deactivate(supporter.id)}
                    >
                      Deactivate
                    </button>
                  {/if}
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
  .form-body {
    display: grid;
    gap: 16px;
    padding: 16px;
  }

  .form-grid {
    display: grid;
    grid-template-columns: repeat(4, minmax(0, 1fr));
    gap: 13px;
  }

  .form-grid label {
    display: grid;
    gap: 5px;
  }

  .form-grid label > span {
    color: var(--text-muted);
    font-size: 10.5px;
    font-weight: 550;
  }

  .form-grid label > span i {
    color: var(--text-faint);
    font-style: normal;
    font-weight: 400;
  }

  .form-check {
    align-self: end;
    display: flex !important;
    align-items: center;
    gap: 8px;
    min-height: 33px;
  }

  .form-check input {
    width: 15px;
    height: 15px;
    accent-color: var(--brand);
  }

  .form-check span {
    color: var(--text-soft);
    font-size: 11.5px;
  }

  .form-actions {
    display: flex;
    justify-content: flex-end;
    border-top: 1px solid var(--line-soft);
    padding-top: 15px;
  }

  @media (max-width: 720px) {
    .form-grid {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }
  }

  @media (max-width: 480px) {
    .form-grid {
      grid-template-columns: 1fr;
    }
  }
</style>
