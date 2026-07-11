import { api } from "./client";
import type { PublicPackageShowResponse } from "./types";

export function showPublicPackage(
  namespace: string,
  name: string,
): Promise<PublicPackageShowResponse> {
  return api.post<PublicPackageShowResponse>("/api/public/packages/show", {
    namespace,
    name,
  });
}
