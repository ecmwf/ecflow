/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_base_Identification_HPP
#define ecflow_base_Identification_HPP

#include "ecflow/base/Cmd.hpp"
#include "ecflow/core/Identity.hpp"

namespace ecf {

Identity identify(const Cmd_ptr& cmd);

} // namespace ecf

#endif /* ecflow_base_Identification_HPP */
