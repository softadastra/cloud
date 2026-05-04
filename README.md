# Softadastra Cloud

Softadastra Cloud is the backend for Softadastra Reliability Cloud.

It helps teams answer one critical question:

> **Does your application lose data when the network fails?**

Softadastra Cloud receives reliability test data from local agents, stores the evidence, computes a reliability score, and exposes the results to the dashboard on `softadastra.com`.

## What Softadastra Cloud does

Softadastra Cloud is a reliability testing control plane. It is designed to collect and analyze what happens when an application faces real-world failure conditions:

- network timeout
- retry failure
- unstable connection
- duplicate operation
- failed recovery
- possible data loss

The first MVP focuses on HTTP APIs. A local agent runs tests against an API, records events, sends them to Softadastra Cloud, and the dashboard shows the result.

## Product promise

> Softadastra shows whether your application loses data when the network fails.

### Why this matters

Many applications work well when the network is stable. But real users often face: bad mobile networks, unstable Wi-Fi, server timeouts, failed retries, browser refreshes, app restarts, partial writes.

These failures can create hidden problems: lost orders, duplicate payments, missing updates, broken sync, incomplete reports, silent data loss.

**Softadastra Cloud makes those failures visible.**

## How it works

1. Create a project
2. Create an agent
3. Run reliability tests locally
4. Submit runs and events to the cloud
5. Compute a reliability score
6. View the result in the dashboard

**High-level flow:**

```
Agent CLI / runtime
        |
        | X-Agent-Key
        v
Softadastra Cloud API
        |
        v
Projects, Agents, Runs, Events, Reports
        |
        v
Reliability Score
        |
        v
Dashboard on softadastra.com
```

## Core concepts

### Project

A project represents an application or API being tested.

```
PulseGrid API
https://pulsegrid.softadastra.com
```

### Agent

An agent is a local runtime or CLI process allowed to submit reliability data for a project. Agents authenticate with API keys.

> The plain API key is shown only once. Only the hash is stored by the backend.

### Run

A run is one reliability test session containing: target URL, HTTP method, scenario, status, start time, finish time.

### Event

An event is one observed fact during a run. Examples: `agent_started`, `request_timeout`, `retry_started`, `recovery_succeeded`, `recovery_failed`, `duplicate_detected`, `data_loss_risk_detected`.

### Report

A report is the final summary of a run. It stores a human-readable summary and the raw JSON output from the agent.

### Reliability score

The reliability score turns events into a risk profile.

```
100 = excellent
0   = dangerous
```

Risk flags: data loss risk, duplicate risk, timeout risk, recovery risk.

## MVP scope

**Included:**
- projects
- agents
- runs
- events
- reports
- reliability scoring
- agent API key authentication
- dashboard API
- SQLite/MySQL-ready backend through Vix DB

**Not included yet:**
- billing
- teams
- organizations
- Kubernetes
- mobile SDK
- distributed agents
- advanced alerting
- P2P telemetry

## Backend architecture

Softadastra Cloud follows a clean modular backend architecture.

```
Routes
  -> Controllers
  -> Services
  -> Repositories
  -> Database
```

Each domain has its own module: `health`, `auth`, `projects`, `agents`, `runs`, `events`, `reports`, `scoring`.

**Example module structure:**
```
modules/projects/
├── Project.hpp
├── ProjectController.hpp
├── ProjectController.cpp
├── ProjectService.hpp
├── ProjectService.cpp
├── ProjectRepository.hpp
├── ProjectRepository.cpp
├── ProjectRoutes.hpp
└── ProjectRoutes.cpp
```

This keeps HTTP logic, business rules, and SQL separated.

## Technology

Softadastra Cloud is built with:

- C++20
- Vix.cpp
- Vix DB
- Vix JSON
- Vix middleware
- Vix tests
- SQLite for local development

Vix provides the runtime-oriented C++ backend foundation. Softadastra Cloud provides the product logic.

## Repository structure

```
cloud/
├── LICENSE
├── README.md
├── CMakeLists.txt
├── CMakePresets.json
├── vix.json
├── .gitignore
├── .env.example
├── src/
├── sql/
├── tests/
└── docs/
```

| Directory | Purpose |
|-----------|---------|
| `src/` | Backend source code |
| `sql/` | SQL schema files |
| `tests/` | Unit and integration tests |
| `docs/` | Architecture, API, database docs |

## Configuration

Configuration is loaded through `CloudConfig`.

**Main values:**

| Key | Description |
|-----|-------------|
| `service_name` | Service identifier |
| `environment` | `development` or `production` |
| `server_host` | Bind address |
| `server_port` | Bind port |
| `cors_allowed_origin` | Allowed CORS origin |
| `dev_mode` | Enables dev helpers |
| `auto_migrate` | Run migrations on startup |
| `jwt_secret` | Dashboard token secret |
| `agent_api_key_secret` | Agent key secret |
| `max_request_body_bytes` | Max request body size |

**Example `.env`:**
```env
CLOUD_SERVICE_NAME=softadastra-cloud
CLOUD_ENVIRONMENT=development

SERVER_HOST=127.0.0.1
SERVER_PORT=8080

CLOUD_CORS_ALLOWED_ORIGIN=http://localhost:5173
CLOUD_DEV_MODE=true
CLOUD_AUTO_MIGRATE=true

CLOUD_JWT_SECRET=change-me
CLOUD_AGENT_API_KEY_SECRET=change-me
CLOUD_MAX_REQUEST_BODY_BYTES=1048576
```

## API groups

```
GET  /health

POST /api/auth/register
POST /api/auth/login
GET  /api/auth/me

GET  /api/projects
POST /api/projects
GET  /api/projects/:id
PUT  /api/projects/:id
DEL  /api/projects/:id

GET  /api/agents
POST /api/projects/:project_id/agents
GET  /api/agents/:id
POST /api/agents/:id/rotate-key
DEL  /api/agents/:id

GET  /api/runs
POST /api/runs
GET  /api/runs/:id
POST /api/runs/:id/status
POST /api/runs/:id/finish
DEL  /api/runs/:id

GET  /api/events
POST /api/runs/:run_id/events
GET  /api/events/:id

GET  /api/reports
POST /api/runs/:run_id/report
GET  /api/reports/:id
GET  /api/runs/:run_id/report
DEL  /api/reports/:id

GET  /api/runs/:run_id/score
```

## Response format

**Success:**
```json
{
  "success": true,
  "message": "Resource loaded",
  "data": {}
}
```

**Error:**
```json
{
  "success": false,
  "error": "validation_failed",
  "message": "Invalid input"
}
```

**List:**
```json
{
  "success": true,
  "message": "Resources loaded",
  "data": [],
  "meta": {
    "total": 10,
    "limit": 20,
    "offset": 0
  }
}
```

## Scoring model

Start from **100**, then subtract:

| Event | Penalty |
|-------|---------|
| `data_loss_risk_detected` | -35 |
| `duplicate_detected` | -20 |
| `request_timeout` | -10 |
| `recovery_failed` | -25 |
| `critical` severity | -15 each, max -30 |
| `error` severity | -8 each, max -24 |
| `warning` severity | -3 each, max -15 |

Score clamped between 0 and 100.

**Grades:**

| Grade | Range |
|-------|-------|
| A | 90 – 100 |
| B | 75 – 89 |
| C | 60 – 74 |
| D | 40 – 59 |
| F | 0 – 39 |

## Development

**Configure and build:**
```bash
vix build
```

**Run the backend:**
```bash
./build-ninja/softadastra-cloud
```

**Run tests:**
```bash
vix tests
```

## Documentation

| File | Content |
|------|---------|
| `docs/architecture.md` | Backend architecture and module design |
| `docs/api.md` | Full API reference |
| `docs/database.md` | Database schema and design |

## Final mental model

```
Agent observes failures locally.
Cloud stores the evidence.
Scoring turns evidence into risk.
Dashboard makes the risk visible.
```

Softadastra Cloud is the control plane for reliability testing.
