/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_service_Log_HPP
#define ecflow_service_Log_HPP

#include <functional>
#include <iostream>
#include <thread>

#include "ecflow/core/Log.hpp"

#define SLOG(LEVEL, MESSAGE)                                                      \
    [&]() {                                                                       \
        using namespace ecf::service::log;                                        \
        LOG(LevelTraits<Level::LEVEL>::mapping_type,                              \
            MESSAGE << " {" << LevelTraits<Level::LEVEL>::name << "}" << Meta{}); \
    }()

namespace ecf::service {

namespace log {

enum class Level { D /*ebug*/, I /*nformation*/, W /*arning*/, E /*rror*/, F /*atal*/ };

template <Level L>
struct LevelTraits;

template <>
struct LevelTraits<Level::D>
{
    static constexpr const char* name               = "D";
    static constexpr ecf::Log::LogType mapping_type = ecf::Log::LogType::DBG;
};

template <>
struct LevelTraits<Level::I>
{
    static constexpr const char* name               = "I";
    static constexpr ecf::Log::LogType mapping_type = ecf::Log::LogType::LOG;
};

template <>
struct LevelTraits<Level::W>
{
    static constexpr const char* name               = "W";
    static constexpr ecf::Log::LogType mapping_type = ecf::Log::LogType::WAR;
};

template <>
struct LevelTraits<Level::E>
{
    static constexpr const char* name               = "E";
    static constexpr ecf::Log::LogType mapping_type = ecf::Log::LogType::ERR;
};

template <>
struct LevelTraits<Level::F>
{
    static constexpr const char* name               = "F";
    static constexpr ecf::Log::LogType mapping_type = ecf::Log::LogType::ERR;
};

struct Meta
{
    std::thread::id id = std::this_thread::get_id();
};

inline std::ostream& operator<<(std::ostream& os, const Meta& meta) {
    os << '[' << meta.id << ']';
    return os;
}

} // namespace log

} // namespace ecf::service

#endif /* ecflow_service_Log_HPP */
