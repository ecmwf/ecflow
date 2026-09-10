/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VSettingsLoader_HPP
#define ecflow_viewer_VSettingsLoader_HPP

#include <string>

// This class enables registered objects to read the settings updated with users settings via
// static function "loadSettings()"

class VSettingsLoader {
public:
    explicit VSettingsLoader();
    virtual ~VSettingsLoader() = default;

    virtual void loadSettings() = 0;
    static void process();

private:
    // No copy allowed
    explicit VSettingsLoader(const VSettingsLoader&)   = delete;
    VSettingsLoader& operator=(const VSettingsLoader&) = delete;
};

template <class T>
class SimpleSettingsLoader : public VSettingsLoader {
public:
    explicit SimpleSettingsLoader()
        : VSettingsLoader() {}

protected:
    void loadSettings() override { T::loadSettings(); }
};

#endif /* ecflow_viewer_VSettingsLoader_HPP */
