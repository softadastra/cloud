import { api } from './client';
import type { PackageVersion, PackageVersionsData } from './types';

export function listPackageVersions(workspaceId: string, packageId: string) {
  return api.post<PackageVersionsData>('/api/package_versions/list', {
    workspace_id: workspaceId,
    package_id: packageId
  });
}

export function publishPackageVersion(input: {
  workspaceId: string;
  packageId: string;
  publishedByUserId: string;
  version: string;
  archiveUrl: string;
  checksumSha256: string;
  manifestJson: string;
  sizeBytes: number;
}) {
  return api.post<{ package_version: PackageVersion }>('/api/package_versions/publish', {
    workspace_id: input.workspaceId,
    package_id: input.packageId,
    published_by_user_id: input.publishedByUserId,
    version: input.version,
    archive_url: input.archiveUrl,
    checksum_sha256: input.checksumSha256,
    manifest_json: input.manifestJson,
    size_bytes: input.sizeBytes
  });
}


export function yankPackageVersion(workspaceId: string, packageId: string, versionId: string) {
  return api.post<{ package_version: PackageVersion }>('/api/package_versions/yank', { workspace_id: workspaceId, package_id: packageId, version_id: versionId });
}

export function unyankPackageVersion(workspaceId: string, packageId: string, versionId: string) {
  return api.post<{ package_version: PackageVersion }>('/api/package_versions/unyank', { workspace_id: workspaceId, package_id: packageId, version_id: versionId });
}

export function deprecatePackageVersion(workspaceId: string, packageId: string, versionId: string, deprecationMessage: string) {
  return api.post<{ package_version: PackageVersion }>('/api/package_versions/deprecate', { workspace_id: workspaceId, package_id: packageId, version_id: versionId, deprecation_message: deprecationMessage });
}

export function deletePackageVersion(workspaceId: string, packageId: string, versionId: string) {
  return api.post<{ package_version: PackageVersion }>('/api/package_versions/delete', { workspace_id: workspaceId, package_id: packageId, version_id: versionId });
}
