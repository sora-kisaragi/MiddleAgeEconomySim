#include "decision/decision_response_json.h"

#include <array>
#include <cstdint>
#include <map>
#include <stdexcept>

namespace economy {
namespace {
[[noreturn]] void invalid() { throw std::invalid_argument("invalid decision.v1 JSON response"); }

void appendUtf8(std::string& output, std::uint32_t cp) {
    if (cp <= 0x7f) {
        output.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7ff) {
        output.push_back(static_cast<char>(0xc0 | (cp >> 6)));
        output.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
    } else if (cp <= 0xffff) {
        output.push_back(static_cast<char>(0xe0 | (cp >> 12)));
        output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
    } else {
        output.push_back(static_cast<char>(0xf0 | (cp >> 18)));
        output.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
    }
}

class Parser {
public:
    explicit Parser(const std::string& input) : input_(input) {
        if (input_.size() > 4096) invalid();
    }

    std::map<std::string, std::string> object() {
        space();
        expect('{');
        std::map<std::string, std::string> fields;
        space();
        if (take('}')) invalid();
        for (;;) {
            space();
            const auto key = string();
            space();
            expect(':');
            space();
            const auto value = string();
            if (!fields.emplace(key, value).second || fields.size() > 5) invalid();
            space();
            if (take('}')) break;
            expect(',');
        }
        space();
        if (index_ != input_.size()) invalid();
        return fields;
    }

private:
    const std::string& input_;
    std::size_t index_ = 0;

    bool take(char c) {
        if (index_ < input_.size() && input_[index_] == c) {
            ++index_;
            return true;
        }
        return false;
    }

    void expect(char c) { if (!take(c)) invalid(); }

    void space() {
        while (index_ < input_.size()) {
            const char c = input_[index_];
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') break;
            ++index_;
        }
    }

    std::uint32_t hex4() {
        std::uint32_t value = 0;
        for (unsigned int i = 0; i < 4; ++i) {
            if (index_ == input_.size()) invalid();
            const char c = input_[index_++];
            value <<= 4;
            if (c >= '0' && c <= '9') value |= static_cast<std::uint32_t>(c - '0');
            else if (c >= 'a' && c <= 'f') value |= static_cast<std::uint32_t>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') value |= static_cast<std::uint32_t>(c - 'A' + 10);
            else invalid();
        }
        return value;
    }

    std::string string() {
        expect('"');
        std::string value;
        while (index_ < input_.size()) {
            const auto byte = static_cast<unsigned char>(input_[index_++]);
            if (byte == '"') return value;
            if (byte == '\\') {
                if (index_ == input_.size()) invalid();
                const char esc = input_[index_++];
                switch (esc) {
                    case '"': value.push_back('"'); break;
                    case '\\': value.push_back('\\'); break;
                    case '/': value.push_back('/'); break;
                    case 'b': value.push_back('\b'); break;
                    case 'f': value.push_back('\f'); break;
                    case 'n': value.push_back('\n'); break;
                    case 'r': value.push_back('\r'); break;
                    case 't': value.push_back('\t'); break;
                    case 'u': {
                        auto cp = hex4();
                        if (cp >= 0xd800 && cp <= 0xdbff) {
                            expect('\\');
                            expect('u');
                            const auto low = hex4();
                            if (low < 0xdc00 || low > 0xdfff) invalid();
                            cp = 0x10000 + ((cp - 0xd800) << 10) + (low - 0xdc00);
                        } else if (cp >= 0xdc00 && cp <= 0xdfff) invalid();
                        appendUtf8(value, cp);
                        break;
                    }
                    default: invalid();
                }
            } else if (byte < 0x20) {
                invalid();
            } else if (byte < 0x80) {
                value.push_back(static_cast<char>(byte));
            } else {
                // Verify unescaped UTF-8, including shortest form and scalar range.
                unsigned int count = 0;
                std::uint32_t cp = 0;
                std::uint32_t minimum = 0;
                if (byte >= 0xc2 && byte <= 0xdf) { count = 1; cp = byte & 0x1f; minimum = 0x80; }
                else if (byte >= 0xe0 && byte <= 0xef) { count = 2; cp = byte & 0x0f; minimum = 0x800; }
                else if (byte >= 0xf0 && byte <= 0xf4) { count = 3; cp = byte & 0x07; minimum = 0x10000; }
                else invalid();
                if (index_ + count > input_.size()) invalid();
                for (unsigned int i = 0; i < count; ++i) {
                    const auto tail = static_cast<unsigned char>(input_[index_++]);
                    if ((tail & 0xc0) != 0x80) invalid();
                    cp = (cp << 6) | (tail & 0x3f);
                }
                if (cp < minimum || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) invalid();
                appendUtf8(value, cp);
            }
        }
        invalid();
    }
};

ReasonCode parseReason(const std::string& value) {
    constexpr std::array<const char*, 11> names = {
        "FOOD_PRIORITY", "PREFERENCE", "WORK_INCOME", "RESTOCK", "MARGIN",
        "CASH_RESERVE", "FINANCE_GAP", "PRODUCTION_TARGET", "PRICE_TARGET",
        "OPERATING_CONDITION", "NO_ACTION"};
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (value == names[i]) return static_cast<ReasonCode>(i);
    }
    invalid();
}
} // namespace

DecisionResponse parseDecisionResponseJson(const std::string& json) {
    const auto fields = Parser(json).object();
    if (fields.size() != 5 || !fields.count("schema_version") ||
        !fields.count("decision_id") || !fields.count("input_hash") ||
        !fields.count("candidate_id") || !fields.count("reason_code")) invalid();
    const auto& version = fields.at("schema_version");
    const auto& decision_id = fields.at("decision_id");
    const auto& input_hash = fields.at("input_hash");
    const auto& candidate_id = fields.at("candidate_id");
    if (version != "decision.v1" || !validDecisionId(decision_id) ||
        !validHash(input_hash) || !validDecisionId(candidate_id)) invalid();
    return {version, decision_id, input_hash, candidate_id, parseReason(fields.at("reason_code"))};
}

} // namespace economy
