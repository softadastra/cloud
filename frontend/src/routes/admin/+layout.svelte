<script lang="ts">
  import { goto } from '$app/navigation';
  import { onMount } from 'svelte';
  import { get } from 'svelte/store';
  import { auth } from '$lib/stores/auth';

  let allowed = false;
  let checked = false;

  async function guard() {
    const state = get(auth);

    if (!state.session?.id) {
      checked = true;
      await goto('/login', { replaceState: true });
      return;
    }

    const status = await auth.refreshCurrentUser();
    const current = get(auth);

    if (status === 'invalid' || status === 'missing') {
      checked = true;
      await goto('/login', { replaceState: true });
      return;
    }

    if (!current.user?.platform_admin) {
      checked = true;
      await goto('/404', { replaceState: true });
      return;
    }

    allowed = true;
    checked = true;
  }

  onMount(() => {
    void guard();
  });
</script>

{#if allowed}
  <slot />
{:else if !checked}
  <section class="admin-route-check" aria-label="Checking access"></section>
{/if}

<style>
  .admin-route-check {
    min-height: 50vh;
  }
</style>
