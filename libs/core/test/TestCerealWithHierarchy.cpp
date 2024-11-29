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
#include "ecflow/core/Serialization.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;
using namespace boost;

// ======================================================================================

class BaseCmd {
public:
    BaseCmd()          = default;
    virtual ~BaseCmd() = default;

    bool operator==(const BaseCmd& rhs) const { return true; }
    void print(std::ostream& os) const {}
    virtual bool equals(BaseCmd* rhs) const = 0;

private:
    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& archive, std::uint32_t const version) {}
};

class Derived1 : public BaseCmd {
public:
    Derived1() = default;
    explicit Derived1(int x) : BaseCmd(), x_(x) {}
    ~Derived1() override = default;

    int get_x() const { return x_; }

    bool equals(BaseCmd* rhs) const override {
        auto* the_rhs = dynamic_cast<Derived1*>(rhs);
        if (!the_rhs)
            return false;
        if (x_ != the_rhs->get_x())
            return false;
        return true;
    }

    void print(std::ostream& os) const {
        os << "Derived1:";
        BaseCmd::print(os);
        os << ": x(" << x_ << ")";
    }

private:
    int x_{0};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar(cereal::base_class<BaseCmd>(this), x_);
    }
};

std::ostream& operator<<(std::ostream& os, Derived1 const& m) {
    m.print(os);
    return os;
}

class CmdContainer {
public:
    CmdContainer() = default;
    explicit CmdContainer(std::shared_ptr<BaseCmd> cmd) : cmd_(cmd) {}

    bool operator==(const CmdContainer& rhs) const {
        if (!cmd_.get() && !rhs.cmd_.get())
            return true;
        if (cmd_.get() && !rhs.cmd_.get())
            return false;
        if (!cmd_.get() && rhs.cmd_.get())
            return false;
        return (cmd_->equals(rhs.cmd_.get()));
    }

    void print(std::ostream& os) const {
        if (cmd_.get())
            cmd_->print(os);
        os << "NULL request";
    }

private:
    std::shared_ptr<BaseCmd> cmd_;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& archive) {
        archive(CEREAL_NVP(cmd_));
    }
};

std::ostream& operator<<(std::ostream& os, CmdContainer const& m) {
    m.print(os);
    return os;
}

CEREAL_REGISTER_TYPE(Derived1)

// =================================================================================

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_CerealWithHierarchy)

BOOST_AUTO_TEST_CASE(test_cereal_save_as_string_and_save_as_filename) {
    ECF_NAME_THIS_TEST();

    std::shared_ptr<BaseCmd> cmd = std::make_shared<Derived1>(10);
    CmdContainer originalCmd(cmd);
    std::string saved_cmd_as_string;

    // SAVE as string and file
    {
        BOOST_REQUIRE_NO_THROW(ecf::save("core.txt", originalCmd)); // save as filename
        ecf::save_as_string(saved_cmd_as_string, originalCmd); // save as string, this is buggy forgets trailing '}'
    }

    // RESTORE from filename and string
    {
        CmdContainer restoredCmd;
        BOOST_REQUIRE_NO_THROW(ecf::restore("core.txt", restoredCmd)); // restore from filename
        BOOST_REQUIRE_MESSAGE(restoredCmd == originalCmd,
                              "restoredCmd " << restoredCmd << "  originalCmd " << originalCmd);
    }
    {
        CmdContainer restoredCmd;
        ecf::restore_from_string(saved_cmd_as_string, restoredCmd); // restore form string fails, due to missing '}'
        BOOST_REQUIRE_MESSAGE(restoredCmd == originalCmd,
                              "restoredCmd " << restoredCmd << "  originalCmd " << originalCmd);
    }

    fs::remove("core.txt");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
