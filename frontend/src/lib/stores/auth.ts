import { browser } from '$app/environment';
import { writable } from 'svelte/store';
import { api } from '$lib/api/client';
import { me } from '$lib/api/auth';
import { ApiError } from '$lib/api/types';
import type { LoginData, Session, User } from '$lib/api/types';

const STORAGE_KEY = 'softadastra.cloud.auth';

export type AuthState = {
  user: User | null;
  session: Session | null;
  sessionExpired: boolean;
  authError: string;
};

const initialState: AuthState = {
  user: null,
  session: null,
  sessionExpired: false,
  authError: ''
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
    const parsed = JSON.parse(raw) as Partial<AuthState>;
    const state: AuthState = {
      user: parsed.user ?? null,
      session: parsed.session ?? null,
      sessionExpired: false,
      authError: ''
    };
    api.setSession(state.session?.id ?? null);
    return state;
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
        localStorage.setItem(
          STORAGE_KEY,
          JSON.stringify({ user: state.user, session: state.session })
        );
      } else {
        localStorage.removeItem(STORAGE_KEY);
      }
    }
  }

  return {
    subscribe: store.subscribe,
    setSession(data: LoginData) {
      const state = { user: data.user, session: data.session, sessionExpired: false, authError: '' };
      persist(state);
      store.set(state);
    },
    async refreshCurrentUser() {
      let current: AuthState = initialState;

      store.update((state) => {
        current = state;
        return state;
      });

      if (!current.session?.id) {
        return 'missing' as const;
      }

      try {
        const data = await me(current.session.id);
        const next = { user: data.user, session: data.session, sessionExpired: false, authError: '' };
        persist(next);
        store.set(next);
        return 'ok' as const;
      } catch (error) {
        if (
          error instanceof ApiError &&
          error.status === 401 &&
          error.code !== 'network_error'
        ) {
          const next = {
            ...initialState,
            sessionExpired: true,
            authError: error.message || 'Session expired. Please log in again.'
          };
          persist(next);
          store.set(next);
          return 'invalid' as const;
        }

        return 'unreachable' as const;
      }
    },
    async ensureAuthLoaded() {
      let current: AuthState = initialState;

      store.update((state) => {
        current = state;
        return state;
      });

      if (!current.session?.id) {
        return 'missing' as const;
      }

      if (current.user && !current.sessionExpired) {
        return 'ready' as const;
      }

      return this.refreshCurrentUser();
    },
    setUser(user: User) {
      store.update((state) => {
        const next = { ...state, user, sessionExpired: false, authError: '' };
        persist(next);
        return next;
      });
    },
    clearAuthError() {
      store.update((state) => {
        const next = { ...state, sessionExpired: false, authError: '' };
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
