//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VSETTINGSLOADER_HPP
#define VSETTINGSLOADER_HPP

#include <string>

//This class enables registered objects to read the settings updated with users settings via
//static function "loadSettings()"

class VSettingsLoader
{
public:
    explicit VSettingsLoader();
    virtual ~VSettingsLoader() {}

    virtual void loadSettings() = 0;
    static void process();

private:
    // No copy allowed
    explicit VSettingsLoader(const VSettingsLoader&);
    VSettingsLoader& operator=(const VSettingsLoader&);
};

template<class T>
class SimpleSettingsLoader : public VSettingsLoader
{
public:
    explicit SimpleSettingsLoader() : VSettingsLoader() {}
protected:
    void loadSettings() { T::loadSettings();}
};

#endif // VSETTINGSLOADER_HPP
