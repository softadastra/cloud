<script lang="ts">
  import { browser, dev } from '$app/environment';
  import { afterNavigate } from '$app/navigation';

  const measurementId = 'G-E765GE5YQD';
  const analyticsHost = 'cloud.softadastra.com';

  let lastPageLocation = '';
  let initialNavigationTracked = false;

  const shouldEnableAnalytics = () =>
    browser &&
    !dev &&
    window.location.hostname === analyticsHost &&
    typeof window.gtag === 'function';

  const trackPageView = () => {
    if (!shouldEnableAnalytics()) {
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
    if (!initialNavigationTracked) {
      initialNavigationTracked = true;
      lastPageLocation = browser ? window.location.href : '';
      return;
    }

    trackPageView();
  });
</script>
