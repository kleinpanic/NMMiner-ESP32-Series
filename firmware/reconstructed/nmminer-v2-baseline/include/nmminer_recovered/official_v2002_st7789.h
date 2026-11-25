#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace nmminer::recovered::v2002_st7789 {

template <typename T>
class ArrayView {
public:
    constexpr ArrayView() = default;
    constexpr ArrayView(const T *data, size_t size) : data_(data), size_(size) {}

    constexpr const T *begin() const { return data_; }
    constexpr const T *end() const { return data_ == nullptr ? nullptr : data_ + size_; }
    constexpr const T *data() const { return data_; }
    constexpr size_t size() const { return size_; }
    constexpr bool empty() const { return size_ == 0; }
    constexpr const T &operator[](size_t index) const { return data_[index]; }

private:
    const T *data_ = nullptr;
    size_t size_ = 0;
};

constexpr std::string_view kFirmwareVersion = "v2.0.02";
constexpr std::string_view kTarget = "esp32-2432s028r-st7789";
constexpr std::string_view kEspIdf = "v4.4.6-dirty";

constexpr uint32_t kAppFlashBase = 0x10000;
constexpr uint32_t kFactoryPartitionSize = 0x380000;
constexpr uint32_t kOfficialLittleFsFlashOffset = 0x390000;
constexpr uint32_t kOfficialLittleFsLength = 0x60000;
constexpr uint32_t kOfficialLittleFsBlockSize = 4096;
constexpr uint32_t kOfficialLittleFsBlockCount = 96;
constexpr uint32_t kOfficialAppEntryVm = 0x40086aa4;
constexpr uint32_t kOfficialAppEntryAppOffset = 0x2970e8;
constexpr uint32_t kOfficialAppEntryFlashOffset = 0x2a70e8;
constexpr uint32_t kOfficialBssClearStart = 0x3ffc4100;
constexpr uint32_t kOfficialBssClearEnd = 0x3ffd6b30;
constexpr uint32_t kOfficialSecondaryClearStart = 0x50000200;
constexpr uint32_t kOfficialSecondaryClearEnd = 0x50000200;
constexpr uint32_t kOfficialConstructorPredicateVm = 0x40130cd0;
constexpr uint32_t kOfficialConstructorSweepVm = 0x40086910;
constexpr uint32_t kOfficialSchedulerHandoffVm = 0x4008766c;

enum class SegmentName : uint8_t {
    Drom,
    Dram,
    Iram0,
    Irom,
    Iram1,
};

struct Segment {
    SegmentName name;
    uint32_t load_address;
    uint32_t length;
    uint32_t segment_header_offset;
    uint32_t payload_offset;
};

struct MappedAddress {
    SegmentName segment;
    uint32_t vaddr;
    uint32_t relative;
    uint32_t app_offset;
    uint32_t flash_offset;
    bool in_factory_partition;
};

constexpr std::array<Segment, 5> kSegments = {{
    {SegmentName::Drom, 0x3f400020, 0x157de8, 0x000018, 0x000020},
    {SegmentName::Dram, 0x3ffbdb60, 0x0065a0, 0x157e08, 0x157e10},
    {SegmentName::Iram0, 0x40080000, 0x001c60, 0x15e3b0, 0x15e3b8},
    {SegmentName::Irom, 0x400d0020, 0x13227c, 0x160018, 0x160020},
    {SegmentName::Iram1, 0x40081c60, 0x017fe4, 0x29229c, 0x2922a4},
}};

std::optional<MappedAddress> mapVmAddress(uint32_t vaddr);
std::string_view segmentName(SegmentName segment);

enum class ScreenId : uint8_t {
    Miner,
    Clock,
    SPrice,
    Weather,
};

enum class WidgetKind : uint8_t {
    Label,
    DrawArea,
    Container,
    Line,
    Timer,
};

enum class PublisherKind : uint8_t {
    Text,
    ValueOrColor,
};

constexpr uint32_t kSlotNone = 0xffffffffu;
constexpr int16_t kDimensionAuto = -1;

struct WidgetSpec {
    ScreenId screen;
    uint32_t create_callsite;
    uint32_t slot_offset;
    WidgetKind kind;
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
    int8_t align;
    int8_t long_mode;
    uint32_t text_drom;
    const char *seed_text;
    uint32_t font_drom;
    uint32_t color_rgb565;
    const char *role;
    uint32_t background_rgb565 = 0;
    uint32_t align_to_slot_offset = kSlotNone;
    const char *layout_note = "";
};

struct PublisherSpec {
    ScreenId screen;
    uint32_t call_vm;
    uint32_t app_offset;
    uint32_t flash_offset;
    PublisherKind kind;
    uint32_t model_record_offset;
    uint32_t bound_screen_slot_offset;
    const char *role;
    const char *confidence;
};

struct BindingSpec {
    ScreenId screen;
    uint32_t install_callsite;
    uint32_t model_record_offset;
    uint32_t callback_vm;
    uint32_t context_slot_offset;
    const char *role;
};

struct WeatherForecastTemplateSpec {
    uint32_t slot_base_offset;
    uint32_t slot_row_stride;
    uint32_t rows;
    uint32_t text_drom;
    const char *seed_text;
    uint32_t font_drom;
    uint32_t color_rgb565;
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
    int8_t align;
    int8_t long_mode;
    const char *role;
};

struct WeatherCallbackSpec {
    uint32_t install_callsite;
    uint32_t model_record_offset;
    uint32_t callback_vm;
    uint32_t context_slot_offset;
    const char *role;
};

struct RuntimeEvidence {
    ScreenId screen;
    uint32_t vm;
    uint32_t app_offset;
    uint32_t flash_offset;
    const char *role;
};

enum class MinerRuntimeEvidenceKind : uint8_t {
    RuntimeStateField,
    WriterCallsite,
    PublisherCallsite,
    TaskObjectPublisher,
    NonUiRoute,
};

struct MinerRuntimeEvidenceSpec {
    MinerRuntimeEvidenceKind kind;
    uint32_t vm;
    uint32_t app_offset;
    uint32_t flash_offset;
    uint32_t source_offset;
    uint32_t model_record_offset;
    uint32_t bound_screen_slot_offset;
    const char *role;
    const char *confidence;
    const char *status;
};

enum class ClockRouteKind : uint8_t {
    RuntimeSingleton,
    RuntimeField,
    TickerCacheField,
    ProducerCallsite,
    Binding,
    ColorPath,
};

struct ClockRouteEvidenceSpec {
    ClockRouteKind kind;
    uint32_t vm;
    uint32_t app_offset;
    uint32_t flash_offset;
    uint32_t source_offset;
    uint32_t target_offset;
    const char *role;
    const char *status;
};

struct MarketConfigSpec {
    const char *key;
    const char *default_value;
    uint32_t key_drom;
    uint32_t key_app_offset;
    uint32_t key_flash_offset;
    uint32_t default_drom;
    uint32_t default_app_offset;
    uint32_t default_flash_offset;
    const char *role;
};

struct AddressEvidence {
    const char *area;
    uint32_t vm_or_drom;
    uint32_t app_offset;
    uint32_t flash_offset;
    const char *role;
};

struct AppLifecycleEvidenceSpec {
    const char *area;
    uint32_t vm;
    uint32_t app_offset;
    uint32_t flash_offset;
    uint32_t literal_vm;
    uint32_t drom_string_vm;
    const char *role;
};

enum class StartupRouteKind : uint8_t {
    Entrypoint,
    EarlyIramInit,
    RomOrIdfHelper,
    MemoryClear,
    SystemConfig,
    ConstructorGate,
    ConstructorSweep,
    FrameworkHook,
    SchedulerHandoff,
};

struct StartupLiteralSpec {
    uint32_t literal_vm;
    uint32_t app_offset;
    uint32_t flash_offset;
    uint32_t value;
    const char *role;
};

struct StartupStepSpec {
    uint8_t order;
    StartupRouteKind kind;
    uint32_t vm;
    uint32_t app_offset;
    uint32_t flash_offset;
    uint32_t target_vm;
    const char *role;
    const char *confidence;
};

enum class TaskRouteKind : uint8_t {
    Helper400d9c68,
    HelperAfterNameObject,
    NameObjectOnly,
};

struct TaskRegistrationEvidenceSpec {
    const char *name;
    TaskRouteKind kind;
    uint32_t call_vm;
    uint32_t app_offset;
    uint32_t flash_offset;
    uint32_t helper_vm;
    uint32_t name_literal_vm;
    uint32_t name_drom;
    uint32_t entry_vm;
    uint32_t stack_or_const;
    uint32_t priority;
    uint32_t object_source;
    const char *role;
    const char *confidence;
};

enum class BringupStepKind : uint8_t {
    Startup,
    MemoryInit,
    ConstructorSweep,
    FrameworkHook,
    BeginRoute,
    HardwareInit,
    TaskRegistration,
    ReadyLog,
    UnresolvedHandoff,
};

struct OfficialBringupStepSpec {
    uint8_t order;
    BringupStepKind kind;
    uint32_t vm;
    uint32_t app_offset;
    uint32_t flash_offset;
    uint32_t target_vm;
    const char *label;
    const char *role;
    const char *status;
};

struct RuntimeServiceSlotSpec {
    uint32_t offset;
    const char *name;
    uint32_t install_vm;
    uint32_t install_app_offset;
    uint32_t install_flash_offset;
    const char *consumer;
    const char *status;
};

struct SourceBaselineGateSpec {
    const char *gate;
    bool satisfied;
    const char *evidence;
};

struct OfficialAppModelSummary {
    size_t bringup_steps;
    size_t task_registrations;
    size_t runtime_slots;
    size_t closed_gates;
    size_t total_gates;
    bool has_entrypoint;
    bool has_begin_route;
    bool has_all_tasks_started_log;
    bool has_scan_task;
    bool has_swarm_task;
    bool has_stratum_task;
    bool has_miner_tasks;
    bool flash_gate_closed;
};

enum class LifecycleValidationKind : uint8_t {
    Entrypoint,
    BssRange,
    ConstructorRoute,
    SchedulerHandoff,
    BeginRoute,
    RequiredTasks,
    RuntimeSlots,
    BehaviorParity,
    FreshBackupGate,
    FlashGuard,
};

struct RecoveredTaskRegistrationPlan {
    const char *name;
    uint32_t call_vm;
    uint32_t entry_vm;
    uint32_t stack_bytes;
    uint32_t priority;
    uint32_t object_source;
    bool creates_task;
    const char *status;
};

struct RecoveredRuntimeServices {
    uint32_t sprice_market_controller_offset;
    uint32_t clock_btc_ticker_cache_offset;
    uint32_t scan_controller_offset;
    uint32_t swarm_controller_offset;
    bool has_sprice_market_controller;
    bool has_clock_btc_ticker_cache;
    bool has_scan_controller;
    bool has_swarm_controller;
};

struct LifecycleValidationResult {
    LifecycleValidationKind kind;
    bool passed;
    uint32_t evidence_vm;
    const char *label;
    const char *evidence;
    const char *next_gate;
};

struct LifecycleValidationSummary {
    size_t passed;
    size_t total;
    bool startup_route_recovered;
    bool task_route_recovered;
    bool runtime_slots_recovered;
    bool behavior_parity_open;
    bool write_gate_open;
    bool flash_guard_locked;
    bool flashable;
};

struct SPricePatchTargetSpec {
    const char *target_id;
    const char *target_class;
    uint32_t vm_or_drom;
    uint32_t app_offset;
    uint32_t flash_offset;
    const char *current_value_or_bytes;
    const char *status;
};

enum class LittleFsImageSet : uint8_t {
    OfficialBundle,
    RecoveredExpanded,
};

struct LittleFsImageSpec {
    LittleFsImageSet set;
    const char *image_path;
    const char *sha256;
    uint32_t flash_offset;
    uint32_t length;
    uint32_t block_size;
    uint32_t block_count;
    size_t screensaver_count;
    const char *role;
};

struct ScreensaverAssetSpec {
    LittleFsImageSet set;
    const char *path;
    size_t byte_size;
    std::array<uint8_t, 8> header;
    const char *role;
};

constexpr size_t kWatchCoinsMaxSymbols = 20;
constexpr size_t kWatchCoinsTokenBytes = 16;
constexpr size_t kWatchCoinsDisplayBytes = kWatchCoinsTokenBytes + 1;

struct WatchCoinsToken {
    char symbol[kWatchCoinsDisplayBytes];
    size_t source_length;
    bool fits_recovered_payload;
};

struct WatchCoinsParseResult {
    std::array<WatchCoinsToken, kWatchCoinsMaxSymbols> tokens;
    size_t token_count;
    size_t truncate_at;
    bool input_was_null;
    bool hit_symbol_cap;
    bool had_trailing_tokens;
};

struct SPriceMarketRow {
    char label[16];
    float value_or_price;
    float change_percent;
};

struct SPriceTickerRecord {
    char symbol[kWatchCoinsTokenBytes];
    float last_price;
    float price_change_percent;
};

struct SPriceBehaviorSummary {
    size_t default_watchcoins;
    size_t default_truncate_at;
    size_t sample_cap_count;
    size_t sample_truncate_at;
    size_t adapted_rows;
    size_t ordered_rows;
    size_t rendered_row_limit;
    bool default_matches_evidence;
    bool watchcoins_cap_recovered;
    bool row_adapter_recovered;
    bool row_ordering_key_recovered;
    bool row_trim_recovered;
    bool row_ordering_recovered;
};

struct SwarmControllerRecoveredLayout {
    uint32_t scan_controller_ptr;
    uint32_t unknown_0x04;
    uint32_t count_0x08;
    float metric_0x0c;
    float metric_0x10;
    float metric_0x14;
    std::array<std::byte, 0x30> reserved_0x18;
    uint32_t unresolved_0x48;
    uint32_t wait_handle_0x4c;
    uint8_t stop_requested;
    uint8_t running;
};

struct SwarmWriterLocalsRecovered {
    float metric_0x0c_component_a;
    float metric_0x0c_component_b;
    float metric_0x10;
    float metric_0x14;
    uint32_t previous_count_0x08;
};

struct SwarmTelemetrySnapshotRecovered {
    uint32_t count_0x08;
    float metric_0x0c;
    float metric_0x10;
    float metric_0x14;
};

struct SwarmPublisherValuesRecovered {
    uint32_t count_0x08;
    float metric_0x0c;
    float metric_0x10;
    uint8_t metric_0x0c_precision;
    uint8_t metric_0x10_precision;
    bool metric_0x14_has_direct_miner_publisher;
    bool exact_float_meanings_recovered;
};

struct SwarmBehaviorSummary {
    size_t writer_records;
    size_t publisher_records;
    bool layout_offsets_recovered;
    bool writer_quartet_recovered;
    bool publisher_pairing_recovered;
    bool probe_count_recovered;
    bool aggregate_float_meaning_recovered;
    bool display_float_meaning_recovered;
    bool auxiliary_direct_publisher_recovered;
    bool precision_threshold_recovered;
};

struct ClockBtcTickerCacheRecoveredLayout {
    std::array<std::byte, 0x158> reserved_0x000;
    uint32_t btc_usd_last_price_low;
    uint32_t btc_usd_last_price_high;
};

struct RuntimeServicesRecoveredLayout {
    std::array<std::byte, 0x74> reserved_0x00;
    uint32_t sprice_market_controller_ptr;
    uint32_t unknown_0x78_ptr;
    uint32_t unknown_0x7c_ptr;
    uint32_t clock_btc_ticker_cache_ptr;
    uint32_t scan_controller_ptr;
    uint32_t swarm_controller_ptr;
};

struct MinerRuntimeStateRecoveredLayout {
    std::array<std::byte, 0x110> reserved_0x000;
    uint32_t difficulty_aggregate_a_low;
    uint32_t difficulty_aggregate_a_high;
    uint32_t difficulty_aggregate_b_low;
    uint32_t difficulty_aggregate_b_high;
    uint32_t difficulty_aggregate_c_low;
    uint32_t difficulty_aggregate_c_high;
    uint32_t network_difficulty_low;
    uint32_t network_difficulty_high;
    uint32_t accepted_share_count;
    uint32_t rejected_share_count;
    uint16_t block_hit_count;
    std::array<std::byte, 0x06> reserved_0x13a;
    uint32_t mining_notify_sequence_count;
    uint32_t unresolved_0x144;
    uint32_t hashrate_delta_a_low;
    uint32_t hashrate_delta_a_high;
    uint32_t hashrate_delta_b_low;
    uint32_t hashrate_delta_b_high;
    uint32_t combined_hashrate_low;
    uint32_t combined_hashrate_high;
};

static_assert(sizeof(SPriceMarketRow) == 24, "official SPrice row stride is 24");
static_assert(offsetof(SPriceMarketRow, label) == 0, "SPrice label offset");
static_assert(offsetof(SPriceMarketRow, value_or_price) == 16, "SPrice value offset");
static_assert(offsetof(SPriceMarketRow, change_percent) == 20, "SPrice change offset");
static_assert(offsetof(ClockBtcTickerCacheRecoveredLayout, btc_usd_last_price_low) == 0x158,
              "Clock BTC ticker cache price low-word offset");
static_assert(offsetof(ClockBtcTickerCacheRecoveredLayout, btc_usd_last_price_high) == 0x15c,
              "Clock BTC ticker cache price high-word offset");
static_assert(offsetof(RuntimeServicesRecoveredLayout, sprice_market_controller_ptr) == 0x74,
              "runtime+0x74 SPrice/WatchCoins market controller pointer");
static_assert(offsetof(RuntimeServicesRecoveredLayout, clock_btc_ticker_cache_ptr) == 0x80,
              "runtime+0x80 Clock BTC ticker cache pointer");
static_assert(offsetof(RuntimeServicesRecoveredLayout, scan_controller_ptr) == 0x84,
              "runtime+0x84 scan controller pointer");
static_assert(offsetof(RuntimeServicesRecoveredLayout, swarm_controller_ptr) == 0x88,
              "runtime+0x88 swarm controller pointer");
static_assert(offsetof(SwarmControllerRecoveredLayout, scan_controller_ptr) == 0x00,
              "swarm controller +0x00 scan object pointer");
static_assert(offsetof(SwarmControllerRecoveredLayout, count_0x08) == 0x08,
              "swarm controller +0x08 probe/count field");
static_assert(offsetof(SwarmControllerRecoveredLayout, metric_0x0c) == 0x0c,
              "swarm controller +0x0c metric field");
static_assert(offsetof(SwarmControllerRecoveredLayout, metric_0x10) == 0x10,
              "swarm controller +0x10 metric field");
static_assert(offsetof(SwarmControllerRecoveredLayout, metric_0x14) == 0x14,
              "swarm controller +0x14 metric field");
static_assert(offsetof(SwarmControllerRecoveredLayout, unresolved_0x48) == 0x48,
              "swarm controller +0x48 unresolved field");
static_assert(offsetof(SwarmControllerRecoveredLayout, wait_handle_0x4c) == 0x4c,
              "swarm controller +0x4c wait/sync handle");
static_assert(offsetof(SwarmControllerRecoveredLayout, stop_requested) == 0x50,
              "swarm controller +0x50 stop flag");
static_assert(offsetof(SwarmControllerRecoveredLayout, running) == 0x51,
              "swarm controller +0x51 running flag");
static_assert(offsetof(MinerRuntimeStateRecoveredLayout, difficulty_aggregate_a_low) == 0x110,
              "Miner state+0x110 difficulty aggregate low-word offset");
static_assert(offsetof(MinerRuntimeStateRecoveredLayout, network_difficulty_low) == 0x128,
              "Miner state+0x128 network difficulty low-word offset");
static_assert(offsetof(MinerRuntimeStateRecoveredLayout, accepted_share_count) == 0x130,
              "Miner state+0x130 accepted share count offset");
static_assert(offsetof(MinerRuntimeStateRecoveredLayout, rejected_share_count) == 0x134,
              "Miner state+0x134 rejected share count offset");
static_assert(offsetof(MinerRuntimeStateRecoveredLayout, block_hit_count) == 0x138,
              "Miner state+0x138 block-hit count offset");
static_assert(offsetof(MinerRuntimeStateRecoveredLayout, mining_notify_sequence_count) == 0x140,
              "Miner state+0x140 mining notify/job sequence count offset");
static_assert(offsetof(MinerRuntimeStateRecoveredLayout, hashrate_delta_a_low) == 0x148,
              "Miner state+0x148 hashrate accumulator start offset");
static_assert(offsetof(MinerRuntimeStateRecoveredLayout, combined_hashrate_low) == 0x158,
              "Miner state+0x158 combined hashrate low-word offset");
static_assert(offsetof(MinerRuntimeStateRecoveredLayout, combined_hashrate_high) == 0x15c,
              "Miner state+0x15c combined hashrate high-word offset");

constexpr uint32_t kSPriceCacheWriterStartVm = 0x400d73be;
constexpr uint32_t kSPriceCacheWriterEndVm = 0x400d7519;
constexpr uint32_t kSPriceCacheBaseOffset = 0x0d94;
constexpr uint32_t kSPriceCacheCountOffset = 0x1244;
constexpr uint32_t kSPriceRuntimeSourceOffset = 0x74;
constexpr uint32_t kSPriceSourceRowsBeginOffset = 0x24;
constexpr uint32_t kSPriceSourceRowsEndOffset = 0x28;
constexpr uint32_t kSPriceSourceRowsCapOffset = 0x2c;
constexpr size_t kSPriceMaxRows = 50;
constexpr size_t kSPriceRenderedRowBytes = 0x180;
constexpr size_t kSPriceRenderedRowLimit = kSPriceRenderedRowBytes / sizeof(SPriceMarketRow);
constexpr size_t kSPriceOrderingKeyOffset = offsetof(SPriceMarketRow, value_or_price);
constexpr uint32_t kRuntimeSPriceMarketControllerOffset = 0x74;
constexpr uint32_t kRuntimeClockBtcTickerCacheOffset = 0x80;
constexpr uint32_t kRuntimeScanControllerOffset = 0x84;
constexpr uint32_t kRuntimeSwarmControllerOffset = 0x88;
constexpr uint32_t kTaskObjectSourceUnresolved = 0xffffffffu;
constexpr uint32_t kSwarmControllerProbeCountOffset = 0x08;
constexpr uint32_t kSwarmControllerAggregateMetricOffset = 0x0c;
constexpr uint32_t kSwarmControllerDisplayMetricOffset = 0x10;
constexpr uint32_t kSwarmControllerAuxMetricOffset = 0x14;
constexpr uint32_t kSwarmControllerStopFlagOffset = 0x50;
constexpr uint32_t kSwarmControllerRunningFlagOffset = 0x51;
constexpr uint32_t kSwarmTaskEntryVm = 0x400dd35c;
constexpr uint32_t kSwarmAggregateWriterVm = 0x400ddb86;
constexpr uint32_t kSwarmProbeCountWriterVm = 0x400ddb8e;
constexpr uint32_t kSwarmDisplayWriterVm = 0x400ddb90;
constexpr uint32_t kSwarmAuxiliaryWriterVm = 0x400ddb93;
constexpr uint32_t kSwarmProbeCountPublisherVm = 0x400d6ea5;
constexpr uint32_t kSwarmAggregatePublisherVm = 0x400d6ee2;
constexpr uint32_t kSwarmDisplayPublisherVm = 0x400d6f31;
constexpr float kSwarmMetricPrecisionThreshold = 1000.0f;
constexpr uint8_t kSwarmAggregateMetricPublisherPrecision = 3;
constexpr uint8_t kSwarmMetricPrecisionBelowThreshold = 4;
constexpr uint8_t kSwarmMetricPrecisionAtOrAboveThreshold = 3;
constexpr uint32_t kClockBtcTickerPriceLowOffset = 0x158;
constexpr uint32_t kClockBtcTickerPriceHighOffset = 0x15c;
constexpr uint32_t kClockBtcTextModelOffset = 0x4f4;
constexpr uint32_t kClockBtcSlotOffset = 0x30;
constexpr uint32_t kClockBtcStaticColorRgb565 = 0xb596;
constexpr uint32_t kMinerDifficultyAggregateStartOffset = 0x110;
constexpr uint32_t kMinerNetworkDifficultyLowOffset = 0x128;
constexpr uint32_t kMinerNetworkDifficultyHighOffset = 0x12c;
constexpr uint32_t kMinerAcceptedShareCountOffset = 0x130;
constexpr uint32_t kMinerRejectedShareCountOffset = 0x134;
constexpr uint32_t kMinerBlockHitCountOffset = 0x138;
constexpr uint32_t kMinerNotifySequenceCountOffset = 0x140;
constexpr uint32_t kMinerHashrateAccumulatorStartOffset = 0x148;
constexpr uint32_t kMinerCombinedHashrateLowOffset = 0x158;
constexpr uint32_t kMinerCombinedHashrateHighOffset = 0x15c;
constexpr std::array<uint8_t, 8> kScreensaverHeader320x240 = {
    0x53, 0x4e, 0x40, 0x01, 0xf0, 0x00, 0x01, 0xa4,
};

std::string_view defaultWatchCoinsList();
WatchCoinsParseResult parseWatchCoinsList(const char *watchcoins, size_t length);
WatchCoinsParseResult parseWatchCoinsList(std::string_view watchcoins);
size_t adaptSPriceTickerRecordsToRows(
    ArrayView<const SPriceTickerRecord> source_records,
    std::array<SPriceMarketRow, kSPriceMaxRows> &destination);
bool spriceRecoveredPricePrecedes(const SPriceMarketRow &lhs,
                                  const SPriceMarketRow &rhs);
size_t orderAndTrimSPriceRowsForRecoveredRender(
    ArrayView<const SPriceMarketRow> source_rows,
    std::array<SPriceMarketRow, kSPriceRenderedRowLimit> &destination);
SPriceBehaviorSummary summarizeSPriceBehaviorRecovery();

uint8_t recoveredSwarmDisplayMetricPrecision(float value);
SwarmTelemetrySnapshotRecovered recoverSwarmTelemetrySnapshot(
    const SwarmWriterLocalsRecovered &locals);
void writeSwarmTelemetrySnapshot(
    SwarmControllerRecoveredLayout &controller,
    const SwarmTelemetrySnapshotRecovered &snapshot);
SwarmPublisherValuesRecovered readSwarmPublisherValues(
    const SwarmControllerRecoveredLayout &controller);
SwarmBehaviorSummary summarizeSwarmBehaviorRecovery();

size_t copySPriceRowsFromRuntimeVector(
    ArrayView<const SPriceMarketRow> source_rows,
    std::array<SPriceMarketRow, kSPriceMaxRows> &destination);

std::string_view screenName(ScreenId screen);
std::string_view widgetKindName(WidgetKind kind);
std::string_view publisherKindName(PublisherKind kind);
std::string_view minerRuntimeEvidenceKindName(MinerRuntimeEvidenceKind kind);
std::string_view clockRouteKindName(ClockRouteKind kind);
std::string_view startupRouteKindName(StartupRouteKind kind);
std::string_view taskRouteKindName(TaskRouteKind kind);
std::string_view bringupStepKindName(BringupStepKind kind);
std::string_view lifecycleValidationKindName(LifecycleValidationKind kind);

ArrayView<const WidgetSpec> widgetSpecs();
ArrayView<const PublisherSpec> publisherSpecs();
ArrayView<const BindingSpec> bindingSpecs();
ArrayView<const WeatherForecastTemplateSpec> weatherForecastTemplateSpecs();
ArrayView<const WeatherCallbackSpec> weatherCallbackSpecs();
ArrayView<const RuntimeEvidence> runtimeEvidence();
ArrayView<const MinerRuntimeEvidenceSpec> minerRuntimeEvidence();
ArrayView<const ClockRouteEvidenceSpec> clockRouteEvidence();
ArrayView<const MarketConfigSpec> marketConfigSpecs();
ArrayView<const AddressEvidence> watchCoinsConfigEvidence();
ArrayView<const AddressEvidence> spriceRowProducerEvidence();
ArrayView<const AppLifecycleEvidenceSpec> appLifecycleEvidence();
ArrayView<const StartupLiteralSpec> startupLiterals();
ArrayView<const StartupStepSpec> startupSteps();
ArrayView<const TaskRegistrationEvidenceSpec> taskRegistrationEvidence();
ArrayView<const OfficialBringupStepSpec> officialBringupSteps();
ArrayView<const RuntimeServiceSlotSpec> runtimeServiceSlots();
ArrayView<const SourceBaselineGateSpec> sourceBaselineGates();
OfficialAppModelSummary summarizeOfficialAppModel();
ArrayView<const RecoveredTaskRegistrationPlan> officialTaskRegistrationPlan();
RecoveredRuntimeServices recoveredRuntimeServices();
ArrayView<const LifecycleValidationResult> lifecycleValidationResults();
LifecycleValidationSummary summarizeLifecycleValidation();
bool sourceBaselineReadyForFlash();
ArrayView<const SPricePatchTargetSpec> spricePatchTargetSpecs();
ArrayView<const LittleFsImageSpec> littleFsImageSpecs();
ArrayView<const ScreensaverAssetSpec> screensaverAssetSpecs();

}  // namespace nmminer::recovered::v2002_st7789
