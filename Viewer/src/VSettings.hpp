//============================================================================
// Copyright 2015 ECMWF.
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
	std::string join(const std::string sep) const;

	std::vector<std::string> path_;
};

class VSettings
{
public:
	VSettings(const std::string& application);

	void write(const std::string &fs);
	bool read(const std::string &fs);
	void clear();

	bool contains(const std::string& key);
	bool containsQs(const std::string& key);

	void beginGroup(const std::string&);
	void endGroup();

	void put(const std::string& key,int val);
	void put(const std::string& key,const std::vector<std::string>& val);
	void putQs(const std::string& key,QVariant val);

	template <typename T>
	T get(const std::string& key,const T& defaultVal)
	{
		return pt_.get<T>(path_.path(key),defaultVal);
	}
	void get(const std::string& key,std::vector<std::string>& val);
	QVariant getQs(const std::string& key);

protected:
	boost::property_tree::ptree pt_;
	QSettings qs_;
	VSettingsPath path_;
};

#endif
