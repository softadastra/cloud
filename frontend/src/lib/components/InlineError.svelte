<script lang="ts">
  export let message = '';
</script>

{#if message}
  <p class="form-error" role="alert">{message}</p>
{/if}
