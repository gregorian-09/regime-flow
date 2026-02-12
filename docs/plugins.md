# Plugins

This section explains how plugins are discovered, loaded, and managed.

## Lifecycle Flow

```mermaid
flowchart LR
  A[Plugin Binary] --> B[PluginRegistry]
  B --> C{Load OK?}
  C -- No --> D[Error State]
  C -- Yes --> E[Loaded]
  E --> F{Start OK?}
  F -- No --> D
  F -- Yes --> G[Active]
  G --> H{Stop}
  H --> I[Stopped]
```


## What It Means

- **PluginRegistry** is the manager that finds and loads plugins.
- If a plugin fails to load, it goes into an **Error State** and is ignored.
- When started successfully, the plugin becomes **Active** and can provide functionality.
- Plugins can be stopped cleanly without restarting the system.


## Interpretation

Interpretation: plugins are loaded and transitioned through states; errors isolate faulty plugins.

