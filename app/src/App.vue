<script setup>
import { computed, onBeforeUnmount, onMounted, ref } from "vue";

import ParkingSpot from "./components/ParkingSpot.vue";

const WS_URL = import.meta.env.VITE_WS_URL || "ws://localhost:3001/ws";
const INITIAL_RETRY_DELAY_MS = 1000;
const MAX_RETRY_DELAY_MS = 10000;

const connectionStatus = ref("connecting");
const lastMessageAt = ref(null);
const retryAttempt = ref(0);
const socketRef = ref(null);
const reconnectTimerRef = ref(null);
const spotsById = ref({});

const summary = computed(() => {
  const spots = Object.values(spotsById.value);
  const occupied = spots.filter((spot) => spot.ocupada).length;

  return {
    total: spots.length,
    occupied,
    available: spots.length - occupied
  };
});

const groupedSpots = computed(() => {
  const groupMap = new Map();

  Object.values(spotsById.value).forEach((spot) => {
    const groupKey = `${spot.siteId}::${spot.deviceId}`;

    if (!groupMap.has(groupKey)) {
      groupMap.set(groupKey, {
        key: groupKey,
        siteId: spot.siteId,
        deviceId: spot.deviceId,
        spots: []
      });
    }

    groupMap.get(groupKey).spots.push(spot);
  });

  return Array.from(groupMap.values())
    .map((group) => ({
      ...group,
      spots: group.spots.sort((left, right) =>
        left.localId.localeCompare(right.localId, "pt-BR", {
          numeric: true,
          sensitivity: "base"
        })
      )
    }))
    .sort((left, right) => {
      const siteComparison = left.siteId.localeCompare(right.siteId, "pt-BR", {
        numeric: true,
        sensitivity: "base"
      });

      if (siteComparison !== 0) {
        return siteComparison;
      }

      return left.deviceId.localeCompare(right.deviceId, "pt-BR", {
        numeric: true,
        sensitivity: "base"
      });
    });
});

const connectionBadgeClass = computed(() => {
  if (connectionStatus.value === "connected") {
    return "border-emerald-400/40 bg-emerald-500/10 text-emerald-200";
  }

  if (connectionStatus.value === "reconnecting") {
    return "border-amber-400/40 bg-amber-500/10 text-amber-200";
  }

  return "border-rose-400/40 bg-rose-500/10 text-rose-200";
});

const connectionLabel = computed(() => {
  if (connectionStatus.value === "connected") {
    return "Conectado";
  }

  if (connectionStatus.value === "reconnecting") {
    return "Reconectando";
  }

  return "Desconectado";
});

function replaceAllSpots(spots) {
  const nextMap = {};

  spots.forEach((spot) => {
    if (spot?.vagaId) {
      nextMap[spot.vagaId] = spot;
    }
  });

  spotsById.value = nextMap;
}

function upsertSpots(spots) {
  const nextMap = { ...spotsById.value };

  spots.forEach((spot) => {
    if (spot?.vagaId) {
      nextMap[spot.vagaId] = spot;
    }
  });

  spotsById.value = nextMap;
}

function clearReconnectTimer() {
  if (reconnectTimerRef.value) {
    window.clearTimeout(reconnectTimerRef.value);
    reconnectTimerRef.value = null;
  }
}

function closeSocket() {
  if (!socketRef.value) {
    return;
  }

  socketRef.value.onopen = null;
  socketRef.value.onmessage = null;
  socketRef.value.onerror = null;
  socketRef.value.onclose = null;
  socketRef.value.close();
  socketRef.value = null;
}

function scheduleReconnect() {
  clearReconnectTimer();
  closeSocket();

  connectionStatus.value = "reconnecting";
  retryAttempt.value += 1;

  const delay = Math.min(
    INITIAL_RETRY_DELAY_MS * 2 ** Math.max(retryAttempt.value - 1, 0),
    MAX_RETRY_DELAY_MS
  );

  reconnectTimerRef.value = window.setTimeout(() => {
    connect();
  }, delay);
}

function handleMessage(event) {
  try {
    const message = JSON.parse(event.data);
    const spots = Array.isArray(message?.data?.spots) ? message.data.spots : [];

    if (message.type === "parking:spots:bootstrap") {
      replaceAllSpots(spots);
      lastMessageAt.value = message.data?.receivedAt || new Date().toISOString();
      return;
    }

    if (message.type === "parking:spots:snapshot") {
      upsertSpots(spots);
      lastMessageAt.value = message.data?.receivedAt || new Date().toISOString();
    }
  } catch (error) {
    console.warn("Falha ao processar mensagem WebSocket", error);
  }
}

function connect() {
  clearReconnectTimer();
  closeSocket();

  connectionStatus.value = retryAttempt.value > 0 ? "reconnecting" : "connecting";

  const socket = new WebSocket(WS_URL);
  socketRef.value = socket;

  socket.onopen = () => {
    connectionStatus.value = "connected";
    retryAttempt.value = 0;
  };

  socket.onmessage = handleMessage;

  socket.onerror = () => {
    if (socketRef.value === socket && connectionStatus.value === "connected") {
      connectionStatus.value = "disconnected";
    }
  };

  socket.onclose = () => {
    if (socketRef.value !== socket) {
      return;
    }

    connectionStatus.value = "disconnected";
    scheduleReconnect();
  };
}

const lastMessageLabel = computed(() => {
  if (!lastMessageAt.value) {
    return "Nenhuma atualizacao recebida";
  }

  const date = new Date(lastMessageAt.value);
  if (Number.isNaN(date.getTime())) {
    return "Timestamp invalido";
  }

  return new Intl.DateTimeFormat("pt-BR", {
    dateStyle: "short",
    timeStyle: "medium"
  }).format(date);
});

onMounted(() => {
  connect();
});

onBeforeUnmount(() => {
  clearReconnectTimer();
  closeSocket();
});
</script>

<template>
  <main class="min-h-screen px-4 py-6 sm:px-6 lg:px-8">
    <div class="mx-auto max-w-7xl">
      <section
        class="overflow-hidden rounded-[2rem] border border-white/10 bg-slate-900/75 shadow-panel backdrop-blur"
      >
        <div class="border-b border-white/10 px-5 py-6 sm:px-8">
          <div
            class="flex flex-col gap-5 lg:flex-row lg:items-end lg:justify-between"
          >
            <div class="max-w-2xl">
              <p class="text-xs font-semibold uppercase tracking-[0.28em] text-cyan-300">
                Smart Parking
              </p>
              <h1 class="mt-3 text-3xl font-semibold tracking-tight text-white sm:text-4xl">
                Dashboard de vagas em tempo real
              </h1>
              <p class="mt-3 text-sm leading-6 text-slate-300 sm:text-base">
                Visualizacao reativa do estado de ocupacao recebido pelo bridge
                Node.js/ws. O painel sincroniza via WebSocket e agrupa vagas por
                site e dispositivo.
              </p>
            </div>

            <div class="flex flex-col items-start gap-3 sm:flex-row sm:flex-wrap">
              <div
                class="rounded-2xl border border-white/10 bg-slate-950/60 px-4 py-3"
              >
                <p class="text-xs uppercase tracking-[0.2em] text-slate-500">
                  Endpoint WS
                </p>
                <p class="mt-2 text-sm font-medium text-slate-100">
                  {{ WS_URL }}
                </p>
              </div>

              <div
                class="rounded-full border px-4 py-3 text-sm font-semibold uppercase tracking-[0.2em]"
                :class="connectionBadgeClass"
              >
                {{ connectionLabel }}
              </div>
            </div>
          </div>

          <div class="mt-6 grid gap-4 sm:grid-cols-2 xl:grid-cols-4">
            <div class="rounded-2xl border border-white/10 bg-white/5 px-4 py-4">
              <p class="text-xs uppercase tracking-[0.22em] text-slate-400">
                Total de vagas
              </p>
              <p class="mt-3 text-3xl font-semibold text-white">
                {{ summary.total }}
              </p>
            </div>

            <div class="rounded-2xl border border-red-500/20 bg-red-500/10 px-4 py-4">
              <p class="text-xs uppercase tracking-[0.22em] text-red-200/80">
                Ocupadas
              </p>
              <p class="mt-3 text-3xl font-semibold text-red-100">
                {{ summary.occupied }}
              </p>
            </div>

            <div class="rounded-2xl border border-green-500/20 bg-green-500/10 px-4 py-4">
              <p class="text-xs uppercase tracking-[0.22em] text-green-200/80">
                Livres
              </p>
              <p class="mt-3 text-3xl font-semibold text-green-100">
                {{ summary.available }}
              </p>
            </div>

            <div class="rounded-2xl border border-white/10 bg-white/5 px-4 py-4">
              <p class="text-xs uppercase tracking-[0.22em] text-slate-400">
                Ultima sincronizacao
              </p>
              <p class="mt-3 text-sm font-medium text-slate-100">
                {{ lastMessageLabel }}
              </p>
            </div>
          </div>
        </div>

        <div class="px-5 py-6 sm:px-8">
          <div
            v-if="groupedSpots.length === 0"
            class="rounded-[1.75rem] border border-dashed border-white/15 bg-slate-950/50 px-6 py-12 text-center"
          >
            <p class="text-lg font-medium text-white">
              Nenhuma vaga disponivel no momento
            </p>
            <p class="mt-3 text-sm leading-6 text-slate-400">
              Aguarde o evento de bootstrap ou um novo snapshot do servidor
              WebSocket para popular o painel.
            </p>
          </div>

          <div v-else class="space-y-8">
            <section
              v-for="group in groupedSpots"
              :key="group.key"
              class="space-y-4"
            >
              <div class="flex flex-col gap-2 sm:flex-row sm:items-end sm:justify-between">
                <div>
                  <p class="text-xs font-semibold uppercase tracking-[0.24em] text-cyan-300">
                    {{ group.siteId }}
                  </p>
                  <h2 class="mt-2 text-2xl font-semibold tracking-tight text-white">
                    {{ group.deviceId }}
                  </h2>
                </div>

                <p class="text-sm text-slate-400">
                  {{ group.spots.length }} vaga(s) monitorada(s)
                </p>
              </div>

              <div class="grid gap-4 md:grid-cols-2 xl:grid-cols-3">
                <ParkingSpot
                  v-for="spot in group.spots"
                  :key="spot.vagaId"
                  :spot="spot"
                />
              </div>
            </section>
          </div>
        </div>
      </section>
    </div>
  </main>
</template>
