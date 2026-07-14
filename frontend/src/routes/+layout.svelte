<script lang="ts">
  import { page } from '$app/stores';
  import AppShell from '$lib/components/AppShell.svelte';
  import '../styles.css';

  const siteUrl =
    import.meta.env.VITE_PUBLIC_SITE_URL ??
    'https://business.softadastra.com';

  const defaultTitle =
    'Softadastra Cloud | Shared workspaces for Vix projects';

  const defaultDescription =
    'Softadastra Cloud organizes shared workspaces, private Vix packages, lockfiles, build reports, permissions, and project activity for C++ teams.';

  const ogImage = siteUrl + '/og-logo.png';

  $: canonicalUrl = siteUrl + $page.url.pathname;

  $: structuredData = JSON.stringify({
    '@context': 'https://schema.org',
    '@graph': [
      {
        '@type': 'Organization',
        '@id': 'https://softadastra.com/#organization',
        name: 'Softadastra',
        url: 'https://softadastra.com/',
        logo: 'https://softadastra.com/logo.svg',
        sameAs: [
          'https://github.com/softadastra',
          'https://github.com/vixcpp/vix',
          'https://x.com/softadastra'
        ]
      },
      {
        '@type': 'WebSite',
        '@id': siteUrl + '/#website',
        name: 'Softadastra Cloud',
        url: siteUrl + '/',
        description: defaultDescription,
        publisher: {
          '@id': 'https://softadastra.com/#organization'
        }
      },
      {
        '@type': 'SoftwareApplication',
        '@id': siteUrl + '/#application',
        name: 'Softadastra Cloud',
        applicationCategory: 'DeveloperApplication',
        operatingSystem: 'Web',
        url: siteUrl + '/',
        description: defaultDescription,
        creator: {
          '@id': 'https://softadastra.com/#organization'
        }
      }
    ]
  });
</script>

<svelte:head>
  <meta
    name="keywords"
    content="Softadastra Cloud, Vix, Vix.cpp, C++, C++ tooling, private packages, lockfiles, build reports, workspace permissions, package registry"
  />

  <meta name="author" content="Softadastra" />

  <link rel="canonical" href={canonicalUrl} />

  <meta property="og:site_name" content="Softadastra Cloud" />
  <meta property="og:type" content="website" />
  <meta property="og:title" content={defaultTitle} />
  <meta property="og:description" content={defaultDescription} />
  <meta property="og:url" content={canonicalUrl} />
  <meta property="og:image" content={ogImage} />
  <meta property="og:image:secure_url" content={ogImage} />
  <meta property="og:image:type" content="image/png" />
  <meta
    property="og:image:alt"
    content="Softadastra Cloud, shared workspaces for Vix projects."
  />

  <meta name="twitter:card" content="summary_large_image" />
  <meta name="twitter:site" content="@softadastra" />
  <meta name="twitter:title" content={defaultTitle} />
  <meta name="twitter:description" content={defaultDescription} />
  <meta name="twitter:image" content={ogImage} />

  <script type="application/ld+json">
    {structuredData}
  </script>
</svelte:head>

<AppShell>
  {#key $page.url.pathname + $page.url.search}
    <slot />
  {/key}
</AppShell>
