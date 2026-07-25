// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#ifndef ecflow_node_ExprParser_HPP
#define ecflow_node_ExprParser_HPP

///
/// @brief Public facade for the trigger/complete expression parser.
///
/// @details This header re-exports the active parser implementation as the unqualified names
/// @c ExprParser and @c SimpleExprParser so that no consumer needs to know which version is
/// active.  The implementation version is selected by the @c using declarations below; to cut
/// over to V2, change both aliases from @c ecf::expression::v1 to @c ecf::expression::v2.
///

#include "ecflow/node/ExprParserV1.hpp"
#include "ecflow/node/SimpleExprParser.hpp"

// ---- Active implementation ----
// Change both lines to ecf::expression::v2::* when V2 is ready.
using ExprParser       = ecf::expression::v1::ExprParser;   ///< @copydoc ecf::expression::v1::ExprParser
using SimpleExprParser = ecf::expression::SimpleExprParser; ///< @copydoc ecf::expression::SimpleExprParser

#endif /* ecflow_node_ExprParser_HPP */
