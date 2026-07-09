import { api } from './client';
import type { NotificationItem, NotificationPreferences } from './types';

export type NotificationListData = { notifications: NotificationItem[] };
export type UnreadCountData = { unread_count: number };

export async function listNotifications(options: { filter?: 'all' | 'unread' | 'read'; workspaceId?: string; projectId?: string; limit?: number } = {}) {
  return api.post<NotificationListData>('/api/notifications/list', {
    filter: options.filter === 'all' ? '' : options.filter,
    workspace_id: options.workspaceId ?? '',
    project_id: options.projectId ?? '',
    limit: options.limit ?? 50
  });
}

export async function unreadNotificationCount() {
  return api.post<UnreadCountData>('/api/notifications/unread_count', {});
}

export async function markNotificationRead(notificationId: string) {
  return api.post<NotificationItem>('/api/notifications/mark_read', { notification_id: notificationId });
}

export async function markAllNotificationsRead() {
  return api.post<UnreadCountData>('/api/notifications/mark_all_read', {});
}

export async function showNotificationPreferences(workspaceId = '') {
  return api.post<NotificationPreferences>('/api/notifications/preferences/show', { workspace_id: workspaceId });
}

export async function updateNotificationPreferences(preferences: Partial<NotificationPreferences> & { workspace_id?: string }) {
  return api.post<NotificationPreferences>('/api/notifications/preferences/update', preferences as Record<string, unknown>);
}

export async function listWorkspaceActivity(workspaceId: string, limit = 20) {
  return api.post<NotificationListData>('/api/activity/list_workspace', { workspace_id: workspaceId, limit });
}

export async function listProjectActivity(workspaceId: string, projectId: string, limit = 20) {
  return api.post<NotificationListData>('/api/activity/list_project', { workspace_id: workspaceId, project_id: projectId, limit });
}
