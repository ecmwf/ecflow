// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <memory>
#include <vector>

#include "ecflow/service/aviso/AvisoBackend.hpp"

namespace ecf::service::aviso::v2 {

///
/// @brief Delivers the notifications of an Aviso v2 server to one Aviso attribute, using the aviso-client library.
///
/// The backend holds one client and one watch. Notifications and errors are collected by the library threads and
/// handed over by drain(). When the watch ends (with an error, or because the server closed the stream), a dedicated
/// timer re-creates it after the retry delay, resuming after the last notification received (or, for the first
/// watch, after the revision given by subscribe()).
///
/// This backend is only available in executables linked with the aviso-client library (e.g. the server).
///
class AvisoV2Backend : public AvisoBackend {
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
    explicit AvisoV2Backend(std::chrono::milliseconds retry_delay = default_retry_delay);

    AvisoV2Backend(const AvisoV2Backend&)            = delete;
    AvisoV2Backend& operator=(const AvisoV2Backend&) = delete;

    ///
    /// @brief Stops the watch, waiting for the library to release it.
    ///
    ~AvisoV2Backend() override;

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

} // namespace ecf::service::aviso::v2
