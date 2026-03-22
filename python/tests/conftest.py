import warnings


warnings.filterwarnings(
    "ignore",
    message=r"The dash_table\.DataTable will be removed from the builtin dash components in a future major version\.",
    category=DeprecationWarning,
)
