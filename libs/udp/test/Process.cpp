/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "Process.hpp"

#include <boost/version.hpp>
#if BOOST_VERSION < 106600

    #error "Boost version >= 1.66.0 is required"

#elif BOOST_VERSION >= 106600 && BOOST_VERSION < 108600

    #define BOOST_PROCESS_VERSION 1
    #include <boost/process.hpp>

namespace bp = boost::process;

#else // BOOST_VERSION >= 108600

    #define BOOST_PROCESS_VERSION 2
    #include <boost/asio.hpp>
    #include <boost/process.hpp>

namespace ba = boost::asio;
    #if BOOST_VERSION < 108800
namespace bp = boost::process::v2;
    #else
namespace bp = boost::process;
    #endif

#endif

namespace ecf::test {

#if BOOST_PROCESS_VERSION == 1
struct Process::Impl
{
    Impl(std::string_view executable, std::vector<std::string_view> args) : handle_{} {
        // Determine the invocation command
        auto invoke_command = std::string(executable);
        for (const auto& arg : args) {
            invoke_command += " " + std::string(arg);
        }

        // Start the process
        handle_ = bp::child{invoke_command};
    }
    ~Impl() = default;

    bp::child handle_;
};
#elif BOOST_PROCESS_VERSION == 2
struct Process::Impl
{
    static std::vector<boost::string_view> to_boost_string_view(std::vector<std::string_view> args) {
        std::vector<boost::string_view> result;
        result.reserve(args.size());
        for (const auto& arg : args) {
            result.emplace_back(arg.data(), arg.size());
        }
        return result;
    }

    Impl(std::string_view executable, std::vector<std::string_view> args)
        : ctx_{},
          handle_{ctx_, executable, to_boost_string_view(args)} {
        // The process is started in the constructor
    }
    ~Impl() = default;

    ba::io_context ctx_;
    bp::process handle_;
};

#else
    #error "Unsupported boost::process version"
#endif

Process::Process() : impl_{nullptr} {
}

Process::Process(Process&& rhs) : impl_{std::move(rhs.impl_)} {
    rhs.impl_ = nullptr;
}

Process::Process(std::string_view executable, std::vector<std::string_view> args)
    : impl_{std::make_unique<Impl>(executable, args)} {
}

Process& Process::operator=(Process&& rhs) {
    if (this != &rhs) {
        impl_     = std::move(rhs.impl_);
        rhs.impl_ = nullptr;
    }
    return *this;
}

Process::~Process() = default;

void Process::terminate() {
    impl_ = nullptr;
}

} // namespace ecf::test
