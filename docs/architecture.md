# Softadastra Cloud Architecture

Softadastra Cloud is the backend of Softadastra Reliability Cloud.

Its role is to receive reliability test data from agents, store every run and event, compute a reliability score, and expose the data to the frontend dashboard hosted on `softadastra.com`.

The backend is built in C++ on top of Vix.

```
Agent CLI / runtime
        |
        | API key
        v
Softadastra Cloud API
        |
        | stores projects, agents, runs, events, reports
        v
Database
        |
        v
Dashboard on softadastra.com
```

## 1. Product goal

Softadastra Reliability Cloud answers one simple question:

> **Does your application lose data when the network fails?**

The MVP focuses on HTTP APIs. An agent runs tests against a target API, simulates reliability conditions, records what happened, and sends the result to the cloud backend.

The cloud backend stores: projects, agents, test runs, run events, final reports, computed reliability scores.

## 2. Core design principles

The backend follows a modular backend architecture. Each domain module owns its own model, repository, service, controller, and routes.

| Layer | Responsibility |
|-------|---------------|
| Routes | Connect URLs to controllers |
| Controller | Reads HTTP input and writes HTTP output |
| Service | Owns business rules |
| Repository | Owns database access |
| Model | Represents domain data |

This keeps the code easy to extend without mixing HTTP, business logic, and SQL in the same file.

## 3. High-level architecture

```
src/
├── main.cpp
├── app/
├── config/
├── database/
├── http/
├── middlewares/
├── modules/
├── security/
├── validation/
└── utils/
```

Each top-level directory has one clear responsibility.

## 4. Application startup

The backend starts from `src/main.cpp`.

**Startup flow:**

```
main.cpp
  |
  v
Load CloudConfig
  |
  v
Open database
  |
  v
Run migrations if enabled
  |
  v
Create AppState
  |
  v
AppFactory creates Vix app
  |
  v
MiddlewareRegistry installs middlewares
  |
  v
Routes are registered
  |
  v
Vix app starts listening
```

## 5. AppState

AppState is the shared application context. It holds the objects needed by controllers and services:

```
AppState
├── config
└── database
```

The goal is to avoid global variables and keep dependencies explicit. Services receive AppState by reference and create repositories from it.

**Example:**
```
ProjectService
├── AppState&
└── ProjectRepository
```

## 6. AppFactory

AppFactory creates the Vix application. It is responsible for: validating AppState, creating the `vix::App`, installing middleware, registering routes, and returning the ready application.

```
AppFactory::create(state)
  |
  ├── MiddlewareRegistry::install(app, state)
  ├── HealthRoutes::register_routes(app, state)
  ├── AuthRoutes::register_routes(app, state)
  ├── ProjectRoutes::register_routes(app, state)
  ├── AgentRoutes::register_routes(app, state)
  ├── RunRoutes::register_routes(app, state)
  ├── EventRoutes::register_routes(app, state)
  └── ReportRoutes::register_routes(app, state)
```

## 7. Configuration

Configuration is handled by `CloudConfig`. It wraps Vix config and exposes only the values needed by Softadastra Cloud.

**Main fields:**
- `service_name`
- `environment`
- `server_host`
- `server_port`
- `cors_allowed_origin`
- `dev_mode`
- `auto_migrate`
- `jwt_secret`
- `agent_api_key_secret`
- `max_request_body_bytes`

> The application should never read raw environment keys everywhere. Raw config keys stay inside `CloudConfig`.

## 8. Database layer

```
src/database/
├── Database.hpp
├── Database.cpp
├── Migrations.hpp
└── Migrations.cpp
```

`Database` opens the Vix database connection. `Migrations` creates the initial schema.

For the MVP, migrations are simple C++ migration functions. Later, this can evolve into a full SQL migration runner.

**Main tables:**

| Table | Purpose |
|-------|---------|
| `users` | Dashboard users |
| `projects` | Applications/APIs being tested |
| `agents` | Local agent instances |
| `runs` | Reliability test sessions |
| `run_events` | Timeline of events during a run |
| `reports` | Final submitted reports |
| `reliability_scores` | Computed scores and risk flags |

## 9. HTTP layer

```
src/http/
├── JsonResponse.hpp
├── JsonResponse.cpp
├── RequestContext.hpp
└── RequestContext.cpp
```

### JsonResponse

`JsonResponse` centralizes all API response formatting. Controllers should not manually build different JSON shapes everywhere.

**Common response helpers:** `ok`, `created`, `data`, `error`, `validation_error`, `unauthorized`, `internal_error`

**Success shape:**
```json
{
  "success": true,
  "message": "Project created",
  "data": {}
}
```

**Error shape:**
```json
{
  "success": false,
  "error": "validation_failed",
  "message": "Invalid input"
}
```

### RequestContext

`RequestContext` stores request-scoped metadata: `request_id`, `agent_public_id`, `project_public_id`, `auth_source`.

It is used by middleware and controllers to understand the current request.

## 10. Middleware layer

```
src/middlewares/
├── MiddlewareRegistry.hpp
├── MiddlewareRegistry.cpp
├── RequestIdMiddleware.hpp
├── RequestIdMiddleware.cpp
├── AgentAuthMiddleware.hpp
└── AgentAuthMiddleware.cpp
```

### MiddlewareRegistry

Installs all middleware in one place. This prevents scattered middleware setup inside `main.cpp`.

### RequestIdMiddleware

Generates a unique request id for each request. Useful for logs, debugging, tracing failed API calls, and support investigation.

### AgentAuthMiddleware

Authenticates agent requests using an API key. The agent sends an API key, the middleware verifies it, then attaches the authenticated agent and project to the request context.

## 11. Domain modules

All business domains live under `src/modules/`. Each domain follows this pattern:

```
Domain/
├── Model.hpp
├── Controller.hpp
├── Controller.cpp
├── Service.hpp
├── Service.cpp
├── Repository.hpp
├── Repository.cpp
├── Routes.hpp
└── Routes.cpp
```

## 12. Health module

```
modules/health/
├── HealthController.hpp
├── HealthController.cpp
├── HealthRoutes.hpp
└── HealthRoutes.cpp
```

Exposes the backend status. Used by deployment checks, monitoring, load balancers, and uptime checks.

**Endpoint:** `GET /health`

## 13. Auth module

```
modules/auth/
├── AuthController.hpp
├── AuthController.cpp
├── AuthService.hpp
├── AuthService.cpp
├── AuthRepository.hpp
├── AuthRepository.cpp
├── AuthRoutes.hpp
└── AuthRoutes.cpp
```

Handles dashboard user authentication. Separate from agent authentication.

| Auth type | Method |
|-----------|--------|
| Dashboard auth | user token |
| Agent auth | agent API key |

This separation is important because humans and agents have different security models.

## 14. Projects module

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

A project represents one application or API being tested.

```
Project: PulseGrid API
Base URL: https://pulsegrid.softadastra.com

Project
  ├── Agents
  └── Runs
```

## 15. Agents module

```
modules/agents/
├── Agent.hpp
├── AgentController.hpp
├── AgentController.cpp
├── AgentService.hpp
├── AgentService.cpp
├── AgentRepository.hpp
├── AgentRepository.cpp
├── AgentRoutes.hpp
└── AgentRoutes.cpp
```

An agent is a local CLI/runtime instance allowed to submit reliability data.

> **Security rule:** Plain API key is shown once. Only the hash is stored.

Agent API keys are used for: creating runs, submitting events, submitting reports.

## 16. Runs module

```
modules/runs/
├── Run.hpp
├── RunController.hpp
├── RunController.cpp
├── RunService.hpp
├── RunService.cpp
├── RunRepository.hpp
├── RunRepository.cpp
├── RunRoutes.hpp
└── RunRoutes.cpp
```

A run represents one reliability test session. It belongs to a project and an optional agent.

A run stores: `target_url`, `method`, `scenario`, `status`, `started_at`, `finished_at`.

**Common statuses:** `created`, `running`, `finished`, `failed`, `cancelled`

## 17. Events module

```
modules/events/
├── Event.hpp
├── EventController.hpp
├── EventController.cpp
├── EventService.hpp
├── EventService.cpp
├── EventRepository.hpp
├── EventRepository.cpp
├── EventRoutes.hpp
└── EventRoutes.cpp
```

Events are the observable timeline of a run.

**Example event types:** `agent_started`, `request_sent`, `request_timeout`, `retry_started`, `recovery_succeeded`, `recovery_failed`, `duplicate_detected`, `data_loss_risk_detected`

**Event severity:** `info`, `warning`, `error`, `critical`

Events are later used by the scoring module.

## 18. Reports module

```
modules/reports/
├── Report.hpp
├── ReportController.hpp
├── ReportController.cpp
├── ReportService.hpp
├── ReportService.cpp
├── ReportRepository.hpp
├── ReportRepository.cpp
├── ReportRoutes.hpp
└── ReportRoutes.cpp
```

A report is the final submitted result of a run. It stores: `summary`, `raw_json`, `created_at`, `updated_at`. A run can have one report. Submitting a report twice for the same run updates the existing report.

## 19. Scoring module

```
modules/scoring/
├── ReliabilityScore.hpp
├── ScoringService.hpp
└── ScoringService.cpp
```

The scoring module computes a reliability score from run events.

**MVP rules — start from 100:**

| Event | Penalty |
|-------|---------|
| `data_loss_risk_detected` | -35 |
| `duplicate_detected` | -20 |
| `request_timeout` | -10 |
| `recovery_failed` | -25 |
| `critical` severity | -15 each, max -30 |
| `error` severity | -8 each, max -24 |
| `warning` severity | -3 each, max -15 |

The final score is clamped between 0 and 100.

**Grades:**

| Grade | Range |
|-------|-------|
| A | 90 – 100 |
| B | 75 – 89 |
| C | 60 – 74 |
| D | 40 – 59 |
| F | 0 – 39 |

**Risk flags:** `data_loss_risk`, `duplicate_risk`, `timeout_risk`, `recovery_risk`

## 20. Security layer

```
src/security/
├── ApiKeyHasher.hpp
├── ApiKeyHasher.cpp
├── TokenService.hpp
└── TokenService.cpp
```

### ApiKeyHasher

Handles agent API key generation, hashing, and verification. For the MVP, the implementation is isolated and replaceable. Later, it should be replaced with a stronger production hash such as HMAC-SHA256.

### TokenService

Handles dashboard user tokens. For the MVP, the token format is simple and isolated. Later, it can be replaced with a real JWT implementation without changing controllers.

## 21. Validation layer

```
src/validation/
├── ValidationError.hpp
├── RunValidator.hpp
├── RunValidator.cpp
├── EventValidator.hpp
└── EventValidator.cpp
```

Validators do not know about HTTP or SQL. They only validate input and return validation errors. This keeps business rules reusable.

## 22. Utils layer

```
src/utils/
├── Time.hpp
├── Time.cpp
├── IdGenerator.hpp
└── IdGenerator.cpp
```

### Time

Centralizes timestamp creation. Database timestamps use milliseconds since Unix epoch UTC.

### IdGenerator

Generates public ids: `usr_xxx`, `prj_xxx`, `agt_xxx`, `run_xxx`, `evt_xxx`, `rpt_xxx`.

Internal database ids stay private. Public ids are used in API responses and URLs.

## 23. Request flow

Example: an agent submits an event.

```
HTTP request
  |
  v
RequestIdMiddleware
  |
  v
AgentAuthMiddleware
  |
  v
EventRoutes
  |
  v
EventController
  |
  v
EventService
  |
  v
EventRepository
  |
  v
Database
  |
  v
JsonResponse
```

The controller handles HTTP. The service handles business rules. The repository handles SQL.

## 24. Full reliability flow

```
1. User creates a project
2. User creates an agent for the project
3. Agent receives one-time API key
4. Agent starts a reliability run
5. Agent sends events during the test
6. Agent finishes the run
7. Cloud computes the score
8. Agent submits a final report
9. Dashboard displays the result
```

## 25. Data ownership

```
User
  └── Project
        ├── Agent
        ├── Run
        │     ├── Events
        │     ├── Report
        │     └── ReliabilityScore
```

## 26. API groups

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

## 27. Testing architecture

**Unit tests** verify isolated logic: utils, validation, security, services, scoring.

**Integration tests** verify database-backed flows: migrations, routes, full reliability flow.

The most important integration test is `FullReliabilityFlowIntegrationTest.cpp`, which validates the full backend business chain:

```
project -> agent -> run -> events -> score -> report
```

## 28. Why this architecture is scalable

Each part has a clear boundary. Adding a new domain does not require changing the whole backend.

**Example — adding billing later:**
```
modules/billing/
├── BillingPlan.hpp
├── BillingController.hpp
├── BillingController.cpp
├── BillingService.hpp
├── BillingService.cpp
├── BillingRepository.hpp
├── BillingRepository.cpp
├── BillingRoutes.hpp
└── BillingRoutes.cpp
```

**Example — adding organizations later:**
```
modules/organizations/
├── Organization.hpp
├── OrganizationController.hpp
├── OrganizationService.hpp
├── OrganizationRepository.hpp
└── OrganizationRoutes.hpp
```

To improve scoring later, only the scoring module changes. To replace simple tokens with JWT, only `TokenService` changes. To replace API key hashing with HMAC-SHA256, only `ApiKeyHasher` changes.

## 29. Current MVP boundaries

**Included:** HTTP API backend, projects, agents, runs, events, reports, simple scoring, dashboard support, agent API key auth, SQLite or MySQL through Vix DB.

**Not included yet:** billing, Kubernetes, multi-region infrastructure, distributed agents, mobile SDK, P2P sync, advanced organization/team permissions, complex alerting.

These can come later after the first working product.

## 30. Final mental model

Softadastra Cloud is not just a dashboard backend. It is the **control plane for reliability testing**.

```
Agent observes failures locally.
Cloud stores the evidence.
Scoring turns evidence into risk.
Dashboard makes the risk visible.
```

```
Vix App
  -> Middleware
  -> Routes
  -> Controllers
  -> Services
  -> Repositories
  -> Database
```

This gives Softadastra Cloud a foundation that can grow from MVP to SaaS without rewriting everything.
