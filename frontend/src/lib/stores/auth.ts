import { browser } from '$app/environment';
import { writable } from 'svelte/store';
import { api } from '$lib/api/client';
import type { LoginData, Session, User } from '$lib/api/types';

const STORAGE_KEY = 'softadastra.cloud.auth';

export type AuthState = {
  user: User | null;
  session: Session | null;
};

const initialState: AuthState = {
  user: null,
  session: null
};

function loadState(): AuthState {
  if (!browser) {
    return initialState;
  }

  const raw = localStorage.getItem(STORAGE_KEY);

  if (!raw) {
    return initialState;
  }

  try {
    const parsed = JSON.parse(raw) as AuthState;
    api.setSession(parsed.session?.id ?? null);
    return parsed;
  } catch {
    localStorage.removeItem(STORAGE_KEY);
    return initialState;
  }
}

function createAuthStore() {
  const store = writable<AuthState>(loadState());

  function persist(state: AuthState) {
    api.setSession(state.session?.id ?? null);

    if (browser) {
      if (state.user && state.session) {
        localStorage.setItem(STORAGE_KEY, JSON.stringify(state));
      } else {
        localStorage.removeItem(STORAGE_KEY);
      }
    }
  }

  return {
    subscribe: store.subscribe,
    setSession(data: LoginData) {
      const state = { user: data.user, session: data.session };
      persist(state);
      store.set(state);
    },
    setUser(user: User) {
      store.update((state) => {
        const next = { ...state, user };
        persist(next);
        return next;
      });
    },
    clear() {
      persist(initialState);
      store.set(initialState);
    }
  };
}

export const auth = createAuthStore();
