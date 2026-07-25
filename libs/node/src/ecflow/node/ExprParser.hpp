/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_ExprParser_HPP
#define ecflow_node_ExprParser_HPP

///
/// @brief Public facade for the trigger/complete expression parser.
///
/// @details This header re-exports the active parser implementation as the unqualified
/// @c ExprParser name so that consumers do not depend on a parser version. The V1-only
/// @c SimpleExprParser fast path remains available for its unit tests until V1 is removed.
///

#include "ecflow/node/ExprParserV2.hpp"
#include "ecflow/node/SimpleExprParser.hpp"

// ---- Active implementation ----
using ExprParser       = ecf::expression::v2::ExprParser;   ///< @copydoc ecf::expression::v2::ExprParser
using SimpleExprParser = ecf::expression::SimpleExprParser; ///< @copydoc ecf::expression::SimpleExprParser

#endif /* ecflow_node_ExprParser_HPP */
