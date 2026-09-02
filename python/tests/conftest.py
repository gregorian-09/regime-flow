import warnings
from pathlib import Path


def _native_extension_available() -> bool:
    project_root = Path(__file__).resolve().parents[2]
    search_roots = [project_root / "python" / "regimeflow"]
    search_roots.extend(sorted(project_root.glob("build*/python")))
    return any(
        any(candidate.glob("_core*.*"))
        for candidate in search_roots
        if candidate.is_dir()
    )


# The Python suite exercises the native extension. A clean checkout can
# collect without a fragile environment variable or an import failure; build
# ``_core`` to collect and run the native tests.
if not _native_extension_available():
    collect_ignore_glob = ["test_*.py"]


warnings.filterwarnings(
    "ignore",
    message=r"The dash_table\.DataTable will be removed from the builtin dash components in a future major version\.",
    category=DeprecationWarning,
)
