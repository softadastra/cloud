<script lang="ts">
  export let loading = false;
  export let disabled = false;
  export let loadingLabel = 'Working…';
  export let type: 'button' | 'submit' = 'submit';
</script>

<button {type} disabled={disabled || loading} aria-busy={loading} on:click>
  {#if loading}
    {loadingLabel}
  {:else}
    <slot />
  {/if}
</button>
