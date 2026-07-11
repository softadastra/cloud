<script lang="ts">
  import { onMount } from 'svelte';
  import InlineError from '$lib/components/InlineError.svelte';
  import AdminShell from '$lib/components/admin/AdminShell.svelte';
  import { getAdminModules, getAdminOverview } from '$lib/api/admin';
  import { ApiError } from '$lib/api/types';
  import type { AdminModule, AdminOverview } from '$lib/api/types';

  type AdminFeedbackItem = {
    id: string;
    category: string;
    title: string;
    status: string;
    created_at: number;
  };

  type AdminSupporterItem = {
    tier: string;
    display_name: string;
    status: string;
    started_at: number;
  };

  type AdminAuditItem = {
    action: string;
    target_type: string;
    target_id: string;
    created_at: number;
  };

  type AdminOverviewView = AdminOverview & {
    platform_admin?: { role?: string };
    recent_feedback?: AdminFeedbackItem[];
    recent_supporters?: AdminSupporterItem[];
    recent_activity?: AdminAuditItem[];
  };

  const headlineStats = [
    { label: 'Users', key: 'users_count', secondaryKey: 'active_users_count', secondaryLabel: 'active', href: '/admin/users' },
    { label: 'Workspaces', key: 'workspaces_count', secondaryKey: 'projects_count', secondaryLabel: 'projects' },
    { label: 'Packages', key: 'packages_count', secondaryKey: 'public_packages_count', secondaryLabel: 'public', href: '/admin/packages' },
    { label: 'Build reports', key: 'build_reports_count', secondaryKey: 'lockfiles_count', secondaryLabel: 'lockfiles' },
    { label: 'Feedback', key: 'feedback_count', secondaryKey: 'open_feedback_count', secondaryLabel: 'open', href: '/admin/feedback' },
    { label: 'Supporters', key: 'founding_supporters_count', secondaryKey: 'founding_builders_count', secondaryLabel: 'builders', href: '/admin/supporters' }
  ];

  const inventoryItems = [
    { label: 'package_versions', key: 'package_versions_count' },
    { label: 'public_packages', key: 'public_packages_count' },
    { label: 'private_packages', key: 'private_packages_count' },
    { label: 'lockfiles', key: 'lockfiles_count' },
    { label: 'build_reports', key: 'build_reports_count' }
  ];

  const moduleRoutes: Record<string, string> = {
    users: '/admin/users',
    packages: '/admin/packages',
    feedback: '/admin/feedback',
    supporters: '/admin/supporters',
    audit: '/admin/audit'
  };

  let overview: AdminOverview | null = null;
  let modules: AdminModule[] = [];
  let loading = true;
  let error = '';

  $: overviewView = overview as AdminOverviewView | null;
  $: adminRole = overviewView?.platform_admin?.role ?? 'admin';
  $: recentFeedback = overviewView?.recent_feedback ?? [];
  $: recentSupporters = overviewView?.recent_supporters ?? [];
  $: recentActivity = overviewView?.recent_activity ?? [];

  $: inactiveUsers = Math.max(
    0,
    statValue('users_count') - statValue('active_users_count')
  );

  $: attentionItems = [
    { label: 'Open feedback', hint: 'Waiting for review', href: '/admin/feedback', value: statValue('open_feedback_count'), urgent: statValue('open_feedback_count') > 0 },
    { label: 'Inactive users', hint: 'Accounts not active', href: '/admin/users', value: inactiveUsers, urgent: false },
    { label: 'Public packages', hint: 'Visible to everyone', href: '/admin/packages', value: statValue('public_packages_count'), urgent: false }
  ];

  function statValue(key: string) {
    const stats = overview?.stats as Record<string, unknown> | undefined;
    const value = stats?.[key];

    return typeof value === 'number' ? value : Number(value ?? 0);
  }

  function formatNumber(value: number) {
    return new Intl.NumberFormat().format(value);
  }

  function formatDate(value: number) {
    if (!value) return 'Unknown date';

    const timestamp = value < 1_000_000_000_000 ? value * 1000 : value;

    return new Intl.DateTimeFormat(undefined, {
      dateStyle: 'medium',
      timeStyle: 'short'
    }).format(new Date(timestamp));
  }

  function humanize(value: string) {
    return value
      .replaceAll('_', ' ')
      .replace(/\b\w/g, (letter) => letter.toUpperCase());
  }

  function moduleHref(module: AdminModule) {
    return moduleRoutes[module.key] ?? '';
  }

  async function load() {
    loading = true;
    error = '';

    try {
      const [overviewData, moduleData] = await Promise.all([
        getAdminOverview(),
        getAdminModules()
      ]);

      overview = overviewData;
      modules = moduleData.modules;
    } catch (err) {
      error =
        err instanceof ApiError
          ? err.message
          : 'Unable to load administration.';
    } finally {
      loading = false;
    }
  }

  onMount(load);
</script>

<svelte:head>
  <title>Administration | Softadastra Cloud</title>

  <meta
    name="description"
    content="Softadastra Cloud platform administration."
  />
</svelte:head>

<AdminShell
  title="Administration"
  crumb="Overview"
  role={overview ? adminRole : null}
  description="Review platform usage, moderate public data and manage operational records."
>
  <InlineError message={error} />

  {#if loading}
    <div class="loading-state" aria-live="polite">
      <span class="loading-spinner"></span>
      <p>Loading platform administration…</p>
    </div>
  {:else if overview}
    <!-- Metrics strip -->
    <section class="metrics" aria-label="Platform statistics">
      {#each headlineStats as stat (stat.key)}
        <svelte:element
          this={stat.href ? 'a' : 'div'}
          class="metric"
          href={stat.href}
        >
          <small>{stat.label}</small>
          <strong>{formatNumber(statValue(stat.key))}</strong>
          <span>
            <b>{formatNumber(statValue(stat.secondaryKey))}</b>
            {stat.secondaryLabel}
          </span>
        </svelte:element>
      {/each}
    </section>

    <!-- Feedback + operational column -->
    <section class="content-grid">
      <article class="panel">
        <header class="panel__head">
          <div>
            <h2>Recent feedback</h2>
            <p>Latest messages submitted from the product.</p>
          </div>

          <a class="panel__action" href="/admin/feedback">View all</a>
        </header>

        {#if recentFeedback.length === 0}
          <div class="empty-panel"><p>No feedback has been submitted.</p></div>
        {:else}
          <div class="table-wrap">
            <table class="data-table">
              <thead>
                <tr>
                  <th>Title</th>
                  <th>Category</th>
                  <th>Status</th>
                  <th class="is-right">Submitted</th>
                </tr>
              </thead>

              <tbody>
                {#each recentFeedback as item (item.id)}
                  <tr>
                    <td class="is-primary">
                      <a href="/admin/feedback">{item.title}</a>
                    </td>
                    <td class="cell-muted">{humanize(item.category)}</td>
                    <td>
                      <span class={`badge status-${item.status}`}>
                        {humanize(item.status)}
                      </span>
                    </td>
                    <td class="is-right is-mono">
                      {formatDate(item.created_at)}
                    </td>
                  </tr>
                {/each}
              </tbody>
            </table>
          </div>
        {/if}
      </article>

      <div class="side-column">
        <article class="panel">
          <header class="panel__head">
            <div>
              <h2>Review queue</h2>
              <p>Records that may require attention.</p>
            </div>
          </header>

          <div class="queue">
            {#each attentionItems as item (item.label)}
              <a href={item.href} class:urgent={item.urgent}>
                <span class="queue__copy">
                  <strong>{item.label}</strong>
                  <small>{item.hint}</small>
                </span>
                <b>{formatNumber(item.value)}</b>
              </a>
            {/each}
          </div>
        </article>

        <article class="panel">
          <header class="panel__head">
            <div>
              <h2>Registry records</h2>
              <p>Technical records currently stored.</p>
            </div>
          </header>

          <div class="readout">
            {#each inventoryItems as item (item.key)}
              <div>
                <span>{item.label}</span>
                <i class="readout__leader" aria-hidden="true"></i>
                <strong>{formatNumber(statValue(item.key))}</strong>
              </div>
            {/each}
          </div>
        </article>

        <article class="panel">
          <header class="panel__head">
            <div>
              <h2>Recent supporters</h2>
              <p>Latest supporter records.</p>
            </div>

            <a class="panel__action" href="/admin/supporters">View all</a>
          </header>

          {#if recentSupporters.length === 0}
            <div class="empty-panel"><p>No supporter records yet.</p></div>
          {:else}
            <div class="supporter-list">
              {#each recentSupporters.slice(0, 4) as supporter}
                <div>
                  <span class="supporter-list__copy">
                    <strong>{supporter.display_name}</strong>
                    <small>{humanize(supporter.tier)}</small>
                  </span>
                  <span class={`badge status-${supporter.status}`}>
                    {humanize(supporter.status)}
                  </span>
                </div>
              {/each}
            </div>
          {/if}
        </article>
      </div>
    </section>

    <!-- Modules -->
    <section class="panel">
      <header class="panel__head">
        <div>
          <h2>Administration modules</h2>
          <p>Open a module to inspect or manage its platform records.</p>
        </div>

        <span class="panel__meta">{modules.length} modules</span>
      </header>

      <div class="module-list">
        {#each modules as module (module.key)}
          <svelte:element
            this={moduleHref(module) ? 'a' : 'div'}
            class="module-row"
            class:unavailable={!moduleHref(module)}
            href={moduleHref(module) || undefined}
          >
            <span class="module-row__key">{module.key}</span>

            <span class="module-row__copy">
              <strong>{module.name}</strong>
              <small>{module.description}</small>
            </span>

            <span class="module-row__meta is-mono">
              {module.actions.length} capabilities
            </span>

            <span class="module-row__status">
              {#if moduleHref(module)}
                Open
                <svg viewBox="0 0 24 24" aria-hidden="true">
                  <path d="m9 18 6-6-6-6"></path>
                </svg>
              {:else}
                Backend only
              {/if}
            </span>
          </svelte:element>
        {/each}
      </div>
    </section>

    <!-- Audit trail -->
    <section class="panel">
      <header class="panel__head">
        <div>
          <h2>Recent administrative activity</h2>
          <p>Latest changes recorded in the platform audit log.</p>
        </div>

        <a class="panel__action" href="/admin/audit">View audit log</a>
      </header>

      {#if recentActivity.length === 0}
        <div class="empty-panel">
          <p>No administrative actions have been recorded.</p>
        </div>
      {:else}
        <div class="audit">
          {#each recentActivity as item}
            <div class="audit__row">
              <time class="is-mono">{formatDate(item.created_at)}</time>
              <span class="audit__marker" aria-hidden="true"></span>
              <span class="audit__copy">
                <strong>{humanize(item.action)}</strong>
                <small>
                  {humanize(item.target_type)}
                  {#if item.target_id}
                    · {item.target_id}
                  {/if}
                </small>
              </span>
            </div>
          {/each}
        </div>
      {/if}
    </section>
  {/if}
</AdminShell>

<style>
  /* Loading */

  .loading-state {
    display: flex;
    min-height: 220px;
    align-items: center;
    justify-content: center;
    gap: 10px;
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    color: var(--text-muted);
    font-size: 12px;
  }

  .loading-spinner {
    width: 15px;
    height: 15px;
    border: 2px solid var(--line-strong);
    border-top-color: var(--brand);
    border-radius: 50%;
    animation: spin 700ms linear infinite;
  }

  /* Metrics strip */

  .metrics {
    display: grid;
    grid-template-columns: repeat(6, minmax(0, 1fr));
    border: 1px solid var(--line);
    border-radius: var(--radius-md);
    background: var(--bg-panel);
    overflow: hidden;
  }

  .metric {
    display: grid;
    gap: 7px;
    align-content: start;
    border-right: 1px solid var(--line-soft);
    color: var(--text);
    padding: 14px 16px 15px;
    transition: background var(--speed) var(--ease);
  }

  .metric:last-child {
    border-right: 0;
  }

  a.metric:hover {
    background: var(--bg-panel-strong);
  }

  .metric small {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 9px;
    letter-spacing: 0.11em;
    text-transform: uppercase;
  }

  .metric strong {
    color: var(--text);
    font-size: 24px;
    font-weight: 650;
    letter-spacing: -0.02em;
    line-height: 1.1;
    font-variant-numeric: tabular-nums;
  }

  .metric span {
    color: var(--text-faint);
    font-size: 10.5px;
  }

  .metric span b {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 10.5px;
    font-weight: 600;
  }

  /* Panel action button */

  .panel__action {
    flex: 0 0 auto;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    background: var(--bg-elevated);
    color: var(--text-soft);
    padding: 4px 10px;
    font-size: 10.5px;
    font-weight: 600;
    transition:
      color var(--speed) var(--ease),
      border-color var(--speed) var(--ease);
  }

  .panel__action:hover {
    border-color: var(--brand);
    color: var(--text);
  }

  .data-table .is-primary a {
    display: block;
    max-width: 260px;
    overflow: hidden;
    color: var(--text-soft);
    font-weight: 600;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .data-table .is-primary a:hover {
    color: var(--link);
    text-decoration: underline;
    text-underline-offset: 3px;
  }

  /* Content grid */

  .content-grid {
    display: grid;
    grid-template-columns: minmax(0, 1.7fr) minmax(280px, 0.8fr);
    gap: 16px;
    align-items: start;
  }

  .side-column {
    display: grid;
    gap: 16px;
  }

  /* Review queue */

  .queue {
    display: grid;
  }

  .queue a {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    border-bottom: 1px solid var(--line-soft);
    border-left: 2px solid transparent;
    padding: 11px 15px 11px 14px;
    transition: background var(--speed) var(--ease);
  }

  .queue a:last-child {
    border-bottom: 0;
  }

  .queue a:hover {
    background: var(--bg-elevated);
  }

  .queue a.urgent {
    border-left-color: var(--warning);
  }

  .queue__copy {
    display: grid;
    gap: 2px;
  }

  .queue__copy strong {
    color: var(--text-soft);
    font-size: 11.5px;
    font-weight: 600;
  }

  .queue__copy small {
    color: var(--text-muted);
    font-size: 9.5px;
  }

  .queue b {
    color: var(--text);
    font-family: var(--font-mono);
    font-size: 13px;
    font-weight: 600;
    font-variant-numeric: tabular-nums;
  }

  /* Registry readout */

  .readout {
    display: grid;
    gap: 2px;
    padding: 12px 16px 14px;
  }

  .readout > div {
    display: flex;
    align-items: baseline;
    gap: 9px;
    padding: 5px 0;
  }

  .readout span {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 10.5px;
  }

  .readout__leader {
    flex: 1;
    border-bottom: 1px dotted var(--line-strong);
    transform: translateY(-3px);
  }

  .readout strong {
    color: var(--text);
    font-family: var(--font-mono);
    font-size: 12px;
    font-weight: 600;
    font-variant-numeric: tabular-nums;
  }

  /* Supporters */

  .supporter-list {
    display: grid;
  }

  .supporter-list > div {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    border-bottom: 1px solid var(--line-soft);
    padding: 10px 15px;
  }

  .supporter-list > div:last-child {
    border-bottom: 0;
  }

  .supporter-list__copy {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .supporter-list__copy strong {
    overflow: hidden;
    color: var(--text-soft);
    font-size: 11px;
    font-weight: 600;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .supporter-list__copy small {
    color: var(--text-muted);
    font-size: 9.5px;
  }

  /* Modules */

  .module-list {
    display: grid;
  }

  .module-row {
    display: grid;
    grid-template-columns: 92px minmax(0, 1fr) auto 96px;
    gap: 18px;
    align-items: center;
    min-height: 56px;
    border-bottom: 1px solid var(--line-soft);
    border-left: 2px solid transparent;
    color: var(--text);
    padding: 10px 16px 10px 14px;
    transition: background var(--speed) var(--ease);
  }

  .module-row:last-child {
    border-bottom: 0;
  }

  a.module-row:hover {
    border-left-color: var(--brand);
    background: var(--bg-elevated);
  }

  .module-row.unavailable {
    opacity: 0.55;
  }

  .module-row__key {
    color: var(--text-muted);
    font-family: var(--font-mono);
    font-size: 9.5px;
    letter-spacing: 0.1em;
    text-transform: uppercase;
  }

  .module-row__copy {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .module-row__copy strong {
    color: var(--text-soft);
    font-size: 12px;
    font-weight: 600;
  }

  .module-row__copy small {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 10.5px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .module-row__meta {
    text-align: right;
  }

  .module-row__status {
    display: inline-flex;
    align-items: center;
    justify-content: flex-end;
    gap: 5px;
    color: var(--text-muted);
    font-size: 10.5px;
    font-weight: 600;
  }

  a.module-row .module-row__status {
    color: var(--link);
  }

  .module-row__status svg {
    width: 13px;
    height: 13px;
    fill: none;
    stroke: currentColor;
    stroke-width: 1.8;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  /* Audit trail */

  .audit {
    display: grid;
    padding: 6px 0;
  }

  .audit__row {
    display: grid;
    grid-template-columns: 150px 14px minmax(0, 1fr);
    gap: 12px;
    align-items: center;
    min-height: 48px;
    padding: 8px 16px;
  }

  .audit__row:hover {
    background: var(--bg-elevated);
  }

  .audit__row time {
    text-align: right;
    white-space: nowrap;
  }

  .audit__marker {
    position: relative;
    display: grid;
    place-items: center;
    height: 100%;
  }

  .audit__marker::before {
    content: '';
    position: absolute;
    top: -14px;
    bottom: -14px;
    width: 1px;
    background: var(--line-soft);
  }

  .audit__marker::after {
    content: '';
    position: relative;
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: var(--brand);
  }

  .audit__row:first-child .audit__marker::before {
    top: 50%;
  }

  .audit__row:last-child .audit__marker::before {
    bottom: 50%;
  }

  .audit__copy {
    display: grid;
    min-width: 0;
    gap: 2px;
  }

  .audit__copy strong {
    color: var(--text-soft);
    font-size: 11.5px;
    font-weight: 600;
  }

  .audit__copy small {
    overflow: hidden;
    color: var(--text-muted);
    font-size: 10px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  @keyframes spin {
    to {
      transform: rotate(360deg);
    }
  }

  /* Responsive */

  @media (max-width: 1100px) {
    .metrics {
      grid-template-columns: repeat(3, minmax(0, 1fr));
    }

    .metric:nth-child(3n) {
      border-right: 0;
    }

    .metric:nth-child(n + 4) {
      border-top: 1px solid var(--line-soft);
    }
  }

  @media (max-width: 920px) {
    .content-grid {
      grid-template-columns: 1fr;
    }

    .module-row {
      grid-template-columns: minmax(0, 1fr) auto;
    }

    .module-row__key,
    .module-row__meta {
      display: none;
    }
  }

  @media (max-width: 620px) {
    .metrics {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }

    .metric:nth-child(3n) {
      border-right: 1px solid var(--line-soft);
    }

    .metric:nth-child(2n) {
      border-right: 0;
    }

    .metric:nth-child(n + 3) {
      border-top: 1px solid var(--line-soft);
    }

    .audit__row {
      grid-template-columns: 14px minmax(0, 1fr);
    }

    .audit__row time {
      display: none;
    }
  }

  @media (prefers-reduced-motion: reduce) {
    .loading-spinner {
      animation-duration: 1.4s;
    }
  }
</style>
