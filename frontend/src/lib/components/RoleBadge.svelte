<script lang="ts">
  import { readableRole, type WorkspaceRole } from '$lib/permissions';
  export let role: WorkspaceRole = 'viewer';
</script>
<span class="badge role-{readableRole(role)}">{readableRole(role)}</span>
