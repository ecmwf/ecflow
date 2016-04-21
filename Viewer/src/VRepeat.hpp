//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VREPEAT_HPP
#define VREPEAT_HPP

#include <string>
#include "RepeatAttr.hpp"

class VRepeat
{
public:
    enum Type {StringType,IntType,NoType};

    virtual ~VRepeat() {}
    int startIndex() const {return 0;}
    virtual int endIndex() const=0;
    virtual int currentIndex() const=0;
    virtual int step() const {return repeat_.step();}
    virtual std::string value(int index) const=0;
    Type type() const {return type_;}

    static VRepeat* make(const Repeat& r);

protected:
    VRepeat(const Repeat& r,Type t) : repeat_(r), type_(t) {}

    const Repeat& repeat_;
    Type type_;
};

class VRepeatDate : public VRepeat
{
public:
    VRepeatDate(const Repeat& r) : VRepeat(r,IntType) {}
    int endIndex() const;
    int currentIndex() const;
    std::string value(int index) const;
};

class VRepeatDay : public VRepeat
{
public:
    VRepeatDay(const Repeat& r) : VRepeat(r,IntType) {}
    int endIndex() const {return 0;}
    int currentIndex() const {return 0;}
    std::string value(int index) const;
};

class VRepeatInt : public VRepeat
{
public:
    VRepeatInt(const Repeat& r) : VRepeat(r,IntType) {}
    int endIndex() const;
    int currentIndex() const;
    std::string value(int index) const;
};

class VRepeatEnum : public VRepeat
{
public:
    VRepeatEnum(const Repeat& r) : VRepeat(r,StringType) {}
    int endIndex() const {return repeat_.end();}
    int currentIndex() const {return repeat_.index_or_value();}
    std::string value(int index) const;
};

class VRepeatString : public VRepeat
{
public:
    VRepeatString(const Repeat& r) : VRepeat(r,StringType) {}
    int endIndex() const {return repeat_.end();}
    int currentIndex() const {return repeat_.index_or_value();}
    std::string value(int index) const;
};


#endif // VREPEAT_HPP

