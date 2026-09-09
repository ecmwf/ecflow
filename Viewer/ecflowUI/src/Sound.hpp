/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_Sound_HPP
#define ecflow_viewer_Sound_HPP

#include <string>
#include <vector>

#include "VProperty.hpp"

class Sound : public VPropertyObserver {
public:
    static Sound* instance();

    const std::vector<std::string>& sysSounds() const { return sysSounds_; }
    const std::string sysDir() const { return sysDir_; }

    void playSystem(const std::string&, int repeat);
    void play(const std::string&, int repeat);
    bool isSoundFile(const std::string& fName) const;

    void notifyChange(VProperty*) override {};

    // Called from VConfigLoader
    static void load(VProperty* group);

protected:
    Sound();
    void setCurrentPlayer(const std::string&);

    static Sound* instance_;

    std::map<std::string, std::string> players_;
    std::string currentPlayer_;
    std::string currentCmd_;

    std::string formats_{".+\\.(wav|mp3|ogg|oga)"};
    std::string sysDir_;
    std::vector<std::string> sysSounds_;
    time_t prevPlayedAt_{0};
    int delay_{2};
    VProperty* prop_{nullptr};
};

#endif /* ecflow_viewer_Sound_HPP */
