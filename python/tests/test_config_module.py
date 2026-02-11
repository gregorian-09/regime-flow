import os

import regimeflow as rf


def test_load_config_file():
    cfg_path = os.path.join(os.path.dirname(__file__), "..", "..", "tests", "fixtures",
                            "config_hmm_ensemble.yaml")
    cfg = rf.load_config(cfg_path)
    assert cfg.has("engine")
    assert cfg.get_path("engine.initial_capital") == 100000


def test_config_set_get_roundtrip():
    cfg = rf.Config()
    cfg.set("alpha", 1)
    cfg.set_path("nested.value", 2.5)
    assert cfg.get("alpha") == 1
    assert cfg.get_path("nested.value") == 2.5
