<script lang="ts">
  export let message = '';
</script>
{#if message}<p class="form-error">{message}</p>{/if}
