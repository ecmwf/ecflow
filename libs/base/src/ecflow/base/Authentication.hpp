/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_base_Authentication_HPP
#define ecflow_base_Authentication_HPP

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/core/Result.hpp"

namespace ecf {

using authentication_t = Result<bool>;

authentication_t is_authentic(const ClientToServerCmd& command, AbstractServer& server);

} // namespace ecf

#endif /* ecflow_base_Authentication_HPP */
