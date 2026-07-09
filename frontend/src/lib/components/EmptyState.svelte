<script lang="ts">
  export let title = 'Nothing here yet.';
  export let body = '';
</script>
<div class="empty-state"><strong>{title}</strong>{#if body}<p>{body}</p>{/if}<slot /></div>
