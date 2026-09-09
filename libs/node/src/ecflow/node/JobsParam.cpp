/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/node/JobsParam.hpp"

bool JobsParam::check_for_job_generation_timeout() {
    if (timed_out_of_job_generation_) {
        return true;
    }
    return check_for_job_generation_timeout(boost::posix_time::microsec_clock::universal_time());
}

bool JobsParam::check_for_job_generation_timeout(const boost::posix_time::ptime& start_time) {
    if (timed_out_of_job_generation_) {
        return true;
    }
    if (!next_poll_time_.is_special() && start_time >= next_poll_time_) {
        set_timed_out_of_job_generation(start_time);
        return true;
    }
    return false;
}

void JobsParam::clear() {
    errorMsg_.clear();
    debugMsg_.clear();
    submitted_.clear();
    user_edit_file_.clear();
    user_edit_variables_.clear();
}
