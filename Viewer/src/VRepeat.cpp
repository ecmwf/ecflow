//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "VRepeat.hpp"

#include <sstream>
#include "RepeatAttr.hpp"

std::map<std::string,std::string> VRepeat::typeNames_;

static long ecf_repeat_date_to_julian(long ddate);
static long ecf_repeat_julian_to_date(long jdate);

long ecf_repeat_julian_to_date(long jdate)
{
    long x,y,d,m,e;
    long day,month,year;

    x = 4 * jdate - 6884477;
    y = (x / 146097) * 100;
    e = x % 146097;
    d = e / 4;

    x = 4 * d + 3;
    y = (x / 1461) + y;
    e = x % 1461;
    d = e / 4 + 1;

    x = 5 * d - 3;
    m = x / 153 + 1;
    e = x % 153;
    d = e / 5 + 1;

    if( m < 11 )
        month = m + 2;
    else
        month = m - 10;


    day = d;
    year = y + m / 11;

    return year * 10000 + month * 100 + day;
}

long ecf_repeat_date_to_julian(long ddate)
{
    long  m1,y1,a,b,c,d,j1;

    long month,day,year;

    year = ddate / 10000;
    ddate %= 10000;
    month  = ddate / 100;
    ddate %= 100;
    day = ddate;

    if (0) {
      a = (14 - month) / 12;
      y1 = year + 4800 - a;
      m1 = month + 12*a - 3;
      j1 = day + (153*m1 + 2)/5 + 365*y1 + y1/4 - y1/100 + y1/400 - 32045;
      return j1 - 0.5;
    }

    if (month > 2)
    {
        m1 = month - 3;
        y1 = year;
    }
    else
    {
        m1 = month + 9;
        y1 = year - 1;
    }
    a = 146097*(y1/100)/4;
    d = y1 % 100;
    b = 1461*d/4;
    c = (153*m1+2)/5+day+1721119;
    j1 = a+b+c;

    return j1;
}

const std::string& VRepeat::valueType(const Repeat& r)
{
    if(typeNames_.empty())
    {
        typeNames_["repeat date"]="date";
        typeNames_["repeat integer"]="integer";
        typeNames_["repeat string"]="string";
        typeNames_["repeat enumerated"]="enumerated";
        typeNames_["repeat day"]="day";
    }

    static std::string noTypeName="";

    std::string t=r.toString();
    for(std::map<std::string,std::string>::const_iterator it=typeNames_.begin(); it != typeNames_.end(); ++it)
    {
        if(t.find(it->first) == 0)
            return it->second;
    }

    return noTypeName;
}

VRepeat* VRepeat::make(const Repeat& r)
{
    const std::string t=VRepeat::valueType(r);
    if(t == "date")
        return new VRepeatDate(r);
    else if(t == "integer")
        return new VRepeatInt(r);
    else if(t == "string")
        return new VRepeatString(r);
    else if(t == "enumerated")
        return new VRepeatEnum(r);
    else if(t == "day")
        return new VRepeatDay(r);

    return NULL;
}

int VRepeatDate::endIndex() const
{
    return (ecf_repeat_date_to_julian(repeat_.end()) -
            ecf_repeat_date_to_julian(repeat_.start())) / repeat_.step() + 1;
}

int VRepeatDate::currentIndex() const
{
    int cur=(ecf_repeat_date_to_julian(repeat_.index_or_value()) -
                ecf_repeat_date_to_julian(repeat_.start())) / repeat_.step();

    //int gui = ecf_repeat_julian_to_date(ecf_repeat_date_to_julian(repeat_.start()) + cur * step());
    return cur;
}

std::string VRepeatDate::value(int index) const
{
    std::stringstream ss;
    ss << (ecf_repeat_julian_to_date
          (ecf_repeat_date_to_julian(repeat_.start()) + index * repeat_.step()));

    return ss.str();
}


int VRepeatInt::endIndex() const
{
   if(repeat_.step() >0)
   {
       int index=(repeat_.end() - repeat_.start()) / repeat_.step() + 1;
       int val=repeat_.start() + index*repeat_.step();
       while(val > repeat_.end() && index >=1)
       {
          index--;
          val=repeat_.start() + index*repeat_.step();
       }
       return index;
   }
   return 0;
}

int VRepeatInt::currentIndex() const
{
     if(repeat_.step() >0)
     {
         return (repeat_.index_or_value() - repeat_.start())/repeat_.step();
     }
     return 0;
}

std::string VRepeatInt::value(int index) const
{
    std::stringstream ss;
    ss << repeat_.start() + index*repeat_.step();
    return ss.str();
}

std::string VRepeatDay::value(int /*index*/) const
{
    std::stringstream ss;
    ss << repeat_.step();
    return ss.str();
}

std::string VRepeatEnum::value(int index) const
{
    return repeat_.value_as_string(index);
}

std::string VRepeatString::value(int index) const
{
    return repeat_.value_as_string(index);
}
