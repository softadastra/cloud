<script lang="ts">
  export let loading = false;
  export let disabled = false;
  export let loadingLabel = 'Working...';
  export let type: 'button' | 'submit' = 'submit';
</script>
<button {type} disabled={disabled || loading} on:click>{loading ? loadingLabel : ''}<slot /></button>
