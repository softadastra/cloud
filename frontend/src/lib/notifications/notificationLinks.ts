import type { NotificationItem } from '$lib/api/types';

function query(params: Record<string, string>) {
  const search = new URLSearchParams();

  for (const [key, value] of Object.entries(params)) {
    if (value) {
      search.set(key, value);
    }
  }

  const value = search.toString();
  return value ? `?${value}` : '';
}

function readData(notification: NotificationItem) {
  try {
    return JSON.parse(notification.data_json || '{}') as Record<string, unknown>;
  } catch {
    return {};
  }
}

function stringValue(value: unknown) {
  return typeof value === 'string' ? value : '';
}

export function notificationHref(notification: NotificationItem) {
  const data = readData(notification);
  const workspaceId = notification.workspace_id || stringValue(data.workspace_id);
  const projectId = notification.project_id || stringValue(data.project_id);
  const packageId = stringValue(data.package_id);

  switch (notification.type) {
    case 'workspace_invite_received':
      return '/dashboard';

    case 'workspace_invite_accepted':
    case 'workspace_invite_declined':
    case 'member_role_changed':
    case 'member_removed':
    case 'member_reactivated':
    case 'member_suspended':
      return `/members${query({ workspace_id: workspaceId })}`;

    case 'build_report_failed':
    case 'build_report_submitted':
      return `/build-reports${query({ workspace_id: workspaceId, project_id: projectId })}`;

    case 'package_version_published':
      return `/package-versions${query({ workspace_id: workspaceId, package_id: packageId })}`;

    case 'package_created':
    case 'package_visibility_changed':
    case 'package_status_changed':
      return `/packages${query({ workspace_id: workspaceId, package_id: packageId })}`;

    case 'lockfile_uploaded':
    case 'lockfile_deleted':
      return `/lockfiles${query({ workspace_id: workspaceId, project_id: projectId })}`;

    case 'token_created':
    case 'token_revoked':
      return `/tokens${query({ workspace_id: workspaceId })}`;

    case 'project_created':
    case 'project_updated':
    case 'project_deleted':
      return `/projects${query({ workspace_id: workspaceId, project_id: projectId })}`;

    case 'feedback_received':
    case 'feedback_status_changed':
      return '/feedback';

    default:
      if (projectId) {
        return `/projects${query({ workspace_id: workspaceId, project_id: projectId })}`;
      }
      if (workspaceId) {
        return `/workspaces${query({ workspace_id: workspaceId })}`;
      }
      return '/notifications';
  }
}
