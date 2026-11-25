#include "nmminer_recovered/official_v2002_st7789.h"

#include <cassert>
#include <cstddef>

namespace rec = nmminer::recovered::v2002_st7789;

int main()
{
    static_assert(offsetof(rec::SwarmControllerRecoveredLayout, scan_controller_ptr) == 0x00);
    static_assert(offsetof(rec::SwarmControllerRecoveredLayout, unknown_0x04) == 0x04);
    static_assert(offsetof(rec::SwarmControllerRecoveredLayout, count_0x08) == 0x08);
    static_assert(offsetof(rec::SwarmControllerRecoveredLayout, metric_0x0c) == 0x0c);
    static_assert(offsetof(rec::SwarmControllerRecoveredLayout, metric_0x10) == 0x10);
    static_assert(offsetof(rec::SwarmControllerRecoveredLayout, metric_0x14) == 0x14);
    static_assert(offsetof(rec::SwarmControllerRecoveredLayout, stop_requested) == 0x50);
    static_assert(offsetof(rec::SwarmControllerRecoveredLayout, running) == 0x51);

    assert(rec::kSwarmTaskEntryVm == 0x400dd35c);
    assert(rec::kSwarmAggregateWriterVm == 0x400ddb86);
    assert(rec::kSwarmProbeCountWriterVm == 0x400ddb8e);
    assert(rec::kSwarmDisplayWriterVm == 0x400ddb90);
    assert(rec::kSwarmAuxiliaryWriterVm == 0x400ddb93);
    assert(rec::kSwarmProbeCountPublisherVm == 0x400d6ea5);
    assert(rec::kSwarmAggregatePublisherVm == 0x400d6ee2);
    assert(rec::kSwarmDisplayPublisherVm == 0x400d6f31);

    const rec::SwarmWriterLocalsRecovered locals{
        1.5f,
        2.25f,
        999.0f,
        17.5f,
        41u,
    };
    const rec::SwarmTelemetrySnapshotRecovered snapshot =
        rec::recoverSwarmTelemetrySnapshot(locals);
    assert(snapshot.count_0x08 == 42u);
    assert(snapshot.metric_0x0c == 3.75f);
    assert(snapshot.metric_0x10 == 999.0f);
    assert(snapshot.metric_0x14 == 17.5f);

    rec::SwarmControllerRecoveredLayout controller{};
    controller.scan_controller_ptr = 0x12345678u;
    rec::writeSwarmTelemetrySnapshot(controller, snapshot);
    assert(controller.scan_controller_ptr == 0x12345678u);
    assert(controller.count_0x08 == snapshot.count_0x08);
    assert(controller.metric_0x0c == snapshot.metric_0x0c);
    assert(controller.metric_0x10 == snapshot.metric_0x10);
    assert(controller.metric_0x14 == snapshot.metric_0x14);

    const rec::SwarmPublisherValuesRecovered published =
        rec::readSwarmPublisherValues(controller);
    assert(published.count_0x08 == 42u);
    assert(published.metric_0x0c == 3.75f);
    assert(published.metric_0x10 == 999.0f);
    assert(published.metric_0x0c_precision == 3);
    assert(published.metric_0x10_precision == 4);
    assert(!published.metric_0x14_has_direct_miner_publisher);
    assert(!published.exact_float_meanings_recovered);

    controller.metric_0x10 = rec::kSwarmMetricPrecisionThreshold;
    assert(rec::readSwarmPublisherValues(controller).metric_0x10_precision == 3);
    assert(rec::recoveredSwarmDisplayMetricPrecision(999.0f) == 4);
    assert(rec::recoveredSwarmDisplayMetricPrecision(1000.0f) == 3);

    const rec::SwarmBehaviorSummary summary =
        rec::summarizeSwarmBehaviorRecovery();
    assert(summary.writer_records == 4);
    assert(summary.publisher_records == 3);
    assert(summary.layout_offsets_recovered);
    assert(summary.writer_quartet_recovered);
    assert(summary.publisher_pairing_recovered);
    assert(summary.probe_count_recovered);
    assert(!summary.aggregate_float_meaning_recovered);
    assert(!summary.display_float_meaning_recovered);
    assert(!summary.auxiliary_direct_publisher_recovered);
    assert(summary.precision_threshold_recovered);

    return 0;
}
