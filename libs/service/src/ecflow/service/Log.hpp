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

#define ALOG(LEVEL, MESSAGE)                                                    \
    [&]() {                                                                     \
        using namespace ecf::service::log;                                      \
        select<Level::LEVEL>() << Meta{} << MESSAGE << ecf::service::log::endl; \
    }()

namespace ecf::service {

namespace log {

enum class Level { D /*ebug*/, I /*nformation*/, W /*arning*/, E /*rror*/, F /*atal*/ };

template <Level L>
struct LevelTraits;

template <>
struct LevelTraits<Level::D>
{
    static constexpr const char* name = "D";
};

template <>
struct LevelTraits<Level::I>
{
    static constexpr const char* name = "I";
};

template <>
struct LevelTraits<Level::W>
{
    static constexpr const char* name = "W";
};

template <>
struct LevelTraits<Level::E>
{
    static constexpr const char* name = "E";
};

template <>
struct LevelTraits<Level::F>
{
    static constexpr const char* name = "F";
};

struct Meta
{
    std::thread::id id = std::this_thread::get_id();
};

struct Manipulator
{
    std::function<void(std::ostream&)> manip;
};

inline Manipulator endl{[](std::ostream& os) { os << std::endl; }};

template <Level L>
struct Channel
{
    void put(char c) { os.put(c); }
    void flush() { os.flush(); }

    friend Channel<L>& operator<<(Channel<L>& channel, Meta&& meta) {
        channel.os << meta.id << " [" << LevelTraits<L>::name << "] - ";
        return channel;
    }

    friend Channel<L>& operator<<(Channel<L>& channel, Manipulator manipulator) {
        manipulator.manip(channel.os);
        return channel;
    }

    template <typename T>
    friend Channel<L>& operator<<(Channel<L>& channel, T&& message) {
        channel.os << std::forward<T>(message);
        return channel;
    }

private:
    std::ostream& os = std::cout;
};

template <Level L>
Channel<L>& select() {
    static Channel<L> channel;
    return channel;
}

} // namespace log

} // namespace ecf::service

#endif /* ecflow_service_Log_HPP */
