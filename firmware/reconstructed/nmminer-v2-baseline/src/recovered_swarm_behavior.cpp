#include "nmminer_recovered/official_v2002_st7789.h"

namespace nmminer::recovered::v2002_st7789 {
namespace {

bool hasRuntimeEvidence(MinerRuntimeEvidenceKind kind,
                        uint32_t vm,
                        uint32_t source_offset)
{
    for (const MinerRuntimeEvidenceSpec &evidence : minerRuntimeEvidence()) {
        if (evidence.kind == kind &&
            evidence.vm == vm &&
            evidence.source_offset == source_offset) {
            return true;
        }
    }

    return false;
}

bool hasSwarmPublisher(uint32_t vm,
                       uint32_t model_record_offset,
                       uint32_t screen_slot_offset)
{
    for (const MinerRuntimeEvidenceSpec &evidence : minerRuntimeEvidence()) {
        if (evidence.kind == MinerRuntimeEvidenceKind::TaskObjectPublisher &&
            evidence.vm == vm &&
            evidence.source_offset == kRuntimeSwarmControllerOffset &&
            evidence.model_record_offset == model_record_offset &&
            evidence.bound_screen_slot_offset == screen_slot_offset) {
            return true;
        }
    }

    return false;
}

size_t countSwarmWriterRecords()
{
    size_t count = 0;
    for (const MinerRuntimeEvidenceSpec &evidence : minerRuntimeEvidence()) {
        if (evidence.kind == MinerRuntimeEvidenceKind::WriterCallsite &&
            (evidence.vm == kSwarmAggregateWriterVm ||
             evidence.vm == kSwarmProbeCountWriterVm ||
             evidence.vm == kSwarmDisplayWriterVm ||
             evidence.vm == kSwarmAuxiliaryWriterVm)) {
            ++count;
        }
    }

    return count;
}

size_t countSwarmPublisherRecords()
{
    size_t count = 0;
    for (const MinerRuntimeEvidenceSpec &evidence : minerRuntimeEvidence()) {
        if (evidence.kind == MinerRuntimeEvidenceKind::TaskObjectPublisher &&
            evidence.source_offset == kRuntimeSwarmControllerOffset &&
            (evidence.vm == kSwarmProbeCountPublisherVm ||
             evidence.vm == kSwarmAggregatePublisherVm ||
             evidence.vm == kSwarmDisplayPublisherVm)) {
            ++count;
        }
    }

    return count;
}

}  // namespace

uint8_t recoveredSwarmDisplayMetricPrecision(float value)
{
    return value < kSwarmMetricPrecisionThreshold
               ? kSwarmMetricPrecisionBelowThreshold
               : kSwarmMetricPrecisionAtOrAboveThreshold;
}

SwarmTelemetrySnapshotRecovered recoverSwarmTelemetrySnapshot(
    const SwarmWriterLocalsRecovered &locals)
{
    return SwarmTelemetrySnapshotRecovered{
        locals.previous_count_0x08 + 1u,
        locals.metric_0x0c_component_a + locals.metric_0x0c_component_b,
        locals.metric_0x10,
        locals.metric_0x14,
    };
}

void writeSwarmTelemetrySnapshot(
    SwarmControllerRecoveredLayout &controller,
    const SwarmTelemetrySnapshotRecovered &snapshot)
{
    controller.count_0x08 = snapshot.count_0x08;
    controller.metric_0x0c = snapshot.metric_0x0c;
    controller.metric_0x10 = snapshot.metric_0x10;
    controller.metric_0x14 = snapshot.metric_0x14;
}

SwarmPublisherValuesRecovered readSwarmPublisherValues(
    const SwarmControllerRecoveredLayout &controller)
{
    return SwarmPublisherValuesRecovered{
        controller.count_0x08,
        controller.metric_0x0c,
        controller.metric_0x10,
        kSwarmAggregateMetricPublisherPrecision,
        recoveredSwarmDisplayMetricPrecision(controller.metric_0x10),
        false,
        false,
    };
}

SwarmBehaviorSummary summarizeSwarmBehaviorRecovery()
{
    const bool writer_quartet =
        hasRuntimeEvidence(MinerRuntimeEvidenceKind::WriterCallsite,
                           kSwarmAggregateWriterVm,
                           kSwarmControllerAggregateMetricOffset) &&
        hasRuntimeEvidence(MinerRuntimeEvidenceKind::WriterCallsite,
                           kSwarmProbeCountWriterVm,
                           kSwarmControllerProbeCountOffset) &&
        hasRuntimeEvidence(MinerRuntimeEvidenceKind::WriterCallsite,
                           kSwarmDisplayWriterVm,
                           kSwarmControllerDisplayMetricOffset) &&
        hasRuntimeEvidence(MinerRuntimeEvidenceKind::WriterCallsite,
                           kSwarmAuxiliaryWriterVm,
                           kSwarmControllerAuxMetricOffset);
    const bool publisher_pairing =
        hasSwarmPublisher(kSwarmProbeCountPublisherVm, 0x33c, 0x58) &&
        hasSwarmPublisher(kSwarmAggregatePublisherVm, 0x310, 0x54) &&
        hasSwarmPublisher(kSwarmDisplayPublisherVm, 0x2e4, 0x50);
    const bool precision_threshold =
        recoveredSwarmDisplayMetricPrecision(999.0f) ==
            kSwarmMetricPrecisionBelowThreshold &&
        recoveredSwarmDisplayMetricPrecision(kSwarmMetricPrecisionThreshold) ==
            kSwarmMetricPrecisionAtOrAboveThreshold;

    return SwarmBehaviorSummary{
        countSwarmWriterRecords(),
        countSwarmPublisherRecords(),
        offsetof(SwarmControllerRecoveredLayout, count_0x08) ==
                kSwarmControllerProbeCountOffset &&
            offsetof(SwarmControllerRecoveredLayout, metric_0x0c) ==
                kSwarmControllerAggregateMetricOffset &&
            offsetof(SwarmControllerRecoveredLayout, metric_0x10) ==
                kSwarmControllerDisplayMetricOffset &&
            offsetof(SwarmControllerRecoveredLayout, metric_0x14) ==
                kSwarmControllerAuxMetricOffset &&
            offsetof(SwarmControllerRecoveredLayout, stop_requested) ==
                kSwarmControllerStopFlagOffset &&
            offsetof(SwarmControllerRecoveredLayout, running) ==
                kSwarmControllerRunningFlagOffset,
        writer_quartet,
        publisher_pairing,
        hasRuntimeEvidence(MinerRuntimeEvidenceKind::WriterCallsite,
                           kSwarmProbeCountWriterVm,
                           kSwarmControllerProbeCountOffset) &&
            hasSwarmPublisher(kSwarmProbeCountPublisherVm, 0x33c, 0x58),
        false,
        false,
        false,
        precision_threshold,
    };
}

}  // namespace nmminer::recovered::v2002_st7789
