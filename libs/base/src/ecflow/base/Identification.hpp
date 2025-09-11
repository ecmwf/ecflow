/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_Identification_HPP
#define ecflow_base_Identification_HPP

#include "ecflow/base/Cmd.hpp"
#include "ecflow/core/Identity.hpp"

namespace ecf {

Identity identify(const Cmd_ptr& cmd);

} // namespace ecf

#endif /* ecflow_base_Identification_HPP */
