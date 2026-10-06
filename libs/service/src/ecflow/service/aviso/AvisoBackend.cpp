// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/service/aviso/AvisoBackend.hpp"

#include <mutex>
#include <utility>

namespace ecf::service::aviso {

namespace {

struct Registration
{
    std::mutex mutex;
    AvisoBackendFactory factory;
};

Registration& registration() {
    static Registration instance;
    return instance;
}

} // namespace

void register_backend(AvisoBackendFactory factory) {
    auto& r = registration();
    std::scoped_lock lock(r.mutex);
    r.factory = std::move(factory);
}

std::unique_ptr<AvisoBackend> make_backend() {
    auto& r = registration();
    std::scoped_lock lock(r.mutex);
    return r.factory ? r.factory() : nullptr;
}

} // namespace ecf::service::aviso
