#!/usr/bin/env python3
"""Compatibility wrapper for the renamed crypto sample downloader."""
from __future__ import annotations

import warnings

from download_intraday_crypto_sample import main


if __name__ == "__main__":
    warnings.warn(
        "tools/download_intraday_spx_sample.py is deprecated; use "
        "tools/download_intraday_crypto_sample.py instead.",
        DeprecationWarning,
        stacklevel=2,
    )
    main()
