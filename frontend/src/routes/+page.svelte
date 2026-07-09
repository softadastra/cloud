<script lang="ts">
  import { browser } from '$app/environment';
  import { auth } from '$lib/stores/auth';
  import { get } from 'svelte/store';

  if (browser) {
    window.location.replace(get(auth).session ? '/dashboard' : '/login');
  }
</script>
