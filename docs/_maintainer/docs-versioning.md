# Docs Versioning

RegimeFlow docs are versioned using `mike` with MkDocs Material.

## Install

```bash
pip install mike mkdocs-material
```

## Build A Version

```bash
mike deploy --push --update-aliases 1.0 latest
```

This publishes a `1.0` version and updates the `latest` alias.

## Set Default Version

```bash
mike set-default --push latest
```

## Local Preview

```bash
mike serve
```

## Notes

- Ensure `extra.version.provider: mike` is set in `mkdocs.yml`.
- Use semantic version tags aligned with releases.
