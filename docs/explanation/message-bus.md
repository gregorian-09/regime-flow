# Message Bus Resiliency

RegimeFlow can broadcast live updates through message queues like Redis Streams or Kafka.
This helps scale live trading across multiple processes.

## MQ Resiliency Flow

```mermaid
flowchart TB
  A[Live Engine] --> B[MQ Adapter]
  B --> C{Connected?}
  C -- Yes --> D[Publish/Consume]
  C -- No --> E[Reconnect Backoff]
  E --> C
  D --> F[Decode Message]
  F --> G[Event Bus]
```


## What It Means

- If the queue is healthy, messages flow in real time.
- If it drops, the adapter retries with backoff (wait longer each time).
- When it reconnects, the stream resumes without restarting the engine.


## Interpretation

Interpretation: the adapter retries with exponential backoff and resumes once the queue is healthy.

