import { browser } from '$app/environment';
import { derived, writable } from 'svelte/store';
import { listNotifications, markAllNotificationsRead, markNotificationRead, unreadNotificationCount } from '$lib/api/notifications';
import type { NotificationItem } from '$lib/api/types';

export type NotificationsState = {
  notifications: NotificationItem[];
  unreadCount: number;
  loading: boolean;
  error: string;
  realtimeConnected: boolean;
};

const initialState: NotificationsState = {
  notifications: [],
  unreadCount: 0,
  loading: false,
  error: '',
  realtimeConnected: false
};

function createNotificationsStore() {
  const store = writable<NotificationsState>(initialState);
  let socket: WebSocket | null = null;

  async function loadNotifications(filter: 'all' | 'unread' | 'read' = 'all') {
    store.update((state) => ({ ...state, loading: true, error: '' }));
    try {
      const [list, count] = await Promise.all([listNotifications({ filter, limit: 50 }), unreadNotificationCount()]);
      store.update((state) => ({ ...state, notifications: list.notifications, unreadCount: count.unread_count, loading: false }));
    } catch (error) {
      store.update((state) => ({ ...state, loading: false, error: error instanceof Error ? error.message : 'Unable to load notifications.' }));
    }
  }

  async function markRead(notificationId: string) {
    const updated = await markNotificationRead(notificationId);
    store.update((state) => ({
      ...state,
      notifications: state.notifications.map((item) => (item.id === updated.id ? updated : item)),
      unreadCount: Math.max(0, state.unreadCount - 1)
    }));
  }

  async function markAllRead() {
    const data = await markAllNotificationsRead();
    const now = Math.floor(Date.now() / 1000);
    store.update((state) => ({
      ...state,
      unreadCount: data.unread_count,
      notifications: state.notifications.map((item) => ({ ...item, read_at: item.read_at || now }))
    }));
  }

  function connectRealtime(sessionId?: string | null) {
    if (!browser || !sessionId || socket) {
      return;
    }

    try {
      const explicitUrl = import.meta.env.VITE_WS_BASE_URL as string | undefined;
      const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
      const host = import.meta.env.DEV ? `${window.location.hostname}:9090` : window.location.host;
      const url = explicitUrl || `${protocol}//${host}/ws`;
      socket = new WebSocket(url);
      socket.onopen = () => {
        socket?.send(JSON.stringify({ type: 'auth.session', payload: { session_id: sessionId } }));
        store.update((state) => ({ ...state, realtimeConnected: true }));
      };
      socket.onclose = () => {
        socket = null;
        store.update((state) => ({ ...state, realtimeConnected: false }));
      };
      socket.onerror = () => {
        socket?.close();
      };
      socket.onmessage = (event) => {
        try {
          const payload = JSON.parse(event.data) as { type?: string; data?: NotificationItem; payload?: Partial<NotificationItem> };
          const item = payload.data ?? payload.payload;
          if (payload.type === 'notification.created' && item) {
            const notification = { ...item, read_at: Number(item.read_at ?? 0), created_at: Number(item.created_at ?? 0) } as NotificationItem;
            store.update((state) => ({ ...state, notifications: [notification, ...state.notifications], unreadCount: state.unreadCount + 1 }));
          }
        } catch {
          // Ignore malformed realtime payloads; persisted API remains the source of truth.
        }
      };
    } catch {
      socket = null;
    }
  }

  function disconnectRealtime() {
    socket?.close();
    socket = null;
    store.update((state) => ({ ...state, realtimeConnected: false }));
  }

  function clear() {
    disconnectRealtime();
    store.set(initialState);
  }

  return {
    subscribe: store.subscribe,
    loadNotifications,
    markRead,
    markAllRead,
    connectRealtime,
    disconnectRealtime,
    clear
  };
}

export const notifications = createNotificationsStore();
export const unreadNotifications = derived(notifications, ($notifications) => $notifications.unreadCount);
