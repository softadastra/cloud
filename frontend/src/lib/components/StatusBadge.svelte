<script lang="ts">
  export let status = '';

  $: value = status || 'unknown';
</script>

<span class="badge status-{value}" title="Status: {value}">{value}</span>
