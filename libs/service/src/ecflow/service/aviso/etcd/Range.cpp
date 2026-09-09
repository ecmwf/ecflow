/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/service/aviso/etcd/Range.hpp"

#include <cassert>

namespace ecf::service::aviso::etcd {

Range::key_t Range::increment_last_byte(key_t val) {
    assert(!val.empty());
    val[val.size() - 1]++;
    return val;
}

} // namespace ecf::service::aviso::etcd
