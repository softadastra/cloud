<script lang="ts">
  export let scope = 'entire_workspace';
  $: label = scope === 'selected_projects' ? 'selected projects' : 'entire workspace';
</script>
<span class="badge scope-{scope}">{label}</span>
