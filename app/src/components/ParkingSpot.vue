<script setup>
const props = defineProps({
  spot: {
    type: Object,
    required: true
  }
});

const dateFormatter = new Intl.DateTimeFormat("pt-BR", {
  dateStyle: "short",
  timeStyle: "medium"
});

function formatDate(value) {
  const date = new Date(value);

  if (Number.isNaN(date.getTime())) {
    return "Timestamp invalido";
  }

  return dateFormatter.format(date);
}
</script>

<template>
  <article
    class="rounded-2xl border p-4 transition-colors duration-300"
    :class="
      spot.ocupada
        ? 'border-red-500/60 bg-occupied-soft text-red-50'
        : 'border-green-500/60 bg-available-soft text-green-50'
    "
  >
    <div class="flex items-start justify-between gap-3">
      <div>
        <p class="text-xs font-medium uppercase tracking-[0.24em] text-white/60">
          Vaga
        </p>
        <h3 class="mt-2 text-3xl font-semibold tracking-tight">
          {{ spot.localId }}
        </h3>
      </div>

      <span
        class="rounded-full px-3 py-1 text-xs font-semibold uppercase tracking-[0.18em]"
        :class="
          spot.ocupada
            ? 'bg-red-200/15 text-red-100'
            : 'bg-green-200/15 text-green-100'
        "
      >
        {{ spot.ocupada ? "Ocupada" : "Livre" }}
      </span>
    </div>

    <dl class="mt-6 space-y-3 text-sm">
      <div class="flex items-center justify-between gap-4">
        <dt class="text-white/60">Ultima atualizacao</dt>
        <dd class="text-right font-medium">
          {{ formatDate(spot.lastUpdate) }}
        </dd>
      </div>

      <div class="flex items-center justify-between gap-4">
        <dt class="text-white/60">Dispositivo</dt>
        <dd class="text-right font-medium">
          {{ spot.deviceId }}
        </dd>
      </div>

      <div class="flex items-center justify-between gap-4">
        <dt class="text-white/60">Site</dt>
        <dd class="text-right font-medium">
          {{ spot.siteId }}
        </dd>
      </div>
    </dl>
  </article>
</template>
