/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/server/AvisoUpdater.hpp"

#include <chrono>
#include <iostream>

namespace ecf {

void CheckAvisoNotifications::operator()(instant_t now, instant_t last, instant_t next, bool is_boundary) {
    if (!is_boundary) {
        // Nothing to do, unless we are "at boundary"
        return;
    }

    std::cout << "CheckingAvisoNotifications: launching background thread to listen to notifications" << std::endl;
    std::thread t([this]() { this->update_notifications(); });
    t.detach();
}

void CheckAvisoNotifications::update_notifications() {
    std::cout << "CheckingAvisoNotifications: start listening to notifications" << std::endl;

    api_.check_aviso_server_for_notifications(roster_);

    std::cout << "CheckingAvisoNotifications: finished listening to notifications" << std::endl;
}

} // namespace ecf
