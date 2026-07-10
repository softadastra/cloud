<script lang="ts">
  import type { SupportPlanTier } from '$lib/api/types';

  export let tier: SupportPlanTier | string = 'founding_supporter';

  $: isBuilder = tier === 'founding_builder';
  $: label = isBuilder ? 'Founding Builder' : 'Founding Supporter';
</script>

<span class:builder={isBuilder} class="supporter-badge">{label}</span>

<style>
  .supporter-badge {
    display: inline-flex;
    width: fit-content;
    align-items: center;
    border: 1px solid var(--brand-line);
    border-radius: 999px;
    background: var(--brand-faint);
    color: var(--brand-bright);
    padding: 3px 9px;
    font-size: 11px;
    font-weight: 750;
    line-height: 1.4;
  }

  .supporter-badge.builder {
    border-color: rgba(245, 158, 11, 0.42);
    background: rgba(245, 158, 11, 0.12);
    color: #fbbf24;
  }
</style>
