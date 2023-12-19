/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <array>
#include <iomanip>
#include <iostream>
#include <thread>

#include <boost/test/unit_test.hpp>

#include "ecflow/server/PeriodicScheduler.hpp"

std::ostream& operator<<(std::ostream& os, const std::chrono::system_clock::time_point& tp) {
    // Create time stamp
    std::time_t time = std::chrono::system_clock::to_time_t(tp);
    std::tm tm       = *std::gmtime(&time);
    std::array<char, 100> buffer{};
    std::strftime(buffer.data(), buffer.size(), "%Y-%m-%dT%H:%M:%S", &tm);

    // Extract milliseconds
    auto total_seconds = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch());
    auto total_millis  = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
    auto millis        = total_millis - std::chrono::duration_cast<std::chrono::milliseconds>(total_seconds);

    os << buffer.data() << "." << std::setw(3) << std::setfill('0') << millis.count();

    return os;
}

BOOST_AUTO_TEST_SUITE(TestPeriodicScheduler)

struct Collector
{
    using instant_t  = ecf::PeriodicScheduler<Collector>::instant_t;
    using instants_t = std::vector<instant_t>;

    void operator()(instant_t now, instant_t last, instant_t next, bool is_boundary = true) {
        std::cout << "Call at " << now << ", with is_boundary? " << (is_boundary ? "true" : "false") << ". Last call was at " << last
                  << ". Next call at " << next << "." << std::endl;

        // Activity must be called at minute boundary!
        if (is_boundary && !instants.empty()) {
            auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch() % 86400) % 60;
            BOOST_CHECK(seconds == std::chrono::seconds(0));
        }
        instants.push_back(now);
    }

    instants_t instants;
};

struct LongLasting
{
    using instant_t = ecf::PeriodicScheduler<LongLasting>::instant_t;

    template <typename DURATION>
    explicit LongLasting(const DURATION& duration) : how_long_{std::chrono::duration_cast<std::chrono::milliseconds>(duration)} {}

    void operator()(instant_t now, instant_t last, instant_t next, bool is_boundary = true) {
        std::cout << "Call at " << now << ", with is_boundary? " << (is_boundary ? "true" : "false") << ". Last call was at " << last
                  << ". Next call at " << next << "." << std::endl;

        std::this_thread::sleep_for(how_long_);
    }

    std::chrono::milliseconds how_long_;
};

BOOST_AUTO_TEST_CASE(test_periodic_scheduler_over_one_minute) {
    // Setup time collector
    Collector collector;
    boost::asio::io_service io_service;
    ecf::PeriodicScheduler scheduler(io_service, std::chrono::seconds(30), collector);
    scheduler.start();

    // Arrange time collector termination
    ecf::Timer teardown(io_service);
    teardown.set([&scheduler](const boost::system::error_code& error) { scheduler.terminate(); }, std::chrono::seconds(62));

    // Run services
    io_service.run();

    BOOST_CHECK_EQUAL(collector.instants.size(), 62L);
}

BOOST_AUTO_TEST_CASE(test_periodic_scheduler_with_long_lasting_activity) {
    // Setup time collector
    LongLasting activity{std::chrono::milliseconds(2499)};
    boost::asio::io_service io_service;
    ecf::PeriodicScheduler scheduler(io_service, std::chrono::seconds(10), activity);
    scheduler.start();

    // Arrange time collector termination
    ecf::Timer teardown(io_service);
    teardown.set([&scheduler](const boost::system::error_code& error) { scheduler.terminate(); }, std::chrono::seconds(60));

    // Run services
    io_service.run();
}

BOOST_AUTO_TEST_SUITE_END()
