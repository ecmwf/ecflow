/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_attribute_AvisoService_HPP
#define ecflow_attribute_AvisoService_HPP

#include <algorithm>
#include <iostream>
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ecf {

struct AvisoRegistry;

///
/// \brief AvisoAPI allows checking if a notification has been registered in the Aviso server
///

struct AvisoAPI
{
    AvisoAPI() = default;

    void check_aviso_server_for_notifications(AvisoRegistry& registry) const;

private:
    mutable std::mutex m_;
};

///
/// \brief BaseService manages a \a service object
///
/// \tparam T the type of the \a service object
///
template <typename T>
struct BaseService
{
    static T& service() {
        static T instance;
        return instance;
    }

private:
    BaseService() = default;
};

///
/// \brief AvisoRegistry allows \b global access to a singleton instance of \ref AvisoRegistry
///
struct AvisoRegistry
{
    AvisoRegistry() = default;

    bool check_notification(const std::string& name) {
        std::lock_guard guard(m_);
        std::cout << "AvisoRegistry: check notification for name: " << name << std::endl;

        if (auto found = roster_.find(name); found != std::end(roster_)) {
            roster_.erase(found);
            std::cout << "Notification found! Reseting for next time!..." << std::endl;
            return true;
        }

        std::cout << ". No notification found!" << std::endl;

        return false;
    }

    void notify(const std::string& name) {
        std::lock_guard guard(m_);

        std::cout << "AvisoRegistry: Registered notification for name: " << name << std::endl;
        roster_[name] = true;
    }

private:
    mutable std::mutex m_;
    std::unordered_map<std::string, bool> roster_;
};

///
/// \brief AvisoService allows \b global access to a singleton instance of \ref AvisoRegistry
///
using AvisoService = BaseService<AvisoRegistry>;

} // namespace ecf

#endif /* ecflow_attribute_AvisoService_HPP */
