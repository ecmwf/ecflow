/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VConfigLoader_HPP
#define ecflow_viewer_VConfigLoader_HPP

#include <string>

class VProperty;

class VConfigLoader {
public:
    explicit VConfigLoader(const std::string& name);
    virtual ~VConfigLoader() = default;

    virtual void load(VProperty* group) = 0;
    static bool process(const std::string& name, VProperty*);

private:
    // No copy allowed
    explicit VConfigLoader(const VConfigLoader&)   = delete;
    VConfigLoader& operator=(const VConfigLoader&) = delete;
};

template <class T>
class SimpleLoader : public VConfigLoader {
    void load(VProperty* prop) override { T::load(prop); }

public:
    explicit SimpleLoader(const std::string& name)
        : VConfigLoader(name) {}
};

#endif /* ecflow_viewer_VConfigLoader_HPP */
