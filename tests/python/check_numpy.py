import sys

try:
    import numpy as np
except Exception as exc:
    print("NumPy is required for python_bindings_smoke. Install in .venv: python -m pip install numpy", file=sys.stderr)
    raise

print(f"NumPy OK: {np.__version__}")
