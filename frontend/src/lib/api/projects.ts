import { api } from './client';
import type { Project, ProjectsData } from './types';

export function listProjects(workspaceId: string) {
  return api.post<ProjectsData>('/api/projects/list', { workspace_id: workspaceId });
}

export function showProject(workspaceId: string, projectId: string) {
  return api.post<{ project: Project }>('/api/projects/show', { workspace_id: workspaceId, id: projectId });
}

export function createProject(input: {
  workspaceId: string;
  ownerUserId: string;
  name: string;
  slug: string;
  description?: string;
  repositoryUrl?: string;
  defaultBranch?: string;
}) {
  return api.post<{ project: Project }>('/api/projects', {
    workspace_id: input.workspaceId,
    owner_user_id: input.ownerUserId,
    name: input.name,
    slug: input.slug,
    description: input.description ?? '',
    repository_url: input.repositoryUrl ?? '',
    default_branch: input.defaultBranch ?? 'main'
  });
}
