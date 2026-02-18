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


_configure_windows_dll_search()

_core = importlib.import_module("_core")

globals().update(_core.__dict__)

__all__ = getattr(_core, "__all__", [])
