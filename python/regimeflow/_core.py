from __future__ import annotations

import importlib.machinery
import importlib.util
import os
import sys
from pathlib import Path

_DLL_DIR_HANDLES = []
_NATIVE_MODULE_NAME = f"{__package__}._native_core" if __package__ else "regimeflow._native_core"


def _add_dll_dir(path: Path) -> None:
    if not path:
        return
    if not path.exists() or not path.is_dir():
        return
    if hasattr(os, "add_dll_directory"):
        _DLL_DIR_HANDLES.append(os.add_dll_directory(str(path)))


def _configure_windows_dll_search() -> None:
    if sys.platform != "win32":
        return
    here = Path(__file__).resolve().parent
    _add_dll_dir(here)
    for entry in sys.path:
        try:
            candidate = Path(entry)
        except TypeError:
            continue
        if not candidate.exists():
            continue
        if any(candidate.glob("_core*.pyd")):
            _add_dll_dir(candidate)
    test_root = os.environ.get("REGIMEFLOW_TEST_ROOT")
    if test_root:
        root = Path(test_root)
        _add_dll_dir(root / "build" / "python")
        _add_dll_dir(root / "build" / "lib")
        _add_dll_dir(root / "build" / "bin")
        _add_dll_dir(root / "build")
        _add_dll_dir(root / "vcpkg_installed" / "x64-windows" / "bin")
        _add_dll_dir(root / "vcpkg_installed" / "x64-windows" / "debug" / "bin")
    extra = os.environ.get("REGIMEFLOW_DLL_DIRS")
    if extra:
        for entry in extra.split(os.pathsep):
            _add_dll_dir(Path(entry))


def _is_core_binary(path: Path) -> bool:
    if not path.exists() or not path.is_file():
        return False
    name = path.name
    return name.startswith("_core") and any(
        name.endswith(suffix) for suffix in importlib.machinery.EXTENSION_SUFFIXES
    )


def _contains_core_binary(path: Path) -> bool:
    if not path.exists() or not path.is_dir():
        return False
    return any(_is_core_binary(entry) for entry in path.iterdir())

def _find_core_binary() -> Path:
    here = Path(__file__).resolve().parent
    search_roots: list[Path] = [here]
    test_root = os.environ.get("REGIMEFLOW_TEST_ROOT")
    if test_root:
        root = Path(test_root)
        search_roots.extend(
            (
                root / "build" / "python",
                root / "build" / "lib",
                root / "build" / "python" / "Release",
                root / "build" / "python" / "Debug",
            )
        )

    for entry in sys.path:
        try:
            candidate = Path(entry)
        except TypeError:
            continue
        search_roots.append(candidate)
        search_roots.append(candidate / "regimeflow")

    for root in search_roots:
        if not root.exists() or not root.is_dir():
            continue
        for candidate in sorted(root.iterdir()):
            if _is_core_binary(candidate):
                return candidate

    raise ImportError("Failed to locate the compiled RegimeFlow _core extension")


def _import_core():
    module_path = _find_core_binary()
    existing = sys.modules.get(_NATIVE_MODULE_NAME)
    if existing is not None and Path(getattr(existing, "__file__", "")).resolve() == module_path.resolve():
        return existing

    spec = importlib.util.spec_from_file_location(_NATIVE_MODULE_NAME, module_path)
    if spec is None or spec.loader is None:
        raise ImportError(f"Failed to load native RegimeFlow module from {module_path}")

    module = importlib.util.module_from_spec(spec)
    sys.modules[_NATIVE_MODULE_NAME] = module
    spec.loader.exec_module(module)
    return module


_configure_windows_dll_search()

_core = _import_core()

globals().update(_core.__dict__)

__all__ = getattr(_core, "__all__", [])
