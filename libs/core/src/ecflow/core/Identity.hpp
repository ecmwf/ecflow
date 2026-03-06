/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Identity_HPP
#define ecflow_core_Identity_HPP

#include <memory>
#include <string>

namespace ecf {

class Username {
public:
    explicit Username(std::string username)
        : v_{std::move(username)} {};

    const std::string& value() const { return v_; }

    friend bool operator==(const Username& lhs, const Username& rhs) { return lhs.v_ == rhs.v_; }
    friend bool operator!=(const Username& lhs, const Username& rhs) { return !(lhs == rhs); }

private:
    std::string v_;
};

class Password {
public:
    explicit Password(std::string pass)
        : v_{std::move(pass)} {};

    const std::string& value() const { return v_; }

    friend bool operator==(const Password& lhs, const Password& rhs) { return lhs.v_ == rhs.v_; }
    friend bool operator!=(const Password& lhs, const Password& rhs) { return !(lhs == rhs); }

private:
    std::string v_;
};

class AbstractIdentity {
public:
    virtual ~AbstractIdentity() = default;

    virtual std::unique_ptr<AbstractIdentity> clone() const = 0;

    virtual Username username() const = 0;
    virtual Password password() const = 0;

    virtual std::string as_string() const = 0;
};

template <class T>
class WrappingIdentity : public AbstractIdentity {
public:
    explicit WrappingIdentity(T&& id)
        : id_(std::move(id)) {}

    [[nodiscard]] std::unique_ptr<AbstractIdentity> clone() const override {
        T clone = id_;
        return std::make_unique<WrappingIdentity>(std::move(clone));
    }

    [[nodiscard]] Username username() const override { return id_.username(); }
    [[nodiscard]] Password password() const override { return id_.password(); }

    [[nodiscard]] std::string as_string() const override { return id_.as_string(); }

private:
    T id_;
};

///
/// @brief None represents...
///
class None {
public:
    [[nodiscard]] const Username& username() const { return empty_username; }
    [[nodiscard]] const Password& password() const { return empty_password; }

    [[nodiscard]] std::string as_string() const { return "None"; }

private:
    inline static Username empty_username{""};
    inline static Password empty_password{""};
};

///
/// @brief UserX represents a user with a username and password, extracted by the client from the environment
/// (i.e. the user is extracted from the $USER, and the password is found on the client side $ECF_PASSWD file).
///
/// The user and password are provided by the client in the inbound request, and the authentication is performed by the
/// ecFlow server itself. This is done by checking the user's credentials against the server side $ECF_PASSWD file.
///
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

///
/// @brief UserX represents a user with a username and password, provided explicitly to the client
/// (i.e. the user is either defined by $ECF_USER or provided by the --user option, and the password is either found on
/// the client side $ECF_CUSTOM_PASSWD file or provided by the --password option).
///
/// The user and password are provided by the client in the inbound request, and the authentication is performed by the
/// ecFlow server itself. This is done by checking the user's credentials against the server side $ECF_CUSTOM_PASSWD
/// file.
///
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

///
/// @brief SecureUserX represents a user for which the credentials where successfully verified.
///
/// This kind of user is used, when using HTTPS, to  encode the fact that external Authentication has been
/// performed.
///
/// This kind of user has a name, but it does not have an associated password (i.e. any eventual authentication
/// token was provided to and used by the external Authentication mechanism, but not passed on to the ecFlow server).
///
/// Since no password is available, the only Authentication mechanism that is applicable is external Authentication,
/// meaning that ecFlow server will not apply any Authentication related to PasswordFile.
///
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

///
/// @brief TaskX represents a task with a pid, password, and try number, extracted by the client from the environment
/// (respectively, $ECF_RID, $ECF_PASS, and $ECF_TRYNO).
///
/// In fact, this object contains "meta" information about the Task and does not identify any actual user.
/// At the moment, the ecFlow server does not have information about the 'user' that issues the "Task Requests".
///
class TaskX {
public:
    explicit TaskX(std::string pid, std::string pass, std::string tryno)
        : pid_(std::move(pid)),
          pass_(std::move(pass)),
          tryno_(std::move(tryno)) {}

    [[nodiscard]] Username username() const { return pid_; }
    [[nodiscard]] Password password() const { return pass_; }

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

    Identity& operator=(const Identity& other) {
        handle_ = other.handle_->clone();
        return *this;
    }

    Identity& operator=(Identity&& other) noexcept {
        handle_ = std::move(other.handle_);
        return *this;
    }

    [[nodiscard]] bool is_user() const {
        return is_a<UserX>(*handle_) || is_a<CustomUserX>(*handle_) || is_a<SecureUserX>(*handle_);
    }
    [[nodiscard]] bool is_task() const { return is_a<TaskX>(*handle_); }

    [[nodiscard]] bool is_custom() const { return is_a<WrappingIdentity<CustomUserX>>(*handle_); }
    [[nodiscard]] bool is_secure() const { return is_a<WrappingIdentity<SecureUserX>>(*handle_); }

    [[nodiscard]] Username username() const { return handle_->username(); }
    [[nodiscard]] Password password() const { return handle_->password(); }

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

#endif // ecflow_core_Identity_HPP
