// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <thread>
#include <variant>
#include <vector>

#include "ecflow/service/aviso/Aviso.hpp"
#include "ecflow/service/aviso/BaseAvisoBackend.hpp"

namespace ecf::test {

///
/// @brief Drains the backend repeatedly, until the predicate holds for the collected responses or the timeout expires.
///
/// @tparam Predicate A callable taking the collected responses, and returning whether to stop draining.
/// @param[in,out] backend   The backend to drain.
/// @param[in]     predicate The condition on the collected responses that stops the draining.
/// @param[in]     timeout   The maximum time spent draining.
/// @return The responses collected, in the order they were drained.
///
template <typename Predicate>
std::vector<ecf::service::aviso::AvisoResponse>
drain_until(ecf::service::aviso::BaseAvisoBackend& backend,
            Predicate predicate,
            std::chrono::milliseconds timeout = std::chrono::seconds{10}) {
    std::vector<ecf::service::aviso::AvisoResponse> collected;
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!predicate(collected) && std::chrono::steady_clock::now() < deadline) {
        auto drained = backend.drain();
        collected.insert(collected.end(), drained.begin(), drained.end());
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
    return collected;
}

///
/// @brief Selects the responses of the given kind.
///
/// @tparam T The kind of response (e.g. AvisoNotification, AvisoError).
/// @param[in] responses The responses to select from.
/// @return The responses of kind T, in their original order.
///
template <typename T>
std::vector<T> select(const std::vector<ecf::service::aviso::AvisoResponse>& responses) {
    std::vector<T> selected;
    for (const auto& response : responses) {
        if (std::holds_alternative<T>(response)) {
            selected.push_back(std::get<T>(response));
        }
    }
    return selected;
}

} // namespace ecf::test
