import sys

try:
    import pytest  # noqa: F401
except Exception:
    print("pytest is required for python_native_tests. Install in .venv: python -m pip install pytest", file=sys.stderr)
    raise

print("pytest OK")
