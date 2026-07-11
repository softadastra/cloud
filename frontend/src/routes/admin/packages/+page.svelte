<script lang="ts">
  import { onMount } from 'svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import AdminShell from '$lib/components/admin/AdminShell.svelte';
  import {
    listAdminPackages,
    moderateAdminPackage
  } from '$lib/api/admin';
  import { ApiError } from '$lib/api/types';
  import type { AdminPackageSummary } from '$lib/api/types';

  let packages: AdminPackageSummary[] = [];
  let error = '';
  let loading = true;

  async function load() {
    error = '';

    try {
      packages = (await listAdminPackages()).packages;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load packages.';
    } finally {
      loading = false;
    }
  }

  async function hide(id: string) {
    if (!confirm('Set this package visibility to private?')) return;

    await moderateAdminPackage(id);
    await load();
  }

  onMount(load);
</script>

<svelte:head>
  <title>Packages | Softadastra Cloud</title>
</svelte:head>

<AdminShell
  title="Packages"
  crumb="Packages"
  description="Moderate package metadata without deleting archives."
>
  <InlineError message={error} />

  <section class="panel">
    <header class="panel__head">
      <div>
        <h2>Published packages</h2>
        <p>Public packages can be moved to private for moderation.</p>
      </div>

      <span class="panel__meta">{packages.length} packages</span>
    </header>

    {#if loading}
      <div class="empty-panel"><p>Loading packages…</p></div>
    {:else if packages.length === 0}
      <div class="empty-panel"><p>No packages have been published.</p></div>
    {:else}
      <div class="table-wrap">
        <table class="data-table">
          <thead>
            <tr>
              <th>Package</th>
              <th>Visibility</th>
              <th>Workspace</th>
              <th>Owner</th>
              <th class="is-right">Action</th>
            </tr>
          </thead>

          <tbody>
            {#each packages as pkg (pkg.id)}
              <tr>
                <td class="is-primary">
                  {pkg.name}
                  {#if pkg.description}
                    <small>{pkg.description}</small>
                  {/if}
                </td>

                <td>
                  <span
                    class={`badge status-${pkg.visibility === 'public' ? 'public' : 'private'}`}
                  >
                    {pkg.visibility === 'public' ? 'Public' : 'Private'}
                  </span>
                </td>

                <td class="cell-muted">{pkg.workspace_name}</td>

                <td class="cell-muted is-mono">{pkg.owner_email}</td>

                <td class="is-right">
                  {#if pkg.visibility === 'public'}
                    <button
                      class="btn btn-danger"
                      onclick={() => hide(pkg.id)}
                    >
                      Set private
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
