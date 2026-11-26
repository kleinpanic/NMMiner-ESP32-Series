import copy
import importlib.util
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location("endurance", Path(__file__).parents[1] / "scripts/check_miner_endurance.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class EnduranceEvidenceTests(unittest.TestCase):
    def setUp(self):
        self.report = {"samples": [{"seconds": i * 5, "epoch": 1000 + i * 5, "total_mhashes": 100 + i * 5 * .98,
            "rate10_khs": 980, "rate60_khs": 980, "pool_authorized": True, "sd_mounted": True,
            "sd_rows_saved": i // 12, "wallet_valid": True, "wallet_age_seconds": i * 5 % 120}
            for i in range(241)], "api_errors": []}
        self.native = {"started_epoch": 900, "elapsed_seconds": 1301, "faults": [],
            "feeds": [{"line": "[CHAIN] 5/5 ok"}] * 10,
            "sha": [{"seconds": 1295, "line": "[SHA] prefilter hits checked 100, mismatched 0, lock all, dropped 0"}]}
        self.serial = "panicminer 0.1.0 starting\n"

    def result(self):
        return module.evaluate(self.report, self.native, self.serial)

    def test_complete_evidence_passes(self):
        self.assertEqual(self.result()["status"], "PASS")
        self.assertAlmostEqual(self.result()["work_khs"], 980)

    def test_abort_and_second_boot_cannot_hide_behind_empty_fault_array(self):
        self.serial += "abort() was called at PC 0x4000\nRebooting...\npanicminer 0.1.0 starting\n"
        result = self.result()
        self.assertEqual(result["status"], "FAIL")
        self.assertFalse(result["checks"]["no_native_faults"])
        self.assertFalse(result["checks"]["one_observed_boot"])

    def test_allocation_failure_and_nan_fail(self):
        for fault in ("String(): No enought memory for the operation.", "String(): No enough memory for the operation.", "avg. hashrate nan KH/s", "SSL - Memory allocation failed"):
            with self.subTest(fault=fault):
                self.serial = "panicminer 0.1.0 starting\n" + fault
                self.assertFalse(self.result()["checks"]["no_native_faults"])

    def test_good_average_does_not_hide_bad_rolling_rate(self):
        self.report["samples"][40]["rate10_khs"] = 944.2
        self.assertEqual(self.result()["status"], "FAIL")

    def test_short_run_and_sampling_gap_fail(self):
        self.report["samples"].pop()
        self.assertFalse(self.result()["checks"]["full_20_minutes"])
        del self.report["samples"][10:20]
        self.assertFalse(self.result()["checks"]["continuous_sampling"])

    def test_counter_reset_and_false_reported_rate_fail(self):
        self.report["samples"][-1]["total_mhashes"] = 50
        result = self.result()
        self.assertFalse(result["checks"]["counters_never_reset"])
        self.assertFalse(result["checks"]["independent_work_mean_in_band"])

    def test_disconnected_pool_stale_wallet_and_unmounted_card_fail(self):
        for key, value in (("pool_authorized", False), ("sd_mounted", False), ("wallet_age_seconds", 181)):
            with self.subTest(key=key):
                candidate = copy.deepcopy(self.report)
                candidate["samples"][100][key] = value
                self.assertEqual(module.evaluate(candidate, self.native, self.serial)["status"], "FAIL")

    def test_partial_feed_or_sha_drop_fail(self):
        self.native["feeds"].append({"line": "[CHAIN] 4/5 ok"})
        self.assertFalse(self.result()["checks"]["all_feed_cycles_complete"])
        self.native["sha"][0]["line"] = "[SHA] prefilter hits checked 100, mismatched 0, lock all, dropped 1"
        self.assertFalse(self.result()["checks"]["sha_checks_clean"])

    def test_stale_serial_monitor_and_wallet_without_refresh_fail(self):
        self.native["elapsed_seconds"] = 100
        self.assertFalse(self.result()["checks"]["native_covers_run"])
        for point in self.report["samples"]:
            point["wallet_age_seconds"] = 0
        self.assertFalse(self.result()["checks"]["wallet_refreshes"])


if __name__ == "__main__":
    unittest.main()
