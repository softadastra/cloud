# Softadastra Cloud Database

Softadastra Cloud stores reliability testing data for projects, agents, runs, events, reports, and computed reliability scores.

The database is intentionally simple for the MVP. The goal is to support the first SaaS backend without over-engineering the schema.

## 1. Database role

The database is the source of truth for:

- dashboard users
- projects
- agents
- runs
- run events
- reports
- reliability scores
- migrations

The backend uses Vix DB, so the same architecture can work with SQLite during development and later move to MySQL/PostgreSQL-style production storage if needed.

## 2. MVP database model

```
users
  |
  v
projects
  |
  ├── agents
  |
  └── runs
        |
        ├── run_events
        ├── reports
        └── reliability_scores
```

## 3. Table overview

| Table | Purpose |
|-------|---------|
| `migrations` | Tracks applied database migrations |
| `users` | Dashboard users |
| `projects` | Applications/APIs being tested |
| `agents` | Local agents allowed to submit data |
| `runs` | Reliability test sessions |
| `run_events` | Timeline of events produced during a run |
| `reports` | Final report submitted for a run |
| `reliability_scores` | Computed score and risk flags for a run |

## 4. Timestamp convention

All timestamps are stored as integers — **milliseconds since Unix epoch UTC**.

Example: `1770000000000`

This keeps timestamps simple, portable, and easy to compare.

## 5. Public ids vs database ids

Every major table uses two ids:

| Field | Usage |
|-------|-------|
| `id` | Internal database id — used for joins |
| `public_id` | External API id — used in API responses and URLs |

**Examples:** `usr_xxx`, `prj_xxx`, `agt_xxx`, `run_xxx`, `evt_xxx`, `rpt_xxx`

> The API should never expose raw internal ids unless there is a deliberate reason.

## 6. Migration table

```sql
CREATE TABLE IF NOT EXISTS migrations (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  name TEXT NOT NULL UNIQUE,
  applied_at INTEGER NOT NULL
);
```

| Column | Description |
|--------|-------------|
| `id` | Internal migration id |
| `name` | Migration file/name |
| `applied_at` | Timestamp in milliseconds |

The MVP currently creates schema through C++ migration functions. Later, this table can be used by a full SQL migration runner.

## 7. Users table

```sql
CREATE TABLE IF NOT EXISTS users (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  public_id TEXT NOT NULL UNIQUE,
  email TEXT NOT NULL UNIQUE,
  password_hash TEXT NOT NULL,
  name TEXT NOT NULL,
  role TEXT NOT NULL DEFAULT 'user',
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL
);
```

| Column | Description |
|--------|-------------|
| `id` | Internal user id |
| `public_id` | Public user id, e.g. `usr_xxx` |
| `email` | Unique user email |
| `password_hash` | Hashed password |
| `name` | Display name |
| `role` | User role |
| `created_at` | Creation timestamp |
| `updated_at` | Last update timestamp |

**Indexes:**
```sql
CREATE INDEX IF NOT EXISTS idx_users_public_id ON users(public_id);
CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
```

> Users are for the dashboard. Agent authentication does not use this table.

## 8. Projects table

```sql
CREATE TABLE IF NOT EXISTS projects (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  public_id TEXT NOT NULL UNIQUE,
  user_id INTEGER,
  name TEXT NOT NULL,
  slug TEXT NOT NULL UNIQUE,
  description TEXT,
  base_url TEXT,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,

  FOREIGN KEY(user_id) REFERENCES users(id)
);
```

| Column | Description |
|--------|-------------|
| `id` | Internal project id |
| `public_id` | Public project id, e.g. `prj_xxx` |
| `user_id` | Owner user id |
| `name` | Project name |
| `slug` | Unique readable slug generated from name |
| `description` | Optional project description |
| `base_url` | Target API base URL |
| `created_at` | Creation timestamp |
| `updated_at` | Last update timestamp |

**Indexes:**
```sql
CREATE INDEX IF NOT EXISTS idx_projects_public_id ON projects(public_id);
CREATE INDEX IF NOT EXISTS idx_projects_user_id ON projects(user_id);
CREATE INDEX IF NOT EXISTS idx_projects_slug ON projects(slug);
```

**Relationship:** `users.id -> projects.user_id`

A project can have many agents and many runs.

## 9. Agents table

```sql
CREATE TABLE IF NOT EXISTS agents (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  public_id TEXT NOT NULL UNIQUE,
  project_id INTEGER NOT NULL,
  name TEXT NOT NULL,
  api_key_hash TEXT NOT NULL,
  last_seen_at INTEGER,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,

  FOREIGN KEY(project_id) REFERENCES projects(id)
);
```

| Column | Description |
|--------|-------------|
| `id` | Internal agent id |
| `public_id` | Public agent id, e.g. `agt_xxx` |
| `project_id` | Internal project id |
| `name` | Agent display name |
| `api_key_hash` | Hashed agent API key |
| `last_seen_at` | Last successful authentication timestamp |
| `created_at` | Creation timestamp |
| `updated_at` | Last update timestamp |

**Indexes:**
```sql
CREATE INDEX IF NOT EXISTS idx_agents_public_id ON agents(public_id);
CREATE INDEX IF NOT EXISTS idx_agents_project_id ON agents(project_id);
CREATE INDEX IF NOT EXISTS idx_agents_api_key_hash ON agents(api_key_hash);
CREATE INDEX IF NOT EXISTS idx_agents_last_seen_at ON agents(last_seen_at);
```

**Relationship:** `projects.id -> agents.project_id`

> **Security rule:** Plain API keys are shown only once. Only the hash is stored in the database.

## 10. Runs table

```sql
CREATE TABLE IF NOT EXISTS runs (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  public_id TEXT NOT NULL UNIQUE,
  project_id INTEGER NOT NULL,
  agent_id INTEGER,
  status TEXT NOT NULL DEFAULT 'created',
  target_url TEXT NOT NULL,
  method TEXT NOT NULL DEFAULT 'GET',
  scenario TEXT NOT NULL,
  started_at INTEGER NOT NULL,
  finished_at INTEGER,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,

  FOREIGN KEY(project_id) REFERENCES projects(id),
  FOREIGN KEY(agent_id) REFERENCES agents(id)
);
```

| Column | Description |
|--------|-------------|
| `id` | Internal run id |
| `public_id` | Public run id, e.g. `run_xxx` |
| `project_id` | Internal project id |
| `agent_id` | Optional internal agent id |
| `status` | Run status |
| `target_url` | Tested URL |
| `method` | HTTP method |
| `scenario` | Reliability scenario |
| `started_at` | Run start timestamp |
| `finished_at` | Run finish timestamp (nullable) |
| `created_at` | Creation timestamp |
| `updated_at` | Last update timestamp |

**Indexes:**
```sql
CREATE INDEX IF NOT EXISTS idx_runs_public_id ON runs(public_id);
CREATE INDEX IF NOT EXISTS idx_runs_project_id ON runs(project_id);
CREATE INDEX IF NOT EXISTS idx_runs_agent_id ON runs(agent_id);
CREATE INDEX IF NOT EXISTS idx_runs_status ON runs(status);
CREATE INDEX IF NOT EXISTS idx_runs_started_at ON runs(started_at);
```

**Relationships:** `projects.id -> runs.project_id`, `agents.id -> runs.agent_id`

**Valid statuses:** `created`, `running`, `finished`, `failed`, `cancelled`

**Final statuses:** `finished`, `failed`, `cancelled`

> A finished run should not be updated back to a non-final status.

## 11. Run events table

```sql
CREATE TABLE IF NOT EXISTS run_events (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  public_id TEXT NOT NULL UNIQUE,
  run_id INTEGER NOT NULL,
  event_type TEXT NOT NULL,
  severity TEXT NOT NULL DEFAULT 'info',
  message TEXT,
  payload TEXT,
  created_at INTEGER NOT NULL,

  FOREIGN KEY(run_id) REFERENCES runs(id)
);
```

| Column | Description |
|--------|-------------|
| `id` | Internal event id |
| `public_id` | Public event id, e.g. `evt_xxx` |
| `run_id` | Internal run id |
| `event_type` | Event type |
| `severity` | `info`, `warning`, `error`, `critical` |
| `message` | Optional human-readable message |
| `payload` | Raw JSON payload stored as text |
| `created_at` | Event timestamp |

**Indexes:**
```sql
CREATE INDEX IF NOT EXISTS idx_run_events_public_id ON run_events(public_id);
CREATE INDEX IF NOT EXISTS idx_run_events_run_id ON run_events(run_id);
CREATE INDEX IF NOT EXISTS idx_run_events_event_type ON run_events(event_type);
CREATE INDEX IF NOT EXISTS idx_run_events_severity ON run_events(severity);
CREATE INDEX IF NOT EXISTS idx_run_events_created_at ON run_events(created_at);
```

**Relationship:** `runs.id -> run_events.run_id`

**Example event types:** `agent_started`, `request_sent`, `request_timeout`, `retry_started`, `recovery_succeeded`, `recovery_failed`, `duplicate_detected`, `data_loss_risk_detected`

**Valid severities:** `info`, `warning`, `error`, `critical`

> `warn` may be accepted at service level and normalized to `warning`.

## 12. Reports table

```sql
CREATE TABLE IF NOT EXISTS reports (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  public_id TEXT NOT NULL UNIQUE,
  run_id INTEGER NOT NULL UNIQUE,
  summary TEXT NOT NULL,
  raw_json TEXT NOT NULL,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,

  FOREIGN KEY(run_id) REFERENCES runs(id)
);
```

| Column | Description |
|--------|-------------|
| `id` | Internal report id |
| `public_id` | Public report id, e.g. `rpt_xxx` |
| `run_id` | Internal run id |
| `summary` | Human-readable report summary |
| `raw_json` | Full raw report payload |
| `created_at` | Creation timestamp |
| `updated_at` | Last update timestamp |

**Indexes:**
```sql
CREATE INDEX IF NOT EXISTS idx_reports_public_id ON reports(public_id);
CREATE INDEX IF NOT EXISTS idx_reports_run_id ON reports(run_id);
```

**Relationship:** `runs.id -> reports.run_id`

> A run can have only one report (`run_id INTEGER NOT NULL UNIQUE`). Submitting a report for the same run again updates the existing report.

## 13. Reliability scores table

```sql
CREATE TABLE IF NOT EXISTS reliability_scores (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  run_id INTEGER NOT NULL UNIQUE,
  score INTEGER NOT NULL,
  grade TEXT NOT NULL,
  data_loss_risk INTEGER NOT NULL DEFAULT 0,
  duplicate_risk INTEGER NOT NULL DEFAULT 0,
  timeout_risk INTEGER NOT NULL DEFAULT 0,
  recovery_risk INTEGER NOT NULL DEFAULT 0,
  created_at INTEGER NOT NULL,

  FOREIGN KEY(run_id) REFERENCES runs(id)
);
```

| Column | Description |
|--------|-------------|
| `id` | Internal score id |
| `run_id` | Internal run id |
| `score` | Score from 0 to 100 |
| `grade` | A, B, C, D, or F |
| `data_loss_risk` | Boolean stored as integer |
| `duplicate_risk` | Boolean stored as integer |
| `timeout_risk` | Boolean stored as integer |
| `recovery_risk` | Boolean stored as integer |
| `created_at` | Score creation timestamp |

**Indexes:**
```sql
CREATE INDEX IF NOT EXISTS idx_reliability_scores_run_id ON reliability_scores(run_id);
CREATE INDEX IF NOT EXISTS idx_reliability_scores_score ON reliability_scores(score);
CREATE INDEX IF NOT EXISTS idx_reliability_scores_grade ON reliability_scores(grade);
```

**Relationship:** `runs.id -> reliability_scores.run_id`

> A run can have one active reliability score (`run_id INTEGER NOT NULL UNIQUE`). The current `ScoringService` computes the score in memory. Later, a score repository can persist this table.

## 14. Full entity relationship

```
users
  id, public_id, email, password_hash, name, role, created_at, updated_at
  |
  | 1 -> many
  v
projects
  id, public_id, user_id, name, slug, description, base_url, created_at, updated_at
  |
  | 1 -> many
  ├───────────────> agents
  |                  id, public_id, project_id, name, api_key_hash, last_seen_at, created_at, updated_at
  |
  | 1 -> many
  v
runs
  id, public_id, project_id, agent_id, status, target_url, method, scenario, started_at, finished_at, created_at, updated_at
  |
  | 1 -> many
  ├───────────────> run_events
  |                  id, public_id, run_id, event_type, severity, message, payload, created_at
  |
  | 1 -> one
  ├───────────────> reports
  |                  id, public_id, run_id, summary, raw_json, created_at, updated_at
  |
  | 1 -> one
  └───────────────> reliability_scores
                     id, run_id, score, grade, data_loss_risk, duplicate_risk, timeout_risk, recovery_risk, created_at
```

## 15. Repository mapping

Each domain repository owns SQL for one table or one domain.

| Repository | Table |
|------------|-------|
| `AuthRepository` | `users` |
| `ProjectRepository` | `projects` |
| `AgentRepository` | `agents` |
| `RunRepository` | `runs` |
| `EventRepository` | `run_events` |
| `ReportRepository` | `reports` |

Scoring currently reads from `RunRepository` and `EventRepository`. Later, scoring can write to `ReliabilityScoreRepository -> reliability_scores`.

## 16. Query patterns

**Load project by public id:**
```sql
SELECT * FROM projects WHERE public_id = ? LIMIT 1;
```

**Load agent by API key hash:**
```sql
SELECT * FROM agents WHERE api_key_hash = ? LIMIT 1;
```

**List runs for project:**
```sql
SELECT * FROM runs
WHERE project_id = ?
ORDER BY started_at DESC
LIMIT ? OFFSET ?;
```

**List events for run:**
```sql
SELECT * FROM run_events
WHERE run_id = ?
ORDER BY created_at ASC
LIMIT ? OFFSET ?;
```

**Count risky events:**
```sql
SELECT COUNT(*) FROM run_events
WHERE run_id = ? AND event_type = ?;
```

**Load report for run:**
```sql
SELECT * FROM reports WHERE run_id = ? LIMIT 1;
```

## 17. Scoring database inputs

The scoring service does not need the report to compute a score. It only needs `runs` and `run_events`.

For one run, it counts:

- `event_type = data_loss_risk_detected`
- `event_type = duplicate_detected`
- `event_type = request_timeout`
- `event_type = recovery_failed`
- `severity = critical`
- `severity = error`
- `severity = warning`

Then it computes: score, grade, risk flags.

## 18. Current MVP scoring rules

Start from **100**:

| Event | Penalty |
|-------|---------|
| `data_loss_risk_detected` | -35 |
| `duplicate_detected` | -20 |
| `request_timeout` | -10 |
| `recovery_failed` | -25 |
| `critical` severity | -15 each, max -30 |
| `error` severity | -8 each, max -24 |
| `warning` severity | -3 each, max -15 |

The final score is clamped: `0 <= score <= 100`

**Grades:**

| Grade | Range |
|-------|-------|
| A | 90 – 100 |
| B | 75 – 89 |
| C | 60 – 74 |
| D | 40 – 59 |
| F | 0 – 39 |

## 19. Recommended indexes

Important access paths: public_id lookups, foreign key lookups, API key hash lookup, run status lookup, event type/severity count, created/started timestamp ordering.

```sql
CREATE INDEX IF NOT EXISTS idx_users_public_id ON users(public_id);
CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);

CREATE INDEX IF NOT EXISTS idx_projects_public_id ON projects(public_id);
CREATE INDEX IF NOT EXISTS idx_projects_user_id ON projects(user_id);
CREATE INDEX IF NOT EXISTS idx_projects_slug ON projects(slug);

CREATE INDEX IF NOT EXISTS idx_agents_public_id ON agents(public_id);
CREATE INDEX IF NOT EXISTS idx_agents_project_id ON agents(project_id);
CREATE INDEX IF NOT EXISTS idx_agents_api_key_hash ON agents(api_key_hash);

CREATE INDEX IF NOT EXISTS idx_runs_public_id ON runs(public_id);
CREATE INDEX IF NOT EXISTS idx_runs_project_id ON runs(project_id);
CREATE INDEX IF NOT EXISTS idx_runs_agent_id ON runs(agent_id);
CREATE INDEX IF NOT EXISTS idx_runs_status ON runs(status);

CREATE INDEX IF NOT EXISTS idx_run_events_run_id ON run_events(run_id);
CREATE INDEX IF NOT EXISTS idx_run_events_event_type ON run_events(event_type);
CREATE INDEX IF NOT EXISTS idx_run_events_severity ON run_events(severity);

CREATE INDEX IF NOT EXISTS idx_reports_run_id ON reports(run_id);
CREATE INDEX IF NOT EXISTS idx_reliability_scores_run_id ON reliability_scores(run_id);
```

## 20. Data deletion rules

For the MVP, deletion is direct:

| Operation | Effect |
|-----------|--------|
| delete report | removes report only |
| delete run | removes run only |
| delete agent | removes agent only |
| delete project | removes project only |

For production, add an explicit cascade strategy. Recommended future rule — deleting a project should either be forbidden if it has runs, soft-delete the project, or cascade delete agents, runs, events, reports, and scores.

> The best SaaS behavior is usually **soft delete**.

## 21. Future production improvements

The MVP database is intentionally simple. Later improvements may include: organizations, team memberships, billing tables, usage counters, audit logs, webhook deliveries, alert rules, soft deletes, background jobs, score persistence, migration runner, database connection pool settings, read models for dashboard performance.

## 22. Future organization model

When Softadastra Cloud adds teams, the ownership model can become:

```
organizations
  └── users (through organization_members)
  └── projects
        ├── agents
        └── runs
```

Possible tables: `organizations`, `organization_members`, `invitations`.

## 23. Future billing model

Billing can be added without changing the reliability core.

Possible tables: `plans`, `subscriptions`, `usage_records`, `invoices`, `payments`.

Usage records can count: number of runs, number of events, number of agents, storage used by raw reports.

## 24. Future audit model

Audit logs should record important dashboard actions.

```sql
CREATE TABLE IF NOT EXISTS audit_logs (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  public_id TEXT NOT NULL UNIQUE,
  user_id INTEGER,
  action TEXT NOT NULL,
  target_type TEXT NOT NULL,
  target_public_id TEXT,
  metadata TEXT,
  created_at INTEGER NOT NULL,

  FOREIGN KEY(user_id) REFERENCES users(id)
);
```

Examples: `project.created`, `agent.created`, `agent.key_rotated`, `run.deleted`, `report.deleted`.

## 25. Database design principles

- Use internal ids for joins.
- Use public ids for APIs.
- Store timestamps in milliseconds UTC.
- Keep agent API keys hashed.
- Keep raw event/report payloads as text for MVP.
- Keep repositories responsible for SQL.
- Keep services responsible for business rules.
- Keep schema simple until product usage proves what needs to scale.

## 26. Final mental model

The database stores evidence.

```
projects  -> define what is tested
agents    -> submit observations
runs      -> group one reliability test
events    -> store what happened
reports   -> summarize the result
scores    -> convert events into risk
```

That is the core of Softadastra Reliability Cloud.
