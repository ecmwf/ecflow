// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string>

namespace cereal {
class access;
}

namespace ecf {

///
/// @brief Represents the owner of a node: an OS user name and an optional group name.
///
/// @details The value is written as `user[:group]`. Both names follow the portable POSIX syntax: 1 to 32
///          characters among letters, digits, '.', '_' and '-', not starting with '-', and not made of digits
///          only (numeric identifiers are not accepted). The value is only checked syntactically; whether the
///          names denote existing accounts is decided elsewhere, by the server.
///
/// @invariant A non-default instance always holds a valid, non-empty user name, and a group name that is
///            either absent or valid.
///
class OwnerAttr {
public:
    ///
    /// @brief Creates an empty owner, only meant to be assigned or restored from a serialised form.
    ///
    /// @details Restoring a serialised form validates it like create(), and throws std::runtime_error when it is
    ///          not a valid owner; an empty owner can therefore not be restored.
    ///
    OwnerAttr() = default;

    ///
    /// @brief Creates an owner from its textual form.
    ///
    /// @param[in] value The owner, as `user` or `user:group`.
    /// @return The owner described by @p value.
    /// @throws std::runtime_error if @p value is not a valid owner; the message names the value.
    ///
    static OwnerAttr create(const std::string& value);

    ///
    /// @brief Checks whether a text is a valid owner value.
    ///
    /// @param[in] value The candidate owner, as `user` or `user:group`.
    /// @return true if @p value is a valid owner value; false otherwise. Never throws.
    ///
    static bool is_valid(const std::string& value) noexcept;

    ///
    /// @brief Returns the user name.
    ///
    /// @return The user name; empty only for a default-constructed instance.
    ///
    [[nodiscard]] const std::string& user() const { return user_; }

    ///
    /// @brief Returns the group name, if any.
    ///
    /// @return The group name, or no value when the owner names a user only.
    ///
    [[nodiscard]] std::optional<std::string> group() const;

    ///
    /// @brief Returns the textual form of the owner.
    ///
    /// @return The owner as `user` or `user:group`.
    ///
    [[nodiscard]] std::string value() const;

    ///
    /// @brief Returns the definition-format form of the attribute.
    ///
    /// @return The attribute as `owner user[:group]`.
    ///
    [[nodiscard]] std::string toString() const;

    ///
    /// @brief Appends the definition-format form of the attribute to a string.
    ///
    /// @param[in,out] ret The string to which `owner user[:group]` is appended.
    ///
    void write(std::string& ret) const;

    ///
    /// @brief Compares two owners.
    ///
    /// @param[in] rhs The owner to compare with.
    /// @return true if both the user and the group names are equal; false otherwise.
    ///
    bool operator==(const OwnerAttr& rhs) const;

private:
    OwnerAttr(std::string user, std::string group);

    std::string user_;
    std::string group_; // empty when no group is given

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar);
};

} // namespace ecf
