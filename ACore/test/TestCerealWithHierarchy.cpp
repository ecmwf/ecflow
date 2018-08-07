//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description
//============================================================================
#include <sstream>
#include <fstream>
#include <iostream>
#include <memory>

#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "Serialization.hpp"

using namespace ecf;
using namespace boost;
using namespace std;
namespace fs = boost::filesystem;

// ======================================================================================

class BaseCmd {
public:
   BaseCmd() = default;
   virtual ~BaseCmd() = default;

   bool operator==(const BaseCmd & rhs) const { return true;}
   void print(std::ostream &os) const {}
   virtual bool equals(BaseCmd* rhs) const = 0;
private:
   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & archive, std::uint32_t const version) {}
};

class Derived1 : public BaseCmd {
public:
   Derived1() : x_(0) {}
   Derived1(int x) : BaseCmd(), x_(x) {}
   virtual ~Derived1() = default;

   int get_x() const { return x_;}

   virtual bool equals(BaseCmd* rhs ) const {
      auto* the_rhs = dynamic_cast<Derived1*>(rhs);
      if (!the_rhs) return false;
      if (x_ !=  the_rhs->get_x()) return false;
      return true;
   }

   void print(std::ostream &os) const {
      os << "Derived1:";
      BaseCmd::print(os);
      os << ": x("<< x_ << ")";
   }
private:
   int x_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version) {
      ar(cereal::base_class<BaseCmd>( this ),
         x_);
   }
};

std::ostream& operator<<(std::ostream &os, Derived1 const &m) {
   m.print(os);
   return os;
}

class CmdContainer {
public:
   CmdContainer() = default;
   CmdContainer(std::shared_ptr<BaseCmd> cmd) : cmd_(cmd) {}

   bool operator==(const CmdContainer& rhs) const {
      if (!cmd_.get() && !rhs.cmd_.get())  return true;
      if (cmd_.get() && !rhs.cmd_.get())   return false;
      if (!cmd_.get() && rhs.cmd_.get())   return false;
      return (cmd_->equals(rhs.cmd_.get()));
   }

   void print(std::ostream &os) const {
      if (cmd_.get()) cmd_->print(os);
      os << "NULL request";
   }

private:
   std::shared_ptr<BaseCmd> cmd_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & archive) {
      archive(CEREAL_NVP(cmd_)  );
   }
};

std::ostream& operator<<(std::ostream &os, CmdContainer const &m) {
   m.print(os);
   return os;
}

CEREAL_REGISTER_TYPE(Derived1);

// =================================================================================

BOOST_AUTO_TEST_SUITE( CoreTestSuite )

BOOST_AUTO_TEST_CASE( test_cereal_save_as_string_and_save_as_filename )
{
   cout << "ACore:: ...test_cereal_save_as_string_and_save_as_filename\n" ;

   std::shared_ptr<BaseCmd> cmd = std::make_shared<Derived1>(10);
   CmdContainer originalCmd(cmd);
   std::string saved_cmd_as_string;

   // SAVE as string and file
   {
      BOOST_REQUIRE_NO_THROW(ecf::save("ACore.txt",originalCmd )); // save as filename
      ecf::save_as_string(saved_cmd_as_string,originalCmd );       // save as string, this is buggy forgets trailing '}'
   }

   // RESTORE from filename and string
   {
      CmdContainer restoredCmd;
      BOOST_REQUIRE_NO_THROW(ecf::restore("ACore.txt", restoredCmd)); // restore from filename
      BOOST_REQUIRE_MESSAGE(restoredCmd  ==  originalCmd, "restoredCmd " << restoredCmd  << "  originalCmd " << originalCmd );
   }
   {
       //cout <<  saved_cmd_as_string << "\n";
       CmdContainer  restoredCmd ;
       ecf::restore_from_string(saved_cmd_as_string , restoredCmd );  // restore form string fails, due to missing '}'
       BOOST_REQUIRE_MESSAGE(restoredCmd == originalCmd, "restoredCmd " << restoredCmd  << "  originalCmd " << originalCmd );
   }

   fs::remove("ACore.txt");
}

BOOST_AUTO_TEST_SUITE_END()
