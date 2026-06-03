/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_VServerSettings_HPP
#define ecflow_viewer_VServerSettings_HPP

#include <set>

#include "VProperty.hpp"

class ServerHandler;

class VServerSettings : public VPropertyObserver {
    friend class ServerHandler;

public:
    enum Param {
        AutoUpdate,
        UpdateRate,
        AdaptiveUpdate,
        AdaptiveUpdateIncrement,
        MaxAdaptiveUpdateRate,
        AdaptiveUpdateMode,
        MaxOutputFileLines,
        ReadFromDisk,
        MaxSizeForTimelineData,
        NodeMenuMode,
        NodeMenuModeForDefStatus,
        NotifyAbortedEnabled,
        NotifyAbortedPopup,
        NotifyAbortedSound,
        NotifyRestartedEnabled,
        NotifyRestartedPopup,
        NotifyRestartedSound,
        NotifyLateEnabled,
        NotifyLatePopup,
        NotifyLateSound,
        NotifyZombieEnabled,
        NotifyZombiePopup,
        NotifyZombieSound,
        NotifyAliasEnabled,
        NotifyAliasPopup,
        NotifyAliasSound,
        UidForServerLogTransfer,
        UnknownParam,
        UserLogServerHost,
        UserLogServerPort
    };

    ///
    /// @brief Access the associated integer value of the given parameter.
    ///
    /// @param par The parameter for which to retrieve the integer value.
    /// @return The integer value for the given parameter, or 0 if the parameter is not found/not an integer.
    ///
    int intValue(Param par) const;

    ///
    /// @brief Access the associated boolean value of the given parameter.
    ///
    /// @param par The parameter for which to retrieve the boolean value.
    /// @return The boolean value for the given parameter, or false if the parameter is not found/not a boolean.
    ///
    bool boolValue(Param par) const;

    ///
    /// @brief Access the associated string value of the given parameter.
    ///
    /// @param par The parameter for which to retrieve the string value.
    /// @return The string value for the given parameter, or an empty string if the parameter is not found/not a string.
    ///
    QString stringValue(Param par) const;

    VProperty* guiProp() const { return guiProp_; }
    bool notificationsEnabled() const;
    static std::string notificationId(Param);
    static Param notificationParam(const std::string& id);

    void saveSettings();

    // From VPropertyObserver
    void notifyChange(VProperty*) override;

    // Called from VConfigLoader
    static void load(VProperty*);

    static void importRcFiles();

protected:
    explicit VServerSettings(ServerHandler* server);
    ~VServerSettings() override;

    ///
    /// @brief Access the property associated with the given parameter.
    ///
    /// @param par The parameter for which to retrieve the property.
    /// @return The property associated with the parameter; or nullptr, if parameter is not found
    ///
    VProperty* property(Param par) const;
    void loadSettings();

    ServerHandler* server_;
    VProperty* prop_;
    VProperty* guiProp_;
    std::map<Param, VProperty*> parToProp_;
    std::map<VProperty*, Param> propToPar_;

    static std::map<Param, std::string> parNames_;
    static VProperty* globalProp_;
    static std::map<Param, std::string> notifyIds_;
};

#endif /* ecflow_viewer_VServerSettings_HPP */
