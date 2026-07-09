<script lang="ts">
  export let title: string;
  export let subtitle: string;
</script>

<section class="auth-panel">
  <div class="identity">
    <div class="logo">S</div>
    <div>
      <p>Softadastra Cloud</p>
      <h1>{title}</h1>
    </div>
  </div>

  <p class="subtitle">{subtitle}</p>
  <slot />
</section>
