// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <boost/test/unit_test.hpp>

#include "ecflow/service/aviso/AvisoBackend.hpp"

namespace {

class FakeBackend : public ecf::service::aviso::AvisoBackend {
public:
    void subscribe(const ecf::service::aviso::AvisoSubscribe& request) override { path = request.path(); }

    std::vector<ecf::service::aviso::AvisoResponse> drain() override {
        return {ecf::service::aviso::AvisoNotification{"mars", 1, "{}", "null"}};
    }

    std::string path;
};

///
/// @brief Unregisters any backend factory when going out of scope, so that each test case starts afresh.
///
struct BackendGuard
{
    ~BackendGuard() { ecf::service::aviso::register_backend({}); }
};

} // namespace

BOOST_AUTO_TEST_SUITE(U_Aviso)

BOOST_AUTO_TEST_SUITE(T_AvisoBackend)

BOOST_AUTO_TEST_CASE(no_backend_is_available_by_default) {
    using namespace ecf::service::aviso;

    BOOST_CHECK(make_backend() == nullptr);
}

BOOST_AUTO_TEST_CASE(can_create_backend_from_registered_factory) {
    using namespace ecf::service::aviso;

    BackendGuard guard;
    register_backend([]() { return std::make_unique<FakeBackend>(); });

    auto backend = make_backend();
    BOOST_REQUIRE(backend != nullptr);

    backend->subscribe(AvisoSubscribe{"/s/f/t:a", "{}", "http://aviso:8000", 0, ""});
    BOOST_CHECK_EQUAL(dynamic_cast<FakeBackend&>(*backend).path, "/s/f/t:a");

    auto responses = backend->drain();
    BOOST_REQUIRE_EQUAL(responses.size(), 1u);
    BOOST_CHECK(std::holds_alternative<AvisoNotification>(responses.front()));
}

BOOST_AUTO_TEST_CASE(each_backend_is_a_new_instance) {
    using namespace ecf::service::aviso;

    BackendGuard guard;
    register_backend([]() { return std::make_unique<FakeBackend>(); });

    auto first  = make_backend();
    auto second = make_backend();
    BOOST_CHECK(first.get() != second.get());
}

BOOST_AUTO_TEST_CASE(can_unregister_factory) {
    using namespace ecf::service::aviso;

    register_backend([]() { return std::make_unique<FakeBackend>(); });
    register_backend({});

    BOOST_CHECK(make_backend() == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
