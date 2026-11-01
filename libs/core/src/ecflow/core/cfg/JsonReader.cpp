// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/core/cfg/JsonReader.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "ecflow/core/cfg/Configuration.hpp"

namespace ecf::cfg {

namespace {

using json = nlohmann::json;

// Escapes a key as a reference token of a JSON Pointer (RFC 6901)
std::string escape(const std::string& key) {
    std::string escaped;
    for (const char c : key) {
        if (c == '~') {
            escaped += "~0";
        }
        else if (c == '/') {
            escaped += "~1";
        }
        else {
            escaped += c;
        }
    }
    return escaped;
}

// Returns the decimal order of magnitude of a JSON number, e.g. 2 for "123.4", -3 for "0.00123" and 400 for "1e400",
// or 0 for zero; the exponent is clamped, as only the sign of the order matters
std::int64_t order_of(const std::string& text) {
    const auto begin   = std::size_t{text[0] == '-' ? 1U : 0U};
    const auto point   = text.find('.', begin);
    const auto e       = text.find_first_of("eE", begin);
    const auto int_end = std::min(point, e);
    const auto integer = text.substr(begin, int_end == std::string::npos ? std::string::npos : int_end - begin);
    std::int64_t order = 0;
    if (integer != "0") {
        order = static_cast<std::int64_t>(integer.size()) - 1; // JSON integer parts have no leading zeros
    }
    else {
        const auto fraction = point == std::string::npos ? std::string{} : text.substr(point + 1, e - point - 1);
        const auto first    = fraction.find_first_not_of('0');
        if (first == std::string::npos) {
            return 0;
        }
        order = -static_cast<std::int64_t>(first) - 1;
    }
    if (e != std::string::npos) {
        auto i              = e + 1;
        const bool negative = text[i] == '-';
        i += (text[i] == '-' || text[i] == '+') ? 1 : 0;
        std::int64_t exponent = 0;
        for (; i < text.size() && exponent < 1000000; ++i) {
            exponent = exponent * 10 + (text[i] - '0');
        }
        order += negative ? -exponent : exponent;
    }
    return order;
}

// Tells whether a JSON number overflows a double, without inspecting a floating-point value: compilers assuming
// finite math (e.g. -ffinite-math-only, implied by the default -fp-model=fast of older icpx) may optimise away any
// such inspection. The library reports a number out of range, including an underflow with libc++; the order of
// magnitude tells an overflow from an underflow.
bool overflows(const std::string& text) {
    std::istringstream in(text);
    in.imbue(std::locale::classic());
    double value = 0;
    in >> value;
    return in.fail() && order_of(text) > 0;
}

// Builds the tree from the events of the SAX parser, refusing duplicate keys and numbers out of range
class TreeBuilder {
public:
    explicit TreeBuilder(std::string_view origin)
        : origin_{origin} {}

    bool null() { return add(CTree::Null{}); }

    bool boolean(bool value) { return add(value); }

    bool number_integer(json::number_integer_t value) { return add(std::int64_t{value}); }

    bool number_unsigned(json::number_unsigned_t value) {
        if (value > static_cast<json::number_unsigned_t>(std::numeric_limits<std::int64_t>::max())) {
            return refuse_integer(std::to_string(value));
        }
        return add(static_cast<std::int64_t>(value));
    }

    bool number_float(json::number_float_t value, const json::string_t& text) {
        // The parser reads an integer too large for 64 bits as a floating-point number
        if (text.find_first_of(".eE") == std::string::npos) {
            return refuse_integer(text);
        }
        // Normally refused by the parser already, unless its check was optimised away
        if (overflows(text)) {
            error_ = located(next_path()) + "number " + text + " overflows a double-precision floating-point number";
            return false;
        }
        return add(double{value});
    }

    bool string(json::string_t& value) { return add(std::move(value)); }

    bool binary(json::binary_t&) {
        // Never produced when parsing JSON text
        return false;
    }

    bool start_object(std::size_t) {
        frames_.push_back(Frame{next_path(), CTree::Mapping{}, {}});
        return true;
    }

    bool key(json::string_t& key) {
        auto& frame          = frames_.back();
        const auto& mapping  = std::get<CTree::Mapping>(frame.container);
        const auto duplicate = [&key](const auto& entry) { return entry.first == key; };
        if (std::find_if(mapping.begin(), mapping.end(), duplicate) != mapping.end()) {
            error_ = located(frame.path + "/" + escape(key)) + "duplicate key '" + key + "'";
            return false;
        }
        frame.key = std::move(key);
        return true;
    }

    bool end_object() { return close(); }

    bool start_array(std::size_t) {
        frames_.push_back(Frame{next_path(), CTree::Sequence{}, {}});
        return true;
    }

    bool end_array() { return close(); }

    bool parse_error(std::size_t, const std::string&, const nlohmann::detail::exception& e) {
        // Drop the identifier of the exception, e.g. "[json.exception.parse_error.101] "
        std::string message = e.what();
        if (const auto end = message.find("] "); message.rfind('[', 0) == 0 && end != std::string::npos) {
            message.erase(0, end + 2);
        }
        error_ = origin_ + ": " + message;
        return false;
    }

    [[nodiscard]] const std::string& error() const { return error_; }

    [[nodiscard]] CTree tree() && { return std::move(*root_); }

private:
    struct Frame
    {
        std::string path;
        CTree::Node container; // a Mapping or a Sequence
        std::string key;       // the key of the next value, in a Mapping
    };

    std::string origin_;
    std::vector<Frame> frames_;
    std::optional<CTree> root_;
    std::string error_;

    std::string next_path() const {
        if (frames_.empty()) {
            return "";
        }
        const auto& frame = frames_.back();
        if (const auto* sequence = std::get_if<CTree::Sequence>(&frame.container)) {
            return frame.path + "/" + std::to_string(sequence->size());
        }
        return frame.path + "/" + escape(frame.key);
    }

    std::string located(const std::string& path) const {
        return path.empty() ? origin_ + ": " : origin_ + ": " + path + ": ";
    }

    bool refuse_integer(const std::string& text) {
        error_ = located(next_path()) + "integer " + text + " is out of the range of a 64-bit signed integer";
        return false;
    }

    bool add(CTree::Node node) {
        place(CTree(next_path(), std::move(node)));
        return true;
    }

    bool close() {
        auto frame = std::move(frames_.back());
        frames_.pop_back();
        place(CTree(std::move(frame.path), std::move(frame.container)));
        return true;
    }

    void place(CTree tree) {
        if (frames_.empty()) {
            root_ = std::move(tree);
            return;
        }
        auto& frame = frames_.back();
        if (auto* sequence = std::get_if<CTree::Sequence>(&frame.container)) {
            sequence->push_back(std::move(tree));
        }
        else {
            std::get<CTree::Mapping>(frame.container).emplace_back(std::move(frame.key), std::move(tree));
        }
    }
};

} // namespace

CTree JsonReader::read(std::string_view text, std::string_view origin) const {
    TreeBuilder builder(origin);
    // Strict: content after the document is refused; comments are not ignored, and thus refused
    const bool strict          = true;
    const bool ignore_comments = false;
    if (!json::sax_parse(text.begin(), text.end(), &builder, json::input_format_t::json, strict, ignore_comments)) {
        throw ConfigurationError(builder.error());
    }
    return std::move(builder).tree();
}

} // namespace ecf::cfg
