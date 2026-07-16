type GtagCommand = 'config' | 'event' | 'js' | 'set';

type Gtag = (
  command: GtagCommand,
  targetIdOrDate: string | Date,
  config?: Record<string, unknown>
) => void;

declare global {
  interface Window {
    dataLayer: unknown[];
    gtag: Gtag;
  }
}

export {};
