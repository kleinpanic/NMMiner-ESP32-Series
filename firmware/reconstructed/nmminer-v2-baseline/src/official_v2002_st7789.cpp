#include "nmminer_recovered/official_v2002_st7789.h"

#include <algorithm>

namespace nmminer::recovered::v2002_st7789 {
namespace {

constexpr WidgetSpec kWidgetSpecs[] = {
    {ScreenId::Miner, 0, 0x04, WidgetKind::Label, 1, 3, 320, kDimensionAuto, 2, 0, 0x3f52c984, " ", 0x3f434bfc, 0xffff, "current time / 12-hour top field"},
    {ScreenId::Miner, 0, 0x08, WidgetKind::Label, 80, 3, 320, kDimensionAuto, 1, 0, 0x3f52c984, " ", 0x3f436028, 0xffff, "top time/date companion"},
    {ScreenId::Miner, 0, 0x0c, WidgetKind::Label, 110, 5, 20, kDimensionAuto, 1, 0, 0x3f400ded, "d", 0x3f4f1758, 0xebe6, "static top unit glyph"},
    {ScreenId::Miner, 0, 0x10, WidgetKind::Label, 120, 3, 150, kDimensionAuto, 1, 0, 0x3f52c984, " ", 0x3f436028, 0xffff, "top subfield unresolved"},
    {ScreenId::Miner, 0, 0x14, WidgetKind::Label, 190, 3, 30, kDimensionAuto, 1, 0, 0x3f4be96d, "glyph U+F1EB", 0x3f4f1758, 0xebe6, "static top Wi-Fi glyph"},
    {ScreenId::Miner, 0, 0x18, WidgetKind::Label, 210, 3, 320, kDimensionAuto, 1, 0, 0x3f52c984, " ", 0x3f436028, 0xffff, "Wi-Fi RSSI/signal text"},
    {ScreenId::Miner, 0, 0x1c, WidgetKind::Label, 240, 3, 320, kDimensionAuto, 1, 0, 0x3f52c984, " ", 0x3f436028, 0xffff, "price-like market stat"},
    {ScreenId::Miner, 0, 0x20, WidgetKind::Label, 153, 30, 30, kDimensionAuto, 1, 0, 0x3f4be971, "glyph U+F019", 0x3f467f48, 0xad55, "row1 label icon"},
    {ScreenId::Miner, 0, 0x24, WidgetKind::Label, 185, 30, 320, kDimensionAuto, 2, 0, 0x3f52c984, " ", 0x3f439030, 0xffff, "mining notify/job sequence count"},
    {ScreenId::Miner, 0, 0x28, WidgetKind::Label, 153, 60, 30, kDimensionAuto, 1, 0, 0x3f4be975, "glyph U+F57D", 0x3f467f48, 0xad55, "row2 label icon"},
    {ScreenId::Miner, 0, 0x2c, WidgetKind::Label, 185, 60, 320, kDimensionAuto, 2, 0, 0x3f52c984, " ", 0x3f439030, 0xffff, "network difficulty"},
    {ScreenId::Miner, 0, 0x30, WidgetKind::Label, 153, 88, 30, kDimensionAuto, 1, 0, 0x3f4be979, "glyph U+F080", 0x3f467f48, 0xad55, "row3 label icon"},
    {ScreenId::Miner, 0, 0x34, WidgetKind::Label, 185, 88, 320, kDimensionAuto, 2, 0, 0x3f52c984, " ", 0x3f439030, 0xffff, "pool/current difficulty"},
    {ScreenId::Miner, 0, 0x38, WidgetKind::Label, 153, 115, 30, kDimensionAuto, 1, 0, 0x3f4be97d, "glyph U+F382", 0x3f467f48, 0xad55, "row4 label icon"},
    {ScreenId::Miner, 0, 0x3c, WidgetKind::Label, 185, 115, 320, kDimensionAuto, 2, 0, 0x3f52c984, " ", 0x3f439030, 0xffff, "accepted/rejected share summary"},
    {ScreenId::Miner, 0, 0x40, WidgetKind::Label, 17, 63, 320, kDimensionAuto, 2, 0, 0x3f52c984, " ", 0x3f4454a0, 0xebe6, "block-hit count"},
    {ScreenId::Miner, 0, 0x44, WidgetKind::Label, 165, 157, 320, kDimensionAuto, 2, 0, 0x3f52c984, " ", 0x3f44046c, 0xebe6, "large runtime-derived stat"},
    {ScreenId::Miner, 0, 0x48, WidgetKind::Label, 11, 123, 150, kDimensionAuto, 1, 0, 0x3f4008eb, "v2.0.02", 0x3f436028, 0xf79e, "firmware version"},
    {ScreenId::Miner, 0, 0x4c, WidgetKind::Label, 18, 175, 250, kDimensionAuto, 1, 0, 0x3f52c984, " ", 0x3f43765c, 0xffff, "pool/status line"},
    {ScreenId::Miner, 0, 0x50, WidgetKind::Label, 3, 215, 130, kDimensionAuto, 1, 0, 0x3f52c984, " ", 0x3f439030, 0xffff, "voltage/vcore-like bottom value"},
    {ScreenId::Miner, 0, 0x58, WidgetKind::Label, 145, 215, 80, kDimensionAuto, 1, 0, 0x3f52c984, " ", 0x3f439030, 0xffff, "bottom-mid telemetry integer"},
    {ScreenId::Miner, 0, 0x54, WidgetKind::Label, 237, 215, 58, kDimensionAuto, 1, 0, 0x3f52c984, " ", 0x3f439030, 0xffff, "bottom-right telemetry numeric"},
    {ScreenId::Miner, 0, kSlotNone, WidgetKind::Label, 290, 218, kDimensionAuto, kDimensionAuto, 1, 0, 0x3f4e42d6, "H/s", 0x3f4f1758, 0xffff, "trailing H/s unit"},

    {ScreenId::Clock, 0, 0x04, WidgetKind::Label, 0, 0, 320, kDimensionAuto, 2, 0, 0x3f52c984, " ", 0x3f44046c, 0x8410, "header line 1"},
    {ScreenId::Clock, 0, 0x08, WidgetKind::Label, 0, 22, 320, kDimensionAuto, 1, 0, 0x3f52c984, " ", 0x3f433e64, 0x8410, "header line 2"},
    {ScreenId::Clock, 0, 0x0c, WidgetKind::Label, 0, 0, kDimensionAuto, kDimensionAuto, 4, 0, 0x3f52c984, " ", 0x3f43765c, 0xb596, "day/date string"},
    {ScreenId::Clock, 0, 0x14, WidgetKind::Label, 80, -31, kDimensionAuto, kDimensionAuto, 9, 0, 0x3f52c984, " ", 0x3f4f5570, 0xb596, "AM/PM indicator", 0, kSlotNone, "width measured from DROM 0x3f51edbe (\"AM\")"},
    {ScreenId::Clock, 0, 0x18, WidgetKind::Label, 125, 170, 320, kDimensionAuto, 1, 0, 0x3f52c984, " ", 0x3f43abb4, 0xb596, "time field 1", 0, kSlotNone, "compound subwidget width measured from DROM 0x3f4738a8 (\"00:00\")"},
    {ScreenId::Clock, 0, 0x1c, WidgetKind::Label, 67, 170, 320, kDimensionAuto, 1, 0, 0x3f52c984, " ", 0x3f43abb4, 0xb596, "time field 2"},
    {ScreenId::Clock, 0, 0x20, WidgetKind::Label, 0, 0, 320, kDimensionAuto, 2, 0, 0x3f52c984, " ", 0x3f44046c, 0xebe6, "orange accent label"},
    {ScreenId::Clock, 0, 0x24, WidgetKind::Label, 0, 20, kDimensionAuto, kDimensionAuto, 3, 0, 0x3f4e42d6, "kH/s", 0x3f4f1758, 0xb596, "kH/s unit row"},
    {ScreenId::Clock, 0, 0x30, WidgetKind::Label, -2, 0, kDimensionAuto, kDimensionAuto, 6, 0, 0x3f52c984, " ", 0x3f4f5570, 0xb596, "BTC price value", 0, kSlotNone, "width = screen_width - 10 - textwidth(slot +0x0c, font 0x3f43765c)"},
    {ScreenId::Clock, 0x400f1dbc, 0x38, WidgetKind::Container, 0, 0, kDimensionAuto, kDimensionAuto, 0, 0, 0, "", 0, 0, "Clock compound sub-widget A"},
    {ScreenId::Clock, 0x400f1dbc, 0x3c, WidgetKind::Container, 0, 0, kDimensionAuto, kDimensionAuto, 9, 0, 0, "", 0, 0, "Clock compound sub-widget B", 0, kSlotNone, "aligned CENTER and width measured from \"00\""},
    {ScreenId::Clock, 0x400f1dbc, 0x40, WidgetKind::Container, 0, 0, kDimensionAuto, kDimensionAuto, 0, 0, 0, "", 0, 0, "Clock compound sub-widget C"},
    {ScreenId::Clock, 0, 0x44, WidgetKind::Timer, 0, 0, kDimensionAuto, kDimensionAuto, 0, 0, 0, "500 ms", 0, 0, "Clock refresh timer"},

    {ScreenId::SPrice, 0, 0x04, WidgetKind::Label, 4, 3, kDimensionAuto, kDimensionAuto, 1, 0, 0x3f4737d4, "---", 0x3f4ff4dc, 0x9d15, "MainCoin/title label"},
    {ScreenId::SPrice, 0, 0x08, WidgetKind::Label, -10, 1, kDimensionAuto, kDimensionAuto, 2, 0, 0x3f4e41b5, "$0", 0x3f406c7c, 0x9d15, "current price label"},
    {ScreenId::SPrice, 0, 0x14, WidgetKind::Label, -2, 13, kDimensionAuto, kDimensionAuto, 3, 0, 0x3f4e42d6, "kH/s", 0x3f4e90b4, 0xffff, "unit / interval label"},
    {ScreenId::SPrice, 0, 0x10, WidgetKind::Label, 18, -3, kDimensionAuto, kDimensionAuto, 0x12, 0, 0x3f52c4d9, "", 0x3f404fc0, 0xe3e4, "percent change label", 0, 0x14, "lv_obj_align_to(slot +0x14, mode 0x12, -3, 4)"},
    {ScreenId::SPrice, 0, 0x18, WidgetKind::Label, 20, 4, kDimensionAuto, kDimensionAuto, 0x14, 0, 0x3f52c4d9, "", 0x3f4eb540, 0x6bd1, "badge / pill label", 0x1906, 0x04, "foreground 0x6bd1; background 0x1906; lv_obj_align_to(slot +0x04, mode 0x14, 4, 0)"},
    {ScreenId::SPrice, 0, 0x24, WidgetKind::DrawArea, 0, 28, 320, 212, 1, 0, 0, "", 0, 0, "kline/pricewall draw surface"},
    {ScreenId::SPrice, 0x400f53f8, 0x7bc, WidgetKind::DrawArea, 4, 82, 200, 30, 1, 0, 0, "", 0x3f4f5570, 0, "WatchCoins child draw area"},

    {ScreenId::Weather, 0x400f3afd, 0x74, WidgetKind::Label, 4, 2, 72, kDimensionAuto, 0, 4, 0x3f4736a2, "00", 0x3f465dbc, 0xffff, "HH hours label"},
    {ScreenId::Weather, 0x400f3b40, 0x78, WidgetKind::Label, 76, 2, 9, kDimensionAuto, 0, 4, 0x3f47247c, ":", 0x3f465dbc, 0xdefb, "time colon"},
    {ScreenId::Weather, 0x400f3b7b, 0x7c, WidgetKind::Label, 85, 2, 72, kDimensionAuto, 0, 4, 0x3f4736a2, "00", 0x3f465dbc, 0xf3e0, "HH minutes label"},
    {ScreenId::Weather, 0x400f3bb7, 0x80, WidgetKind::Label, 0, 0, kDimensionAuto, kDimensionAuto, 0, 0, 0x3f52c4d9, "", 0x3f402a88, 0xad55, "time suffix label"},
    {ScreenId::Weather, 0x400f3bdd, kSlotNone, WidgetKind::Label, 8, 51, kDimensionAuto, kDimensionAuto, 0, 0, 0x3f4e41e7, "icon U+F051", 0x3f467494, 0xfd44, "left time-row icon"},
    {ScreenId::Weather, 0x400f3c09, 0x6c, WidgetKind::Label, 30, 51, kDimensionAuto, kDimensionAuto, 0, 0, 0x3f4e424b, "--:--", 0x3f402a88, 0xfd44, "left time value"},
    {ScreenId::Weather, 0x400f3c38, kSlotNone, WidgetKind::Label, 83, 51, kDimensionAuto, kDimensionAuto, 0, 0, 0x3f4e41eb, "icon U+F07A", 0x3f467494, 0x8c51, "humidity icon"},
    {ScreenId::Weather, 0x400f3c61, 0x14, WidgetKind::Label, 98, 51, kDimensionAuto, kDimensionAuto, 0, 0, 0x3f4e41ef, "--%", 0x3f402a88, 0x8c51, "humidity value"},
    {ScreenId::Weather, 0x400f3c8b, kSlotNone, WidgetKind::Label, 8, 67, kDimensionAuto, kDimensionAuto, 0, 0, 0x3f4e41f3, "icon U+F052", 0x3f467494, 0xfb88, "second time-row icon"},
    {ScreenId::Weather, 0x400f3cb4, 0x70, WidgetKind::Label, 30, 68, kDimensionAuto, kDimensionAuto, 0, 0, 0x3f4e424b, "--:--", 0x3f402a88, 0xfb88, "second time value"},
    {ScreenId::Weather, 0x400f3ce0, kSlotNone, WidgetKind::Label, 83, 68, kDimensionAuto, kDimensionAuto, 0, 0, 0x3f4e41f7, "icon U+F079", 0x3f467494, 0x8c51, "pressure icon"},
    {ScreenId::Weather, 0x400f3d09, 0x1c, WidgetKind::Label, 98, 69, kDimensionAuto, kDimensionAuto, 0, 0, 0x3f4e41fb, "--hPa", 0x3f402a88, 0x8c51, "pressure value"},
    {ScreenId::Weather, 0x400f3d33, 0x60, WidgetKind::Label, 0, 183, 312, kDimensionAuto, 3, 1, 0x3f4737d4, "---", 0x3f4eb540, 0xad55, "wide status/location line"},
    {ScreenId::Weather, 0x400f3d75, 0x10, WidgetKind::Label, 165, 0, kDimensionAuto, kDimensionAuto, 0, 0, 0x3f4e4201, "icon U+F185", 0x3f469348, 0xfea9, "main weather icon"},
    {ScreenId::Weather, 0x400f3da0, 0x04, WidgetKind::Label, -36, 0, kDimensionAuto, kDimensionAuto, 3, 0, 0x3f4e420b, "--", 0x3f40912c, 0xffff, "main temperature digits"},
    {ScreenId::Weather, 0x400f3dcc, 0x08, WidgetKind::Label, -8, 5, kDimensionAuto, kDimensionAuto, 3, 0, 0x3f4e423f, "degree-C suffix", 0x3f4f1758, 0xffff, "temperature unit seed"},
    {ScreenId::Weather, 0x400f3df7, 0x0c, WidgetKind::Label, 243, 51, 71, kDimensionAuto, 0, 4, 0x3f4e4205, "Feels --", 0x3f402a88, 0xad55, "feels-like value"},
    {ScreenId::Weather, 0x400f3e2f, kSlotNone, WidgetKind::Label, 157, 68, kDimensionAuto, kDimensionAuto, 0, 0, 0x3f473847, "|", 0x3f402a88, 0x8c51, "separator"},
    {ScreenId::Weather, 0x400f3e5b, kSlotNone, WidgetKind::Label, 164, 68, kDimensionAuto, kDimensionAuto, 0, 0, 0x3f4e420e, "icon U+F72E", 0x3f467494, 0x8c51, "wind icon"},
    {ScreenId::Weather, 0x400f3e85, 0x18, WidgetKind::Label, 178, 69, kDimensionAuto, kDimensionAuto, 0, 0, 0x3f4e4212, "--km/h", 0x3f402a88, 0x8c51, "wind value"},
    {ScreenId::Weather, 0x400f3eaf, kSlotNone, WidgetKind::Label, 236, 68, kDimensionAuto, kDimensionAuto, 0, 0, 0x3f473847, "|", 0x3f402a88, 0x8c51, "separator"},
    {ScreenId::Weather, 0x400f3ed8, 0x5c, WidgetKind::Label, 243, 69, 70, kDimensionAuto, 0, 4, 0x3f4e4219, "PM2.5 -", 0x3f402a88, 0x8c51, "AQI label"},
    {ScreenId::Weather, 0x400f3f2a, kSlotNone, WidgetKind::Line, 8, 84, 304, 1, 0, 0, 0, "", 0, 0, "forecast separator line"},
    {ScreenId::Weather, 0x400f3f6a, kSlotNone, WidgetKind::Container, 8, 87, 304, 30, 0, 0, 0, "", 0, 0, "forecast row container template"},
    {ScreenId::Weather, 0x400f407f, kSlotNone, WidgetKind::Line, 8, 181, 304, 1, 0, 0, 0, "", 0, 0, "footer separator line"},
    {ScreenId::Weather, 0x400f40b6, kSlotNone, WidgetKind::Label, 8, -6, kDimensionAuto, kDimensionAuto, 4, 0, 0x3f4008eb, "v2.0.02", 0x3f4eb540, 0x8c51, "firmware version footer"},
    {ScreenId::Weather, 0x400f40e2, 0x64, WidgetKind::Label, -18, 0, kDimensionAuto, kDimensionAuto, 5, 0, 0x3f4737d4, "---", 0x3f406c7c, 0xebe6, "footer dynamic value"},
    {ScreenId::Weather, 0x400f4112, kSlotNone, WidgetKind::Label, 25, -3, kDimensionAuto, kDimensionAuto, 5, 0, 0x3f4e42d6, "kH/s", 0x3f4eb540, 0xffff, "footer unit label"},
    {ScreenId::Weather, 0x400f413d, 0x68, WidgetKind::Label, -4, -2, kDimensionAuto, kDimensionAuto, 6, 0, 0x3f4e4243, "Updated --:--", 0x3f4eb540, 0x52aa, "last update label"},
};

constexpr PublisherSpec kPublisherSpecs[] = {
    {ScreenId::Miner, 0x400d672e, 0x16672e, 0x17672e, PublisherKind::Text, 0x068, 0x04, "current time / 12-hour top field", "high"},
    {ScreenId::Miner, 0x400d68a9, 0x1668a9, 0x1768a9, PublisherKind::Text, 0x094, 0x08, "top time/date companion", "med-low"},
    {ScreenId::Miner, 0x400d68e6, 0x1668e6, 0x1768e6, PublisherKind::Text, 0x0c0, 0x10, "top subfield", "low"},
    {ScreenId::Miner, 0x400d6939, 0x166939, 0x176939, PublisherKind::Text, 0x0ec, 0x18, "Wi-Fi RSSI/signal text", "high"},
    {ScreenId::Miner, 0x400d6966, 0x166966, 0x176966, PublisherKind::ValueOrColor, 0x118, 0x20, "RSSI quality color", "high"},
    {ScreenId::Miner, 0x400d699e, 0x16699e, 0x17699e, PublisherKind::Text, 0x2b8, 0x4c, "pool/status line", "high"},
    {ScreenId::Miner, 0x400d69f9, 0x1669f9, 0x1769f9, PublisherKind::Text, 0x260, 0x44, "runtime+0x80 +0x158/+0x15c formatted stat", "med"},
    {ScreenId::Miner, 0x400d6a49, 0x166a49, 0x176a49, PublisherKind::Text, 0x0ec, 0x18, "Wi-Fi RSSI/signal fallback", "high"},
    {ScreenId::Miner, 0x400d6a65, 0x166a65, 0x176a65, PublisherKind::ValueOrColor, 0x118, 0x20, "RSSI fallback color", "high"},
    {ScreenId::Miner, 0x400d6a9e, 0x166a9e, 0x176a9e, PublisherKind::Text, 0x234, 0x40, "block-hit count", "high"},
    {ScreenId::Miner, 0x400d6ad1, 0x166ad1, 0x176ad1, PublisherKind::Text, 0x154, 0x24, "mining notify/job sequence count", "med-high"},
    {ScreenId::Miner, 0x400d6af8, 0x166af8, 0x176af8, PublisherKind::ValueOrColor, 0x180, 0x30, "notify/job sequence change color", "med"},
    {ScreenId::Miner, 0x400d6b35, 0x166b35, 0x176b35, PublisherKind::Text, 0x190, 0x2c, "network difficulty", "med"},
    {ScreenId::Miner, 0x400d6ba6, 0x166ba6, 0x176ba6, PublisherKind::ValueOrColor, 0x1e8, 0x38, "difficulty/best-diff color", "med"},
    {ScreenId::Miner, 0x400d6cca, 0x166cca, 0x176cca, PublisherKind::Text, 0x1bc, 0x34, "pool/current difficulty", "med"},
    {ScreenId::Miner, 0x400d6d55, 0x166d55, 0x176d55, PublisherKind::Text, 0x1f8, 0x3c, "accepted/rejected share summary", "high"},
    {ScreenId::Miner, 0x400d6d9b, 0x166d9b, 0x176d9b, PublisherKind::ValueOrColor, 0x224, 0x38, "share-summary/diff icon color", "med"},
    {ScreenId::Miner, 0x400d6e1a, 0x166e1a, 0x176e1a, PublisherKind::Text, 0x128, 0x1c, "SPrice/WatchCoins market stat", "med"},
    {ScreenId::Miner, 0x400d6e57, 0x166e57, 0x176e57, PublisherKind::ValueOrColor, 0x144, 0x1c, "SPrice/WatchCoins change color", "high"},
    {ScreenId::Miner, 0x400d6ea5, 0x166ea5, 0x176ea5, PublisherKind::Text, 0x33c, 0x58, "swarm probe/alive count", "med-high"},
    {ScreenId::Miner, 0x400d6ec2, 0x166ec2, 0x176ec2, PublisherKind::ValueOrColor, 0x358, 0x58, "bottom-mid telemetry color", "low"},
    {ScreenId::Miner, 0x400d6ee2, 0x166ee2, 0x176ee2, PublisherKind::Text, 0x310, 0x54, "swarm aggregate metric", "med-low"},
    {ScreenId::Miner, 0x400d6efd, 0x166efd, 0x176efd, PublisherKind::ValueOrColor, 0x32c, 0x54, "bottom-right telemetry color", "low"},
    {ScreenId::Miner, 0x400d6f31, 0x166f31, 0x176f31, PublisherKind::Text, 0x2e4, 0x50, "swarm display metric", "med-low"},
    {ScreenId::Miner, 0x400d6f4a, 0x166f4a, 0x176f4a, PublisherKind::ValueOrColor, 0x300, 0x50, "bottom-left telemetry color", "low"},
    {ScreenId::Miner, 0x400d6fc9, 0x166fc9, 0x176fc9, PublisherKind::Text, 0x28c, 0x48, "firmware version", "high"},

    {ScreenId::Clock, 0x400d8272, 0x168272, 0x178272, PublisherKind::Text, 0x470, 0x28, "generic Clock slot +0x28 text", "med"},
    {ScreenId::Clock, 0x400d82a1, 0x1682a1, 0x1782a1, PublisherKind::Text, 0x4f4, 0x30, "BTC price text source record", "high"},
    {ScreenId::Clock, 0x400d8335, 0x168335, 0x178335, PublisherKind::Text, 0x49c, 0x04, "header text A", "high"},
    {ScreenId::Clock, 0x400d836e, 0x16836e, 0x17836e, PublisherKind::ValueOrColor, 0x4b8, 0x04, "header color A", "high"},
    {ScreenId::Clock, 0x400d83cd, 0x1683cd, 0x1783cd, PublisherKind::Text, 0x4c8, 0x08, "header text B", "high"},
    {ScreenId::Clock, 0x400d8409, 0x168409, 0x178409, PublisherKind::ValueOrColor, 0x4e4, 0x08, "header color B", "high"},
    {ScreenId::Clock, 0x400d8479, 0x168479, 0x178479, PublisherKind::Text, 0x520, 0x0c, "right/date text, not BTC price", "high"},

    {ScreenId::SPrice, 0x400d7506, 0x167506, 0x177506, PublisherKind::ValueOrColor, 0x1244, kSlotNone, "WatchCoins count publish", "high"},
    {ScreenId::SPrice, 0x400f4dfc, 0x184dfc, 0x194dfc, PublisherKind::Text, 0x1268, kSlotNone, "PricePageMode pricewall compare", "high"},
    {ScreenId::SPrice, 0x400f53ac, 0x1853ac, 0x1953ac, PublisherKind::ValueOrColor, 0x124c, kSlotNone, "WatchCoins page rotate", "high"},
    {ScreenId::SPrice, 0x400f54e4, 0x1854e4, 0x1954e4, PublisherKind::Text, 0x0d94, 0x7bc, "WatchCoins row renderer", "high"},
    {ScreenId::SPrice, 0x400f536c, 0x18536c, 0x19536c, PublisherKind::Text, 0x0d88, 0x24, "kline cache copy", "high"},
};

constexpr BindingSpec kBindingSpecs[] = {
    {ScreenId::SPrice, 0x400f69a4, 0x0560, 0x400f4d64, kSlotNone, "MainCoin/title text; realigns badge"},
    {ScreenId::SPrice, 0x400f69a4, 0x0570, 0x400f4d90, 0x04, "title color"},
    {ScreenId::SPrice, 0x400f69a4, 0x058c, 0x400f4d4c, 0x08, "price/current value text"},
    {ScreenId::SPrice, 0x400f69a4, 0x059c, 0x400f4d90, 0x08, "price color"},
    {ScreenId::SPrice, 0x400f69a4, 0x05e4, 0x400f4d4c, 0x18, "badge/mode text"},
    {ScreenId::SPrice, 0x400f69a4, 0x05f4, 0x400f4d90, 0x18, "badge color"},
    {ScreenId::SPrice, 0x400f69a4, 0x0270, 0x400f4db0, kSlotNone, "percent/change text; invalidates watch draw object"},
    {ScreenId::SPrice, 0x400f69a4, 0x0280, 0x400f4d90, 0x10, "percent/change color"},
    {ScreenId::SPrice, 0x400f69a4, 0x0d88, 0x400f536c, kSlotNone, "copy kline cache into screen+0x28 and invalidate chart"},
    {ScreenId::SPrice, 0x400f69a4, 0x1268, 0x400f4dfc, kSlotNone, "PricePageMode compare against pricewall"},
    {ScreenId::SPrice, 0x400f69a4, 0x124c, 0x400f53ac, kSlotNone, "rotate WatchCoins page index"},
};

constexpr WeatherForecastTemplateSpec kWeatherForecastTemplateSpecs[] = {
    {0x20, 4, 3, 0x3f4e4221, "--/--", 0x3f404fc0, 0xbdd7, 0, 0, kDimensionAuto, kDimensionAuto, 7, 0, "forecast day/date text"},
    {0x44, 4, 3, 0x3f4e4227, "icon U+F0C2", 0x3f467f48, 0x9d15, 65, 0, kDimensionAuto, kDimensionAuto, 7, 0, "forecast weather icon seed"},
    {0x50, 4, 3, 0x3f4e422b, "PM2.5/10 (-/-)", 0x3f401d0c, 0x8c51, 96, 0, 135, kDimensionAuto, 7, 4, "forecast AQI/PM text"},
    {0x38, 4, 3, 0x3f4e423a, "--~-- degree-C", 0x3f403afc, 0xebe6, 0, 0, kDimensionAuto, kDimensionAuto, 8, 0, "forecast temperature range"},
};

constexpr WeatherCallbackSpec kWeatherCallbackSpecs[] = {
    {0x400f4187, 0x1278 + 0x420, 0x400f6a74, kSlotNone, "direct text callback"},
    {0x400f419a, 0x1278 + 0x430, 0x400f38c0, 0x60, "status/location context callback"},
    {0x400f41ab, 0x260, 0x400f6ac0, kSlotNone, "formatter callback"},
    {0x400f41c5, 0x270, 0x400f386c, 0x64, "footer dynamic value callback"},
    {0x400f41db, 0x3a4, 0x400f3928, 0x00, "HH:MM split callback"},
    {0x400f41f0, 0x3d0, 0x400f386c, 0x00, "suffix/text callback"},
};

constexpr RuntimeEvidence kRuntimeEvidence[] = {
    {ScreenId::Miner, 0x400f4b24, 0x184b24, 0x194b24, "Miner dynamic model binding fan-out"},
    {ScreenId::Miner, 0x400d6104, 0x166104, 0x176104, "string-record publisher"},
    {ScreenId::Miner, 0x400d38cc, 0x1638cc, 0x1738cc, "value/color-record publisher"},
    {ScreenId::Clock, 0x400f1eb0, 0x181eb0, 0x191eb0, "BTC price text/layout owner"},
    {ScreenId::Clock, 0x400d57c0, 0x1657c0, 0x1757c0, "runtime singleton accessor"},
    {ScreenId::Clock, 0x400d6219, 0x166219, 0x176219, "runtime+0x80 Clock BTC ticker-cache stack store"},
    {ScreenId::Clock, 0x400d621b, 0x16621b, 0x17621b, "runtime+0x74 SPrice/WatchCoins market-controller stack store a1+36"},
    {ScreenId::Miner, 0x400d621b, 0x16621b, 0x17621b, "Miner market-stat publishers source: runtime+0x74 stored to stack a1+36"},
    {ScreenId::Miner, 0x400d621d, 0x16621d, 0x17621d, "Miner bottom swarm publishers source: runtime+0x88 stored to stack a1+48"},
    {ScreenId::Miner, 0x400de20b, 0x16e20b, 0x17e20b, "runtime service constructor calls scan/swarm installer 0x400da778"},
    {ScreenId::Miner, 0x400da894, 0x16a894, 0x17a894, "stores swarm controller pointer to runtime+0x88"},
    {ScreenId::Miner, 0x400da90a, 0x16a90a, 0x17a90a, "registers task entry 0x400dd35c named (swarm) with param runtime+0x88"},
    {ScreenId::Miner, 0x400ddb8e, 0x16db8e, 0x17db8e, "swarm task writes object+0x08 count consumed by Miner slot +0x58"},
    {ScreenId::Clock, 0x400d81e4, 0x1681e4, 0x1781e4, "Clock BTC ticker-cache last-price double low-word read"},
    {ScreenId::Clock, 0x400d81ed, 0x1681ed, 0x1781ed, "Clock BTC ticker-cache last-price double high-word read"},
    {ScreenId::Clock, 0x400d82a1, 0x1682a1, 0x1782a1, "BTC price publish to global+0x4f4"},
    {ScreenId::Clock, 0x400d836e, 0x16836e, 0x17836e, "Clock header color A publish, not BTC price slot"},
    {ScreenId::Clock, 0x400d8409, 0x168409, 0x178409, "Clock header color B publish, not BTC price slot"},
    {ScreenId::SPrice, 0x400f69a4, 0x1869a4, 0x1969a4, "SPrice binding table / consumer fan-out"},
    {ScreenId::SPrice, 0x400d5800, 0x165800, 0x175800, "runtime init zeros runtime+0x74"},
    {ScreenId::SPrice, 0x400ddf61, 0x16df61, 0x17df61, "runtime+0x74 setter stores static market object"},
    {ScreenId::SPrice, 0x400db278, 0x16b278, 0x17b278, "market task entry for runtime+0x74 object"},
    {ScreenId::SPrice, 0x400e3a80, 0x173a80, 0x183a80, "WatchCoins 24-byte row producer"},
    {ScreenId::SPrice, 0x400d73be, 0x1673be, 0x1773be, "SPrice cache writer start"},
    {ScreenId::SPrice, 0x400d7416, 0x167416, 0x177416, "SPrice row copy loop"},
    {ScreenId::SPrice, 0x400d7506, 0x167506, 0x177506, "SPrice count publish to global+0x1244"},
    {ScreenId::SPrice, 0x400f54e4, 0x1854e4, 0x1954e4, "WatchCoins populated-cache renderer"},
    {ScreenId::SPrice, 0x400e7978, 0x177978, 0x187978, "WatchCoins wrapper before body 0x400e798c"},
    {ScreenId::SPrice, 0x400e798c, 0x17798c, 0x18798c, "WatchCoins config/tokenizer body"},
    {ScreenId::SPrice, 0x400e7b21, 0x177b21, 0x187b21, "WatchCoins comma-scan loop; count capped at 20"},
    {ScreenId::SPrice, 0x400db307, 0x16b307, 0x17b307, "market config load/save xref for WatchCoins defaults"},
    {ScreenId::SPrice, 0x400db511, 0x16b511, 0x17b511, "market config path calls ticker wrapper"},
    {ScreenId::SPrice, 0x400db684, 0x16b684, 0x17b684, "market config default WatchCoins xref"},
    {ScreenId::SPrice, 0x400e3e84, 0x173e84, 0x183e84, "ticker wrapper stores parsed fields into caller object"},
    {ScreenId::SPrice, 0x400e3b84, 0x173b84, 0x183b84, "ticker parser for lastPrice and priceChangePercent"},
    {ScreenId::Weather, 0x400f3ab4, 0x183ab4, 0x193ab4, "Weather static builder"},
    {ScreenId::Weather, 0x400f3928, 0x183928, 0x193928, "Weather HH:MM formatter callback"},
    {ScreenId::Weather, 0x400f3a34, 0x183a34, 0x193a34, "Weather dispatcher / on-show"},
};

constexpr AppLifecycleEvidenceSpec kAppLifecycleEvidence[] = {
    {"startup", 0x40086aa4, 0x2970e8, 0x2a70e8, 0, 0,
     "official app entrypoint from ESP32 image header"},
    {"init", 0x400da489, 0x16a489, 0x17a489, 0x400d082c, 0x3f400c0f,
     "LVGL task creation failure log route"},
    {"init", 0x400da749, 0x16a749, 0x17a749, 0x400d083c, 0x3f400c40,
     "hardware initialization log route"},
    {"init", 0x400da76b, 0x16a76b, 0x17a76b, 0x400d0848, 0x3f400c62,
     "peripherals-ready log route"},
    {"task", 0x400da82e, 0x16a82e, 0x17a82e, 0x400d085c, 0x3f400c75,
     "scan task registration; name literal '(scan)'"},
    {"task", 0x400da90a, 0x16a90a, 0x17a90a, 0x400d0874, 0x3f400c7c,
     "swarm task registration; name literal '(swarm)'"},
    {"task", 0x400dad56, 0x16ad56, 0x17ad56, 0x400d08a4, 0x3f400c8c,
     "stratum task registration; name literal '(stratum)'"},
    {"task", 0x400dadcd, 0x16adcd, 0x17adcd, 0x400d08b8, 0x3f400c96,
     "tMiner task helper after name/object setup; name literal '(tMiner)'"},
    {"task", 0x400dae29, 0x16ae29, 0x17ae29, 0x400d08d4, 0x3f400c9f,
     "nMiner name/object setup callsite; helper call follows at 0x400dae8a"},
    {"task", 0x400dae8a, 0x16ae8a, 0x17ae8a, 0, 0,
     "nMiner task helper call after name/object setup"},
    {"begin", 0x400de1e4, 0x16e1e4, 0x17e1e4, 0, 0,
     "begin-style service/task setup routine entry"},
    {"begin", 0x400de25d, 0x16e25d, 0x17e25d, 0x400d0a88, 0x3f401068,
     "post-setup log route for 'NMMiner::begin: all tasks started'"},
};

constexpr StartupLiteralSpec kStartupLiteralSpecs[] = {
    {0x40080760, 0x15eb18, 0x16eb18, 0x40080000,
     "IRAM0 base literal passed to early init helper 0x4009640c"},
    {0x4008079c, 0x15eb54, 0x16eb54, kOfficialBssClearEnd,
     "BSS/data clear upper bound used by 0x40086acd"},
    {0x400807a0, 0x15eb58, 0x16eb58, kOfficialBssClearStart,
     "BSS/data clear lower bound used by 0x40086acd"},
    {0x400807a4, 0x15eb5c, 0x16eb5c, kOfficialSecondaryClearEnd,
     "conditional secondary clear upper bound; same as lower bound in this image"},
    {0x400807a8, 0x15eb60, 0x16eb60, kOfficialSecondaryClearStart,
     "conditional secondary clear lower bound; same as upper bound in this image"},
    {0x400807d0, 0x15eb88, 0x16eb88, 0x400081d4,
     "ROM/helper pointer called with core arguments 0 and 1"},
    {0x400807d4, 0x15eb8c, 0x16eb8c, kOfficialConstructorPredicateVm,
     "IROM predicate gate before constructor sweep 0x40086910"},
    {0x400807d8, 0x15eb90, 0x16eb90, 0x40167080,
     "IROM init hook after direct call 0x4008cb38"},
    {0x400807dc, 0x15eb94, 0x16eb94, 0x4015ebe0,
     "IROM init hook called with stack-local boot context at 0x40086b17"},
    {0x400807e0, 0x15eb98, 0x16eb98, 0x4013b464,
     "IROM hook after startup wait loop at 0x40086bf5"},
    {0x400807e4, 0x15eb9c, 0x16eb9c, 0x4013b584,
     "IROM hook after 0x4013b464"},
    {0x400807e8, 0x15eba0, 0x16eba0, 0x4015e0ec,
     "IROM hook after 0x4013b584"},
    {0x400807ec, 0x15eba4, 0x16eba4, 0x4015eb30,
     "conditional IROM hook when the early core/reset result equals 5"},
};

constexpr StartupStepSpec kStartupStepSpecs[] = {
    {1, StartupRouteKind::Entrypoint,
     0x40086aa4, 0x2970e8, 0x2a70e8, 0,
     "official ESP-IDF app entrypoint from image header", "high"},
    {2, StartupRouteKind::EarlyIramInit,
     0x40086aaa, 0x2970ee, 0x2a70ee, 0x4009640c,
     "early IRAM init helper called with literal 0x40080000", "med"},
    {3, StartupRouteKind::RomOrIdfHelper,
     0x40086ab3, 0x2970f7, 0x2a70f7, 0x400081d4,
     "indirect ROM/helper call with a10=0; return copied to a4", "med"},
    {4, StartupRouteKind::RomOrIdfHelper,
     0x40086abf, 0x297103, 0x2a7103, 0x400081d4,
     "indirect ROM/helper call with a10=1", "med"},
    {5, StartupRouteKind::MemoryClear,
     0x40086acd, 0x297111, 0x2a7111, 0x4008deac,
     "clears range 0x3ffc4100..0x3ffd6b30", "high"},
    {6, StartupRouteKind::MemoryClear,
     0x40086ade, 0x297122, 0x2a7122, 0x4008deac,
     "conditional secondary clear over empty 0x50000200..0x50000200 range", "med"},
    {7, StartupRouteKind::SystemConfig,
     0x40086ae4, 0x297128, 0x2a7128, 0x400869e0,
     "direct system/cache configuration helper before constructor gate", "med"},
    {8, StartupRouteKind::ConstructorGate,
     0x40086aea, 0x29712e, 0x2a712e, kOfficialConstructorPredicateVm,
     "IROM predicate; nonzero path calls constructor sweep 0x40086910", "med"},
    {9, StartupRouteKind::ConstructorSweep,
     0x40086af2, 0x297136, 0x2a7136, kOfficialConstructorSweepVm,
     "constructor/init-function sweep gated by 0x40130cd0 result", "med-high"},
    {10, StartupRouteKind::FrameworkHook,
     0x40086af5, 0x297139, 0x2a7139, 0x4008cb38,
     "direct framework init helper", "med"},
    {11, StartupRouteKind::FrameworkHook,
     0x40086afb, 0x29713f, 0x2a713f, 0x40167080,
     "indirect IROM init hook after 0x4008cb38", "med"},
    {12, StartupRouteKind::FrameworkHook,
     0x40086afe, 0x297142, 0x2a7142, 0x4008cb40,
     "direct framework init helper", "med"},
    {13, StartupRouteKind::FrameworkHook,
     0x40086b04, 0x297148, 0x2a7148, 0x40167070,
     "indirect IROM init hook after 0x4008cb40", "med"},
    {14, StartupRouteKind::FrameworkHook,
     0x40086b0e, 0x297152, 0x2a7152, 0,
     "sets startup byte flag at literal 0x3ffd019f before stack-context hook", "high"},
    {15, StartupRouteKind::FrameworkHook,
     0x40086b1a, 0x29715e, 0x2a715e, 0x4015ebe0,
     "indirect IROM init hook with stack-local boot context", "med"},
    {16, StartupRouteKind::FrameworkHook,
     0x40086bf5, 0x297239, 0x2a7239, 0x4013b464,
     "IROM hook after startup wait loop", "med"},
    {17, StartupRouteKind::FrameworkHook,
     0x40086bf8, 0x29723c, 0x2a723c, 0x4013b584,
     "IROM hook following 0x4013b464", "med"},
    {18, StartupRouteKind::FrameworkHook,
     0x40086c04, 0x297248, 0x2a7248, 0x4015e0ec,
     "IROM hook following 0x4013b584", "med"},
    {19, StartupRouteKind::FrameworkHook,
     0x40086c17, 0x29725b, 0x2a725b, 0x400915f0,
     "direct framework call before scheduler/task handoff helpers", "med"},
    {20, StartupRouteKind::SchedulerHandoff,
     0x40086c28, 0x29726c, 0x2a726c, kOfficialSchedulerHandoffVm,
     "framework/scheduler handoff helper with stack context and 0x1c200 literal", "med"},
    {21, StartupRouteKind::FrameworkHook,
     0x40086c31, 0x297275, 0x2a7275, 0x4015eb30,
     "conditional IROM hook when early result equals 5", "med"},
    {22, StartupRouteKind::FrameworkHook,
     0x40086c37, 0x29727b, 0x2a727b, 0x4013b758,
     "late indirect IROM startup hook", "med"},
};

constexpr TaskRegistrationEvidenceSpec kTaskRegistrationEvidence[] = {
    {"(button)", TaskRouteKind::Helper400d9c68,
     0x400da765, 0x16a765, 0x17a765, 0x400d9c68,
     0x400d0840, 0x3f400c59, 0x400d5078, 0x00000c00, 3,
     kTaskObjectSourceUnresolved,
     "hardware button task gated by peripheral flag",
     "high-callsite"},
    {"(scan)", TaskRouteKind::Helper400d9c68,
     0x400da82e, 0x16a82e, 0x17a82e, 0x400d9c68,
     0x400d085c, 0x3f400c75, 0x400dbe38, 0x00000a00, 4,
     kRuntimeScanControllerOffset,
     "scan task; object pointer is stored at runtime+0x84 before helper call",
     "high-callsite"},
    {"(swarm)", TaskRouteKind::Helper400d9c68,
     0x400da90a, 0x16a90a, 0x17a90a, 0x400d9c68,
     0x400d0874, 0x3f400c7c, 0x400dd35c, 0x00000c00, 5,
     kRuntimeSwarmControllerOffset,
     "swarm task; object pointer is stored at runtime+0x88 and links runtime+0x84 scan object",
     "high-callsite"},
    {"(stratum)", TaskRouteKind::Helper400d9c68,
     0x400dad56, 0x16ad56, 0x17ad56, 0x400d9c68,
     0x400d08a4, 0x3f400c8c, 0x400ef724, 0x00001800, 9,
     kTaskObjectSourceUnresolved,
     "stratum task helper call from miner-service setup path",
     "high-callsite"},
    {"(tMiner)", TaskRouteKind::HelperAfterNameObject,
     0x400dadcd, 0x16adcd, 0x17adcd, 0x400d9c68,
     0x400d08b8, 0x3f400c96, 0x400e3f00, 0x00000800, 1,
     0x3ffc4130,
     "task helper after 0x400dada3 name/object setup; a12/object field semantics still unresolved",
     "med-high-callsite"},
    {"(nMiner)", TaskRouteKind::NameObjectOnly,
     0x400dae29, 0x16ae29, 0x17ae29, 0x4012ddb4,
     0x400d08d4, 0x3f400c9f, 0, 0, 0,
     0x3ffc4108,
     "name/object setup call; not the task helper itself",
     "high-string-xref"},
    {"(nMiner)", TaskRouteKind::HelperAfterNameObject,
     0x400dae8a, 0x16ae8a, 0x17ae8a, 0x400d9c68,
     0x400d08d4, 0x3f400c9f, 0x400e3f00, 0x00000800, 2,
     0x3ffc4108,
     "task helper after 0x400dae29 name/object setup; entry reuse needs Ghidra type recovery",
     "med-callsite"},
};

constexpr OfficialBringupStepSpec kOfficialBringupSteps[] = {
    {1, BringupStepKind::Startup,
     kOfficialAppEntryVm, kOfficialAppEntryAppOffset, kOfficialAppEntryFlashOffset, 0,
     "app_entry", "official app entrypoint from image header", "high"},
    {2, BringupStepKind::Startup,
     0x40086aaa, 0x2970ee, 0x2a70ee, 0x4009640c,
     "early_iram_init", "passes literal 0x40080000 into early helper", "helper name unresolved"},
    {3, BringupStepKind::MemoryInit,
     0x40086acd, 0x297111, 0x2a7111, 0x4008deac,
     "clear_bss", "clears 0x3ffc4100..0x3ffd6b30", "high"},
    {4, BringupStepKind::ConstructorSweep,
     0x40086af2, 0x297136, 0x2a7136, kOfficialConstructorSweepVm,
     "constructor_sweep", "constructor/init sweep gated by 0x40130cd0", "med-high"},
    {5, BringupStepKind::FrameworkHook,
     0x40086b1a, 0x29715e, 0x2a715e, 0x4015ebe0,
     "framework_hook", "IROM init hook with stack-local boot context", "exact helper unresolved"},
    {6, BringupStepKind::UnresolvedHandoff,
     0x40086c28, 0x29726c, 0x2a726c, kOfficialSchedulerHandoffVm,
     "scheduler_handoff", "framework/scheduler handoff helper", "downstream app task handoff unresolved"},
    {7, BringupStepKind::BeginRoute,
     0x400de1e4, 0x16e1e4, 0x17e1e4, 0,
     "NMMiner_begin_route", "official begin-style service/task setup entry", "med-structural"},
    {8, BringupStepKind::HardwareInit,
     0x400da749, 0x16a749, 0x17a749, 0x400d083c,
     "hardware_init_log", "loads/logs 'Initializing hardware...'", "high-string-xref"},
    {9, BringupStepKind::HardwareInit,
     0x400da76b, 0x16a76b, 0x17a76b, 0x400d0848,
     "peripherals_ready_log", "loads/logs 'Peripherals ready.'", "high-string-xref"},
    {10, BringupStepKind::TaskRegistration,
     0x400da82e, 0x16a82e, 0x17a82e, 0x400dbe38,
     "(scan)", "registers scan task; stack 0x0a00 priority 4 param runtime+0x84", "high-callsite"},
    {11, BringupStepKind::TaskRegistration,
     0x400da90a, 0x16a90a, 0x17a90a, 0x400dd35c,
     "(swarm)", "registers swarm task; stack 0x0c00 priority 5 param runtime+0x88", "high-callsite"},
    {12, BringupStepKind::TaskRegistration,
     0x400dad56, 0x16ad56, 0x17ad56, 0x400ef724,
     "(stratum)", "registers stratum task; stack 0x1800 priority 9", "high-callsite"},
    {13, BringupStepKind::TaskRegistration,
     0x400dadcd, 0x16adcd, 0x17adcd, 0x400e3f00,
     "(tMiner)", "registers tMiner after name/object setup; object 0x3ffc4130", "med-high-callsite"},
    {14, BringupStepKind::TaskRegistration,
     0x400dae8a, 0x16ae8a, 0x17ae8a, 0x400e3f00,
     "(nMiner)", "registers nMiner after split name/object setup; object 0x3ffc4108", "entry reuse needs type recovery"},
    {15, BringupStepKind::ReadyLog,
     0x400de25d, 0x16e25d, 0x17e25d, 0x400d0a88,
     "all_tasks_started", "loads/logs 'NMMiner::begin: all tasks started'", "high-string-xref"},
};

constexpr RuntimeServiceSlotSpec kRuntimeServiceSlots[] = {
    {kRuntimeSPriceMarketControllerOffset, "runtime+0x74 sprice_market_controller",
     0x400ddf61, 0x16df61, 0x17df61,
     "SPrice cache writer and Miner market-stat publisher", "proven setter and consumers"},
    {kRuntimeClockBtcTickerCacheOffset, "runtime+0x80 clock_btc_ticker_cache",
     0x400dac58, 0x16ac58, 0x17ac58,
     "Clock BTC price route and Miner slot +0x44 source family", "proven separate from miner hashrate"},
    {kRuntimeScanControllerOffset, "runtime+0x84 scan_controller",
     0x400da778, 0x16a778, 0x17a778,
     "(scan) task parameter", "proven setup and task registration"},
    {kRuntimeSwarmControllerOffset, "runtime+0x88 swarm_controller",
     0x400da894, 0x16a894, 0x17a894,
     "(swarm) task parameter and Miner bottom telemetry publishers", "exact float meanings unresolved/no-touch"},
};

constexpr SourceBaselineGateSpec kSourceBaselineGates[] = {
    {"official v2.0.02 ST7789 provenance", true,
     "local official bundle/backup and recovered address maps are pinned to esp32-2432s028r-st7789"},
    {"compile-only PlatformIO harness", true,
     "ESP32 Arduino/LVGL harness builds with official ST7789 partition geometry"},
    {"recovered startup/begin/task model", true,
     "startup literals/steps, begin route, runtime slots, and task registrations are typed in this model"},
    {"official behavior parity", false,
     "mining, Wi-Fi/config, touch, full UI behavior, and task execution are not reconstructed yet"},
    {"fresh-backup write gate", false,
     "no fresh named 4 MB backup artifact was created for a reconstructed write"},
    {"flash approval", false,
     "guard_no_upload.py blocks upload/program/erase/uploadfs/uploadfsota by default"},
};

constexpr MinerRuntimeEvidenceSpec kMinerRuntimeEvidence[] = {
    {MinerRuntimeEvidenceKind::RuntimeStateField, 0, 0, 0,
     kMinerAcceptedShareCountOffset, kSlotNone, kSlotNone,
     "accepted_share_count", "high",
     "state+0x130; writer 0x400f1607; UI summary publisher 0x400d6d55"},
    {MinerRuntimeEvidenceKind::RuntimeStateField, 0, 0, 0,
     kMinerRejectedShareCountOffset, kSlotNone, kSlotNone,
     "rejected_share_count", "med-high",
     "state+0x134; rejected path is mapped but objdump alignment still needs Ghidra cleanup"},
    {MinerRuntimeEvidenceKind::RuntimeStateField, 0, 0, 0,
     kMinerBlockHitCountOffset, kSlotNone, kSlotNone,
     "block_hit_count", "high",
     "state+0x138; writer 0x400f16a4; UI publisher 0x400d6a9e"},
    {MinerRuntimeEvidenceKind::RuntimeStateField, 0, 0, 0,
     kMinerNotifySequenceCountOffset, kSlotNone, kSlotNone,
     "mining_notify_sequence_count", "med-high",
     "state+0x140; writer 0x400f1258 stores incremented ctx+0x48; publisher 0x400d6ad1; distinct from accepted state+0x130"},
    {MinerRuntimeEvidenceKind::RuntimeStateField, 0, 0, 0,
     kMinerCombinedHashrateLowOffset, kSlotNone, kSlotNone,
     "combined_hashrate_low_word", "high",
     "state+0x158; hashrate accumulator 0x400f1817; formatter read 0x400f1a17"},
    {MinerRuntimeEvidenceKind::RuntimeStateField, 0, 0, 0,
     kMinerCombinedHashrateHighOffset, kSlotNone, kSlotNone,
     "combined_hashrate_high_word", "high",
     "state+0x15c; paired high word for combined hashrate"},

    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400f1607, 0x181607, 0x191607,
     kMinerAcceptedShareCountOffset, kSlotNone, kSlotNone,
     "accepted share counter increment/store", "high",
     "inside share-result block 0x400f15d0..0x400f174c"},
    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400f16a4, 0x1816a4, 0x1916a4,
     kMinerBlockHitCountOffset, kSlotNone, kSlotNone,
     "block-hit counter increment/store", "high",
     "inside share-result block; consumed by 0x400d6a9e"},
    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400f16f4, 0x1816f4, 0x1916f4,
     kMinerRejectedShareCountOffset, kSlotNone, kSlotNone,
     "rejected share counter path", "med-high",
     "share-result block evidence; align exact instruction in Ghidra before byte edits"},
    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400daab7, 0x16aab7, 0x17aab7,
     kMinerNotifySequenceCountOffset, kSlotNone, kSlotNone,
     "notify/job sequence reset", "med-high",
     "object reset path clears +0x140 near accepted/rejected reset and +0x158/+0x15c init"},
    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400ef844, 0x17f844, 0x18f844,
     kMinerNotifySequenceCountOffset, kSlotNone, kSlotNone,
     "notify/job sequence seed", "med-high",
     "start/seed path writes literal 1 into state+0x140"},
    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400ef9b8, 0x17f9b8, 0x18f9b8,
     kMinerNotifySequenceCountOffset, kSlotNone, kSlotNone,
     "notify/job sequence seed", "med-high",
     "alternate start/seed path writes literal 1 into state+0x140"},
    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400f0370, 0x180370, 0x190370,
     kMinerNotifySequenceCountOffset, kSlotNone, kSlotNone,
     "notify/job sequence increment", "med-high",
     "supporting runtime path increments state+0x140 in place"},
    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400f076f, 0x18076f, 0x19076f,
     kMinerNotifySequenceCountOffset, kSlotNone, kSlotNone,
     "notify/job sequence increment", "med-high",
     "supporting runtime path increments state+0x140 in place"},
    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400f1258, 0x181258, 0x191258,
     kMinerNotifySequenceCountOffset, kSlotNone, kSlotNone,
     "mining.notify/job sequence mirror", "high",
     "after mining.notify parsing, increments ctx+0x48 then stores it into [ctx+0x78]+0x140"},
    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400f1817, 0x181817, 0x191817,
     kMinerHashrateAccumulatorStartOffset, kSlotNone, kSlotNone,
     "hashrate accumulator stores state+0x148..+0x15c", "high",
     "stores deltas and combined hashrate words used by runtime/log formatters; inspected ranges do not bridge this to Miner slot +0x44"},
    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400f1a17, 0x181a17, 0x191a17,
     kMinerCombinedHashrateLowOffset, kSlotNone, kSlotNone,
     "combined hashrate formatter read", "high",
     "reads state+0x158/+0x15c and writes formatted ctx+0x1dc"},
    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400de20b, 0x16e20b, 0x17e20b,
     kRuntimeSwarmControllerOffset, kSlotNone, kSlotNone,
     "runtime service constructor calls scan/swarm installer", "high",
     "constructor path calls 0x400da778, proving runtime+0x88 is part of normal startup"},
    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400da778, 0x16a778, 0x17a778,
     kRuntimeScanControllerOffset, kSlotNone, kSlotNone,
     "scan/swarm controller installer", "high",
     "stores scan object to runtime+0x84 before constructing the dependent swarm object"},
    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400da894, 0x16a894, 0x17a894,
     kRuntimeSwarmControllerOffset, kSlotNone, kSlotNone,
     "swarm controller install", "high",
     "stores object pointer to runtime+0x88 and links runtime+0x84 scan object at swarm+0x00"},
    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400da90a, 0x16a90a, 0x17a90a,
     kRuntimeSwarmControllerOffset, kSlotNone, kSlotNone,
     "swarm task registration", "high",
     "registers task entry 0x400dd35c with DROM name 0x3f400c7c '(swarm)', stack 0x0c00, priority 5, param runtime+0x88 object"},
    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400dd35c, 0x16d35c, 0x17d35c,
     kRuntimeSwarmControllerOffset, kSlotNone, kSlotNone,
     "swarm task entry", "high",
     "task uses runtime+0x88 object, scan mutex at object+0x00, stop/running bytes +0x50/+0x51, and writes display fields"},
    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400ddb86, 0x16db86, 0x17db86,
     kSwarmControllerAggregateMetricOffset, kSlotNone, kSlotNone,
     "swarm aggregate metric store", "med-low",
     "swarm task stores f0 into object+0x0c; consumed by 0x400d6ee2, exact response-field meaning unresolved"},
    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400ddb8e, 0x16db8e, 0x17db8e,
     kSwarmControllerProbeCountOffset, kSlotNone, kSlotNone,
     "swarm probe/alive count store", "med-high",
     "swarm task increments and stores object+0x08; consumed by 0x400d6ea5"},
    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400ddb90, 0x16db90, 0x17db90,
     kSwarmControllerDisplayMetricOffset, kSlotNone, kSlotNone,
     "swarm display metric store", "med-low",
     "swarm task stores f3 into object+0x10; consumed by 0x400d6f31, exact response-field meaning unresolved"},
    {MinerRuntimeEvidenceKind::WriterCallsite, 0x400ddb93, 0x16db93, 0x17db93,
     kSwarmControllerAuxMetricOffset, kSlotNone, kSlotNone,
     "swarm auxiliary metric store", "low-med",
     "swarm task stores f0 into object+0x14; no direct Miner UI publisher identified in the current trio"},

    {MinerRuntimeEvidenceKind::PublisherCallsite, 0x400d6d55, 0x166d55, 0x176d55,
     kMinerAcceptedShareCountOffset, 0x1f8, 0x3c,
     "accepted/rejected share summary", "high",
     "combines state+0x130/+0x134 into Miner slot +0x3c"},
    {MinerRuntimeEvidenceKind::PublisherCallsite, 0x400d6a9e, 0x166a9e, 0x176a9e,
     kMinerBlockHitCountOffset, 0x234, 0x40,
     "block-hit count", "high",
     "reads state+0x138 and publishes global+0x234 to Miner slot +0x40"},
    {MinerRuntimeEvidenceKind::PublisherCallsite, 0x400d69f9, 0x1669f9, 0x1769f9,
     kClockBtcTickerPriceLowOffset, 0x260, 0x44,
     "runtime+0x80 +0x158/+0x15c formatted stat", "med",
     "inspected ranges disprove a direct mining hashrate bridge: runtime+0x80 is separate from ctx+0x78 and no sync was found"},
    {MinerRuntimeEvidenceKind::PublisherCallsite, 0x400d6ad1, 0x166ad1, 0x176ad1,
     kMinerNotifySequenceCountOffset, 0x154, 0x24,
     "mining notify/job sequence count", "med-high",
     "state+0x140 is written by 0x400f1258; do not alias to accepted count"},
    {MinerRuntimeEvidenceKind::PublisherCallsite, 0x400d6af8, 0x166af8, 0x176af8,
     kMinerNotifySequenceCountOffset, 0x180, 0x30,
     "notify/job sequence change color", "med",
     "color changes when state+0x140 changes; slot/layout meaning remains no-touch"},
    {MinerRuntimeEvidenceKind::PublisherCallsite, 0x400d6b35, 0x166b35, 0x176b35,
     kMinerNetworkDifficultyLowOffset, 0x190, 0x2c,
     "network difficulty", "med",
     "reads state+0x128/+0x12c before FUN_400f9200 formatting"},
    {MinerRuntimeEvidenceKind::PublisherCallsite, 0x400d6cca, 0x166cca, 0x176cca,
     kMinerDifficultyAggregateStartOffset, 0x1bc, 0x34,
     "pool/current difficulty aggregate", "med",
     "formats aggregate values under state+0x110..+0x124"},

    {MinerRuntimeEvidenceKind::TaskObjectPublisher, 0x400d6e1a, 0x166e1a, 0x176e1a,
     kRuntimeSPriceMarketControllerOffset, 0x128, 0x1c,
     "SPrice/WatchCoins market stat from runtime+0x74", "med",
     "stack a1+36 is loaded from runtime+0x74 and stored at 0x400d621b; publishes object+0x04, not Miner runtime state"},
    {MinerRuntimeEvidenceKind::TaskObjectPublisher, 0x400d6e57, 0x166e57, 0x176e57,
     kRuntimeSPriceMarketControllerOffset, 0x144, 0x1c,
     "SPrice/WatchCoins change color from runtime+0x74", "high rule / med object",
     "green/red rule reads runtime+0x74 object+0x08 <= 0; no Miner state edit"},
    {MinerRuntimeEvidenceKind::TaskObjectPublisher, 0x400d6ea5, 0x166ea5, 0x176ea5,
     kRuntimeSwarmControllerOffset, 0x33c, 0x58,
     "bottom-mid swarm probe/alive count from runtime+0x88", "med-high",
     "stack a1+48 is loaded from runtime+0x88 and stored at 0x400d621d; object+0x08 is written by swarm task at 0x400ddb8e"},
    {MinerRuntimeEvidenceKind::TaskObjectPublisher, 0x400d6ee2, 0x166ee2, 0x176ee2,
     kRuntimeSwarmControllerOffset, 0x310, 0x54,
     "bottom-right swarm aggregate metric from runtime+0x88", "med-low",
     "runtime+0x88 object+0x0c is written by swarm task at 0x400ddb86 and formatted by FUN_400f9200 precision 3; exact meaning unresolved/no-touch"},
    {MinerRuntimeEvidenceKind::TaskObjectPublisher, 0x400d6f31, 0x166f31, 0x176f31,
     kRuntimeSwarmControllerOffset, 0x2e4, 0x50,
     "bottom-left swarm display metric from runtime+0x88", "med-low",
     "runtime+0x88 object+0x10 is written by swarm task at 0x400ddb90; 1000.0 threshold remains exact-meaning unresolved/no-touch"},

    {MinerRuntimeEvidenceKind::NonUiRoute, 0x400f1bc2, 0x181bc2, 0x191bc2,
     0, kSlotNone, kSlotNone,
     "Serial/UART telemetry row dispatch", "high",
     "calls FUN_4012d668; log route only, not a UI/event hook"},
    {MinerRuntimeEvidenceKind::NonUiRoute, 0x400f1bd2, 0x181bd2, 0x191bd2,
     0, kSlotNone, kSlotNone,
     "temporary string cleanup sequence", "high",
     "cleanup range 0x400f1bd2..0x400f1c2d via FUN_4012da8c; not a notification path"},
};

constexpr ClockRouteEvidenceSpec kClockRouteEvidence[] = {
    {ClockRouteKind::RuntimeSingleton, 0x400d57c0, 0x1657c0, 0x1757c0, 0, 0,
     "runtime service singleton accessor/init",
     "initializes distinct pointer slots including runtime+0x74, runtime+0x80, runtime+0x84, and runtime+0x88"},
    {ClockRouteKind::TickerCacheField, 0x400daac3, 0x16aac3, 0x17aac3,
     kClockBtcTickerPriceLowOffset, kRuntimeClockBtcTickerCacheOffset,
     "Clock BTC ticker-cache last-price low word zero-init",
     "same cache field later read by the BTC price formatter"},
    {ClockRouteKind::TickerCacheField, 0x400daac9, 0x16aac9, 0x17aac9,
     kClockBtcTickerPriceHighOffset, kRuntimeClockBtcTickerCacheOffset,
     "Clock BTC ticker-cache last-price high word zero-init",
     "same cache field later read by the BTC price formatter"},
    {ClockRouteKind::RuntimeField, 0x400dac58, 0x16ac58, 0x17ac58,
     kRuntimeClockBtcTickerCacheOffset, 0,
     "stores static Clock BTC ticker-cache object pointer to runtime+0x80",
     "separates the Clock BTC source from the SPrice/WatchCoins controller at runtime+0x74"},
    {ClockRouteKind::RuntimeField, 0x400d620a, 0x16620a, 0x17620a,
     kRuntimeClockBtcTickerCacheOffset, 0x28,
     "loads runtime+0x80 before the Clock update block stores stack a1+40",
     "this stack local is the BTC price source object"},
    {ClockRouteKind::RuntimeField, 0x400d6213, 0x166213, 0x176213,
     kRuntimeSPriceMarketControllerOffset, 0x24,
     "loads runtime+0x74 before the Clock update block stores stack a1+36",
     "this is the SPrice/WatchCoins market controller and nearby header-color source"},
    {ClockRouteKind::RuntimeField, 0x400d6216, 0x166216, 0x176216,
     kRuntimeSwarmControllerOffset, 0x30,
     "loads runtime+0x88 before the Clock update block stores stack a1+48",
     "swarm controller source object; no-touch for Clock BTC source/color work"},
    {ClockRouteKind::ProducerCallsite, 0x400d81dc, 0x1681dc, 0x1781dc,
     0x28, kRuntimeClockBtcTickerCacheOffset,
     "loads stack a1+40 as the Clock BTC ticker-cache pointer",
     "feeds the BTC price formatter"},
    {ClockRouteKind::TickerCacheField, 0x400d81e4, 0x1681e4, 0x1781e4,
     kClockBtcTickerPriceLowOffset, kClockBtcTextModelOffset,
     "reads Clock BTC last-price double low word from cache+0x158",
     "direct input to formatted text published at global+0x4f4"},
    {ClockRouteKind::TickerCacheField, 0x400d81ed, 0x1681ed, 0x1781ed,
     kClockBtcTickerPriceHighOffset, kClockBtcTextModelOffset,
     "reads Clock BTC last-price double high word from cache+0x15c",
     "direct input to formatted text published at global+0x4f4"},
    {ClockRouteKind::ProducerCallsite, 0x400d82a1, 0x1682a1, 0x1782a1,
     kClockBtcTickerPriceLowOffset, kClockBtcTextModelOffset,
     "publishes formatted BTC price string through FUN_400d6104(global+0x4f4)",
     "text producer only; not a color writer"},
    {ClockRouteKind::Binding, 0x400f1eb0, 0x181eb0, 0x191eb0,
     kClockBtcTextModelOffset, kClockBtcSlotOffset,
     "global+0x4f4 subscriber updates Clock slot+0x30",
     "bottom-right BTC price label text/layout owner"},
    {ClockRouteKind::ColorPath, 0x400d82c5, 0x1682c5, 0x1782c5,
     kRuntimeSPriceMarketControllerOffset, 0,
     "green/red decision reads the runtime+0x74 side path",
     "not fed by runtime+0x80 Clock BTC ticker-cache"},
    {ClockRouteKind::ColorPath, 0x400d836e, 0x16836e, 0x17836e,
     kRuntimeSPriceMarketControllerOffset, 0x4b8,
     "publishes selected color to global+0x4b8",
     "binds to Clock header slot+0x04, not BTC slot+0x30"},
    {ClockRouteKind::ColorPath, 0x400d8409, 0x168409, 0x178409,
     kRuntimeSPriceMarketControllerOffset, 0x4e4,
     "publishes selected color to global+0x4e4",
     "binds to Clock header slot+0x08, not BTC slot+0x30"},
    {ClockRouteKind::ColorPath, 0x400f421c, 0x18421c, 0x19421c,
     0x4b8, kClockBtcSlotOffset,
     "generic Clock color subscriber helper",
     "checked Clock color publishers do not target global+0x4f4 or slot+0x30"},
};

constexpr MarketConfigSpec kMarketConfigSpecs[] = {
    {"MainCoin",      "BTC", 0x3f400cc2, 0x000cc2, 0x010cc2, 0x3f400ccb, 0x000ccb, 0x010ccb, "main ticker symbol"},
    {"WatchCoins",    "BTC,ETH,BNB,LTC,EOS,ETC,XRP,BCH,ZEC,NEO", 0x3f400ccf, 0x000ccf, 0x010ccf, 0x3f400cda, 0x000cda, 0x010cda, "comma-separated watch list; tokenizer caps at 20 symbols"},
    {"KlineRotate",   "20s", 0x3f400d02, 0x000d02, 0x010d02, 0x3f400d0e, 0x000d0e, 0x010d0e, "kline carousel interval"},
    {"KlineInterval", "1h", 0x3f400d12, 0x000d12, 0x010d12, 0x3f400d20, 0x000d20, 0x010d20, "Binance kline interval"},
    {"PricePageMode", "kline", 0x3f400d23, 0x000d23, 0x010d23, 0x3f400d31, 0x000d31, 0x010d31, "market page mode"},
};

constexpr AddressEvidence kWatchCoinsConfigEvidence[] = {
    {"WatchCoins wrapper",        0x400e7978, 0x177978, 0x187978, "thin wrapper; dereferences pointer args then calls 0x400e7250"},
    {"WatchCoins body",           0x400e798c, 0x17798c, 0x18798c, "real body entry; source-equivalent recovered in SPrice_watchcoins_config.recovered.c"},
    {"WatchCoins literal pool",   0x400d10ec, 0x1610ec, 0x1710ec, "second pool pointer to DROM WatchCoins"},
    {"WatchCoins has-key call",   0x400e7b2d, 0x177b2d, 0x187b2d, "checks whether WatchCoins is present"},
    {"WatchCoins get-string call", 0x400e7b3b, 0x177b3b, 0x187b3b, "fetches WatchCoins string"},
    {"WatchCoins cap constant",   0x400e7b65, 0x177b65, 0x187b65, "20-symbol cap"},
    {"WatchCoins truncate call",  0x400e7b85, 0x177b85, 0x187b85, "substring/truncate before the 21st symbol"},
    {"WatchCoins comma literal",  0x400e7b9b, 0x177b9b, 0x187b9b, "ASCII comma test"},
    {"WatchCoins direct caller",  0x400e7f66, 0x177f66, 0x187f66, "direct call to body 0x400e798c from caller 0x400e7f54"},
    {"Ticker parser",             0x400e3b84, 0x173b84, 0x183b84, "extracts lastPrice and priceChangePercent"},
    {"Ticker wrapper",            0x400e3e84, 0x173e84, 0x183e84, "stores parsed ticker fields into caller object"},
    {"SPrice cache writer",        0x400d73be, 0x1673be, 0x1773be, "downstream consumer of already-built runtime+0x74 source rows"},
};

constexpr AddressEvidence kSPriceRowProducerEvidence[] = {
    {"runtime init clears source",  0x400d5800, 0x165800, 0x175800, "runtime singleton first init zeroes +0x74"},
    {"market object constructor",  0x400dddc8, 0x16ddc8, 0x17ddc8, "initializes static object 0x3ffc4618 row vector and kline metadata"},
    {"runtime+0x74 setter",        0x400ddf61, 0x16df61, 0x17df61, "stores static market object pointer into runtime+0x74"},
    {"market task registration",   0x400ddfcd, 0x16dfcd, 0x17dfcd, "registers task fn 0x400db278 named (market), stack 0x0a00, priority 5, param market object"},
    {"market task entry",          0x400db278, 0x16b278, 0x17b278, "task parameter is runtime+0x74 market object"},
    {"WatchCoins split helper",    0x400db24c, 0x16b24c, 0x17b24c, "comma-split WatchCoins vector helper for 16-byte string elements"},
    {"ticker wrapper call",        0x400db511, 0x16b511, 0x17b511, "market task calls ticker wrapper 0x400e3e84"},
    {"market row prep",            0x400e2c28, 0x172c28, 0x182c28, "upstream parser/fetch path before row rebuild"},
    {"row producer callsite",      0x400dbbd1, 0x16bbd1, 0x17bbd1, "calls row producer with same market object"},
    {"row producer",               0x400e3a80, 0x173a80, 0x183a80, "rebuilds object+0x24/+0x28/+0x2c 24-byte SPrice source rows"},
    {"row copy helper",            0x400e26cc, 0x1726cc, 0x1826cc, "copies row string-like payload plus floats at +16/+20"},
    {"row vector reserve helper",  0x400e27ac, 0x1727ac, 0x1827ac, "reserves 24-byte row-vector capacity"},
    {"row insert/grow helper",     0x400e2844, 0x172844, 0x182844, "inserts row when vector is at capacity"},
    {"row swap/sort helper",       0x400e3894, 0x173894, 0x183894, "swaps row payloads and float fields during row ordering"},
    {"SPrice cache writer",        0x400d73be, 0x1673be, 0x1773be, "copies producer rows into global+0xd94 and count global+0x1244"},
};

constexpr SPricePatchTargetSpec kSPricePatchTargetSpecs[] = {
    {"BIN02-LIT-SPRICE-WATCHCOINS-DEFAULT", "DROM literal", 0x3f400cda, 0x000cda, 0x010cda,
     "BTC,ETH,BNB,LTC,EOS,ETC,XRP,BCH,ZEC,NEO",
     "local literal-patcher eligible; same/shorter ASCII only; defaults-only and NVS/config may override"},
    {"BIN02-LIT-SPRICE-USDT-SUFFIX", "DROM literal", 0x3f4008fb, 0x0008fb, 0x0108fb,
     "USDT", "blocked/no-touch; changes every market pair suffix"},
    {"BIN02-LIT-SPRICE-BINANCE-BATCH-URL", "DROM literal", 0x3f47222c, 0x07222c, 0x08222c,
     "http://data-api.binance.vision:80/api/v3/ticker/24hr?symbols=",
     "blocked/no-touch; network endpoint and parser/trust surface"},
    {"BIN02-LIT-SPRICE-PRICE-PAGE-KLINE", "DROM literal", 0x3f400d31, 0x000d31, 0x010d31,
     "kline", "blocked/no-touch; pricewall does not fit in-place"},
    {"BIN02-PTR-SPRICE-PRICE-PAGE-DEFAULT", "IROM pointer literal", 0x400d0914, 0x160914, 0x170914,
     "31 0d 40 3f", "blocked; shared default pointer to kline"},
    {"BIN02-INS-SPRICE-CACHE-WRITER-PUBLISH-CAP", "IROM instruction", 0x400d7428, 0x167428, 0x177428,
     "3c 24", "blocked except lower-cap research; original movi.n a4, 50"},
    {"BIN02-INS-SPRICE-WATCHCOINS-TOKEN-CAP", "IROM instruction", 0x400e7b65, 0x177b65, 0x187b65,
     "1c 48", "blocked except lower/same-contract research; original movi.n a8, 20"},
    {"BIN02-INS-SPRICE-ROW-TRIM-CAP-A", "IROM instruction", 0x400e390a, 0x17390a, 0x18390a,
     "72 a1 80", "blocked; original movi a7, 0x180"},
    {"BIN02-INS-SPRICE-ROW-TRIM-CAP-B", "IROM instruction", 0x400e3b28, 0x173b28, 0x183b28,
     "22 a1 80", "blocked; original movi a2, 0x180"},
    {"BIN02-CS-SPRICE-BATCH-PARSER-CALL", "IROM callsite", 0x400dbbc9, 0x16bbc9, 0x17bbc9,
     "e5 05 07", "no-go; skipping can leave stale/half-updated state"},
    {"BIN02-CS-SPRICE-ROW-PRODUCER-CALL", "IROM callsite", 0x400dbbd1, 0x16bbd1, 0x17bbd1,
     "e5 ea 07", "no-go; skipping can leave stale/half-updated state"},
    {"BIN02-INS-SPRICE-RUNTIME74-SETTER", "IROM instruction", 0x400ddf61, 0x16df61, 0x17df61,
     "42 62 1d", "no-go structural object/vector path"},
};

constexpr LittleFsImageSpec kLittleFsImageSpecs[] = {
    {LittleFsImageSet::OfficialBundle,
     "firmware/v2.0.02/esp32-2432s028r-st7789/littlefs.bin",
     "700681e91194ca233dfdef62725541f917270604279253c4233e5551bf93a2cf",
     kOfficialLittleFsFlashOffset,
     kOfficialLittleFsLength,
     kOfficialLittleFsBlockSize,
     kOfficialLittleFsBlockCount,
     2,
     "official v2.0.02 ST7789 bundle LittleFS; authoritative baseline"},
    {LittleFsImageSet::RecoveredExpanded,
     "hardware/recovered-assets/spiffs.bin",
     "44adf024f9b3291d55e3aa28d58536bca201cfb4249d95aa375dd32d9693144d",
     kOfficialLittleFsFlashOffset,
     kOfficialLittleFsLength,
     kOfficialLittleFsBlockSize,
     kOfficialLittleFsBlockCount,
     17,
     "observed/recovered expanded LittleFS asset image; valid donor evidence, not byte-identical to official bundle"},
};

constexpr ScreensaverAssetSpec kScreensaverAssetSpecs[] = {
    {LittleFsImageSet::OfficialBundle, "/ss/saver_320_240_001.ss", 69975,
     kScreensaverHeader320x240, "official bundle asset"},
    {LittleFsImageSet::OfficialBundle, "/ss/saver_320_240_002.ss", 83057,
     kScreensaverHeader320x240, "official bundle asset also present in recovered expanded image"},
    {LittleFsImageSet::RecoveredExpanded, "/ss/saver_320_240_002.ss", 83057,
     kScreensaverHeader320x240, "recovered expanded asset"},
    {LittleFsImageSet::RecoveredExpanded, "/ss/saver_320_240_003.ss", 13240,
     kScreensaverHeader320x240, "recovered expanded asset used in local add/replace proof"},
    {LittleFsImageSet::RecoveredExpanded, "/ss/saver_320_240_004.ss", 10334,
     kScreensaverHeader320x240, "recovered expanded asset"},
    {LittleFsImageSet::RecoveredExpanded, "/ss/saver_320_240_005.ss", 13318,
     kScreensaverHeader320x240, "recovered expanded asset"},
    {LittleFsImageSet::RecoveredExpanded, "/ss/saver_320_240_006.ss", 6321,
     kScreensaverHeader320x240, "recovered expanded asset"},
    {LittleFsImageSet::RecoveredExpanded, "/ss/saver_320_240_007.ss", 15316,
     kScreensaverHeader320x240, "recovered expanded asset"},
    {LittleFsImageSet::RecoveredExpanded, "/ss/saver_320_240_008.ss", 4403,
     kScreensaverHeader320x240, "recovered expanded asset"},
    {LittleFsImageSet::RecoveredExpanded, "/ss/saver_320_240_009.ss", 12608,
     kScreensaverHeader320x240, "recovered expanded asset"},
    {LittleFsImageSet::RecoveredExpanded, "/ss/saver_320_240_010.ss", 11489,
     kScreensaverHeader320x240, "recovered expanded asset"},
    {LittleFsImageSet::RecoveredExpanded, "/ss/saver_320_240_011.ss", 14754,
     kScreensaverHeader320x240, "recovered expanded asset"},
    {LittleFsImageSet::RecoveredExpanded, "/ss/saver_320_240_012.ss", 13553,
     kScreensaverHeader320x240, "recovered expanded asset"},
    {LittleFsImageSet::RecoveredExpanded, "/ss/saver_320_240_013.ss", 2685,
     kScreensaverHeader320x240, "recovered expanded asset"},
    {LittleFsImageSet::RecoveredExpanded, "/ss/saver_320_240_014.ss", 1136,
     kScreensaverHeader320x240, "recovered expanded asset"},
    {LittleFsImageSet::RecoveredExpanded, "/ss/saver_320_240_015.ss", 3164,
     kScreensaverHeader320x240, "recovered expanded asset"},
    {LittleFsImageSet::RecoveredExpanded, "/ss/saver_320_240_016.ss", 8359,
     kScreensaverHeader320x240, "recovered expanded asset"},
    {LittleFsImageSet::RecoveredExpanded, "/ss/saver_320_240_017.ss", 15280,
     kScreensaverHeader320x240, "recovered expanded asset"},
    {LittleFsImageSet::RecoveredExpanded, "/ss/saver_320_240_018.ss", 17545,
     kScreensaverHeader320x240, "recovered expanded asset"},
};

}  // namespace

std::optional<MappedAddress> mapVmAddress(uint32_t vaddr)
{
    for (const Segment &segment : kSegments) {
        const uint32_t relative = vaddr - segment.load_address;
        if (vaddr >= segment.load_address && relative < segment.length) {
            const uint32_t app_offset = segment.payload_offset + relative;
            return MappedAddress{
                segment.name,
                vaddr,
                relative,
                app_offset,
                kAppFlashBase + app_offset,
                app_offset < kFactoryPartitionSize,
            };
        }
    }

    return std::nullopt;
}

std::string_view segmentName(SegmentName segment)
{
    switch (segment) {
    case SegmentName::Drom:
        return "DROM";
    case SegmentName::Dram:
        return "DRAM";
    case SegmentName::Iram0:
        return "IRAM0";
    case SegmentName::Irom:
        return "IROM";
    case SegmentName::Iram1:
        return "IRAM1";
    }
    return "unknown";
}

size_t copySPriceRowsFromRuntimeVector(
    ArrayView<const SPriceMarketRow> source_rows,
    std::array<SPriceMarketRow, kSPriceMaxRows> &destination)
{
    if (source_rows.empty() || source_rows.data() == nullptr) {
        return 0;
    }

    const size_t count = std::min(source_rows.size(), destination.size());
    std::copy_n(source_rows.begin(), count, destination.begin());
    return count;
}

std::string_view screenName(ScreenId screen)
{
    switch (screen) {
    case ScreenId::Miner:
        return "Miner320x240";
    case ScreenId::Clock:
        return "Clock320x240";
    case ScreenId::SPrice:
        return "SPrice320x240";
    case ScreenId::Weather:
        return "Weather320x240";
    }
    return "unknown";
}

std::string_view widgetKindName(WidgetKind kind)
{
    switch (kind) {
    case WidgetKind::Label:
        return "label";
    case WidgetKind::DrawArea:
        return "draw_area";
    case WidgetKind::Container:
        return "container";
    case WidgetKind::Line:
        return "line";
    case WidgetKind::Timer:
        return "timer";
    }
    return "unknown";
}

std::string_view publisherKindName(PublisherKind kind)
{
    switch (kind) {
    case PublisherKind::Text:
        return "text";
    case PublisherKind::ValueOrColor:
        return "value_or_color";
    }
    return "unknown";
}

std::string_view minerRuntimeEvidenceKindName(MinerRuntimeEvidenceKind kind)
{
    switch (kind) {
    case MinerRuntimeEvidenceKind::RuntimeStateField:
        return "runtime_state_field";
    case MinerRuntimeEvidenceKind::WriterCallsite:
        return "writer_callsite";
    case MinerRuntimeEvidenceKind::PublisherCallsite:
        return "publisher_callsite";
    case MinerRuntimeEvidenceKind::TaskObjectPublisher:
        return "task_object_publisher";
    case MinerRuntimeEvidenceKind::NonUiRoute:
        return "non_ui_route";
    }
    return "unknown";
}

std::string_view clockRouteKindName(ClockRouteKind kind)
{
    switch (kind) {
    case ClockRouteKind::RuntimeSingleton:
        return "runtime_singleton";
    case ClockRouteKind::RuntimeField:
        return "runtime_field";
    case ClockRouteKind::TickerCacheField:
        return "ticker_cache_field";
    case ClockRouteKind::ProducerCallsite:
        return "producer_callsite";
    case ClockRouteKind::Binding:
        return "binding";
    case ClockRouteKind::ColorPath:
        return "color_path";
    }
    return "unknown";
}

std::string_view startupRouteKindName(StartupRouteKind kind)
{
    switch (kind) {
    case StartupRouteKind::Entrypoint:
        return "entrypoint";
    case StartupRouteKind::EarlyIramInit:
        return "early_iram_init";
    case StartupRouteKind::RomOrIdfHelper:
        return "rom_or_idf_helper";
    case StartupRouteKind::MemoryClear:
        return "memory_clear";
    case StartupRouteKind::SystemConfig:
        return "system_config";
    case StartupRouteKind::ConstructorGate:
        return "constructor_gate";
    case StartupRouteKind::ConstructorSweep:
        return "constructor_sweep";
    case StartupRouteKind::FrameworkHook:
        return "framework_hook";
    case StartupRouteKind::SchedulerHandoff:
        return "scheduler_handoff";
    }
    return "unknown";
}

std::string_view taskRouteKindName(TaskRouteKind kind)
{
    switch (kind) {
    case TaskRouteKind::Helper400d9c68:
        return "helper_400d9c68";
    case TaskRouteKind::HelperAfterNameObject:
        return "helper_after_name_object";
    case TaskRouteKind::NameObjectOnly:
        return "name_object_only";
    }
    return "unknown";
}

std::string_view bringupStepKindName(BringupStepKind kind)
{
    switch (kind) {
    case BringupStepKind::Startup:
        return "startup";
    case BringupStepKind::MemoryInit:
        return "memory_init";
    case BringupStepKind::ConstructorSweep:
        return "constructor_sweep";
    case BringupStepKind::FrameworkHook:
        return "framework_hook";
    case BringupStepKind::BeginRoute:
        return "begin_route";
    case BringupStepKind::HardwareInit:
        return "hardware_init";
    case BringupStepKind::TaskRegistration:
        return "task_registration";
    case BringupStepKind::ReadyLog:
        return "ready_log";
    case BringupStepKind::UnresolvedHandoff:
        return "unresolved_handoff";
    }
    return "unknown";
}

ArrayView<const WidgetSpec> widgetSpecs()
{
    return {kWidgetSpecs, std::size(kWidgetSpecs)};
}

ArrayView<const PublisherSpec> publisherSpecs()
{
    return {kPublisherSpecs, std::size(kPublisherSpecs)};
}

ArrayView<const BindingSpec> bindingSpecs()
{
    return {kBindingSpecs, std::size(kBindingSpecs)};
}

ArrayView<const WeatherForecastTemplateSpec> weatherForecastTemplateSpecs()
{
    return {kWeatherForecastTemplateSpecs, std::size(kWeatherForecastTemplateSpecs)};
}

ArrayView<const WeatherCallbackSpec> weatherCallbackSpecs()
{
    return {kWeatherCallbackSpecs, std::size(kWeatherCallbackSpecs)};
}

ArrayView<const RuntimeEvidence> runtimeEvidence()
{
    return {kRuntimeEvidence, std::size(kRuntimeEvidence)};
}

ArrayView<const MinerRuntimeEvidenceSpec> minerRuntimeEvidence()
{
    return {kMinerRuntimeEvidence, std::size(kMinerRuntimeEvidence)};
}

ArrayView<const ClockRouteEvidenceSpec> clockRouteEvidence()
{
    return {kClockRouteEvidence, std::size(kClockRouteEvidence)};
}

ArrayView<const MarketConfigSpec> marketConfigSpecs()
{
    return {kMarketConfigSpecs, std::size(kMarketConfigSpecs)};
}

ArrayView<const AddressEvidence> watchCoinsConfigEvidence()
{
    return {kWatchCoinsConfigEvidence, std::size(kWatchCoinsConfigEvidence)};
}

ArrayView<const AddressEvidence> spriceRowProducerEvidence()
{
    return {kSPriceRowProducerEvidence, std::size(kSPriceRowProducerEvidence)};
}

ArrayView<const AppLifecycleEvidenceSpec> appLifecycleEvidence()
{
    return {kAppLifecycleEvidence, std::size(kAppLifecycleEvidence)};
}

ArrayView<const StartupLiteralSpec> startupLiterals()
{
    return {kStartupLiteralSpecs, std::size(kStartupLiteralSpecs)};
}

ArrayView<const StartupStepSpec> startupSteps()
{
    return {kStartupStepSpecs, std::size(kStartupStepSpecs)};
}

ArrayView<const TaskRegistrationEvidenceSpec> taskRegistrationEvidence()
{
    return {kTaskRegistrationEvidence, std::size(kTaskRegistrationEvidence)};
}

ArrayView<const OfficialBringupStepSpec> officialBringupSteps()
{
    return {kOfficialBringupSteps, std::size(kOfficialBringupSteps)};
}

ArrayView<const RuntimeServiceSlotSpec> runtimeServiceSlots()
{
    return {kRuntimeServiceSlots, std::size(kRuntimeServiceSlots)};
}

ArrayView<const SourceBaselineGateSpec> sourceBaselineGates()
{
    return {kSourceBaselineGates, std::size(kSourceBaselineGates)};
}

OfficialAppModelSummary summarizeOfficialAppModel()
{
    OfficialAppModelSummary summary{};
    summary.bringup_steps = std::size(kOfficialBringupSteps);
    summary.task_registrations = std::size(kTaskRegistrationEvidence);
    summary.runtime_slots = std::size(kRuntimeServiceSlots);
    summary.total_gates = std::size(kSourceBaselineGates);

    for (const SourceBaselineGateSpec &gate : kSourceBaselineGates) {
        if (gate.satisfied) {
            ++summary.closed_gates;
        }
        if (std::string_view{gate.gate} == "flash approval" && !gate.satisfied) {
            summary.flash_gate_closed = true;
        }
    }

    for (const OfficialBringupStepSpec &step : kOfficialBringupSteps) {
        if (step.vm == kOfficialAppEntryVm) {
            summary.has_entrypoint = true;
        } else if (step.vm == 0x400de1e4u) {
            summary.has_begin_route = true;
        } else if (step.vm == 0x400de25du) {
            summary.has_all_tasks_started_log = true;
        }
    }

    bool has_tminer = false;
    bool has_nminer_helper = false;
    for (const TaskRegistrationEvidenceSpec &task : kTaskRegistrationEvidence) {
        const std::string_view name{task.name};
        if (name == "(scan)") {
            summary.has_scan_task = true;
        } else if (name == "(swarm)") {
            summary.has_swarm_task = true;
        } else if (name == "(stratum)") {
            summary.has_stratum_task = true;
        } else if (name == "(tMiner)") {
            has_tminer = true;
        } else if (name == "(nMiner)" &&
                   task.kind == TaskRouteKind::HelperAfterNameObject) {
            has_nminer_helper = true;
        }
    }
    summary.has_miner_tasks = has_tminer && has_nminer_helper;

    return summary;
}

ArrayView<const SPricePatchTargetSpec> spricePatchTargetSpecs()
{
    return {kSPricePatchTargetSpecs, std::size(kSPricePatchTargetSpecs)};
}

ArrayView<const LittleFsImageSpec> littleFsImageSpecs()
{
    return {kLittleFsImageSpecs, std::size(kLittleFsImageSpecs)};
}

ArrayView<const ScreensaverAssetSpec> screensaverAssetSpecs()
{
    return {kScreensaverAssetSpecs, std::size(kScreensaverAssetSpecs)};
}

}  // namespace nmminer::recovered::v2002_st7789
