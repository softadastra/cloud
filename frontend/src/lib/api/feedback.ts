import { api } from './client';
import type { FeedbackItem } from './types';

export type FeedbackListData = { feedback: FeedbackItem[] };

export async function createFeedback(input: { category: string; title: string; message: string; workspaceId?: string }) {
  return api.post<FeedbackItem>('/api/feedback/create', {
    category: input.category,
    title: input.title,
    message: input.message,
    workspace_id: input.workspaceId ?? ''
  });
}

export async function listMyFeedback(limit = 50) {
  return api.post<FeedbackListData>('/api/feedback/list_mine', { limit });
}

export async function listWorkspaceFeedback(workspaceId: string, limit = 50) {
  return api.post<FeedbackListData>('/api/feedback/list_workspace', { workspace_id: workspaceId, limit });
}
