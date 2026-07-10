import { api } from "./client";
import type { PublicProfileResponse } from "./types";

export function getPublicProfile(
  username: string,
  year?: number,
): Promise<PublicProfileResponse> {
  return api.post<PublicProfileResponse>("/api/public/users/show", {
    username,
    ...(year !== undefined ? { year } : {}),
  });
}
