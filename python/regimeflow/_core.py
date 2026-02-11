import importlib

_core = importlib.import_module("_core")

globals().update(_core.__dict__)

__all__ = getattr(_core, "__all__", [])
