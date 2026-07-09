import { api } from './client';
import type { BuildReportsData } from './types';

export function listBuildReports(workspaceId: string, projectId: string) {
  return api.post<BuildReportsData>('/api/build_reports/list', {
    workspace_id: workspaceId,
    project_id: projectId
  });
}
