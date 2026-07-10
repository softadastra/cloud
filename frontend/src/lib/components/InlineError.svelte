<script lang="ts">
  import { auth } from '$lib/stores/auth';

  export let message = '';

  $: isStaleSessionMessage =
    (message === 'Session expired. Please log in again.' ||
      message === 'Authentication is required. Please log in.') &&
    !$auth.sessionExpired;

  $: visibleMessage = isStaleSessionMessage ? '' : message;
</script>

{#if visibleMessage}
  <p class="form-error" role="alert">{visibleMessage}</p>
{/if}
