(() => {
  const MERMAID_CONFIG = {
    startOnLoad: false,
    securityLevel: "loose",
    theme: "base",
    flowchart: {
      useMaxWidth: true,
      htmlLabels: true,
      curve: "basis"
    },
    sequence: {
      useMaxWidth: true,
      wrap: true,
      actorMargin: 56,
      messageMargin: 28,
      noteMargin: 18,
      mirrorActors: false
    },
    class: {
      useMaxWidth: true
    },
    themeVariables: {
      fontFamily: "'IBM Plex Sans', 'Segoe UI', sans-serif",
      primaryColor: "#113247",
      primaryTextColor: "#f6f7f2",
      primaryBorderColor: "#f2c14e",
      secondaryColor: "#1d5c63",
      secondaryTextColor: "#f8fbf8",
      secondaryBorderColor: "#a7d8c9",
      tertiaryColor: "#f4efe6",
      tertiaryTextColor: "#173042",
      tertiaryBorderColor: "#d17b49",
      lineColor: "#4c6b8a",
      mainBkg: "#113247",
      secondBkg: "#1d5c63",
      tertiaryBkg: "#f4efe6",
      clusterBkg: "#f8fbfd",
      clusterBorder: "#97a9b7",
      edgeLabelBackground: "#f7f1dd",
      actorBkg: "#113247",
      actorBorder: "#f2c14e",
      actorTextColor: "#f6f7f2",
      actorLineColor: "#97a9b7",
      signalColor: "#173042",
      signalTextColor: "#173042",
      labelBoxBkgColor: "#f7f1dd",
      labelBoxBorderColor: "#d17b49",
      labelTextColor: "#173042",
      loopTextColor: "#173042",
      noteBkgColor: "#fff4d6",
      noteBorderColor: "#d17b49",
      noteTextColor: "#173042",
      activationBorderColor: "#d17b49",
      activationBkgColor: "#cfe5df",
      classText: "#173042",
      cScale0: "#113247",
      cScale1: "#1d5c63",
      cScale2: "#2f7f6f",
      cScale3: "#f2c14e",
      cScale4: "#d17b49",
      cScale5: "#f4efe6",
      cScaleLabel0: "#f6f7f2",
      cScaleLabel1: "#f6f7f2",
      cScaleLabel2: "#f6f7f2",
      cScaleLabel3: "#173042",
      cScaleLabel4: "#f6f7f2",
      cScaleLabel5: "#173042"
    }
  };

  let initialized = false;

  const renderMermaid = async () => {
    if (!window.mermaid) {
      return;
    }
    if (!initialized) {
      window.mermaid.initialize(MERMAID_CONFIG);
      initialized = true;
    }

    const nodes = document.querySelectorAll("pre.mermaid, div.mermaid");
    if (!nodes.length) {
      return;
    }

    for (const node of nodes) {
      if (node.tagName.toLowerCase() === "pre") {
        const replacement = document.createElement("div");
        replacement.className = "mermaid diagram-card";
        replacement.textContent = node.textContent || "";
        node.replaceWith(replacement);
      } else {
        node.classList.add("diagram-card");
      }
    }

    await window.mermaid.run({
      querySelector: "div.mermaid"
    });
  };

  document.addEventListener("DOMContentLoaded", () => {
    void renderMermaid();
  });

  if (window.document$ && typeof window.document$.subscribe === "function") {
    window.document$.subscribe(() => {
      void renderMermaid();
    });
  }
})();
