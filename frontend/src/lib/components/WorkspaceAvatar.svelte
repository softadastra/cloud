<script lang="ts">
  import { browser } from '$app/environment';
  import type { Workspace } from '$lib/api/types';

  export let workspace: Workspace | null | undefined = null;
  export let size = 'md';
  export let previewUrl = '';

  const API_BASE_URL =
    import.meta.env.VITE_API_BASE_URL ??
    (browser ? `${window.location.protocol}//${window.location.hostname}:8080` : '');

  let failedUrl = '';

  $: initial = (workspace?.name || workspace?.slug || 'W').slice(0, 1).toUpperCase();
  $: rawUrl = previewUrl || workspace?.avatar_url || '';
  $: version = previewUrl ? 0 : workspace?.avatar_updated_at || 0;
  $: resolvedUrl = rawUrl
    ? rawUrl.startsWith('blob:') || rawUrl.startsWith('http')
      ? rawUrl
      : `${API_BASE_URL}${rawUrl}`
    : '';
  $: src = resolvedUrl && version ? `${resolvedUrl}?v=${version}` : resolvedUrl;
  $: showImage = Boolean(src && src !== failedUrl);
</script>

<span class:small={size === 'sm'} class:large={size === 'lg'} class="workspace-avatar">
  {#if showImage}
    <img
      src={src}
      alt=""
      aria-hidden="true"
      onerror={() => {
        failedUrl = src;
      }}
    />
  {:else}
    {initial}
  {/if}
</span>

<style>
  .workspace-avatar {
    display: grid;
    width: 40px;
    height: 40px;
    flex: 0 0 auto;
    place-items: center;
    overflow: hidden;
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    background: var(--bg-elevated);
    color: var(--text-soft);
    font-size: 14px;
    font-weight: 650;
  }

  .workspace-avatar.small {
    width: 30px;
    height: 30px;
    font-size: 11px;
  }

  .workspace-avatar.large {
    width: 92px;
    height: 92px;
    border-radius: 18px;
    font-size: 28px;
  }

  .workspace-avatar img {
    width: 100%;
    height: 100%;
    object-fit: cover;
  }
</style>
