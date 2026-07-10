<script lang="ts">
  import RoleBadge from './RoleBadge.svelte';

  export let eyebrow = '';
  export let title = '';
  export let workspaceName = '';
  export let role = '';
</script>

<section class="page-header">
  <div>
    {#if eyebrow}
      <p class="eyebrow">{eyebrow}</p>
    {/if}

    <h1>{title}</h1>

    {#if workspaceName}
      <p class="context-line compact">
        {workspaceName}
        {#if role}
          · <RoleBadge {role} />
        {/if}
      </p>
    {/if}
  </div>

  <slot />
</section>
