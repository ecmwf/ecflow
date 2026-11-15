// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <array>
#include <bitset>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include "ecflow/core/Identity.hpp"

namespace cereal {
class access;
}

namespace ecf {

///
/// @brief Identifies a role that a Permissions entry grants to a user.
///
/// A role is added by declaring an enumerator before Count and listing it, with its letter, in the table of
/// all_roles().
///
enum class Role : std::size_t {
    Read,        ///< r: read nodes and their attributes
    Write,       ///< w: change nodes and their attributes
    Execute,     ///< x: run, requeue and otherwise act on nodes
    Log,         ///< l: change the server log
    Owner,       ///< o: load and delete suites
    Permissions, ///< p: change Permissions attributes
    Count        ///< Number of roles; not a role
};

///
/// @brief Returns every role, in the canonical order in which their letters are printed.
///
/// @return The roles r, w, x, l, o and p, in this order.
///
const std::array<Role, static_cast<std::size_t>(Role::Count)>& all_roles();

///
/// @brief Returns the letter that denotes a role in the textual forms.
///
/// @param role A role other than Role::Count.
/// @return The lowercase letter of the role.
///
char letter_of(Role role);

///
/// @brief Holds a set of roles.
///
class Roles {
public:
    ///
    /// @brief Creates an empty set of roles.
    ///
    Roles() = default;

    ///
    /// @brief Creates a set holding the given roles.
    ///
    /// @param roles The roles of the set; repetitions are ignored.
    ///
    Roles(std::initializer_list<Role> roles);

    ///
    /// @brief Returns whether the set holds a role.
    ///
    /// @param role The role looked up.
    /// @return true when the set holds @p role.
    ///
    [[nodiscard]] bool has(Role role) const { return bits_.test(static_cast<std::size_t>(role)); }

    ///
    /// @brief Adds a role to the set.
    ///
    /// @param role The role added; adding a role already held leaves the set unchanged.
    ///
    void add(Role role) { bits_.set(static_cast<std::size_t>(role)); }

    ///
    /// @brief Returns whether the set holds no role.
    ///
    /// @return true when the set is empty.
    ///
    [[nodiscard]] bool empty() const { return bits_.none(); }

    ///
    /// @brief Returns whether the set holds every role of another set.
    ///
    /// @param other The roles looked up.
    /// @return true when @p other is a subset of this set.
    ///
    [[nodiscard]] bool contains(const Roles& other) const { return (bits_ & other.bits_) == other.bits_; }

    ///
    /// @brief Returns the letters of the roles held, in canonical order.
    ///
    /// @return The letters, e.g. "rwx"; empty for an empty set.
    ///
    [[nodiscard]] std::string to_string() const;

    ///
    /// @brief Returns the roles held by both sets.
    ///
    /// @param lhs A set of roles.
    /// @param rhs Another set of roles.
    /// @return The intersection of @p lhs and @p rhs.
    ///
    friend Roles operator&(const Roles& lhs, const Roles& rhs) { return Roles(lhs.bits_ & rhs.bits_); }

    ///
    /// @brief Returns the roles held by either set.
    ///
    /// @param lhs A set of roles.
    /// @param rhs Another set of roles.
    /// @return The union of @p lhs and @p rhs.
    ///
    friend Roles operator|(const Roles& lhs, const Roles& rhs) { return Roles(lhs.bits_ | rhs.bits_); }

    ///
    /// @brief Returns whether two sets hold the same roles.
    ///
    /// @param lhs A set of roles.
    /// @param rhs Another set of roles.
    /// @return true when both sets hold the same roles.
    ///
    friend bool operator==(const Roles& lhs, const Roles& rhs) { return lhs.bits_ == rhs.bits_; }

    ///
    /// @brief Returns whether two sets differ.
    ///
    /// @param lhs A set of roles.
    /// @param rhs Another set of roles.
    /// @return true when the sets do not hold the same roles.
    ///
    friend bool operator!=(const Roles& lhs, const Roles& rhs) { return !(lhs == rhs); }

    ///
    /// @brief Returns a value identifying the set, for hashing.
    ///
    /// @return The bits of the set, one per role.
    ///
    [[nodiscard]] unsigned long to_ulong() const { return bits_.to_ulong(); }

private:
    using bits_t = std::bitset<static_cast<std::size_t>(Role::Count)>;

    explicit Roles(bits_t bits)
        : bits_(bits) {}

    bits_t bits_;
};

///
/// @brief Holds the value of a Permissions attribute: the roles granted to each listed user.
///
/// The value has two textual forms:
///
/// - the command-line form, a comma-separated list of entries <user>:<letters>, e.g. "alice:rwxs,bob:r";
/// - the JSON form, a list of objects, e.g. [{"user": "alice", "permissions": "rwxs"}].
///
/// The letters are those of the roles (r, w, x, l, o, p) and of the sticky flag (s), lowercase and without
/// repetition, in any order on input and in that order on output.
///
/// @invariant The value holds at least one entry.
/// @invariant Entries are sorted by user name, and each user is listed once.
/// @invariant Every user name follows the user name model (is_valid_user_name()), and every entry grants at least one
///            role.
///
class Permissions {
public:
    ///
    /// @brief Holds the roles granted to one user, and whether the entry is sticky.
    ///
    struct Entry
    {
        std::string user;
        Roles roles;
        bool sticky{false};

        ///
        /// @brief Returns whether two entries name the same user, with the same roles and flag.
        ///
        /// @param lhs An entry.
        /// @param rhs Another entry.
        /// @return true when the entries are equal.
        ///
        friend bool operator==(const Entry& lhs, const Entry& rhs) {
            return lhs.user == rhs.user && lhs.roles == rhs.roles && lhs.sticky == rhs.sticky;
        }
    };

    ///
    /// @brief Creates a value from its entries, in any order.
    ///
    /// @param entries The entries of the value.
    /// @throws std::runtime_error when the entries break an invariant of the class.
    ///
    explicit Permissions(std::vector<Entry> entries);

    ///
    /// @brief Parses the command-line form.
    ///
    /// @param text The command-line form, e.g. "alice:rwxs,bob:r".
    /// @return The parsed value.
    /// @throws std::runtime_error when the text is not a valid command-line form.
    ///
    static Permissions parse_cli(std::string_view text);

    ///
    /// @brief Parses the JSON form.
    ///
    /// @param text The JSON form, e.g. [{"user": "alice", "permissions": "rwxs"}].
    /// @return The parsed value.
    /// @throws std::runtime_error when the text is not a valid JSON form.
    ///
    static Permissions parse_json(std::string_view text);

    ///
    /// @brief Returns the command-line form, in canonical order.
    ///
    /// @return The command-line form, e.g. "alice:rwxs,bob:r".
    ///
    [[nodiscard]] std::string to_cli() const;

    ///
    /// @brief Returns the JSON form, in canonical order and without white space.
    ///
    /// @return The JSON form, e.g. [{"permissions":"rwxs","user":"alice"}].
    ///
    [[nodiscard]] std::string to_json() const;

    ///
    /// @brief Returns the entries, sorted by user name.
    ///
    /// @return The entries.
    ///
    [[nodiscard]] const std::vector<Entry>& entries() const { return entries_; }

    ///
    /// @brief Returns the entry of a user.
    ///
    /// @param user The user name looked up.
    /// @return The entry of @p user, or nullptr when the user is not listed; valid while the value is unchanged.
    ///
    [[nodiscard]] const Entry* find(std::string_view user) const;

    ///
    /// @brief Returns a hash of the value; equal values have equal hashes.
    ///
    /// @return The hash.
    ///
    [[nodiscard]] std::size_t hash() const;

    ///
    /// @brief Returns whether two values hold the same entries.
    ///
    /// @param lhs A value.
    /// @param rhs Another value.
    /// @return true when the values are equal, whatever the order in which their entries were given.
    ///
    friend bool operator==(const Permissions& lhs, const Permissions& rhs) { return lhs.entries_ == rhs.entries_; }

    ///
    /// @brief Returns whether two values differ.
    ///
    /// @param lhs A value.
    /// @param rhs Another value.
    /// @return true when the values are not equal.
    ///
    friend bool operator!=(const Permissions& lhs, const Permissions& rhs) { return !(lhs == rhs); }

private:
    std::vector<Entry> entries_;

    // Serialisation as the JSON form, which validates the value when loading
    Permissions() = default;
    friend class cereal::access;
    template <class Archive>
    std::string save_minimal(const Archive& ar) const;
    template <class Archive>
    void load_minimal(const Archive& ar, const std::string& value);
};

} // namespace ecf

///
/// @brief Hashes a Permissions value, so that it can key unordered containers.
///
template <>
struct std::hash<ecf::Permissions>
{
    ///
    /// @brief Returns the hash of a value.
    ///
    /// @param permissions The value hashed.
    /// @return The same hash as ecf::Permissions::hash().
    ///
    std::size_t operator()(const ecf::Permissions& permissions) const noexcept { return permissions.hash(); }
};
