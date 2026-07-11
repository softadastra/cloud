<script lang="ts">
  import { onMount } from 'svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import AdminShell from '$lib/components/admin/AdminShell.svelte';
  import { listAdminAuditLogs } from '$lib/api/admin';
  import { ApiError } from '$lib/api/types';
  import type { AdminAuditLog } from '$lib/api/types';

  let logs: AdminAuditLog[] = [];
  let error = '';
  let loading = true;

  async function load() {
    try {
      logs = (await listAdminAuditLogs()).audit_logs;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load audit logs.';
    } finally {
      loading = false;
    }
  }

  function formatDate(value: number | string) {
    const numeric = Number(value);
    if (!numeric) return '—';

    const timestamp =
      numeric < 1_000_000_000_000 ? numeric * 1000 : numeric;

    return new Intl.DateTimeFormat(undefined, {
      dateStyle: 'medium',
      timeStyle: 'short'
    }).format(new Date(timestamp));
  }

  function humanize(value: string) {
    return value
      .replaceAll('_', ' ')
      .replace(/\b\w/g, (letter) => letter.toUpperCase());
  }

  onMount(load);
</script>

<svelte:head>
  <title>Audit logs | Softadastra Cloud</title>
</svelte:head>

<AdminShell
  title="Audit logs"
  crumb="Audit"
  description="Read-only history of every platform administration action."
>
  <InlineError message={error} />

  <section class="panel">
    <header class="panel__head">
      <div>
        <h2>Action history</h2>
        <p>Most recent administrative changes across the platform.</p>
      </div>

      <span class="panel__meta">{logs.length} entries</span>
    </header>

    {#if loading}
      <div class="empty-panel"><p>Loading audit logs…</p></div>
    {:else if logs.length === 0}
      <div class="empty-panel">
        <p>No administrative actions have been recorded.</p>
      </div>
    {:else}
      <div class="table-wrap">
        <table class="data-table">
          <thead>
            <tr>
              <th>Action</th>
              <th>Target</th>
              <th>Admin</th>
              <th class="is-right">Recorded</th>
            </tr>
          </thead>

          <tbody>
            {#each logs as log (log.created_at + log.action + log.target_id)}
              <tr>
                <td class="is-primary">{humanize(log.action)}</td>

                <td class="cell-muted">
                  {humanize(log.target_type)}
                  {#if log.target_id}
                    <span class="target-id">{log.target_id}</span>
                  {/if}
                </td>

                <td class="cell-muted is-mono">{log.admin_user_id}</td>

                <td class="is-right is-mono">
                  {formatDate(log.created_at)}
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
  .target-id {
    display: inline-block;
    margin-left: 4px;
    color: var(--text-faint);
    font-family: var(--font-mono);
    font-size: 10px;
  }
</style>
