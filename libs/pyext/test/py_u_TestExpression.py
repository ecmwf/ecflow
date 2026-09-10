# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

import ecflow as ecf


def test_create_expression_from_part():
    part = ecf.PartExpression("t1 == complete")
    expr = ecf.Expression(part)

    assert expr.parts == [part]
    assert expr.get_expression() == "t1 == complete"


def test_create_expression_from_several_parts():
    expr = ecf.Expression(ecf.PartExpression("t1 == complete OR t4 == complete"))
    expr.add(ecf.PartExpression("t5 == active", True))
    expr.add(ecf.PartExpression("t7 == active", False))

    assert expr.parts == [
        ecf.PartExpression("t1 == complete OR t4 == complete"),
        ecf.PartExpression("t5 == active", True),
        ecf.PartExpression("t7 == active", False),
    ]
    assert (
        expr.get_expression()
        == "t1 == complete OR t4 == complete AND t5 == active OR t7 == active"
    )
