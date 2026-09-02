from __future__ import annotations

import importlib.util
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


def _is_core_binary(path: Path) -> bool:
    if not path.exists() or not path.is_file():
        return False
    name = path.name
    return name.startswith("_core") and any(name.endswith(suffix) for suffix in (".so", ".pyd"))


def _find_core_binary() -> Path:
    here = Path(__file__).resolve().parent
    search_roots: list[Path] = [here]
    project_root = here.parents[1]
    # A source checkout may use any CMake build directory (for example
    # ``build-gcc`` or ``build-asan``). Installed packages find the extension
    # beside this file and do not rely on this developer convenience.
    search_roots.extend(sorted(project_root.glob("build*/python")))
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
        search_roots.extend(sorted(root.glob("build*/python")))

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
    for module_name in (f"{__package__}._core", "_core"):
        existing = sys.modules.get(module_name)
        if existing is not None and Path(getattr(existing, "__file__", "")).resolve() == module_path.resolve():
            return existing

    spec = importlib.util.spec_from_file_location(f"{__package__}._core", module_path)
    if spec is None or spec.loader is None:
        raise ImportError(f"Failed to load native RegimeFlow module from {module_path}")

    module = importlib.util.module_from_spec(spec)
    sys.modules[f"{__package__}._core"] = module
    sys.modules["_core"] = module
    spec.loader.exec_module(module)
    return module


_configure_windows_dll_search()

_core = _import_core()


# Re-export the extension's public surface explicitly. This preserves normal
# module introspection and ``from regimeflow._bootstrap import Name`` without
# copying private import metadata from the native module into this wrapper.
__all__ = [name for name in dir(_core) if not name.startswith("_")]
for _export_name in __all__:
    globals()[_export_name] = getattr(_core, _export_name)
del _export_name


def __getattr__(name: str):
    """Forward extension attributes that are not part of the stable public surface."""
    try:
        return getattr(_core, name)
    except AttributeError as exc:
        raise AttributeError(f"module {__name__!r} has no attribute {name!r}") from exc


def __dir__() -> list[str]:
    return sorted(set(globals()) | set(dir(_core)))
