<script lang="ts">
  export let scope: 'entire_workspace' | 'selected_projects' | string = 'entire_workspace';

  $: label =
    scope === 'selected_projects' ? 'Selected projects' : 'Entire workspace';
</script>

<span class="badge scope-{scope}" title="Access scope: {label}">{label}</span>
