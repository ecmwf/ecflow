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

class AbstractIdentity {
public:
    virtual ~AbstractIdentity() = default;

    virtual std::unique_ptr<AbstractIdentity> clone() const = 0;

    virtual std::string username() const = 0;
    virtual std::string password() const = 0;

    virtual std::string as_string() const = 0;
};

template <class T>
class WrappingIdentity : public AbstractIdentity {
public:
    explicit WrappingIdentity(T&& id) : id_(std::move(id)) {}

    [[nodiscard]] std::unique_ptr<AbstractIdentity> clone() const override {
        T clone = id_;
        return std::make_unique<WrappingIdentity>(std::move(clone));
    }

    [[nodiscard]] std::string username() const override { return id_.username(); }
    [[nodiscard]] std::string password() const override { return id_.password(); }

    [[nodiscard]] std::string as_string() const override { return id_.as_string(); }

private:
    T id_;
};

class None {
public:
    [[nodiscard]] std::string username() const { return ""; }
    [[nodiscard]] std::string password() const { return ""; }

    [[nodiscard]] std::string as_string() const { return "None"; }
};

class UserX {
public:
    explicit UserX(std::string username, std::string password)
        : username_(std::move(username)),
          password_(std::move(password)) {}

    [[nodiscard]] std::string username() const { return username_; }
    [[nodiscard]] std::string password() const { return password_; }

    [[nodiscard]] std::string as_string() const { return "{UserX: " + username_ + "}"; }

private:
    std::string username_;
    std::string password_;
};

class CustomUserX {
public:
    explicit CustomUserX(std::string username, std::string password)
        : username_(std::move(username)),
          password_(std::move(password)) {}

    [[nodiscard]] std::string username() const { return username_; }
    [[nodiscard]] std::string password() const { return password_; }

    [[nodiscard]] std::string as_string() const { return "{UserX: " + username_ + ":" + password_ + "}"; }

private:
    std::string username_;
    std::string password_;
};

class SecureUserX {
public:
    explicit SecureUserX(std::string username) : username_(std::move(username)), password_{} {}

    [[nodiscard]] std::string username() const { return username_; }
    [[nodiscard]] std::string password() const { return password_; }

    [[nodiscard]] std::string as_string() const { return "{SecuredUserX: " + username_ + ":" + password_ + "}"; }

private:
    std::string username_;
    std::string password_;
};

class TaskX {
public:
    explicit TaskX(std::string pid, std::string pass, std::string tryno)
        : pid_(std::move(pid)),
          pass_(std::move(pass)),
          tryno_(std::move(tryno)) {}

    [[nodiscard]] std::string username() const { return pid_; }
    [[nodiscard]] std::string password() const { return pass_; }

    [[nodiscard]] std::string as_string() const { return "{TaskX: " + pid_ + ":" + pass_ + ":" + tryno_ + "}"; }

private:
    std::string pid_;
    std::string pass_;
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

    Identity(const Identity& other) : handle_{other.handle_->clone()} {}
    Identity(Identity&& other) noexcept : handle_{std::move(other.handle_)} {}
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

    [[nodiscard]] std::string username() const { return handle_->username(); }
    [[nodiscard]] std::string password() const { return handle_->password(); }

    [[nodiscard]] std::string as_string() const { return handle_->as_string(); }

private:
    template <class T>
    explicit Identity(T&& t) : handle_{std::make_unique<WrappingIdentity<T>>(std::move(t))} {}

    std::unique_ptr<AbstractIdentity> handle_;
};

inline std::ostream& operator<<(std::ostream& os, const Identity& id) {
    os << id.as_string();
    return os;
}

} // namespace ecf

#endif // ecflow_core_Identity_HPP
