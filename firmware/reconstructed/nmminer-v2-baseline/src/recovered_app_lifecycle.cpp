#include "nmminer_recovered/official_v2002_st7789.h"

#include <array>

namespace nmminer::recovered::v2002_st7789 {
namespace {

constexpr RecoveredTaskRegistrationPlan kRecoveredTaskRegistrationPlans[] = {
    {"(button)", 0x400da765, 0x400d5078, 0x00000c00, 3,
     kTaskObjectSourceUnresolved, true, "hardware button task; peripheral-gated"},
    {"(scan)", 0x400da82e, 0x400dbe38, 0x00000a00, 4,
     kRuntimeScanControllerOffset, true, "scan task; parameter sourced from runtime+0x84"},
    {"(swarm)", 0x400da90a, 0x400dd35c, 0x00000c00, 5,
     kRuntimeSwarmControllerOffset, true, "swarm task; parameter sourced from runtime+0x88"},
    {"(stratum)", 0x400dad56, 0x400ef724, 0x00001800, 9,
     kTaskObjectSourceUnresolved, true, "stratum task; miner-service setup path"},
    {"(tMiner)", 0x400dadcd, 0x400e3f00, 0x00000800, 1,
     0x3ffc4130, true, "tMiner task after name/object setup"},
    {"(nMiner)", 0x400dae29, 0, 0, 0,
     0x3ffc4108, false, "nMiner name/object setup only; helper follows"},
    {"(nMiner)", 0x400dae8a, 0x400e3f00, 0x00000800, 2,
     0x3ffc4108, true, "nMiner task helper after split setup"},
};

bool hasStartupStep(uint32_t vm)
{
    for (const StartupStepSpec &step : startupSteps()) {
        if (step.vm == vm) {
            return true;
        }
    }
    return false;
}

bool hasStartupTarget(uint32_t target_vm)
{
    for (const StartupStepSpec &step : startupSteps()) {
        if (step.target_vm == target_vm) {
            return true;
        }
    }
    return false;
}

bool hasStartupLiteralValue(uint32_t value)
{
    for (const StartupLiteralSpec &literal : startupLiterals()) {
        if (literal.value == value) {
            return true;
        }
    }
    return false;
}

bool hasRuntimeSlot(uint32_t offset)
{
    for (const RuntimeServiceSlotSpec &slot : runtimeServiceSlots()) {
        if (slot.offset == offset) {
            return true;
        }
    }
    return false;
}

bool hasCreatedTask(const char *name, uint32_t entry_vm)
{
    const std::string_view wanted_name{name};
    for (const RecoveredTaskRegistrationPlan &task : kRecoveredTaskRegistrationPlans) {
        if (task.creates_task &&
            std::string_view{task.name} == wanted_name &&
            task.entry_vm == entry_vm) {
            return true;
        }
    }
    return false;
}

bool sourceGateSatisfied(const char *gate_name)
{
    const std::string_view wanted_name{gate_name};
    for (const SourceBaselineGateSpec &gate : sourceBaselineGates()) {
        if (std::string_view{gate.gate} == wanted_name) {
            return gate.satisfied;
        }
    }
    return false;
}

bool sourceGateOpen(const char *gate_name)
{
    const std::string_view wanted_name{gate_name};
    for (const SourceBaselineGateSpec &gate : sourceBaselineGates()) {
        if (std::string_view{gate.gate} == wanted_name) {
            return !gate.satisfied;
        }
    }
    return true;
}

const std::array<LifecycleValidationResult, 10> &lifecycleValidationStorage()
{
    static const std::array<LifecycleValidationResult, 10> results = {{
        {LifecycleValidationKind::Entrypoint,
         hasStartupStep(kOfficialAppEntryVm),
         kOfficialAppEntryVm,
         "official entrypoint",
         "startup table contains the ESP32 image entrypoint 0x40086aa4",
         "map downstream scheduler handoff into executable source"},
        {LifecycleValidationKind::BssRange,
         hasStartupLiteralValue(kOfficialBssClearStart) &&
             hasStartupLiteralValue(kOfficialBssClearEnd) &&
             hasStartupStep(0x40086acdu),
         0x40086acd,
         "BSS clear range",
         "startup literals encode 0x3ffc4100..0x3ffd6b30 and clear callsite 0x40086acd",
         "name exact IDF helper once framework symbols are resolved"},
        {LifecycleValidationKind::ConstructorRoute,
         hasStartupTarget(kOfficialConstructorPredicateVm) &&
             hasStartupTarget(kOfficialConstructorSweepVm),
         0x40086af2,
         "constructor gate/sweep",
         "startup route contains 0x40130cd0 predicate and 0x40086910 sweep",
         "recover constructor table semantics before executable parity claim"},
        {LifecycleValidationKind::SchedulerHandoff,
         hasStartupTarget(kOfficialSchedulerHandoffVm),
         0x40086c28,
         "scheduler handoff",
         "startup route reaches helper 0x4008766c with stack context",
         "map the downstream app task handoff before replacing Arduino setup loop"},
        {LifecycleValidationKind::BeginRoute,
         summarizeOfficialAppModel().has_begin_route &&
             summarizeOfficialAppModel().has_all_tasks_started_log,
         0x400de1e4,
         "begin route",
         "app lifecycle evidence reaches 0x400de1e4 and all-tasks-started log 0x400de25d",
         "turn begin-style route into source behavior, not only evidence tables"},
        {LifecycleValidationKind::RequiredTasks,
         hasCreatedTask("(scan)", 0x400dbe38) &&
             hasCreatedTask("(swarm)", 0x400dd35c) &&
             hasCreatedTask("(stratum)", 0x400ef724) &&
             hasCreatedTask("(tMiner)", 0x400e3f00) &&
             hasCreatedTask("(nMiner)", 0x400e3f00),
         0x400da82e,
         "required task registrations",
         "scan, swarm, stratum, tMiner, and nMiner task helper records are represented",
         "recover exact task object types and swarm float meanings"},
        {LifecycleValidationKind::RuntimeSlots,
         hasRuntimeSlot(kRuntimeSPriceMarketControllerOffset) &&
             hasRuntimeSlot(kRuntimeClockBtcTickerCacheOffset) &&
             hasRuntimeSlot(kRuntimeScanControllerOffset) &&
             hasRuntimeSlot(kRuntimeSwarmControllerOffset),
         0x400d57c0,
         "runtime service slots",
         "runtime slots +0x74/+0x80/+0x84/+0x88 are separated and named",
         "convert service slot model into runtime object source"},
        {LifecycleValidationKind::BehaviorParity,
         sourceGateSatisfied("official behavior parity"),
         0,
         "official behavior parity",
         "mining, Wi-Fi/config, touch, full UI behavior, and task execution remain open",
         "do not flash until behavior-equivalent baseline validation exists"},
        {LifecycleValidationKind::FreshBackupGate,
         sourceGateSatisfied("fresh-backup write gate"),
         0,
         "fresh backup write gate",
         "no fresh named 4 MB backup has been taken for a reconstructed write",
         "preserve a fresh full flash backup before any future write"},
        {LifecycleValidationKind::FlashGuard,
         sourceGateOpen("flash approval"),
         0,
         "flash guard locked",
         "guard_no_upload.py refuses upload/program/erase/uploadfs/uploadfsota by default",
         "keep locked until behavior parity and backup gates pass"},
    }};
    return results;
}

}  // namespace

std::string_view lifecycleValidationKindName(LifecycleValidationKind kind)
{
    switch (kind) {
    case LifecycleValidationKind::Entrypoint:
        return "entrypoint";
    case LifecycleValidationKind::BssRange:
        return "bss_range";
    case LifecycleValidationKind::ConstructorRoute:
        return "constructor_route";
    case LifecycleValidationKind::SchedulerHandoff:
        return "scheduler_handoff";
    case LifecycleValidationKind::BeginRoute:
        return "begin_route";
    case LifecycleValidationKind::RequiredTasks:
        return "required_tasks";
    case LifecycleValidationKind::RuntimeSlots:
        return "runtime_slots";
    case LifecycleValidationKind::BehaviorParity:
        return "behavior_parity";
    case LifecycleValidationKind::FreshBackupGate:
        return "fresh_backup_gate";
    case LifecycleValidationKind::FlashGuard:
        return "flash_guard";
    }
    return "unknown";
}

ArrayView<const RecoveredTaskRegistrationPlan> officialTaskRegistrationPlan()
{
    return {kRecoveredTaskRegistrationPlans, std::size(kRecoveredTaskRegistrationPlans)};
}

RecoveredRuntimeServices recoveredRuntimeServices()
{
    return RecoveredRuntimeServices{
        kRuntimeSPriceMarketControllerOffset,
        kRuntimeClockBtcTickerCacheOffset,
        kRuntimeScanControllerOffset,
        kRuntimeSwarmControllerOffset,
        hasRuntimeSlot(kRuntimeSPriceMarketControllerOffset),
        hasRuntimeSlot(kRuntimeClockBtcTickerCacheOffset),
        hasRuntimeSlot(kRuntimeScanControllerOffset),
        hasRuntimeSlot(kRuntimeSwarmControllerOffset),
    };
}

ArrayView<const LifecycleValidationResult> lifecycleValidationResults()
{
    const auto &results = lifecycleValidationStorage();
    return {results.data(), results.size()};
}

LifecycleValidationSummary summarizeLifecycleValidation()
{
    LifecycleValidationSummary summary{};
    summary.total = lifecycleValidationResults().size();
    bool begin_route_recovered = false;
    bool required_tasks_recovered = false;

    for (const LifecycleValidationResult &result : lifecycleValidationResults()) {
        if (result.passed) {
            ++summary.passed;
        }

        switch (result.kind) {
        case LifecycleValidationKind::Entrypoint:
        case LifecycleValidationKind::BssRange:
        case LifecycleValidationKind::ConstructorRoute:
        case LifecycleValidationKind::SchedulerHandoff:
            summary.startup_route_recovered =
                summary.startup_route_recovered || result.passed;
            break;
        case LifecycleValidationKind::RequiredTasks:
            required_tasks_recovered = result.passed;
            break;
        case LifecycleValidationKind::RuntimeSlots:
            summary.runtime_slots_recovered = result.passed;
            break;
        case LifecycleValidationKind::BehaviorParity:
            summary.behavior_parity_open = !result.passed;
            break;
        case LifecycleValidationKind::FreshBackupGate:
            summary.write_gate_open = !result.passed;
            break;
        case LifecycleValidationKind::FlashGuard:
            summary.flash_guard_locked = result.passed;
            break;
        case LifecycleValidationKind::BeginRoute:
            begin_route_recovered = result.passed;
            break;
        }
    }

    summary.startup_route_recovered =
        hasStartupStep(kOfficialAppEntryVm) &&
        hasStartupStep(0x40086acdu) &&
        hasStartupTarget(kOfficialConstructorSweepVm) &&
        hasStartupTarget(kOfficialSchedulerHandoffVm);
    summary.task_route_recovered = begin_route_recovered && required_tasks_recovered;
    summary.flashable = sourceBaselineReadyForFlash();
    return summary;
}

bool sourceBaselineReadyForFlash()
{
    return sourceGateSatisfied("official behavior parity") &&
           sourceGateSatisfied("fresh-backup write gate") &&
           sourceGateSatisfied("flash approval");
}

}  // namespace nmminer::recovered::v2002_st7789
