/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Filesystem.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Serialisation.hpp"

using namespace ecf;
using namespace boost;

class MyClass {
public:
    enum State { UNKNOWN = 0, COMPLETE = 1, QUEUED = 2, ABORTED = 3, SUBMITTED = 4, ACTIVE = 5, SUSPENDED = 6 };
    MyClass() = default;
    bool operator==(const MyClass& rhs) const { return x == rhs.x && y == rhs.y && state_ == rhs.state_; }
    void print(std::ostream& os) const { os << "MyClass: x(" << x << ") y(" << y << ") state(" << state_ << ")"; }

private:
    // This method lets cereal know which data members to serialize
    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& archive, std::uint32_t const version) {
        archive(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(state_));
    }
    int x{2}, y{2};
    State state_{State::SUSPENDED};
};
CEREAL_CLASS_VERSION(MyClass, 1)

class MyTop : public MyClass {
public:
    MyTop() = default;
    void set(int x, int y, int z) {
        x_ = x;
        y_ = y;
        z_ = z;
    }
    bool operator==(const MyTop& rhs) const {
        return (MyClass::operator==(rhs)) && x_ == rhs.x_ && y_ == rhs.y_ && z_ == rhs.z_;
    }
    void print(std::ostream& os) const {
        os << "MyTop:";
        MyClass::print(os);
        os << ": x(" << x_ << ") y(" << y_ << ") z(" << z_ << ")";
    }

private:
    // This method lets cereal know which data members to serialize
    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& archive, std::uint32_t const version) {
        archive(cereal::base_class<MyClass>(this), CEREAL_NVP(x_), CEREAL_NVP(y_), CEREAL_NVP(z_));
    }
    int x_{1}, y_{1}, z_{1};
};
CEREAL_CLASS_VERSION(MyTop, 1)

std::ostream& operator<<(std::ostream& os, MyTop const& m) {
    m.print(os);
    return os;
}

// =================================================================================

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Cereal)

BOOST_AUTO_TEST_CASE(test_cereal_json) {
    ECF_NAME_THIS_TEST();

    std::string path = "test_cereal_json";
    {
        std::ofstream os(path);
        cereal::JSONOutputArchive oarchive(os); // Create an output archive

        MyTop m1, m2, m3;
        oarchive(cereal::make_nvp("MyTop", m1), m2, m3); // Write the data to the archive
    } // archive goes out of scope, ensuring all contents are flushed

    {
        BOOST_CHECK_MESSAGE(fs::exists(path), " Expected file to exist");

        std::ifstream is(path);
        cereal::JSONInputArchive iarchive(is); // Create an input archive

        MyTop m1, m2, m3;
        iarchive(m1, m2, m3); // Read the data from the archive

        fs::remove(path); // Remove the file. Comment out for debugging
    }
}

BOOST_AUTO_TEST_CASE(test_cereal_json2) {
    ECF_NAME_THIS_TEST();

    MyTop m1;
    m1.set(10, 10, 10);
    std::string path = "test_cereal_json2";
    { ecf::doSave(path, m1); }
    { ecf::doRestore(path, m1); }
    { ecf::doSaveAndRestore<MyTop>(path); }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
