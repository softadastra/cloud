<script lang="ts">
  import type { ContributionGridDay } from '$lib/api/types';

  export let days: ContributionGridDay[] = [];
  export let year = new Date().getUTCFullYear();
  export let years: number[] = [];
  export let loading = false;
  export let onYearChange: (year: number) => void = () => {};

  const weekdayLabels = ['', 'Mon', '', 'Wed', '', 'Fri', ''];

  type CalendarCell = ContributionGridDay & { outside: boolean };
  type MonthLabel = { name: string; column: number };

  $: normalizedYears = [...new Set(years)].sort((a, b) => b - a);
  $: dayMap = new Map(days.map((day) => [day.date, day]));
  $: calendar = buildCalendar(year, dayMap);
  $: total = days.reduce((sum, day) => sum + day.count, 0);
  $: months = buildMonthLabels(year, calendar.start);

  function isoDate(date: Date) {
    return date.toISOString().slice(0, 10);
  }

  function buildCalendar(selectedYear: number, values: Map<string, ContributionGridDay>) {
    const first = new Date(Date.UTC(selectedYear, 0, 1));
    const last = new Date(Date.UTC(selectedYear, 11, 31));
    const start = new Date(first);
    const mondayOffset = (first.getUTCDay() + 6) % 7;
    start.setUTCDate(start.getUTCDate() - mondayOffset);

    const end = new Date(last);
    const sundayOffset = (7 - end.getUTCDay()) % 7;
    end.setUTCDate(end.getUTCDate() + sundayOffset);

    const cells: CalendarCell[] = [];
    for (const cursor = new Date(start); cursor <= end; cursor.setUTCDate(cursor.getUTCDate() + 1)) {
      const date = isoDate(cursor);
      const item = values.get(date);
      cells.push({
        date,
        count: item?.count ?? 0,
        level: item?.level ?? 0,
        outside: cursor.getUTCFullYear() !== selectedYear
      });
    }

    return { cells, start, weeks: cells.length / 7 };
  }

  function buildMonthLabels(selectedYear: number, gridStart: Date): MonthLabel[] {
    const formatter = new Intl.DateTimeFormat(undefined, { month: 'short', timeZone: 'UTC' });
    return Array.from({ length: 12 }, (_, month) => {
      const first = new Date(Date.UTC(selectedYear, month, 1));
      const distance = Math.floor((first.getTime() - gridStart.getTime()) / 86_400_000);
      return { name: formatter.format(first), column: Math.floor(distance / 7) + 1 };
    }).filter((item, index, items) => index === 0 || item.column !== items[index - 1].column);
  }

  function label(day: CalendarCell) {
    const date = new Intl.DateTimeFormat(undefined, {
      day: 'numeric', month: 'long', year: 'numeric', timeZone: 'UTC'
    }).format(new Date(`${day.date}T00:00:00Z`));
    const amount = day.count === 1 ? '1 public contribution' : `${day.count} public contributions`;
    return `${amount} on ${date}`;
  }

  function selectYear(event: Event) {
    onYearChange(Number((event.currentTarget as HTMLSelectElement).value));
  }
</script>

<section class:loading class="contribution-chart" aria-busy={loading}>
  <header class="chart-header">
    <div>
      <strong>{total.toLocaleString()} public {total === 1 ? 'contribution' : 'contributions'}</strong>
      <span>Package publishing and public profile activity during {year}</span>
    </div>

    {#if normalizedYears.length > 1}
      <label class="year-picker">
        <span class="sr-only">Contribution year</span>
        <select value={year} onchange={selectYear} disabled={loading}>
          {#each normalizedYears as availableYear}
            <option value={availableYear}>{availableYear}</option>
          {/each}
        </select>
      </label>
    {/if}
  </header>

  <div class="chart-scroll">
    <div class="calendar" style={`--weeks: ${calendar.weeks}`}>
      <div class="month-row" aria-hidden="true">
        {#each months as month}
          <span style={`grid-column: ${month.column}`}>{month.name}</span>
        {/each}
      </div>

      <div class="weekday-row" aria-hidden="true">
        {#each weekdayLabels as weekday}<span>{weekday}</span>{/each}
      </div>

      <div class="cells" aria-label={`Public contribution activity for ${year}`}>
        {#each calendar.cells as day (day.date)}
          <span
            class:outside={day.outside}
            class={`cell level-${Math.max(0, Math.min(4, day.level))}`}
            title={label(day)}
            aria-label={label(day)}
          ></span>
        {/each}
      </div>
    </div>
  </div>

  <footer class="chart-footer" aria-hidden="true">
    <span>Less</span>
    {#each [0, 1, 2, 3, 4] as level}<span class={`cell level-${level}`}></span>{/each}
    <span>More</span>
  </footer>
</section>

<style>
  .contribution-chart { display: grid; gap: 18px; padding: 18px; transition: opacity .18s ease; }
  .contribution-chart.loading { opacity: .55; }
  .chart-header { display: flex; align-items: flex-start; justify-content: space-between; gap: 16px; }
  .chart-header div { display: grid; gap: 4px; }
  .chart-header strong { color: var(--text); font-size: 13px; }
  .chart-header div span { color: var(--text-muted); font-size: 11.5px; }
  .year-picker select { min-width: 92px; min-height: 32px; border: 1px solid var(--line); border-radius: 8px; background: var(--bg-elevated); color: var(--text-soft); padding: 0 30px 0 10px; font-size: 12px; }
  .chart-scroll { overflow-x: auto; padding-bottom: 4px; scrollbar-width: thin; }
  .calendar { display: grid; grid-template-columns: 30px auto; grid-template-rows: 18px auto; column-gap: 8px; width: max-content; min-width: 100%; }
  .month-row { grid-column: 2; display: grid; grid-template-columns: repeat(var(--weeks), 12px); gap: 4px; height: 18px; color: var(--text-muted); font-size: 9.5px; }
  .month-row span { white-space: nowrap; }
  .weekday-row { grid-row: 2; display: grid; grid-template-rows: repeat(7, 12px); gap: 4px; color: var(--text-muted); font-size: 9px; line-height: 12px; }
  .cells { grid-column: 2; grid-row: 2; display: grid; grid-auto-flow: column; grid-template-rows: repeat(7, 12px); grid-auto-columns: 12px; gap: 4px; }
  .cell { display: block; width: 12px; height: 12px; border: 1px solid var(--line-soft); border-radius: 3px; background: var(--bg-ink-soft); }
  .cell.outside { visibility: hidden; }
  .cell {
    display: block;
    width: 12px;
    height: 12px;
    border: 1px solid var(--line-soft);
    border-radius: 3px;
    background: var(--bg-ink-soft);
    transition:
      border-color 120ms ease,
      background 120ms ease,
      transform 120ms ease;
  }

  .cells .cell:hover {
    transform: scale(1.18);
  }
.level-1 {
  border-color: rgba(34, 197, 94, 0.28);
  background: rgba(34, 197, 94, 0.16);
}

.level-2 {
  border-color: rgba(34, 197, 94, 0.45);
  background: rgba(34, 197, 94, 0.32);
}

.level-3 {
  border-color: rgba(34, 197, 94, 0.72);
  background: rgba(34, 197, 94, 0.58);
}

.level-4 {
  border-color: #4ade80;
  background: #22c55e;
}
  .chart-footer { display: flex; align-items: center; justify-content: flex-end; gap: 5px; color: var(--text-muted); font-size: 10px; }
  .chart-footer .cell { width: 10px; height: 10px; }
  .sr-only { position: absolute; width: 1px; height: 1px; overflow: hidden; clip: rect(0, 0, 0, 0); white-space: nowrap; }
  @media (max-width: 560px) { .contribution-chart { padding: 14px; } .chart-header { align-items: stretch; flex-direction: column; } .year-picker select { width: 100%; } }
</style>

