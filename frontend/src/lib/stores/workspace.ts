import { browser } from '$app/environment';
import { derived, writable } from 'svelte/store';
import type { Workspace } from '$lib/api/types';

const STORAGE_KEY = 'softadastra.cloud.selected_workspace_id';

export type WorkspaceState = {
  workspaces: Workspace[];
  selectedWorkspace: Workspace | null;
};

function storedWorkspaceId() {
  return browser ? localStorage.getItem(STORAGE_KEY) ?? '' : '';
}

const store = writable<WorkspaceState>({ workspaces: [], selectedWorkspace: null });

function mergeWorkspace(previous: Workspace | undefined, incoming: Workspace) {
  if (!previous) {
    return incoming;
  }

  return {
    ...previous,
    ...incoming,
    avatar_url: Object.prototype.hasOwnProperty.call(incoming, 'avatar_url')
      ? incoming.avatar_url
      : previous.avatar_url,
    avatar_storage_path: Object.prototype.hasOwnProperty.call(incoming, 'avatar_storage_path')
      ? incoming.avatar_storage_path
      : previous.avatar_storage_path,
    avatar_updated_at: Object.prototype.hasOwnProperty.call(incoming, 'avatar_updated_at')
      ? incoming.avatar_updated_at
      : previous.avatar_updated_at
  };
}

function mergeWorkspaces(previous: Workspace[], incoming: Workspace[]) {
  return incoming.map((workspace) =>
    mergeWorkspace(previous.find((item) => item.id === workspace.id), workspace)
  );
}

function choose(workspaces: Workspace[], preferredId = '') {
  const id = preferredId || storedWorkspaceId();
  return workspaces.find((workspace) => workspace.id === id) ?? workspaces[0] ?? null;
}

export const workspaceContext = {
  subscribe: store.subscribe,
  setWorkspaces(workspaces: Workspace[], preferredId = '') {
    store.update((state) => {
      const merged = mergeWorkspaces(state.workspaces, workspaces);
      const selectedWorkspace = choose(merged, preferredId);
      if (browser && selectedWorkspace) localStorage.setItem(STORAGE_KEY, selectedWorkspace.id);
      return { workspaces: merged, selectedWorkspace };
    });
  },
  setSelectedWorkspace(workspaceId: string) {
    store.update((state) => {
      const selectedWorkspace = state.workspaces.find((workspace) => workspace.id === workspaceId) ?? null;
      if (browser && selectedWorkspace) localStorage.setItem(STORAGE_KEY, selectedWorkspace.id);
      return { ...state, selectedWorkspace };
    });
  },
  updateWorkspace(updated: Workspace) {
    store.update((state) => {
      const workspaces = state.workspaces.map((workspace) =>
        workspace.id === updated.id ? mergeWorkspace(workspace, updated) : workspace
      );
      const selectedWorkspace =
        state.selectedWorkspace?.id === updated.id
          ? mergeWorkspace(state.selectedWorkspace, updated)
          : workspaces.find((workspace) => workspace.id === state.selectedWorkspace?.id) ?? state.selectedWorkspace;

      return { workspaces, selectedWorkspace };
    });
  },
  clear() {
    if (browser) localStorage.removeItem(STORAGE_KEY);
    store.set({ workspaces: [], selectedWorkspace: null });
  }
};

export const currentUserRole = derived(workspaceContext, ($workspaceContext) => $workspaceContext.selectedWorkspace?.current_user_role ?? 'viewer');
