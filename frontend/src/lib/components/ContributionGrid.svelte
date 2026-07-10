<script lang="ts">
  import type { ContributionGridDay } from '$lib/api/types';

  export let days: ContributionGridDay[] = [];

  function label(day: ContributionGridDay) {
    const text = day.count === 1 ? '1 public contribution' : `${day.count} public contributions`;
    return `${text} on ${day.date}`;
  }
</script>

<div class="contribution-grid" aria-label="Public contribution activity">
  <div class="grid-cells">
    {#each days as day (day.date)}
      <span
        class={`cell level-${Math.max(0, Math.min(4, day.level))}`}
        title={label(day)}
        aria-label={label(day)}
      ></span>
    {/each}
  </div>

  <div class="legend" aria-hidden="true">
    <span>Less</span>
    <span class="cell level-0"></span>
    <span class="cell level-1"></span>
    <span class="cell level-2"></span>
    <span class="cell level-3"></span>
    <span class="cell level-4"></span>
    <span>More</span>
  </div>
</div>

<style>
  .contribution-grid {
    display: grid;
    gap: 10px;
    padding: 16px;
  }

  .grid-cells {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(10px, 1fr));
    gap: 4px;
    align-items: center;
  }

  .cell {
    display: block;
    width: 10px;
    height: 10px;
    border: 1px solid var(--line-soft);
    border-radius: 2px;
    background: var(--bg-ink-soft);
  }

  .level-1 {
    border-color: rgba(125, 211, 252, 0.28);
    background: rgba(125, 211, 252, 0.22);
  }

  .level-2 {
    border-color: rgba(52, 211, 153, 0.32);
    background: rgba(52, 211, 153, 0.32);
  }

  .level-3 {
    border-color: rgba(34, 197, 94, 0.42);
    background: rgba(34, 197, 94, 0.48);
  }

  .level-4 {
    border-color: rgba(22, 163, 74, 0.56);
    background: rgba(22, 163, 74, 0.75);
  }

  .legend {
    display: flex;
    align-items: center;
    justify-content: flex-end;
    gap: 5px;
    color: var(--text-muted);
    font-size: 10.5px;
  }

  @media (max-width: 560px) {
    .grid-cells {
      grid-template-columns: repeat(auto-fill, minmax(9px, 1fr));
    }

    .cell {
      width: 9px;
      height: 9px;
    }
  }
</style>
