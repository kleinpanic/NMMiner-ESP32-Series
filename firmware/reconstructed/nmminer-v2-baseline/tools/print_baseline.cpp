#include "nmminer_recovered/official_v2002_st7789.h"

#include <array>
#include <cinttypes>
#include <cstdio>

namespace rec = nmminer::recovered::v2002_st7789;

int main()
{
    const rec::OfficialAppModelSummary summary = rec::summarizeOfficialAppModel();
    const rec::LifecycleValidationSummary lifecycle = rec::summarizeLifecycleValidation();

    std::printf("NMMiner recovered baseline: %.*s %.*s\n",
                static_cast<int>(rec::kFirmwareVersion.size()),
                rec::kFirmwareVersion.data(),
                static_cast<int>(rec::kTarget.size()),
                rec::kTarget.data());

    for (uint32_t address : {0x40086aa4u, 0x40086910u, 0x4008693cu,
                             0x400869e0u, 0x40086b0eu, 0x40130cd0u,
                             0x40167080u, 0x4015ebe0u,
                             0x400de1e4u, 0x400de25du, 0x400da489u,
                             0x400da749u, 0x400da76bu, 0x400da82eu,
                             0x400da90au, 0x400dad56u, 0x400dadcdu,
                             0x400dae29u, 0x400dae8au,
                             0x400ddf61u, 0x400db278u, 0x400e3a80u,
                             0x400d73beu, 0x400f54e4u, 0x400e798cu,
                             0x400e7b21u, 0x400f1607u, 0x400d6ad1u,
                             0x400d621bu, 0x400d621du, 0x400de20bu,
                             0x400da778u, 0x400da894u, 0x400da90au,
                             0x400dd35cu, 0x400ddb86u, 0x400ddb8eu,
                             0x400ddb90u, 0x400ddb93u, 0x400f1a17u,
                             0x3f4008ebu}) {
        const auto mapped = rec::mapVmAddress(address);
        if (!mapped) {
            std::printf("addr 0x%08" PRIx32 ": unmapped\n", address);
            continue;
        }
        std::printf("addr 0x%08" PRIx32 ": %.*s app=0x%" PRIx32 " flash=0x%" PRIx32 "\n",
                    address,
                    static_cast<int>(rec::segmentName(mapped->segment).size()),
                    rec::segmentName(mapped->segment).data(),
                    mapped->app_offset,
                    mapped->flash_offset);
    }

    std::array<size_t, 4> widget_counts = {};
    for (const rec::WidgetSpec &widget : rec::widgetSpecs()) {
        widget_counts[static_cast<size_t>(widget.screen)]++;
    }

    std::array<size_t, 4> publisher_counts = {};
    for (const rec::PublisherSpec &publisher : rec::publisherSpecs()) {
        publisher_counts[static_cast<size_t>(publisher.screen)]++;
    }

    for (rec::ScreenId screen : {rec::ScreenId::Miner, rec::ScreenId::Clock,
                                 rec::ScreenId::SPrice, rec::ScreenId::Weather}) {
        const size_t idx = static_cast<size_t>(screen);
        std::printf("%.*s: widgets=%zu publishers=%zu\n",
                    static_cast<int>(rec::screenName(screen).size()),
                    rec::screenName(screen).data(),
                    widget_counts[idx],
                    publisher_counts[idx]);
    }

    std::printf("SPrice bindings=%zu Weather forecast templates=%zu Weather callbacks=%zu\n",
                rec::bindingSpecs().size(),
                rec::weatherForecastTemplateSpecs().size(),
                rec::weatherCallbackSpecs().size());
    std::printf("App lifecycle evidence=%zu\n",
                rec::appLifecycleEvidence().size());
    std::printf("Startup literals=%zu startup steps=%zu bss=0x%" PRIx32 "..0x%" PRIx32 "\n",
                rec::startupLiterals().size(),
                rec::startupSteps().size(),
                rec::kOfficialBssClearStart,
                rec::kOfficialBssClearEnd);
    for (const rec::StartupStepSpec &step : rec::startupSteps()) {
        if (step.vm == rec::kOfficialAppEntryVm ||
            step.vm == 0x40086acdu ||
            step.vm == 0x40086af2u ||
            step.vm == 0x40086c28u) {
            std::printf("Startup #%u 0x%08" PRIx32 " kind=%.*s target=0x%" PRIx32
                        " confidence=%s role=%s\n",
                        static_cast<unsigned>(step.order),
                        step.vm,
                        static_cast<int>(rec::startupRouteKindName(step.kind).size()),
                        rec::startupRouteKindName(step.kind).data(),
                        step.target_vm,
                        step.confidence,
                        step.role);
        }
    }
    for (const rec::AppLifecycleEvidenceSpec &evidence : rec::appLifecycleEvidence()) {
        if (std::string_view{evidence.area} == "task" ||
            evidence.vm == rec::kOfficialAppEntryVm ||
            evidence.vm == 0x400de25du) {
            std::printf("Lifecycle %s 0x%08" PRIx32 " app=0x%" PRIx32
                        " literal=0x%" PRIx32 " drom=0x%" PRIx32 " role=%s\n",
                        evidence.area,
                        evidence.vm,
                        evidence.app_offset,
                        evidence.literal_vm,
                        evidence.drom_string_vm,
                        evidence.role);
        }
    }
    std::printf("Official model bringup=%zu runtime_slots=%zu gates=%zu/%zu flash_gate=%s\n",
                summary.bringup_steps,
                summary.runtime_slots,
                summary.closed_gates,
                summary.total_gates,
                summary.flash_gate_closed ? "locked" : "open");
    std::printf("Lifecycle validation=%zu/%zu startup=%u tasks=%u runtime=%u parity_open=%u write_gate_open=%u flashable=%u\n",
                lifecycle.passed,
                lifecycle.total,
                lifecycle.startup_route_recovered ? 1u : 0u,
                lifecycle.task_route_recovered ? 1u : 0u,
                lifecycle.runtime_slots_recovered ? 1u : 0u,
                lifecycle.behavior_parity_open ? 1u : 0u,
                lifecycle.write_gate_open ? 1u : 0u,
                lifecycle.flashable ? 1u : 0u);
    for (const rec::LifecycleValidationResult &result : rec::lifecycleValidationResults()) {
        std::printf("Lifecycle check %-20s %s vm=0x%08" PRIx32
                    " label=%s next=%s\n",
                    rec::lifecycleValidationKindName(result.kind).data(),
                    result.passed ? "pass" : "open",
                    result.evidence_vm,
                    result.label,
                    result.next_gate);
    }
    std::printf("Required task coverage scan=%u swarm=%u stratum=%u miners=%u begin=%u ready_log=%u\n",
                summary.has_scan_task ? 1u : 0u,
                summary.has_swarm_task ? 1u : 0u,
                summary.has_stratum_task ? 1u : 0u,
                summary.has_miner_tasks ? 1u : 0u,
                summary.has_begin_route ? 1u : 0u,
                summary.has_all_tasks_started_log ? 1u : 0u);
    for (const rec::OfficialBringupStepSpec &step : rec::officialBringupSteps()) {
        if (step.kind == rec::BringupStepKind::BeginRoute ||
            step.kind == rec::BringupStepKind::TaskRegistration ||
            step.kind == rec::BringupStepKind::ReadyLog) {
            std::printf("Bringup #%u 0x%08" PRIx32 " kind=%.*s label=%s target=0x%" PRIx32
                        " status=%s\n",
                        static_cast<unsigned>(step.order),
                        step.vm,
                        static_cast<int>(rec::bringupStepKindName(step.kind).size()),
                        rec::bringupStepKindName(step.kind).data(),
                        step.label,
                        step.target_vm,
                        step.status);
        }
    }
    for (const rec::RuntimeServiceSlotSpec &slot : rec::runtimeServiceSlots()) {
        std::printf("Runtime slot +0x%02" PRIx32 " %s install=0x%08" PRIx32
                    " status=%s\n",
                    slot.offset,
                    slot.name,
                    slot.install_vm,
                    slot.status);
    }
    for (const rec::RecoveredTaskRegistrationPlan &task : rec::officialTaskRegistrationPlan()) {
        std::printf("Task plan %s call=0x%08" PRIx32 " entry=0x%08" PRIx32
                    " stack=0x%" PRIx32 " priority=%" PRIu32
                    " object=0x%" PRIx32 " creates=%u status=%s\n",
                    task.name,
                    task.call_vm,
                    task.entry_vm,
                    task.stack_bytes,
                    task.priority,
                    task.object_source,
                    task.creates_task ? 1u : 0u,
                    task.status);
    }
    for (const rec::SourceBaselineGateSpec &gate : rec::sourceBaselineGates()) {
        std::printf("Gate %-36s %s -- %s\n",
                    gate.gate,
                    gate.satisfied ? "closed" : "open",
                    gate.evidence);
    }
    std::printf("Task registration evidence=%zu\n",
                rec::taskRegistrationEvidence().size());
    for (const rec::TaskRegistrationEvidenceSpec &task : rec::taskRegistrationEvidence()) {
        std::printf("Task %s kind=%.*s call=0x%08" PRIx32 " helper=0x%08" PRIx32
                    " name_lit=0x%" PRIx32 " drom=0x%" PRIx32
                    " entry=0x%" PRIx32 " stack=0x%" PRIx32
                    " priority=%" PRIu32 " object=0x%" PRIx32
                    " confidence=%s\n",
                    task.name,
                    static_cast<int>(rec::taskRouteKindName(task.kind).size()),
                    rec::taskRouteKindName(task.kind).data(),
                    task.call_vm,
                    task.helper_vm,
                    task.name_literal_vm,
                    task.name_drom,
                    task.entry_vm,
                    task.stack_or_const,
                    task.priority,
                    task.object_source,
                    task.confidence);
    }
    std::printf("Miner runtime evidence=%zu accepted=0x%" PRIx32 " rejected=0x%" PRIx32
                " block_hits=0x%" PRIx32 " notify_seq=0x%" PRIx32
                " combined_hashrate=0x%" PRIx32 "/0x%" PRIx32 "\n",
                rec::minerRuntimeEvidence().size(),
                rec::kMinerAcceptedShareCountOffset,
                rec::kMinerRejectedShareCountOffset,
                rec::kMinerBlockHitCountOffset,
                rec::kMinerNotifySequenceCountOffset,
                rec::kMinerCombinedHashrateLowOffset,
                rec::kMinerCombinedHashrateHighOffset);
    for (const rec::MinerRuntimeEvidenceSpec &evidence : rec::minerRuntimeEvidence()) {
        if (evidence.vm == 0x400f1607u ||
            evidence.vm == 0x400d69f9u ||
            evidence.vm == 0x400d6ad1u ||
            evidence.vm == 0x400d6d55u ||
            evidence.vm == 0x400d6e1au ||
            evidence.vm == 0x400d6ea5u ||
            evidence.vm == 0x400de20bu ||
            evidence.vm == 0x400da894u ||
            evidence.vm == 0x400da90au ||
            evidence.vm == 0x400ddb86u ||
            evidence.vm == 0x400ddb8eu ||
            evidence.vm == 0x400ddb90u ||
            evidence.vm == 0x400f1258u ||
            evidence.vm == 0x400f1a17u ||
            evidence.vm == 0x400f1bc2u) {
            std::printf("Miner runtime 0x%08" PRIx32 " kind=%.*s source=0x%" PRIx32
                        " model=0x%" PRIx32 " slot=0x%" PRIx32 " confidence=%s role=%s\n",
                        evidence.vm,
                        static_cast<int>(rec::minerRuntimeEvidenceKindName(evidence.kind).size()),
                        rec::minerRuntimeEvidenceKindName(evidence.kind).data(),
                        evidence.source_offset,
                        evidence.model_record_offset,
                        evidence.bound_screen_slot_offset,
                        evidence.confidence,
                        evidence.role);
        }
    }
    std::printf("Market config keys=%zu WatchCoins evidence records=%zu row-producer evidence records=%zu\n",
                rec::marketConfigSpecs().size(),
                rec::watchCoinsConfigEvidence().size(),
                rec::spriceRowProducerEvidence().size());
    std::printf("Clock route evidence=%zu runtime_sprice=0x%" PRIx32 " runtime_btc=0x%" PRIx32
                " runtime_swarm=0x%" PRIx32 " btc_price_field=0x%" PRIx32
                "/0x%" PRIx32 " static_color=0x%" PRIx32 "\n",
                rec::clockRouteEvidence().size(),
                rec::kRuntimeSPriceMarketControllerOffset,
                rec::kRuntimeClockBtcTickerCacheOffset,
                rec::kRuntimeSwarmControllerOffset,
                rec::kClockBtcTickerPriceLowOffset,
                rec::kClockBtcTickerPriceHighOffset,
                rec::kClockBtcStaticColorRgb565);
    for (const rec::ClockRouteEvidenceSpec &evidence : rec::clockRouteEvidence()) {
        if (evidence.vm == 0x400d82a1u ||
            evidence.vm == 0x400d836eu ||
            evidence.vm == 0x400d8409u) {
            std::printf("Clock route 0x%08" PRIx32 " kind=%.*s source=0x%" PRIx32
                        " target=0x%" PRIx32 " status=%s\n",
                        evidence.vm,
                        static_cast<int>(rec::clockRouteKindName(evidence.kind).size()),
                        rec::clockRouteKindName(evidence.kind).data(),
                        evidence.source_offset,
                        evidence.target_offset,
                        evidence.status);
        }
    }
    std::printf("SPrice patch target records=%zu\n",
                rec::spricePatchTargetSpecs().size());
    std::printf("LittleFS images=%zu screensaver asset records=%zu official_fs_offset=0x%" PRIx32 "\n",
                rec::littleFsImageSpecs().size(),
                rec::screensaverAssetSpecs().size(),
                rec::kOfficialLittleFsFlashOffset);
    for (const rec::LittleFsImageSpec &image : rec::littleFsImageSpecs()) {
        std::printf("LittleFS %s count=%zu size=0x%" PRIx32 " block=%" PRIu32 "\n",
                    image.image_path,
                    image.screensaver_count,
                    image.length,
                    image.block_size);
    }
    for (const rec::MarketConfigSpec &spec : rec::marketConfigSpecs()) {
        if (std::string_view{spec.key} == "WatchCoins") {
            std::printf("WatchCoins default=%s key_drom=0x%" PRIx32 " default_drom=0x%" PRIx32 "\n",
                        spec.default_value,
                        spec.key_drom,
                        spec.default_drom);
        }
    }
    const rec::SPriceBehaviorSummary sprice_summary =
        rec::summarizeSPriceBehaviorRecovery();
    const rec::SwarmBehaviorSummary swarm_summary =
        rec::summarizeSwarmBehaviorRecovery();
    const rec::WatchCoinsParseResult default_watchcoins =
        rec::parseWatchCoinsList(rec::defaultWatchCoinsList());
    const rec::WatchCoinsParseResult cap_watchcoins =
        rec::parseWatchCoinsList(
            std::string_view{"A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V"});
    std::printf("SPrice behavior default_tokens=%zu default_truncate=%zu"
                " cap_tokens=%zu cap_truncate=%zu cap_trailing=%u"
                " adapted_rows=%zu ordered_rows=%zu render_limit=%zu"
                " watchcoins_recovered=%u row_adapter=%u row_order_key=%u"
                " row_trim=%u row_ordering_exact=%u\n",
                sprice_summary.default_watchcoins,
                sprice_summary.default_truncate_at,
                sprice_summary.sample_cap_count,
                sprice_summary.sample_truncate_at,
                cap_watchcoins.had_trailing_tokens ? 1u : 0u,
                sprice_summary.adapted_rows,
                sprice_summary.ordered_rows,
                sprice_summary.rendered_row_limit,
                sprice_summary.watchcoins_cap_recovered ? 1u : 0u,
                sprice_summary.row_adapter_recovered ? 1u : 0u,
                sprice_summary.row_ordering_key_recovered ? 1u : 0u,
                sprice_summary.row_trim_recovered ? 1u : 0u,
                sprice_summary.row_ordering_recovered ? 1u : 0u);
    std::printf("Swarm behavior writer_records=%zu publisher_records=%zu"
                " layout=%u writer_quartet=%u publisher_pairing=%u"
                " count_recovered=%u metric0c_meaning=%u metric10_meaning=%u"
                " metric14_direct_publisher=%u precision_threshold=%u\n",
                swarm_summary.writer_records,
                swarm_summary.publisher_records,
                swarm_summary.layout_offsets_recovered ? 1u : 0u,
                swarm_summary.writer_quartet_recovered ? 1u : 0u,
                swarm_summary.publisher_pairing_recovered ? 1u : 0u,
                swarm_summary.probe_count_recovered ? 1u : 0u,
                swarm_summary.aggregate_float_meaning_recovered ? 1u : 0u,
                swarm_summary.display_float_meaning_recovered ? 1u : 0u,
                swarm_summary.auxiliary_direct_publisher_recovered ? 1u : 0u,
                swarm_summary.precision_threshold_recovered ? 1u : 0u);
    std::printf("WatchCoins default token list:");
    for (size_t i = 0; i < default_watchcoins.token_count; ++i) {
        std::printf(" %s", default_watchcoins.tokens[i].symbol);
    }
    std::printf("\n");
    std::printf("WatchCoins cap sample token20=%s token_count=%zu truncate_at=%zu\n",
                cap_watchcoins.tokens[rec::kWatchCoinsMaxSymbols - 1].symbol,
                cap_watchcoins.token_count,
                cap_watchcoins.truncate_at);
    for (const rec::SPricePatchTargetSpec &target : rec::spricePatchTargetSpecs()) {
        if (std::string_view{target.target_id} == "BIN02-LIT-SPRICE-WATCHCOINS-DEFAULT") {
            std::printf("Patch target %s app=0x%" PRIx32 " status=%s\n",
                        target.target_id,
                        target.app_offset,
                        target.status);
        }
    }

    std::array<rec::SPriceMarketRow, rec::kSPriceMaxRows> cache = {};
    const rec::SPriceTickerRecord ticker_records[] = {
        {"BTC", 105000.0f, 1.25f},
        {"XEC", 0.00002f, -0.70f},
    };
    std::array<rec::SPriceMarketRow, rec::kSPriceMaxRows> source_rows = {};
    const size_t adapted = rec::adaptSPriceTickerRecordsToRows(
        rec::ArrayView<const rec::SPriceTickerRecord>{ticker_records, 2},
        source_rows);
    std::array<rec::SPriceMarketRow, rec::kSPriceRenderedRowLimit> rendered_rows = {};
    const size_t rendered = rec::orderAndTrimSPriceRowsForRecoveredRender(
        rec::ArrayView<const rec::SPriceMarketRow>{source_rows.data(), adapted},
        rendered_rows);
    const size_t copied = rec::copySPriceRowsFromRuntimeVector(
        rec::ArrayView<const rec::SPriceMarketRow>{source_rows.data(), adapted},
        cache);
    std::printf("SPrice row adapter rows=%zu writer copied rows=%zu"
                " rendered rows=%zu stride=%zu source_offset=0x%" PRIx32 "\n",
                adapted,
                copied,
                rendered,
                sizeof(rec::SPriceMarketRow),
                rec::kSPriceRuntimeSourceOffset);

    return 0;
}
