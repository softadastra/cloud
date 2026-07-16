<script lang="ts">
  import { browser, dev } from '$app/environment';
  import { afterNavigate } from '$app/navigation';
  import { PUBLIC_GA_MEASUREMENT_ID } from '$env/static/public';

  const measurementId = PUBLIC_GA_MEASUREMENT_ID;
  const analyticsHost = 'cloud.softadastra.com';
  const scriptId = 'google-analytics-gtag';

  let initialized = false;
  let lastPageLocation = '';

  const shouldEnableAnalytics = () =>
    browser &&
    !dev &&
    measurementId.length > 0 &&
    window.location.hostname === analyticsHost;

  const ensureGoogleAnalytics = () => {
    if (!shouldEnableAnalytics()) {
      return false;
    }

    if (initialized) {
      return true;
    }

    window.dataLayer = window.dataLayer ?? [];
    window.gtag =
      window.gtag ??
      ((...args: Parameters<Window['gtag']>) => {
        window.dataLayer.push(args);
      });

    if (!document.getElementById(scriptId)) {
      const script = document.createElement('script');
      script.id = scriptId;
      script.async = true;
      script.src = `https://www.googletagmanager.com/gtag/js?id=${measurementId}`;
      document.head.appendChild(script);
    }

    window.gtag('js', new Date());
    window.gtag('config', measurementId, {
      send_page_view: false
    });

    initialized = true;
    return true;
  };

  const trackPageView = () => {
    if (!ensureGoogleAnalytics()) {
      return;
    }

    const pageLocation = window.location.href;

    if (pageLocation === lastPageLocation) {
      return;
    }

    lastPageLocation = pageLocation;

    window.gtag('config', measurementId, {
      page_path: window.location.pathname + window.location.search,
      page_location: pageLocation,
      page_title: document.title
    });
  };

  afterNavigate(() => {
    trackPageView();
  });
</script>
