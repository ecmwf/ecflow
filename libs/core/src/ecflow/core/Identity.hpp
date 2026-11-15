// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace ecf {

///
/// @brief Checks a user name against the user name model, followed by every user name ecFlow reads.
///
/// A valid user name is a non-empty string of valid UTF-8 (RFC 3629: no overlong form, no surrogate, no code point
/// above U+10FFFF) that:
///
/// - contains no white space, i.e. no character with the Unicode property White_Space;
/// - contains no control character (U+0000 to U+001F, U+007F to U+009F);
/// - contains none of '#', ',', ':' and ';';
/// - does not start with '-';
/// - is not '*'.
///
/// User names are compared exactly, byte for byte, without case folding or Unicode normalisation.
///
/// @param[in] name The user name checked.
/// @param[out] error Receives the reason when the name is invalid, naming it in its printable form; left unchanged
///             otherwise.
/// @return true when the name is valid.
///
bool is_valid_user_name(std::string_view name, std::string& error);

///
/// @brief Returns a user name in quotes, with its white space and control characters written as <U+XXXX>, and the
///        bytes that are not valid UTF-8 as <0xXX>, so that it can be logged and shown safely.
///
/// @param[in] name Any text, valid or not.
/// @return The printable form, e.g. 'a<U+00A0>b'.
///
std::string printable_user_name(std::string_view name);

///
/// @brief Returns a text with its control characters written as <U+XXXX>, and the bytes that are not valid UTF-8 as
///        <0xXX>, so that it can be logged and shown safely; other characters, white space included, are kept.
///
/// @param[in] text Any text, valid or not.
/// @return The printable form; the text itself when it holds neither.
///
std::string printable_text(std::string_view text);

///
/// @brief Strongly-typed wrapper for a user login name.
///
/// Distinguishes a username from a password (and from a bare std::string) at
/// compile time, preventing the two from being accidentally interchanged when
/// passed across the authentication/authorisation trust boundary.
///
class Username {
public:
    ///
    /// @brief Construct a username from its textual value.
    ///
    /// @param username the login name; may be empty (e.g. for an anonymous identity)
    ///
    explicit Username(std::string username)
        : v_{std::move(username)} {}

    ///
    /// @brief Return the underlying login name.
    ///
    /// @return a reference to the wrapped value, valid for the lifetime of this object
    ///
    [[nodiscard]] const std::string& value() const { return v_; }

    ///
    /// @brief Compare two usernames for equality of their textual value.
    ///
    friend bool operator==(const Username& lhs, const Username& rhs) { return lhs.v_ == rhs.v_; }
    ///
    /// @brief Compare two usernames for inequality of their textual value.
    ///
    friend bool operator!=(const Username& lhs, const Username& rhs) { return !(lhs == rhs); }

private:
    std::string v_;
};

///
/// @brief Strongly-typed wrapper for a user password.
///
/// Distinguishes a password from a username (and from a bare std::string) at
/// compile time, preventing the two from being accidentally interchanged when
/// passed across the authentication trust boundary.
///
class Password {
public:
    ///
    /// @brief Construct a password from its textual value.
    ///
    /// @param pass the password; may be empty (e.g. for a secure/omitted identity)
    ///
    explicit Password(std::string pass)
        : v_{std::move(pass)} {}

    ///
    /// @brief Return the underlying password.
    ///
    /// @return a reference to the wrapped value, valid for the lifetime of this object
    ///
    [[nodiscard]] const std::string& value() const { return v_; }

    ///
    /// @brief Compare two passwords for equality of their textual value.
    ///
    friend bool operator==(const Password& lhs, const Password& rhs) { return lhs.v_ == rhs.v_; }
    ///
    /// @brief Compare two passwords for inequality of their textual value.
    ///
    friend bool operator!=(const Password& lhs, const Password& rhs) { return !(lhs == rhs); }

private:
    std::string v_;
};

class AbstractIdentity {
public:
    virtual ~AbstractIdentity() = default;

    virtual std::unique_ptr<AbstractIdentity> clone() const = 0;

    virtual const Username& username() const = 0;
    virtual const Password& password() const = 0;

    virtual std::string as_string() const = 0;

    ///
    /// @brief Returns whether the identity is the absence of identity (None).
    ///
    /// @return true for None; answered without run-time type information, so that the answer holds across shared
    ///         libraries.
    ///
    virtual bool is_none() const = 0;
};

class None;

template <class T>
class WrappingIdentity : public AbstractIdentity {
public:
    explicit WrappingIdentity(T&& id)
        : id_(std::move(id)) {}

    [[nodiscard]] std::unique_ptr<AbstractIdentity> clone() const override {
        T clone = id_;
        return std::make_unique<WrappingIdentity>(std::move(clone));
    }

    [[nodiscard]] const Username& username() const override { return id_.username(); }
    [[nodiscard]] const Password& password() const override { return id_.password(); }

    [[nodiscard]] std::string as_string() const override { return id_.as_string(); }

    [[nodiscard]] bool is_none() const override { return std::is_same_v<T, None>; }

private:
    T id_;
};

class None {
public:
    [[nodiscard]] const Username& username() const { return empty_username; }
    [[nodiscard]] const Password& password() const { return empty_password; }

    [[nodiscard]] std::string as_string() const { return "None"; }

private:
    inline static Username empty_username{""};
    inline static Password empty_password{""};
};

class UserX {
public:
    explicit UserX(std::string username, std::string password)
        : username_(std::move(username)),
          password_(std::move(password)) {}

    [[nodiscard]] const Username& username() const { return username_; }
    [[nodiscard]] const Password& password() const { return password_; }

    [[nodiscard]] std::string as_string() const { return "{UserX: " + username_.value() + "}"; }

private:
    Username username_;
    Password password_;
};

class CustomUserX {
public:
    explicit CustomUserX(std::string username, std::string password)
        : username_(std::move(username)),
          password_(std::move(password)) {}

    [[nodiscard]] const Username& username() const { return username_; }
    [[nodiscard]] const Password& password() const { return password_; }

    [[nodiscard]] std::string as_string() const { return "{CustomUserX: " + username_.value() + "}"; }

private:
    Username username_;
    Password password_;
};

class SecureUserX {
public:
    explicit SecureUserX(std::string username)
        : username_(std::move(username)) {}

    [[nodiscard]] const Username& username() const { return username_; }
    [[nodiscard]] const Password& password() const { return empty; }

    [[nodiscard]] std::string as_string() const { return "{SecuredUserX: " + username_.value() + "}"; }

private:
    Username username_;
    inline static Password empty{""};
};

class TaskX {
public:
    explicit TaskX(std::string pid, std::string pass, std::string tryno)
        : pid_(std::move(pid)),
          pass_(std::move(pass)),
          tryno_(std::move(tryno)) {}

    [[nodiscard]] const Username& username() const { return pid_; }
    [[nodiscard]] const Password& password() const { return pass_; }

    [[nodiscard]] std::string as_string() const {
        return "{TaskX: " + pid_.value() + ":" + pass_.value() + ":" + tryno_ + "}";
    }

private:
    Username pid_;
    Password pass_;
    std::string tryno_;
};

template <class Derived, class Base = AbstractIdentity>
struct IsA
{
    bool operator()(const Base& object) const { return dynamic_cast<const Derived*>(&object) != nullptr; }
};

template <class D, class U>
bool is_a(const U& object) {
    return IsA<D>()(object);
}

namespace detail {

template <typename X, typename Y, typename... Ys>
struct is_one_of
{
    constexpr static bool value = std::is_same_v<X, Y> || is_one_of<X, Ys...>::value;
};

template <typename X, typename Y>
struct is_one_of<X, Y>
{
    constexpr static bool value = std::is_same_v<X, Y>;
};

template <typename X, typename... Xs>
constexpr bool is_one_of_v = is_one_of<X, Xs...>::value;

} // namespace detail

class Identity {
public:
    [[nodiscard]] static Identity make_none() { return Identity{None{}}; }
    [[nodiscard]] static Identity make_user(const std::string& username, const std::string& password) {
        return Identity{UserX{username, password}};
    }
    [[nodiscard]] static Identity make_custom_user(const std::string& username, const std::string& password) {
        return Identity{CustomUserX{username, password}};
    }
    [[nodiscard]] static Identity make_secure_user(const std::string& username) {
        return Identity{SecureUserX{username}};
    }

    [[nodiscard]] static Identity make_task(const std::string& pid, const std::string& pass, const std::string& tryno) {
        return Identity{TaskX{pid, pass, tryno}};
    }

    Identity(const Identity& other)
        : handle_{other.handle_->clone()} {}
    Identity(Identity&& other) noexcept
        : handle_{std::move(other.handle_)} {}
    ~Identity() = default;

    Identity& operator=(Identity other) {
        using std::swap;
        std::swap(handle_, other.handle_);
        return *this;
    }

    [[nodiscard]] bool is_user() const {
        return is_a<WrappingIdentity<UserX>>(*handle_) || is_a<WrappingIdentity<CustomUserX>>(*handle_) ||
               is_a<WrappingIdentity<SecureUserX>>(*handle_);
    }
    [[nodiscard]] bool is_task() const { return is_a<WrappingIdentity<TaskX>>(*handle_); }

    ///
    /// @brief Returns whether this is the absence of identity, i.e. no identity was established.
    ///
    /// @return true for an identity made by make_none().
    ///
    [[nodiscard]] bool is_none() const { return handle_->is_none(); }

    [[nodiscard]] bool is_custom() const { return is_a<WrappingIdentity<CustomUserX>>(*handle_); }
    [[nodiscard]] bool is_secure() const { return is_a<WrappingIdentity<SecureUserX>>(*handle_); }

    [[nodiscard]] const Username& username() const { return handle_->username(); }
    [[nodiscard]] const Password& password() const { return handle_->password(); }

    [[nodiscard]] std::string as_string() const { return handle_->as_string(); }

private:
    template <class T,
              typename = std::enable_if_t<detail::is_one_of_v<T, None, UserX, CustomUserX, SecureUserX, TaskX>>>
    explicit Identity(T&& t)
        : handle_{std::make_unique<WrappingIdentity<T>>(std::forward<T>(t))} {}

    std::unique_ptr<AbstractIdentity> handle_;
};

inline std::ostream& operator<<(std::ostream& os, const Identity& id) {
    os << id.as_string();
    return os;
}

} // namespace ecf
