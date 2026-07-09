import { api } from './client';
import type { BuildReport, BuildReportsData } from './types';

export function listBuildReports(workspaceId: string, projectId: string) {
  return api.post<BuildReportsData>('/api/build_reports/list', {
    workspace_id: workspaceId,
    project_id: projectId
  });
}

export function submitBuildReport(input: {
  workspaceId: string;
  projectId: string;
  submittedByUserId: string;
  status: string;
  target: string;
  profile?: string;
  branch?: string;
  commitSha?: string;
  toolchain?: string;
  summaryJson?: string;
  diagnosticsJson?: string;
  durationMs?: number;
  warningsCount?: number;
  errorsCount?: number;
}) {
  return api.post<{ build_report: BuildReport }>('/api/build_reports/submit', {
    workspace_id: input.workspaceId,
    project_id: input.projectId,
    submitted_by_user_id: input.submittedByUserId,
    status: input.status,
    target: input.target,
    profile: input.profile ?? 'debug',
    branch: input.branch ?? '',
    commit_sha: input.commitSha ?? '',
    toolchain: input.toolchain ?? '',
    summary_json: input.summaryJson ?? '{}',
    diagnostics_json: input.diagnosticsJson ?? '[]',
    duration_ms: input.durationMs ?? 0,
    warnings_count: input.warningsCount ?? 0,
    errors_count: input.errorsCount ?? 0
  });
}
