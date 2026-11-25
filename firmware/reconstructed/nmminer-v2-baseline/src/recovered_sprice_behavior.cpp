#include "nmminer_recovered/official_v2002_st7789.h"

#include <algorithm>
#include <cstring>

namespace nmminer::recovered::v2002_st7789 {
namespace {

void copyDisplaySymbol(char (&destination)[kWatchCoinsDisplayBytes],
                       const char *source,
                       size_t length)
{
    std::fill(destination, destination + kWatchCoinsDisplayBytes, '\0');
    if (source == nullptr) {
        return;
    }

    const size_t copy_length = std::min(length, kWatchCoinsTokenBytes);
    std::memcpy(destination, source, copy_length);
}

void copyRowPayload(char (&destination)[kWatchCoinsTokenBytes],
                    const char *source,
                    size_t length)
{
    std::fill(destination, destination + kWatchCoinsTokenBytes, '\0');
    if (source == nullptr) {
        return;
    }

    const size_t copy_length = std::min(length, kWatchCoinsTokenBytes);
    std::memcpy(destination, source, copy_length);
}

constexpr std::string_view kCapSample =
    "A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V";

constexpr std::array<std::string_view, 10> kDefaultWatchCoinsTokens = {{
    "BTC", "ETH", "BNB", "LTC", "EOS", "ETC", "XRP", "BCH", "ZEC", "NEO",
}};

size_t boundedSymbolLength(const char *source)
{
    size_t length = 0;
    while (length < kWatchCoinsTokenBytes && source[length] != '\0') {
        ++length;
    }
    return length;
}

bool tokenMatches(const WatchCoinsToken &token, std::string_view expected)
{
    return token.source_length == expected.size() &&
           std::memcmp(token.symbol, expected.data(), expected.size()) == 0;
}

bool defaultWatchCoinsMatchesEvidence(const WatchCoinsParseResult &parse)
{
    if (parse.token_count != kDefaultWatchCoinsTokens.size() ||
        parse.truncate_at != defaultWatchCoinsList().size()) {
        return false;
    }

    for (size_t i = 0; i < kDefaultWatchCoinsTokens.size(); ++i) {
        if (!tokenMatches(parse.tokens[i], kDefaultWatchCoinsTokens[i])) {
            return false;
        }
    }

    return true;
}

}  // namespace

std::string_view defaultWatchCoinsList()
{
    for (const MarketConfigSpec &spec : marketConfigSpecs()) {
        if (std::string_view{spec.key} == "WatchCoins") {
            return spec.default_value;
        }
    }

    return {};
}

WatchCoinsParseResult parseWatchCoinsList(const char *watchcoins, size_t length)
{
    WatchCoinsParseResult result{};
    result.truncate_at = length;

    if (watchcoins == nullptr) {
        result.input_was_null = true;
        return result;
    }

    size_t token_start = 0;
    size_t i = 0;

    while (i <= length) {
        if (i == length || watchcoins[i] == ',') {
            if (result.token_count < result.tokens.size()) {
                WatchCoinsToken &token = result.tokens[result.token_count];
                token.source_length = i - token_start;
                token.fits_recovered_payload =
                    token.source_length <= kWatchCoinsTokenBytes;
                copyDisplaySymbol(token.symbol,
                                  watchcoins + token_start,
                                  token.source_length);
            }

            ++result.token_count;
            if (result.token_count == kWatchCoinsMaxSymbols) {
                result.truncate_at = i;
                break;
            }

            token_start = i + 1;
        }
        ++i;
    }

    result.hit_symbol_cap = result.token_count >= kWatchCoinsMaxSymbols;
    result.had_trailing_tokens = result.truncate_at < length;
    return result;
}

WatchCoinsParseResult parseWatchCoinsList(std::string_view watchcoins)
{
    static constexpr char kEmptyWatchCoins[] = "";
    return parseWatchCoinsList(watchcoins.empty() ? kEmptyWatchCoins : watchcoins.data(),
                               watchcoins.size());
}

size_t adaptSPriceTickerRecordsToRows(
    ArrayView<const SPriceTickerRecord> source_records,
    std::array<SPriceMarketRow, kSPriceMaxRows> &destination)
{
    if (source_records.empty() || source_records.data() == nullptr) {
        return 0;
    }

    const size_t count = std::min(source_records.size(), destination.size());
    for (size_t i = 0; i < count; ++i) {
        copyRowPayload(destination[i].label,
                       source_records[i].symbol,
                       boundedSymbolLength(source_records[i].symbol));
        destination[i].value_or_price = source_records[i].last_price;
        destination[i].change_percent = source_records[i].price_change_percent;
    }

    return count;
}

bool spriceRecoveredPricePrecedes(const SPriceMarketRow &lhs,
                                  const SPriceMarketRow &rhs)
{
    return lhs.value_or_price > rhs.value_or_price;
}

size_t orderAndTrimSPriceRowsForRecoveredRender(
    ArrayView<const SPriceMarketRow> source_rows,
    std::array<SPriceMarketRow, kSPriceRenderedRowLimit> &destination)
{
    destination.fill(SPriceMarketRow{});

    if (source_rows.empty() || source_rows.data() == nullptr) {
        return 0;
    }

    std::array<SPriceMarketRow, kSPriceMaxRows> ordered = {};
    const size_t candidate_count = std::min(source_rows.size(), ordered.size());
    std::copy_n(source_rows.data(), candidate_count, ordered.begin());
    std::sort(ordered.begin(),
              ordered.begin() + static_cast<std::ptrdiff_t>(candidate_count),
              spriceRecoveredPricePrecedes);

    const size_t rendered_count = std::min(candidate_count, destination.size());
    std::copy_n(ordered.begin(), rendered_count, destination.begin());
    return rendered_count;
}

SPriceBehaviorSummary summarizeSPriceBehaviorRecovery()
{
    const WatchCoinsParseResult default_parse =
        parseWatchCoinsList(defaultWatchCoinsList());
    const WatchCoinsParseResult cap_parse = parseWatchCoinsList(kCapSample);

    std::array<SPriceMarketRow, kSPriceMaxRows> rows = {};
    const SPriceTickerRecord records[] = {
        {"ETH", 2500.0f, -0.50f},
        {"BTC", 105000.0f, 1.25f},
        {"XEC", 0.00002f, -0.70f},
    };
    const size_t adapted = adaptSPriceTickerRecordsToRows(
        ArrayView<const SPriceTickerRecord>{records, 3},
        rows);
    std::array<SPriceMarketRow, kSPriceRenderedRowLimit> ordered = {};
    const size_t ordered_rows = orderAndTrimSPriceRowsForRecoveredRender(
        ArrayView<const SPriceMarketRow>{rows.data(), adapted},
        ordered);

    const bool adapted_payload_matches =
        adapted == 3 &&
        std::memcmp(rows[0].label, records[0].symbol, kWatchCoinsTokenBytes) == 0 &&
        rows[0].value_or_price == records[0].last_price &&
        rows[0].change_percent == records[0].price_change_percent &&
        sizeof(SPriceMarketRow) == 24 &&
        kSPriceOrderingKeyOffset == 16;
    const bool ordered_payload_matches =
        ordered_rows == adapted &&
        std::memcmp(ordered[0].label, records[1].symbol, 3) == 0 &&
        ordered[0].value_or_price == records[1].last_price &&
        ordered[1].value_or_price == records[0].last_price &&
        ordered[2].value_or_price == records[2].last_price;

    return SPriceBehaviorSummary{
        default_parse.token_count,
        default_parse.truncate_at,
        cap_parse.token_count,
        cap_parse.truncate_at,
        adapted,
        ordered_rows,
        kSPriceRenderedRowLimit,
        defaultWatchCoinsMatchesEvidence(default_parse),
        cap_parse.token_count == kWatchCoinsMaxSymbols &&
            cap_parse.had_trailing_tokens,
        adapted_payload_matches,
        ordered_payload_matches,
        kSPriceRenderedRowLimit == 16,
        false,
    };
}

}  // namespace nmminer::recovered::v2002_st7789
