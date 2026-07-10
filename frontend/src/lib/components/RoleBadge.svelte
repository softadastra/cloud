<script lang="ts">
  import { readableRole, type WorkspaceRole } from '$lib/permissions';

  export let role: WorkspaceRole = 'viewer';

  $: readable = readableRole(role);
</script>

<span class="badge role-{readable}" title="Role: {readable}">{readable}</span>
