import copy
import importlib.util
import sys
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location("endurance", Path(__file__).parents[1] / "scripts/check_miner_endurance.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
sys.path.insert(0, str(Path(__file__).parents[1] / "scripts"))
import sample_miner_api


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


class NativeCaptureParserTests(unittest.TestCase):
    def test_boot_restart_and_allocation_failure_are_recorded(self):
        state = module.native_state(1000)
        for second, line in enumerate(("panicminer 0.1.0 starting", "[POOL] wallet worker authorized by pool",
                "String(): No enought memory for the operation.", "abort() was called at PC 0x4000",
                "Rebooting...", "panicminer 0.1.0 starting")):
            module.record_native_line(state, line, second)
        self.assertEqual(len(state["boots"]), 2)
        self.assertGreaterEqual(len(state["faults"]), 4)
        self.assertEqual(len(state["events"]), 1)

    def test_rates_feed_sha_and_stack_are_classified(self):
        state = module.native_state(1000)
        for second, line in enumerate((">>> Completed 2 share(s), avg. hashrate 972.5 KH/s",
                "[CHAIN] 5/5 ok in 9000 ms", "[CHAIN] scratch used 0, peak 22220, SDK fallbacks 0",
                "[SHA] prefilter hits checked 100, mismatched 0, lock all, dropped 0",
                "[STACK] Monitor minimum free 2500 bytes")):
            module.record_native_line(state, line, second)
        self.assertEqual(state["rates"][0]["khs"], 972.5)
        self.assertEqual(len(state["feeds"]), 2)
        self.assertEqual(len(state["sha"]), 1)
        self.assertEqual(len(state["events"]), 1)
        self.assertFalse(state["faults"])

    def test_nonfinite_and_corrupt_rates_fail_without_throwing(self):
        state = module.native_state(1000)
        for value in ("nan", "inf", "corrupt"):
            module.record_native_line(state, f"avg. hashrate {value} KH/s", 1)
        self.assertFalse(state["rates"])
        self.assertGreaterEqual(len(state["faults"]), 3)


class ApiSamplerTests(unittest.TestCase):
    def test_selected_fields_exclude_wallet_values_and_settings(self):
        data = {"miner": {"hashrate10sKhs": 980, "hashrate60sKhs": 980, "totalMHashes": 100,
            "poolAuthorized": True, "heapFreeBytes": 25000, "chipTempC": 45,
            "sharesAccepted": 0, "sharesRejected": 0, "bestVerifiedDifficulty": 0, "poolDifficulty": 1},
            "sd": {"mounted": True, "rowsSaved": 2},
            "wallet": {"valid": True, "ageSeconds": 10, "balanceSats": 123456, "pendingSats": 654321},
            "settings": {"password": "synthetic-unused"}}
        point = sample_miner_api.status_point(data, 1000, .1)
        self.assertTrue(sample_miner_api.ready(point))
        self.assertNotIn(123456, point.values())
        self.assertNotIn(654321, point.values())
        self.assertNotIn("settings", point)

    def test_warmup_rejects_missing_data_bad_rates_and_disconnected_devices(self):
        good = {"rate10_khs": 980, "rate60_khs": 980, "pool_authorized": True,
                "sd_mounted": True, "wallet_valid": True, "wallet_age_seconds": 0}
        for key, value in (("rate10_khs", float("nan")), ("rate60_khs", 944.2),
                           ("pool_authorized", False), ("sd_mounted", False),
                           ("wallet_valid", False), ("wallet_age_seconds", -1), ("wallet_age_seconds", 181)):
            with self.subTest(key=key, value=value):
                self.assertFalse(sample_miner_api.ready({**good, key: value}))


if __name__ == "__main__":
    unittest.main()
