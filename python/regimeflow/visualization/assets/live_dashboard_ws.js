(function () {
  const PROFILE_LIMITS = {
    balanced: {
      account_curve: 480,
      trades: 80,
      tester_journal: 120
    },
    low_latency: {
      account_curve: 240,
      trades: 48,
      tester_journal: 72
    },
    high_history: {
      account_curve: 1200,
      trades: 240,
      tester_journal: 360
    }
  };

  function normalizeProfile(value) {
    const key = String(value || "balanced").trim().toLowerCase().replace(/-/g, "_");
    if (key === "low_latency" || key === "high_history" || key === "balanced") {
      return key;
    }
    return "balanced";
  }

  function detectProfile() {
    if (window.__regimeflowLiveProfile) {
      return normalizeProfile(window.__regimeflowLiveProfile);
    }
    const profilePill = document.getElementById("live-profile");
    if (!profilePill || typeof profilePill.textContent !== "string") {
      return "balanced";
    }
    const text = profilePill.textContent.trim();
    const prefix = "profile=";
    if (!text.startsWith(prefix)) {
      return "balanced";
    }
    return normalizeProfile(text.slice(prefix.length));
  }

  function activeLimits() {
    return PROFILE_LIMITS[detectProfile()] || PROFILE_LIMITS.balanced;
  }

  function trimArray(values, limit) {
    if (!Array.isArray(values)) {
      return [];
    }
    if (!limit || values.length <= limit) {
      return values;
    }
    return values.slice(values.length - limit);
  }

  function mergePayload(current, message) {
    const limits = activeLimits();
    const envelope = message && typeof message === "object" ? message : {};
    const payload = envelope.payload && typeof envelope.payload === "object" ? envelope.payload : {};
    if (envelope.type === "snapshot") {
      if (Array.isArray(payload.account_curve)) {
        payload.account_curve = trimArray(payload.account_curve, limits.account_curve);
      }
      if (Array.isArray(payload.trades)) {
        payload.trades = trimArray(payload.trades, limits.trades);
      }
      if (Array.isArray(payload.tester_journal)) {
        payload.tester_journal = trimArray(payload.tester_journal, limits.tester_journal);
      }
      return payload;
    }
    if (envelope.type !== "delta") {
      return current;
    }

    const next = Object.assign({}, current || {});
    const append = payload.append && typeof payload.append === "object" ? payload.append : {};

    Object.keys(payload).forEach(function (key) {
      if (key !== "append") {
        next[key] = payload[key];
      }
    });

    next.account_curve = trimArray((next.account_curve || []).concat(append.account_curve || []), limits.account_curve);
    next.trades = trimArray((next.trades || []).concat(append.trades || []), limits.trades);
    next.tester_journal = trimArray((next.tester_journal || []).concat(append.tester_journal || []), limits.tester_journal);
    return next;
  }

  function connectLiveDashboardSocket() {
    if (!window.dash_clientside || !window.dash_clientside.set_props) {
      window.setTimeout(connectLiveDashboardSocket, 300);
      return;
    }

    const protocol = window.location.protocol === "https:" ? "wss" : "ws";
    const socketUrl = protocol + "://" + window.location.host + "/ws/live-dashboard";
    const socket = new WebSocket(socketUrl);
    let lastAnnouncedProfile = null;
    let profileTimer = null;

    function sendProfileUpdate(messageType) {
      if (socket.readyState !== WebSocket.OPEN) {
        return;
      }
      const profile = detectProfile();
      if (profile === lastAnnouncedProfile && messageType !== "hello") {
        return;
      }
      lastAnnouncedProfile = profile;
      try {
        socket.send(JSON.stringify({type: messageType, profile: profile}));
      } catch (error) {
        console.error("live-dashboard websocket profile send error", error);
      }
    }

    socket.onmessage = function (event) {
      try {
        const message = JSON.parse(event.data);
        const nextSnapshot = mergePayload(window.__regimeflowLiveSnapshot || {}, message);
        window.__regimeflowLiveSnapshot = nextSnapshot;
        window.dash_clientside.set_props("live-ws-message", {data: message});
        window.dash_clientside.set_props("live-ws-snapshot", {data: nextSnapshot});
      } catch (error) {
        console.error("live-dashboard websocket payload error", error);
      }
    };

    socket.onopen = function () {
      sendProfileUpdate("hello");
      profileTimer = window.setInterval(function () {
        sendProfileUpdate("client_profile");
      }, 1000);
    };

    socket.onclose = function () {
      if (profileTimer !== null) {
        window.clearInterval(profileTimer);
        profileTimer = null;
      }
      window.setTimeout(connectLiveDashboardSocket, 1000);
    };

    socket.onerror = function () {
      socket.close();
    };
  }

  if (window.location.pathname === "/") {
    if (document.readyState === "loading") {
      document.addEventListener("DOMContentLoaded", connectLiveDashboardSocket);
    } else {
      connectLiveDashboardSocket();
    }
  }
})();
