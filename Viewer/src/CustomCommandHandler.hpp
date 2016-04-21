//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================
#ifndef VIEWER_SRC_CUSTOMCOMMANDHANDLER_HPP_
#define VIEWER_SRC_CUSTOMCOMMANDHANDLER_HPP_

#include <string>
#include <deque>


class VSettings;

class CustomCommand
{
public:
	CustomCommand(const std::string &name, const std::string &command, bool context);
	std::string name()    const {return name_;};
	std::string command() const {return command_;};
	bool inContextMenu()  const {return inContextMenu_;};
	std::string contextString() const {return (inContextMenu_ ? "yes" : "no");};
	void set(const std::string &name, const std::string &command, bool context);
	void save(VSettings *vs);


private:
	std::string name_;
	std::string command_;
	bool        inContextMenu_;
};


class CustomCommandHandler
{
public:
	CustomCommandHandler();

	virtual CustomCommand* add(const std::string& name, const std::string& command, bool context, bool WriteSettings) = 0;
	CustomCommand* replace(int index, const std::string& name, const std::string& command, bool context);
	CustomCommand* replace(int index, const CustomCommand &cmd);
	CustomCommand* duplicate(int index);
	void remove(int index);
	//void remove(const std::string& name);
	//void remove(CustomCommand*);
	//CustomCommand* find(const std::string& name) const;

	//void save();
	//void save(CustomCommand*);
	void init(const std::string& configFilePath);
	//const std::vector<NodeQuery*>& items() const {return items_;}
	CustomCommand* find(const std::string& name) const;
	int findIndexFromName(const std::string& name) const;
	int numCommands() {return items_.size();};
	CustomCommand *commandFromIndex(int i) {return items_[i];};
	bool stringToBool(std::string &str);
	void swapCommandsByIndex(int i1, int i2);
	void writeSettings();

protected:
	void readSettings();

	std::string dirPath_;
	const std::string suffix_;
	std::deque<CustomCommand*> items_;
};



// ----------------------------------------------------------------------------------------------
// specialisation of CustomCommandHandler to handle the commands that the user has manually saved
// ----------------------------------------------------------------------------------------------

class CustomSavedCommandHandler : public CustomCommandHandler
{
public:
    CustomSavedCommandHandler() {};
    CustomCommand* add(const std::string& name, const std::string& command, bool context, bool saveSettings);

    static CustomSavedCommandHandler* instance();

protected:
    static CustomSavedCommandHandler* instance_;
};



// --------------------------------------------------------------------------------------------
// specialisation of CustomCommandHandler to handle the commands that the user has recently run
// --------------------------------------------------------------------------------------------

class CustomCommandHistoryHandler : public CustomCommandHandler
{
public:
    CustomCommandHistoryHandler();
    CustomCommand* add(const std::string& name, const std::string& command, bool context, bool saveSettings);
    static CustomCommandHistoryHandler* instance();

protected:
    static CustomCommandHistoryHandler* instance_;
    int maxCommands_;
};




#endif /* VIEWER_SRC_CUSTOMCOMMANDHANDLER_HPP_ */
