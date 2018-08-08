//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VSETTINGS_HPP_
#define VSETTINGS_HPP_

#include <boost/property_tree/ptree.hpp>
#include <QSettings>

#include <vector>

class VSettingsPath
{
public:
	void add(const std::string& key);
	void pop();

	std::string path() const;
	std::string path(const std::string& key) const;

protected:
	std::string join(const std::string& sep) const;

	std::vector<std::string> path_;
};

//This class stores the settings as a boost property tree and read/write them into/from json.
class VSettings
{
public:
	explicit VSettings(const std::string& file);
	explicit VSettings(boost::property_tree::ptree pt);
    virtual ~VSettings() = default;

	//bool read(const std::string &fs);
	//virtual void write(const std::string &fs);

    bool fileExists() const;
	bool read(bool failIfFileDoesNotExist = true);
	virtual void write();

	virtual void clear();
	bool contains(const std::string& key);
	bool containsFullPath(const std::string& key);

	virtual void beginGroup(const std::string&);
	virtual void endGroup();

	void put(const std::string& key,int val);
	//void put(const std::string& key,bool val);
	void put(const std::string& key,const std::string& val);
    void put(const std::string& key,const std::vector<std::string>& val);
    void put(const std::string& key,const std::vector<int>& val);
	void put(const std::string& key,const std::vector<VSettings>& val);
	void putAsBool(const std::string& key,bool val);

	template <typename T>
	T get(const std::string& key,const T& defaultVal)
	{
		return pt_.get<T>(path_.path(key),defaultVal);
	}
    void get(const std::string& key,std::vector<std::string>& val);
    void get(const std::string& key,std::vector<int>& val);
	bool getAsBool(const std::string& key,bool defaultVal);
	void get(const std::string& key,std::vector<VSettings>& val);

	const boost::property_tree::ptree & propertyTree() const {return pt_;}

protected:
	boost::property_tree::ptree pt_;
	VSettingsPath path_;
	std::string file_;
};

//This class uses both the boost property tree and QSettings to store and manage the settings. The idea
//is that Qt based settings like window geometry, window state etc. are handled by QSettings while
//all the others by the boost property tree.
class VComboSettings : public VSettings
{
public:
	VComboSettings(const std::string& file,const std::string& qsFile);
	~VComboSettings() override;

	//void write(const std::string &fs);
	void clear() override;

	void write() override;

	bool containsQs(const std::string& key);

	void beginGroup(const std::string&) override;
	void endGroup() override;

	void putQs(const std::string& key,QVariant val);
	QVariant getQs(const std::string& key);

protected:
	QSettings qs_;
};

#endif
