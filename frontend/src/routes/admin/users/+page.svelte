<script lang="ts">
  import { usePageRefresh } from '$lib/stores/pageRefresh';
  import InlineError from '$lib/components/InlineError.svelte';
  import AdminShell from '$lib/components/admin/AdminShell.svelte';
  import {
    disableAdminUserPublicProfile,
    listAdminUsers
  } from '$lib/api/admin';
  import { ApiError } from '$lib/api/types';
  import type { AdminUserSummary } from '$lib/api/types';

  let users: AdminUserSummary[] = [];
  let error = '';
  let search = '';
  let loading = true;

  async function load() {
    error = '';

    try {
      users = (await listAdminUsers(search)).users;
    } catch (err) {
      error =
        err instanceof ApiError ? err.message : 'Unable to load users.';
    } finally {
      loading = false;
    }
  }

  async function disableProfile(id: string) {
    if (!confirm('Disable this public profile?')) return;

    await disableAdminUserPublicProfile(id);
    await load();
  }

  function formatDate(value: number | string) {
    const numeric = Number(value);
    if (!numeric) return '—';

    const timestamp =
      numeric < 1_000_000_000_000 ? numeric * 1000 : numeric;

    return new Intl.DateTimeFormat(undefined, {
      dateStyle: 'medium'
    }).format(new Date(timestamp));
  }

  function humanize(value: string) {
    return value
      .replaceAll('_', ' ')
      .replace(/\b\w/g, (letter) => letter.toUpperCase());
  }

  usePageRefresh(load);
</script>

<svelte:head>
  <title>Users | Softadastra Cloud</title>
</svelte:head>

<AdminShell
  title="Users"
  crumb="Users"
  description="Inspect accounts, profiles and public visibility."
>
  <InlineError message={error} />

  <div class="search-bar">
    <svg viewBox="0 0 24 24" aria-hidden="true">
      <circle cx="11" cy="11" r="7" />
      <path d="m21 21-4.3-4.3" />
    </svg>

    <input
      class="field search-field"
      bind:value={search}
      placeholder="Search by email or username"
      oninput={load}
    />
  </div>

  <section class="panel">
    <header class="panel__head">
      <div>
        <h2>All accounts</h2>
        <p>Every registered user and their public presence.</p>
      </div>

      <span class="panel__meta">{users.length} users</span>
    </header>

    {#if loading}
      <div class="empty-panel"><p>Loading users…</p></div>
    {:else if users.length === 0}
      <div class="empty-panel"><p>No users match this search.</p></div>
    {:else}
      <div class="table-wrap">
        <table class="data-table">
          <thead>
            <tr>
              <th>User</th>
              <th>Public profile</th>
              <th>Supporter</th>
              <th class="is-right">Created</th>
              <th class="is-right">Action</th>
            </tr>
          </thead>

          <tbody>
            {#each users as user (user.id)}
              <tr>
                <td class="is-primary">
                  {user.email}
                  <small>@{user.username || 'none'}</small>
                </td>

                <td>
                  {user.display_name || 'No name'}
                  <span
                    class={`badge ${user.public_profile_enabled ? 'is-yes' : 'is-no'}`}
                  >
                    {user.public_profile_enabled ? 'Public' : 'Private'}
                  </span>
                </td>

                <td class="cell-muted">
                  {user.supporter_tier
                    ? humanize(user.supporter_tier)
                    : '—'}
                </td>

                <td class="is-right is-mono">
                  {formatDate(user.created_at)}
                </td>

                <td class="is-right">
                  {#if user.public_profile_enabled}
                    <button
                      class="btn btn-danger"
                      onclick={() => disableProfile(user.id)}
                    >
                      Disable profile
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
  .search-bar {
    position: relative;
    display: flex;
    align-items: center;
  }

  .search-bar svg {
    position: absolute;
    left: 12px;
    width: 15px;
    height: 15px;
    fill: none;
    stroke: var(--text-faint);
    stroke-width: 1.8;
    stroke-linecap: round;
    pointer-events: none;
  }

  .search-field {
    padding-left: 34px;
  }
</style>
