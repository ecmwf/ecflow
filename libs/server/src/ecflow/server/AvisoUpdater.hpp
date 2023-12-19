/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_server_AvisoUpdater_HPP
#define ecflow_server_AvisoUpdater_HPP

#include <algorithm>

#include "ecflow/attribute/AvisoService.hpp"
#include "ecflow/server/PeriodicScheduler.hpp"

namespace ecf {

struct CheckAvisoNotifications
{
    using instant_t = std::chrono::system_clock::time_point;

    void operator()(instant_t now, instant_t last, instant_t next, bool is_boundary);

private:
    void update_notifications();

private:
    AvisoRegistry& roster_ = AvisoService::service();
    AvisoAPI api_;
};

using AvisoUpdater = ecf::PeriodicScheduler<CheckAvisoNotifications>;

} // namespace ecf

#endif /* ecflow_server_AvisoUpdater_HPP */
