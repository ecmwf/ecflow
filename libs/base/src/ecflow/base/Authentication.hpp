/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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
