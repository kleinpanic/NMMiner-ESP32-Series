#include "nmminer_recovered/official_v2002_st7789.h"

#include <array>
#include <cassert>
#include <cstring>
#include <string_view>

namespace rec = nmminer::recovered::v2002_st7789;

int main()
{
    static_assert(rec::kSPriceRenderedRowBytes == 0x180);
    static_assert(rec::kSPriceRenderedRowLimit == 16);
    static_assert(rec::kSPriceOrderingKeyOffset == 16);

    const rec::WatchCoinsParseResult null_parse =
        rec::parseWatchCoinsList(nullptr, 12);
    assert(null_parse.input_was_null);
    assert(null_parse.token_count == 0);
    assert(null_parse.truncate_at == 12);

    const rec::WatchCoinsParseResult empty_parse =
        rec::parseWatchCoinsList(std::string_view{});
    assert(!empty_parse.input_was_null);
    assert(empty_parse.token_count == 1);
    assert(empty_parse.truncate_at == 0);

    const rec::WatchCoinsParseResult default_parse =
        rec::parseWatchCoinsList(rec::defaultWatchCoinsList());
    assert(default_parse.token_count == 10);
    assert(default_parse.truncate_at == rec::defaultWatchCoinsList().size());
    assert(std::strcmp(default_parse.tokens[0].symbol, "BTC") == 0);
    assert(std::strcmp(default_parse.tokens[9].symbol, "NEO") == 0);

    constexpr std::string_view exactly_20 =
        "A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T";
    const rec::WatchCoinsParseResult exact_parse =
        rec::parseWatchCoinsList(exactly_20);
    assert(exact_parse.token_count == rec::kWatchCoinsMaxSymbols);
    assert(exact_parse.truncate_at == exactly_20.size());
    assert(!exact_parse.had_trailing_tokens);
    assert(std::strcmp(exact_parse.tokens[19].symbol, "T") == 0);

    constexpr std::string_view overflow =
        "A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U";
    const rec::WatchCoinsParseResult overflow_parse =
        rec::parseWatchCoinsList(overflow);
    assert(overflow_parse.token_count == rec::kWatchCoinsMaxSymbols);
    assert(overflow_parse.truncate_at == exactly_20.size());
    assert(overflow_parse.had_trailing_tokens);
    assert(std::strcmp(overflow_parse.tokens[19].symbol, "T") == 0);

    const rec::WatchCoinsParseResult long_token_parse =
        rec::parseWatchCoinsList(std::string_view{"ABCDEFGHIJKLMNOP,ZZZ"});
    assert(long_token_parse.tokens[0].source_length == rec::kWatchCoinsTokenBytes);
    assert(long_token_parse.tokens[0].fits_recovered_payload);
    assert(long_token_parse.tokens[0].symbol[rec::kWatchCoinsTokenBytes] == '\0');

    std::array<rec::SPriceMarketRow, rec::kSPriceMaxRows> rows = {};
    const size_t invalid_adapted = rec::adaptSPriceTickerRecordsToRows(
        rec::ArrayView<const rec::SPriceTickerRecord>{nullptr, 1},
        rows);
    assert(invalid_adapted == 0);

    rec::SPriceTickerRecord record = {};
    for (size_t i = 0; i < rec::kWatchCoinsTokenBytes; ++i) {
        record.symbol[i] = static_cast<char>('A' + i);
    }
    record.last_price = 123.5f;
    record.price_change_percent = -2.25f;

    const size_t adapted = rec::adaptSPriceTickerRecordsToRows(
        rec::ArrayView<const rec::SPriceTickerRecord>{&record, 1},
        rows);
    assert(adapted == 1);
    assert(std::memcmp(rows[0].label, record.symbol, rec::kWatchCoinsTokenBytes) == 0);
    assert(rows[0].value_or_price == record.last_price);
    assert(rows[0].change_percent == record.price_change_percent);

    std::array<rec::SPriceMarketRow, rec::kSPriceMaxRows> unordered_rows = {};
    for (size_t i = 0; i < rec::kSPriceRenderedRowLimit + 1; ++i) {
        unordered_rows[i].label[0] = static_cast<char>('A' + i);
        unordered_rows[i].value_or_price = static_cast<float>(i + 1);
        unordered_rows[i].change_percent =
            (i % 2 == 0) ? -static_cast<float>(i) : static_cast<float>(i);
    }

    assert(rec::spriceRecoveredPricePrecedes(unordered_rows[16], unordered_rows[0]));
    assert(!rec::spriceRecoveredPricePrecedes(unordered_rows[0], unordered_rows[16]));

    std::array<rec::SPriceMarketRow, rec::kSPriceRenderedRowLimit> rendered_rows = {};
    const size_t rendered = rec::orderAndTrimSPriceRowsForRecoveredRender(
        rec::ArrayView<const rec::SPriceMarketRow>{unordered_rows.data(),
                                                   rec::kSPriceRenderedRowLimit + 1},
        rendered_rows);
    assert(rendered == rec::kSPriceRenderedRowLimit);
    assert(rendered_rows[0].label[0] == 'Q');
    assert(rendered_rows[0].value_or_price == 17.0f);
    assert(rendered_rows[15].label[0] == 'B');
    assert(rendered_rows[15].value_or_price == 2.0f);

    const size_t invalid_rendered = rec::orderAndTrimSPriceRowsForRecoveredRender(
        rec::ArrayView<const rec::SPriceMarketRow>{nullptr, 1},
        rendered_rows);
    assert(invalid_rendered == 0);

    std::array<rec::SPriceMarketRow, rec::kSPriceMaxRows> cache = {};
    const size_t invalid_copied = rec::copySPriceRowsFromRuntimeVector(
        rec::ArrayView<const rec::SPriceMarketRow>{nullptr, 1},
        cache);
    assert(invalid_copied == 0);

    return 0;
}
