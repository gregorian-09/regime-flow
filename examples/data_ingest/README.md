# Data Ingest

CSV ingestion and normalization example. Converts raw CSV into validated bars.

## Files

- `config.yaml` - ingest config
- `input/` - raw CSV
- `output/` - normalized output

## Run

```bash
./build/bin/regimeflow_ingest --config examples/data_ingest/config.yaml
```
