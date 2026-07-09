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

function choose(workspaces: Workspace[], preferredId = '') {
  const id = preferredId || storedWorkspaceId();
  return workspaces.find((workspace) => workspace.id === id) ?? workspaces[0] ?? null;
}

export const workspaceContext = {
  subscribe: store.subscribe,
  setWorkspaces(workspaces: Workspace[], preferredId = '') {
    const selectedWorkspace = choose(workspaces, preferredId);
    if (browser && selectedWorkspace) localStorage.setItem(STORAGE_KEY, selectedWorkspace.id);
    store.set({ workspaces, selectedWorkspace });
  },
  setSelectedWorkspace(workspaceId: string) {
    store.update((state) => {
      const selectedWorkspace = state.workspaces.find((workspace) => workspace.id === workspaceId) ?? null;
      if (browser && selectedWorkspace) localStorage.setItem(STORAGE_KEY, selectedWorkspace.id);
      return { ...state, selectedWorkspace };
    });
  },
  clear() {
    if (browser) localStorage.removeItem(STORAGE_KEY);
    store.set({ workspaces: [], selectedWorkspace: null });
  }
};

export const currentUserRole = derived(workspaceContext, ($workspaceContext) => $workspaceContext.selectedWorkspace?.current_user_role ?? 'viewer');
