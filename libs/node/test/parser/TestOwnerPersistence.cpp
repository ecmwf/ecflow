// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <boost/test/unit_test.hpp>

#include "PersistHelper.hpp"
#include "ecflow/attribute/OwnerAttr.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Parser)

BOOST_AUTO_TEST_SUITE(T_OwnerPersistence)

namespace {

Defs make_owned_defs() {
    Defs defs;
    suite_ptr s = defs.add_suite("s");
    s->addOwner(OwnerAttr::create("alice"));
    family_ptr f = s->add_family("f");
    f->addOwner(OwnerAttr::create("charlie:ops"));
    f->add_task("t1")->addOwner(OwnerAttr::create("bob"));
    f->add_task("t2");
    s->add_task("t3");
    return defs;
}

} // namespace

BOOST_AUTO_TEST_CASE(test_owner_persist_and_reload_in_every_style) {
    ECF_NAME_THIS_TEST();

    Defs defs = make_owned_defs();
    for (auto style : {PrintStyle::DEFS, PrintStyle::STATE, PrintStyle::MIGRATE, PrintStyle::NET}) {
        PersistHelper helper;
        BOOST_CHECK_MESSAGE(helper.test_persist_and_reload(defs, style),
                            PrintStyle::to_string(style) << ": " << helper.errorMsg());
    }
}

BOOST_AUTO_TEST_CASE(test_owner_checkpoint_and_reload) {
    ECF_NAME_THIS_TEST();

    Defs defs = make_owned_defs();
    {
        PersistHelper helper;
        BOOST_CHECK_MESSAGE(helper.test_defs_checkpt_and_reload(defs), helper.errorMsg());
    }
    {
        PersistHelper helper;
        BOOST_CHECK_MESSAGE(helper.test_cereal_checkpt_and_reload(defs), helper.errorMsg());
    }
    {
        PersistHelper helper;
        BOOST_CHECK_MESSAGE(helper.test_state_persist_and_reload_with_checkpt(defs), helper.errorMsg());
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
