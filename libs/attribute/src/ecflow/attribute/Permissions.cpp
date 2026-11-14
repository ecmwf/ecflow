// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/attribute/Permissions.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace ecf {

namespace {

constexpr char sticky_letter = 's';

struct RoleLetter
{
    Role role;
    char letter;
};

// The canonical order of the letters; a new role is listed here, with its letter
constexpr std::array<RoleLetter, static_cast<std::size_t>(Role::Count)> role_letters{{{Role::Read, 'r'},
                                                                                      {Role::Write, 'w'},
                                                                                      {Role::Execute, 'x'},
                                                                                      {Role::Log, 'l'},
                                                                                      {Role::Owner, 'o'},
                                                                                      {Role::Permissions, 'p'}}};

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error("Invalid Permissions: " + message);
}

std::string in_quotes(std::string_view text) {
    return "'" + std::string(text) + "'";
}

Permissions::Entry make_entry(std::string_view user, std::string_view letters) {
    if (std::string error; !is_valid_user_name(user, error)) {
        fail(error);
    }

    Permissions::Entry entry{std::string(user), Roles{}, false};
    std::string seen;
    for (char c : letters) {
        if (seen.find(c) != std::string::npos) {
            fail("letter " + in_quotes(std::string(1, c)) + " repeated in the entry of user " + in_quotes(user));
        }
        seen += c;

        if (c == sticky_letter) {
            entry.sticky = true;
            continue;
        }
        auto found = std::find_if(
            role_letters.begin(), role_letters.end(), [c](const RoleLetter& rl) { return rl.letter == c; });
        if (found == role_letters.end()) {
            fail("unknown letter " + in_quotes(std::string(1, c)) + " in the entry of user " + in_quotes(user));
        }
        entry.roles.add(found->role);
    }
    return entry;
}

// Returns whether the text is well-formed UTF-8: no overlong form, no surrogate, nothing above U+10FFFF
bool is_valid_utf8(std::string_view text) {
    std::size_t i = 0;
    while (i < text.size()) {
        const auto c       = static_cast<unsigned char>(text[i]);
        std::size_t length = 0;
        std::uint32_t code = 0;
        std::uint32_t min  = 0;
        if (c < 0x80) {
            ++i;
            continue;
        }
        if ((c & 0xE0) == 0xC0) {
            length = 2, code = c & 0x1F, min = 0x80;
        }
        else if ((c & 0xF0) == 0xE0) {
            length = 3, code = c & 0x0F, min = 0x800;
        }
        else if ((c & 0xF8) == 0xF0) {
            length = 4, code = c & 0x07, min = 0x10000;
        }
        else {
            return false;
        }
        if (i + length > text.size()) {
            return false;
        }
        for (std::size_t k = 1; k < length; ++k) {
            const auto cc = static_cast<unsigned char>(text[i + k]);
            if ((cc & 0xC0) != 0x80) {
                return false;
            }
            code = (code << 6) | (cc & 0x3F);
        }
        if (code < min || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) {
            return false;
        }
        i += length;
    }
    return true;
}

} // namespace

const std::array<Role, static_cast<std::size_t>(Role::Count)>& all_roles() {
    static const auto roles = [] {
        std::array<Role, static_cast<std::size_t>(Role::Count)> result{};
        std::transform(
            role_letters.begin(), role_letters.end(), result.begin(), [](const RoleLetter& rl) { return rl.role; });
        return result;
    }();
    return roles;
}

char letter_of(Role role) {
    auto found = std::find_if(
        role_letters.begin(), role_letters.end(), [role](const RoleLetter& rl) { return rl.role == role; });
    return found == role_letters.end() ? '?' : found->letter;
}

Roles::Roles(std::initializer_list<Role> roles) {
    for (auto role : roles) {
        add(role);
    }
}

std::string Roles::to_string() const {
    std::string letters;
    for (const auto& rl : role_letters) {
        if (has(rl.role)) {
            letters += rl.letter;
        }
    }
    return letters;
}

bool is_valid_user_name(std::string_view name, std::string& error) {
    if (name.empty()) {
        error = "empty user name";
        return false;
    }
    if (name == "*") {
        error = "user name '*' is not allowed";
        return false;
    }
    if (!is_valid_utf8(name)) {
        error = "user name is not valid UTF-8";
        return false;
    }
    if (name.front() == '-') {
        error = "user name " + in_quotes(name) + " starts with '-'";
        return false;
    }
    for (char c : name) {
        if (std::isspace(static_cast<unsigned char>(c)) != 0) {
            error = "user name " + in_quotes(name) + " contains white space";
            return false;
        }
        if (c == '#' || c == ',' || c == ':' || c == ';') {
            error = "user name " + in_quotes(name) + " contains " + in_quotes(std::string(1, c));
            return false;
        }
    }
    return true;
}

Permissions::Permissions(std::vector<Entry> entries)
    : entries_(std::move(entries)) {
    if (entries_.empty()) {
        fail("no entry");
    }
    std::sort(entries_.begin(), entries_.end(), [](const Entry& a, const Entry& b) { return a.user < b.user; });
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        const auto& entry = entries_[i];
        if (std::string error; !is_valid_user_name(entry.user, error)) {
            fail(error);
        }
        if (entry.roles.empty()) {
            fail("the entry of user " + in_quotes(entry.user) + " grants no role");
        }
        if (i > 0 && entries_[i - 1].user == entry.user) {
            fail("user " + in_quotes(entry.user) + " listed more than once");
        }
    }
}

Permissions Permissions::parse_cli(std::string_view text) {
    std::vector<Entry> entries;
    std::size_t begin = 0;
    while (true) {
        auto end   = text.find(',', begin);
        auto token = text.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
        if (token.empty()) {
            fail("empty entry in " + in_quotes(text));
        }
        auto colon = token.find(':');
        if (colon == std::string_view::npos) {
            fail("entry " + in_quotes(token) + " has no ':'");
        }
        entries.push_back(make_entry(token.substr(0, colon), token.substr(colon + 1)));
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1;
    }
    return Permissions(std::move(entries));
}

Permissions Permissions::parse_json(std::string_view text) {
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(text);
    }
    catch (const nlohmann::json::exception& e) {
        fail(std::string("malformed JSON: ") + e.what());
    }
    if (!json.is_array()) {
        fail("the JSON form is not a list");
    }

    std::vector<Entry> entries;
    for (const auto& item : json) {
        if (!item.is_object() || item.size() != 2 || !item.contains("user") || !item.contains("permissions")) {
            fail("each element of the JSON form must hold exactly 'user' and 'permissions'");
        }
        const auto& user        = item.at("user");
        const auto& permissions = item.at("permissions");
        if (!user.is_string() || !permissions.is_string()) {
            fail("'user' and 'permissions' must be strings");
        }
        entries.push_back(make_entry(user.get<std::string>(), permissions.get<std::string>()));
    }
    return Permissions(std::move(entries));
}

std::string Permissions::to_cli() const {
    std::string text;
    for (const auto& entry : entries_) {
        if (!text.empty()) {
            text += ',';
        }
        text += entry.user;
        text += ':';
        text += entry.roles.to_string();
        if (entry.sticky) {
            text += sticky_letter;
        }
    }
    return text;
}

std::string Permissions::to_json() const {
    auto json = nlohmann::json::array();
    for (const auto& entry : entries_) {
        auto letters = entry.roles.to_string();
        if (entry.sticky) {
            letters += sticky_letter;
        }
        json.push_back({{"user", entry.user}, {"permissions", letters}});
    }
    return json.dump();
}

const Permissions::Entry* Permissions::find(std::string_view user) const {
    auto found = std::lower_bound(
        entries_.begin(), entries_.end(), user, [](const Entry& e, std::string_view u) { return e.user < u; });
    return (found != entries_.end() && found->user == user) ? &*found : nullptr;
}

std::size_t Permissions::hash() const {
    std::size_t seed = entries_.size();
    auto combine = [&seed](std::size_t value) { seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2); };
    for (const auto& entry : entries_) {
        combine(std::hash<std::string>{}(entry.user));
        combine(entry.roles.to_ulong());
        combine(entry.sticky ? 1U : 0U);
    }
    return seed;
}

} // namespace ecf
