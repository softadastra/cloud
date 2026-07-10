<script lang="ts">
  export let title: string;
  export let subtitle: string;
</script>

<section class="auth-panel">
  <div class="identity">
  <img
    class="logo"
    src="/brand/softadastra-cloud.svg"
    alt="Softadastra"
  />

  <div>
    <p class="eyebrow">Softadastra Cloud</p>
    <h1>{title}</h1>
  </div>
</div>

  <p class="subtitle">{subtitle}</p>

  <slot />
</section>
