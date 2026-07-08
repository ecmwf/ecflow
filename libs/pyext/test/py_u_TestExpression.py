#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

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
