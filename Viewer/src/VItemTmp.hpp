//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VITEMTMP_HPP
#define VITEMTMP_HPP

#include <boost/shared_ptr.hpp>

class VAttribute;
class VItem;

class VItemTmp;
typedef boost::shared_ptr<VItemTmp> VItemTmp_ptr;

class VItemTmp
{
public:
    ~VItemTmp();

    VItem* item() const {return item_;}
    VAttribute* attribute() const;
    static VItemTmp_ptr create(VItem* item) {return VItemTmp_ptr(new VItemTmp(item));}

private:
    VItemTmp(VItem* item) : item_(item) {}
    VItem* item_;

};

#endif // VITEMTMP_HPP

