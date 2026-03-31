from __future__ import annotations

import importlib
import os
import sys
from pathlib import Path

_DLL_DIR_HANDLES = []


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


def _contains_core_binary(path: Path) -> bool:
    if not path.exists() or not path.is_dir():
        return False
    return any(path.glob("_core*.so")) or any(path.glob("_core*.pyd"))


def _prefer_core_path() -> Path | None:
    test_root = os.environ.get("REGIMEFLOW_TEST_ROOT")
    if test_root:
        root = Path(test_root)
        for candidate in (root / "build" / "python", root / "build" / "lib"):
            if _contains_core_binary(candidate):
                return candidate

    for entry in sys.path:
        try:
            candidate = Path(entry)
        except TypeError:
            continue
        if _contains_core_binary(candidate):
            return candidate
    return None


def _import_core():
    preferred = _prefer_core_path()
    if preferred is not None:
        preferred_str = str(preferred)
        if preferred_str in sys.path:
            sys.path.remove(preferred_str)
        sys.path.insert(0, preferred_str)

        existing = sys.modules.get("_core")
        existing_file = getattr(existing, "__file__", "")
        if existing is not None and existing_file:
            try:
                if not Path(existing_file).resolve().is_relative_to(preferred.resolve()):
                    sys.modules.pop("_core", None)
            except Exception:
                if preferred_str not in existing_file:
                    sys.modules.pop("_core", None)

    return importlib.import_module("_core")


_configure_windows_dll_search()

_core = _import_core()

globals().update(_core.__dict__)

__all__ = getattr(_core, "__all__", [])
