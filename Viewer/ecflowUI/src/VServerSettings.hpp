//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================


#ifndef VSERVERSETTINGS_HPP_
#define VSERVERSETTINGS_HPP_

#include "VProperty.hpp"

#include <set>

class ServerHandler;

class VServerSettings : public VPropertyObserver
{
	friend class ServerHandler;

public:
    enum Param {AutoUpdate, UpdateRate,
               AdaptiveUpdate,AdaptiveUpdateIncrement,MaxAdaptiveUpdateRate,AdaptiveUpdateMode,
               MaxOutputFileLines,ReadFromDisk,
               NodeMenuMode,
	           NotifyAbortedEnabled, NotifyAbortedPopup, NotifyAbortedSound,
			   NotifyRestartedEnabled, NotifyRestartedPopup, NotifyRestartedSound,
			   NotifyLateEnabled, NotifyLatePopup, NotifyLateSound,
			   NotifyZombieEnabled, NotifyZombiePopup, NotifyZombieSound,
               NotifyAliasEnabled, NotifyAliasPopup, NotifyAliasSound, UnknownParam};

	int intValue(Param par) const;
	bool boolValue(Param par) const;
    QString stringValue(Param par) const;
    VProperty* guiProp() const {return guiProp_;}
	bool notificationsEnabled() const;
    static std::string notificationId(Param);
    static Param notificationParam(const std::string& id);

	void saveSettings();

	//From VPropertyObserver
	void notifyChange(VProperty*);

	//Called from VConfigLoader
	static void load(VProperty*);

	static void importRcFiles();

protected:
	explicit VServerSettings(ServerHandler* server);
	~VServerSettings();

	VProperty* property(Param par) const;
	void loadSettings();

	ServerHandler* server_;
	VProperty* prop_;
	VProperty* guiProp_;
	std::map<Param,VProperty*> parToProp_;
	std::map<VProperty*,Param> propToPar_;

	static std::map<Param,std::string> parNames_;
	static VProperty* globalProp_;
	static std::map<Param,std::string> notifyIds_;
};

#endif
