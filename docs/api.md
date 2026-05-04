# Softadastra Cloud API

Softadastra Cloud exposes the backend API used by the Softadastra Reliability Cloud dashboard and by local agents.

- The dashboard uses user/dashboard endpoints.
- Agents use API-key protected endpoints to submit reliability runs, events, and reports.

```
Frontend dashboard
  -> user token
  -> Softadastra Cloud API

Agent CLI/runtime
  -> agent API key
  -> Softadastra Cloud API
```

## 1. Base URL

**Local development:**
```
http://127.0.0.1:8080
```

**Production example:**
```
https://api.softadastra.com
```

## 2. Response format

All JSON responses follow one consistent shape.

**Success response:**
```json
{
  "success": true,
  "message": "Resource loaded",
  "data": {}
}
```

**Error response:**
```json
{
  "success": false,
  "error": "validation_failed",
  "message": "Invalid input"
}
```

**List response:**
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

## 3. Common HTTP status codes

| Code | Meaning |
|------|---------|
| 200 | OK — request succeeded |
| 201 | Created — resource created |
| 400 | Bad Request — invalid input |
| 401 | Unauthorized — missing or invalid authentication |
| 404 | Not Found — resource not found |
| 409 | Conflict — resource conflict |
| 500 | Internal Error — unexpected backend failure |

## 4. Authentication

Softadastra Cloud has two authentication models.

| Model | Usage |
|-------|-------|
| Dashboard user auth | token |
| Agent auth | API key |

**Dashboard token** — used by the frontend dashboard:
```
Authorization: Bearer <token>
```

**Agent API key** — used by agents:
```
X-Agent-Key: <api_key>
```

> The plain API key is shown only once when the agent is created. Only the hash is stored in the database.

## 5. Health API

### `GET /health`

Returns backend health information.

**Response:**
```json
{
  "success": true,
  "message": "Softadastra Cloud is healthy",
  "data": {
    "service": "softadastra-cloud",
    "environment": "development",
    "status": "ok"
  }
}
```

## 6. Auth API

The auth API is used by dashboard users.

### `POST /api/auth/register`

Creates a dashboard user.

**Request:**
```json
{
  "email": "user@example.com",
  "password": "secret-password",
  "name": "Gaspard"
}
```

**Response:**
```json
{
  "success": true,
  "message": "User registered",
  "data": {
    "user": {
      "id": "usr_xxx",
      "email": "user@example.com",
      "name": "Gaspard"
    },
    "token": "v1.xxx"
  }
}
```

### `POST /api/auth/login`

Authenticates a dashboard user.

**Request:**
```json
{
  "email": "user@example.com",
  "password": "secret-password"
}
```

**Response:**
```json
{
  "success": true,
  "message": "User authenticated",
  "data": {
    "user": {
      "id": "usr_xxx",
      "email": "user@example.com",
      "name": "Gaspard"
    },
    "token": "v1.xxx"
  }
}
```

### `GET /api/auth/me`

Returns the current dashboard user.

**Headers:**
```
Authorization: Bearer <token>
```

**Response:**
```json
{
  "success": true,
  "message": "User loaded",
  "data": {
    "id": "usr_xxx",
    "email": "user@example.com",
    "name": "Gaspard"
  }
}
```

## 7. Projects API

A project represents one application or API being tested.

### `GET /api/projects`

Lists projects.

**Query parameters:** `limit=20`, `offset=0`

**Response:**
```json
{
  "success": true,
  "message": "Projects loaded",
  "data": [
    {
      "id": "prj_xxx",
      "name": "PulseGrid API",
      "slug": "pulsegrid-api",
      "description": "Production monitoring API",
      "base_url": "https://pulsegrid.softadastra.com",
      "created_at": 1770000000000,
      "updated_at": 1770000000000
    }
  ],
  "meta": {
    "total": 1,
    "limit": 20,
    "offset": 0
  }
}
```

### `POST /api/projects`

Creates a project.

**Request:**
```json
{
  "name": "PulseGrid API",
  "description": "Production monitoring API",
  "base_url": "https://pulsegrid.softadastra.com"
}
```

**Response:**
```json
{
  "success": true,
  "message": "Project created",
  "data": {
    "id": "prj_xxx",
    "name": "PulseGrid API",
    "slug": "pulsegrid-api",
    "description": "Production monitoring API",
    "base_url": "https://pulsegrid.softadastra.com",
    "created_at": 1770000000000,
    "updated_at": 1770000000000
  }
}
```

### `GET /api/projects/:id`

Loads one project.

**Response:**
```json
{
  "success": true,
  "message": "Project loaded",
  "data": {
    "id": "prj_xxx",
    "name": "PulseGrid API",
    "slug": "pulsegrid-api",
    "description": "Production monitoring API",
    "base_url": "https://pulsegrid.softadastra.com",
    "created_at": 1770000000000,
    "updated_at": 1770000000000
  }
}
```

### `PUT /api/projects/:id`

Updates a project.

**Request:**
```json
{
  "name": "PulseGrid API",
  "description": "Updated description",
  "base_url": "https://pulsegrid.softadastra.com"
}
```

**Response:**
```json
{
  "success": true,
  "message": "Project updated",
  "data": {
    "id": "prj_xxx",
    "name": "PulseGrid API",
    "slug": "pulsegrid-api",
    "description": "Updated description",
    "base_url": "https://pulsegrid.softadastra.com",
    "created_at": 1770000000000,
    "updated_at": 1770000100000
  }
}
```

### `DELETE /api/projects/:id`

Deletes a project.

**Response:**
```json
{
  "success": true,
  "message": "Project deleted",
  "data": {
    "id": "prj_xxx",
    "name": "PulseGrid API"
  }
}
```

## 8. Agents API

An agent is a local CLI/runtime instance allowed to submit reliability data for a project.

### `GET /api/agents`

Lists all agents.

**Query parameters:** `limit=20`, `offset=0`

**Response:**
```json
{
  "success": true,
  "message": "Agents loaded",
  "data": [
    {
      "id": "agt_xxx",
      "project_id": "prj_xxx",
      "name": "Production Agent",
      "last_seen_at": 1770000000000,
      "created_at": 1770000000000,
      "updated_at": 1770000000000
    }
  ],
  "meta": {
    "total": 1,
    "limit": 20,
    "offset": 0
  }
}
```

### `POST /api/projects/:project_id/agents`

Creates an agent for a project.

**Request:**
```json
{
  "name": "Production Agent"
}
```

**Response:**
```json
{
  "success": true,
  "message": "Agent created",
  "data": {
    "agent": {
      "id": "agt_xxx",
      "project_id": "prj_xxx",
      "name": "Production Agent",
      "last_seen_at": null,
      "created_at": 1770000000000,
      "updated_at": 1770000000000
    },
    "api_key": "sa_agent_key_xxx"
  }
}
```

> **Important:** The `api_key` is returned only once. Store it locally in the agent configuration.

### `GET /api/agents/:id`

Loads one agent.

**Response:**
```json
{
  "success": true,
  "message": "Agent loaded",
  "data": {
    "id": "agt_xxx",
    "project_id": "prj_xxx",
    "name": "Production Agent",
    "last_seen_at": 1770000000000,
    "created_at": 1770000000000,
    "updated_at": 1770000000000
  }
}
```

### `POST /api/agents/:id/rotate-key`

Rotates an agent API key.

**Response:**
```json
{
  "success": true,
  "message": "Agent API key rotated",
  "data": {
    "agent": {
      "id": "agt_xxx",
      "project_id": "prj_xxx",
      "name": "Production Agent",
      "last_seen_at": 1770000000000,
      "created_at": 1770000000000,
      "updated_at": 1770000100000
    },
    "api_key": "sa_agent_key_new_xxx"
  }
}
```

### `DELETE /api/agents/:id`

Deletes an agent.

**Response:**
```json
{
  "success": true,
  "message": "Agent deleted",
  "data": {
    "id": "agt_xxx",
    "name": "Production Agent"
  }
}
```

## 9. Runs API

A run represents one reliability test session.

### `GET /api/runs`

Lists all runs.

**Query parameters:** `limit=20`, `offset=0`

**Response:**
```json
{
  "success": true,
  "message": "Runs loaded",
  "data": [
    {
      "id": "run_xxx",
      "project_id": "prj_xxx",
      "agent_id": "agt_xxx",
      "status": "running",
      "target_url": "https://api.example.com/orders",
      "method": "POST",
      "scenario": "unstable_network",
      "started_at": 1770000000000,
      "finished_at": null,
      "created_at": 1770000000000,
      "updated_at": 1770000000000
    }
  ],
  "meta": {
    "total": 1,
    "limit": 20,
    "offset": 0
  }
}
```

### `POST /api/runs`

Creates a run.

**Headers:**
```
X-Agent-Key: <api_key>
```

**Request:**
```json
{
  "project_id": "prj_xxx",
  "agent_id": "agt_xxx",
  "target_url": "https://api.example.com/orders",
  "method": "POST",
  "scenario": "unstable_network"
}
```

**Response:**
```json
{
  "success": true,
  "message": "Run created",
  "data": {
    "id": "run_xxx",
    "project_id": "prj_xxx",
    "agent_id": "agt_xxx",
    "status": "running",
    "target_url": "https://api.example.com/orders",
    "method": "POST",
    "scenario": "unstable_network",
    "started_at": 1770000000000,
    "finished_at": null,
    "created_at": 1770000000000,
    "updated_at": 1770000000000
  }
}
```

### `GET /api/runs/:id`

Loads one run.

**Response:**
```json
{
  "success": true,
  "message": "Run loaded",
  "data": {
    "id": "run_xxx",
    "project_id": "prj_xxx",
    "agent_id": "agt_xxx",
    "status": "running",
    "target_url": "https://api.example.com/orders",
    "method": "POST",
    "scenario": "unstable_network",
    "started_at": 1770000000000,
    "finished_at": null,
    "created_at": 1770000000000,
    "updated_at": 1770000000000
  }
}
```

### `POST /api/runs/:id/status`

Updates a run status.

**Request:**
```json
{
  "status": "running"
}
```

**Valid statuses:** `created`, `running`, `finished`, `failed`, `cancelled`

**Response:**
```json
{
  "success": true,
  "message": "Run updated",
  "data": {
    "id": "run_xxx",
    "status": "running",
    "updated_at": 1770000100000
  }
}
```

### `POST /api/runs/:id/finish`

Finishes a run.

**Request:**
```json
{
  "status": "finished"
}
```

**Valid final statuses:** `finished`, `failed`, `cancelled`

**Response:**
```json
{
  "success": true,
  "message": "Run finished",
  "data": {
    "id": "run_xxx",
    "status": "finished",
    "finished_at": 1770000200000,
    "updated_at": 1770000200000
  }
}
```

### `DELETE /api/runs/:id`

Deletes a run.

**Response:**
```json
{
  "success": true,
  "message": "Run deleted",
  "data": {
    "id": "run_xxx"
  }
}
```

## 10. Events API

Events represent the observable timeline of a run.

### `GET /api/events`

Lists all events.

**Query parameters:** `limit=50`, `offset=0`

**Response:**
```json
{
  "success": true,
  "message": "Events loaded",
  "data": [
    {
      "id": "evt_xxx",
      "run_id": "run_xxx",
      "event_type": "request_timeout",
      "severity": "warning",
      "message": "Request timed out after 5000ms",
      "payload": "{\"duration_ms\":5000}",
      "created_at": 1770000000000
    }
  ],
  "meta": {
    "total": 1,
    "limit": 50,
    "offset": 0
  }
}
```

### `POST /api/runs/:run_id/events`

Creates an event for a run.

**Headers:**
```
X-Agent-Key: <api_key>
```

**Request:**
```json
{
  "event_type": "request_timeout",
  "severity": "warning",
  "message": "Request timed out after 5000ms",
  "payload": {
    "duration_ms": 5000,
    "attempt": 1
  }
}
```

> The backend stores payload as raw JSON text.

**Response:**
```json
{
  "success": true,
  "message": "Event created",
  "data": {
    "id": "evt_xxx",
    "run_id": "run_xxx",
    "event_type": "request_timeout",
    "severity": "warning",
    "message": "Request timed out after 5000ms",
    "payload": "{\"duration_ms\":5000,\"attempt\":1}",
    "created_at": 1770000000000
  }
}
```

### `GET /api/events/:id`

Loads one event.

**Response:**
```json
{
  "success": true,
  "message": "Event loaded",
  "data": {
    "id": "evt_xxx",
    "run_id": "run_xxx",
    "event_type": "request_timeout",
    "severity": "warning",
    "message": "Request timed out after 5000ms",
    "payload": "{\"duration_ms\":5000,\"attempt\":1}",
    "created_at": 1770000000000
  }
}
```

## 11. Reports API

A report is the final result submitted for a run.

### `GET /api/reports`

Lists all reports.

**Query parameters:** `limit=20`, `offset=0`

**Response:**
```json
{
  "success": true,
  "message": "Reports loaded",
  "data": [
    {
      "id": "rpt_xxx",
      "run_id": "run_xxx",
      "summary": "Reliability test completed with timeout risk",
      "raw_json": "{\"score\":87,\"grade\":\"B\"}",
      "created_at": 1770000000000,
      "updated_at": 1770000000000
    }
  ],
  "meta": {
    "total": 1,
    "limit": 20,
    "offset": 0
  }
}
```

### `POST /api/runs/:run_id/report`

Creates or updates the report for a run.

**Headers:**
```
X-Agent-Key: <api_key>
```

**Request:**
```json
{
  "summary": "Reliability test completed with timeout risk",
  "raw_json": {
    "score": 87,
    "grade": "B",
    "risks": ["timeout"]
  }
}
```

**Response:**
```json
{
  "success": true,
  "message": "Report saved",
  "data": {
    "id": "rpt_xxx",
    "run_id": "run_xxx",
    "summary": "Reliability test completed with timeout risk",
    "raw_json": "{\"score\":87,\"grade\":\"B\",\"risks\":[\"timeout\"]}",
    "created_at": 1770000000000,
    "updated_at": 1770000000000
  }
}
```

### `GET /api/reports/:id`

Loads one report.

**Response:**
```json
{
  "success": true,
  "message": "Report loaded",
  "data": {
    "id": "rpt_xxx",
    "run_id": "run_xxx",
    "summary": "Reliability test completed with timeout risk",
    "raw_json": "{\"score\":87,\"grade\":\"B\",\"risks\":[\"timeout\"]}",
    "created_at": 1770000000000,
    "updated_at": 1770000000000
  }
}
```

### `GET /api/runs/:run_id/report`

Loads the report attached to a run.

**Response:**
```json
{
  "success": true,
  "message": "Report loaded",
  "data": {
    "id": "rpt_xxx",
    "run_id": "run_xxx",
    "summary": "Reliability test completed with timeout risk",
    "raw_json": "{\"score\":87,\"grade\":\"B\",\"risks\":[\"timeout\"]}",
    "created_at": 1770000000000,
    "updated_at": 1770000000000
  }
}
```

### `DELETE /api/reports/:id`

Deletes a report.

**Response:**
```json
{
  "success": true,
  "message": "Report deleted",
  "data": {
    "id": "rpt_xxx"
  }
}
```

### `DELETE /api/runs/:run_id/report`

Deletes the report attached to a run.

**Response:**
```json
{
  "success": true,
  "message": "Report deleted",
  "data": {
    "id": "rpt_xxx",
    "run_id": "run_xxx"
  }
}
```

## 12. Scoring API

The scoring API computes the reliability score of a run from its events.

### `GET /api/runs/:run_id/score`

Computes and returns the run reliability score.

**Response:**
```json
{
  "success": true,
  "message": "Reliability score computed",
  "data": {
    "run_id": "run_xxx",
    "score": 87,
    "grade": "B",
    "risks": {
      "data_loss": false,
      "duplicate": false,
      "timeout": true,
      "recovery": false
    },
    "created_at": 1770000000000
  }
}
```

**Scoring rules:**

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

The final score is clamped between **0** and **100**.

**Grades:**

| Grade | Score range |
|-------|-------------|
| A | 90 – 100 |
| B | 75 – 89 |
| C | 60 – 74 |
| D | 40 – 59 |
| F | 0 – 39 |

## 13. Agent flow

The normal agent-side API flow:

1. Agent authenticates with `X-Agent-Key`
2. Agent creates a run
3. Agent submits events
4. Agent finishes the run
5. Agent submits a report
6. Cloud computes score

**Example flow:**
```
POST /api/runs
POST /api/runs/:run_id/events
POST /api/runs/:run_id/events
POST /api/runs/:run_id/finish
POST /api/runs/:run_id/report
GET  /api/runs/:run_id/score
```

## 14. Common validation rules

**Project:**
- `name` — required, 2 to 120 characters
- `description` — max 500 characters
- `base_url` — empty or must start with `http://` or `https://`

**Agent:**
- `project_id` — required
- `name` — required, 2 to 120 characters

**Run:**
- `project_id` — required
- `target_url` — required, `http://` or `https://`, max 2048 characters
- `method` — `GET`, `POST`, `PUT`, `PATCH`, `DELETE`, `HEAD`, `OPTIONS`
- `scenario` — required, 2 to 120 characters

**Event:**
- `run_id` — required
- `event_type` — required, 2 to 120 characters
- `severity` — `info`, `warning`, `error`, `critical`
- `message` — max 1000 characters
- `payload` — max 1 MB

**Report:**
- `run_id` — required
- `summary` — required, 2 to 2000 characters
- `raw_json` — required, max 5 MB

## 15. Error codes

| Code | Meaning |
|------|---------|
| `validation_failed` | Invalid input |
| `unauthorized` | Missing or invalid authentication |
| `project_not_found` | Project does not exist |
| `agent_not_found` | Agent does not exist |
| `run_not_found` | Run does not exist |
| `event_not_found` | Event does not exist |
| `report_not_found` | Report does not exist |
| `conflict` | Resource conflict |
| `internal_error` | Unexpected backend failure |

## 16. Dashboard data model

The dashboard primarily consumes: projects, agents, runs, events, reports, scores.

**Recommended dashboard pages:**
- Projects page
- Project detail page
- Agents page
- Run detail page
- Reports page
- Reliability score section

## 17. MVP API boundary

**Included:**
- health
- auth
- projects
- agents
- runs
- events
- reports
- scoring

**Not included yet:**
- billing
- teams
- organizations
- webhooks
- advanced alerts
- multi-agent distributed runs
- Kubernetes integrations
- mobile SDK telemetry
- P2P telemetry

These can be added later without breaking the current API structure.
