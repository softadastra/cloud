import { api } from './client';
import type {
  AdminAuditLog, AdminFeedbackSummary, AdminModule, AdminOverview,
  AdminPackageSummary, AdminSupporterInput, AdminSupporterSummary, AdminUserSummary
} from './types';

export function getAdminOverview() { return api.post<AdminOverview>('/api/admin/overview', {}); }
export function getAdminModules() { return api.post<{ modules: AdminModule[] }>('/api/admin/modules', {}); }
export function listAdminUsers(search = '') { return api.post<{ users: AdminUserSummary[] }>('/api/admin/users/list', { search }); }
export function disableAdminUserPublicProfile(userId: string) { return api.post<{ updated: boolean }>('/api/admin/users/update_profile_visibility', { user_id: userId, public_profile_enabled: false }); }
export function listAdminPackages() { return api.post<{ packages: AdminPackageSummary[] }>('/api/admin/packages/list', {}); }
export function moderateAdminPackage(packageId: string) { return api.post<{ updated: boolean }>('/api/admin/packages/moderate', { package_id: packageId, action: 'hide_public_package' }); }
export function listAdminFeedback() { return api.post<{ feedback: AdminFeedbackSummary[] }>('/api/admin/feedback/list', {}); }
export function updateAdminFeedbackStatus(feedbackId: string, status: string) { return api.post<{ updated: boolean }>('/api/admin/feedback/update_status', { feedback_id: feedbackId, status }); }
export function listAdminSupporters() { return api.post<{ supporters: AdminSupporterSummary[] }>('/api/admin/supporters/list', {}); }
export function createAdminSupporter(input: AdminSupporterInput) { return api.post<{ id: string }>('/api/admin/supporters/create', input as Record<string, unknown>); }
export function updateAdminSupporter(input: AdminSupporterInput & { id: string }) { return api.post<{ updated: boolean }>('/api/admin/supporters/update', input as Record<string, unknown>); }
export function deactivateAdminSupporter(id: string) { return api.post<{ updated: boolean }>('/api/admin/supporters/deactivate', { id }); }
export function listAdminAuditLogs() { return api.post<{ audit_logs: AdminAuditLog[] }>('/api/admin/audit_logs/list', {}); }
