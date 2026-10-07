// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <memory>
#include <string_view>
#include <vector>

#include "ecflow/service/aviso/Aviso.hpp"

namespace ecf::service::aviso {

///
/// @brief Defines the interface of a backend, which delivers the notifications of an Aviso server to one Aviso
/// attribute.
///
/// A backend serves exactly one attribute, between a call to subscribe() and the destruction of the backend.
/// Notifications and errors are collected in the background, and handed over by drain(), which is called by the
/// thread that owns the attribute.
///
/// The concrete backend is provided by the executable that handles Aviso attributes (e.g. the server) through
/// register_backend(); in any other executable no backend is available.
///
class BaseAvisoBackend {
public:
    virtual ~BaseAvisoBackend() = default;

    ///
    /// @brief Starts collecting the notifications described by the given request.
    ///
    /// @param request The fully resolved subscription request.
    ///
    virtual void subscribe(const AvisoSubscribe& request) = 0;

    ///
    /// @brief Returns, and forgets, the notifications and errors collected since the previous call.
    ///
    /// @return The collected responses, in the order in which they were received.
    ///
    virtual std::vector<AvisoResponse> drain() = 0;
};

///
/// @brief Creates a backend, ready to accept a subscription.
///
using AvisoBackendFactory = std::function<std::unique_ptr<BaseAvisoBackend>()>;

///
/// @brief Registers the factory used to create the backend of every Aviso attribute.
///
/// @param factory The factory; an empty factory unregisters the current one.
///
void register_backend(AvisoBackendFactory factory);

///
/// @brief Creates a backend, using the registered factory.
///
/// @return The new backend, or nullptr when no factory is registered.
///
std::unique_ptr<BaseAvisoBackend> make_backend();

///
/// @brief Describes why an Aviso attribute cannot receive notifications when no backend is available.
///
inline constexpr std::string_view no_backend = "Aviso notifications are not supported by this ecFlow build";

} // namespace ecf::service::aviso
