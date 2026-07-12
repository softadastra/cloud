import { onMount } from 'svelte';
import { get, writable } from 'svelte/store';

export type PageRefreshReason = 'manual' | 'navigation';

export type PageRefreshState = {
  revision: number;
  reason: PageRefreshReason;
};

const initialState: PageRefreshState = {
  revision: 0,
  reason: 'navigation'
};

export const pageRefresh = writable<PageRefreshState>(initialState);

export function requestPageRefresh(reason: PageRefreshReason = 'manual') {
  pageRefresh.update((state) => ({
    revision: state.revision + 1,
    reason
  }));
}

export function usePageRefresh(load: () => void | Promise<void>) {
  onMount(() => {
    let seenRevision = get(pageRefresh).revision;

    const unsubscribe = pageRefresh.subscribe((state) => {
      if (state.revision === seenRevision) {
        return;
      }

      seenRevision = state.revision;
      void load();
    });

    void load();

    return unsubscribe;
  });
}
