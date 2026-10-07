// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <memory>
#include <vector>

#include "ecflow/service/aviso/BaseAvisoBackend.hpp"

namespace ecf::service::aviso {

///
/// @brief Delivers the notifications of an Aviso server to one Aviso attribute, using the aviso-client library.
///
/// The backend holds one client and one watch. Notifications and errors are collected by the library threads and
/// handed over by drain(). The library reconnects by itself after routine interruptions (e.g. the server closing the
/// stream at the end of its connection lifetime); when the watch ends nevertheless (i.e. after an error the library
/// does not recover from), a dedicated timer re-creates it after the retry delay, resuming after the last notification
/// received (or, for the first watch, after the revision given by subscribe()).
///
/// This backend is only available in executables linked with the aviso-client library (e.g. the server).
///
class AvisoBackend : public BaseAvisoBackend {
public:
    ///
    /// @brief The default delay before re-creating a watch that ended.
    ///
    static constexpr std::chrono::milliseconds default_retry_delay{std::chrono::seconds{60}};

    ///
    /// @brief Creates a backend.
    ///
    /// @param[in] retry_delay The delay before re-creating a watch that ended.
    ///
    explicit AvisoBackend(std::chrono::milliseconds retry_delay = default_retry_delay);

    AvisoBackend(const AvisoBackend&)            = delete;
    AvisoBackend& operator=(const AvisoBackend&) = delete;

    ///
    /// @brief Stops the watch, waiting for the library to release it.
    ///
    ~AvisoBackend() override;

    void subscribe(const AvisoSubscribe& request) override;

    std::vector<AvisoResponse> drain() override;

    ///
    /// @brief Registers this backend as the backend of every Aviso attribute.
    ///
    static void register_as_default();

private:
    struct Impl;

    std::chrono::milliseconds retry_delay_;
    std::shared_ptr<Impl> impl_;
};

} // namespace ecf::service::aviso
