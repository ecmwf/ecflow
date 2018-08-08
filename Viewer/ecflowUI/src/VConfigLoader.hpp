//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VCONFIGLOADER_HPP_
#define VCONFIGLOADER_HPP_

#include <string>

class VProperty;

class VConfigLoader 
{
public:
    explicit VConfigLoader(const std::string& name);
    virtual ~VConfigLoader(); 

    virtual void load(VProperty* group) = 0;
    static bool process(const std::string& name,VProperty*);
    
private:
    // No copy allowed
    explicit VConfigLoader(const VConfigLoader&) = delete;
    VConfigLoader& operator=(const VConfigLoader&) = delete;
};

template<class T>
class SimpleLoader : public VConfigLoader {
    void load(VProperty* prop) { T::load(prop); }
public:
    explicit SimpleLoader(const std::string& name) : VConfigLoader(name) {}
};

#endif
