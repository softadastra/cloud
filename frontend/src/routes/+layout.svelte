<script lang="ts">
  import { page } from '$app/stores';
  import AppShell from '$lib/components/AppShell.svelte';
  import '../styles.css';
</script>

<AppShell>
  {#key `${$page.url.pathname}${$page.url.search}`}
    <slot />
  {/key}
</AppShell>
