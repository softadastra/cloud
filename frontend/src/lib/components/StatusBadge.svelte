<script lang="ts">
  export let status = '';
</script>
<span class="badge status-{status || 'unknown'}">{status || 'unknown'}</span>
