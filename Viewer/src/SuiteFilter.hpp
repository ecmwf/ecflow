//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SUITEFILTER_HPP_
#define SUITEFILTER_HPP_

#include <string>
#include <vector>

template <class T>
class FlagSet
{
public:
    FlagSet() : flags_(0) {}

    void clear() {flags_=0;}
    void set(T flag ) { flags_ |= (1 << flag); }
    void unset(T flag ) { flags_ &= ~ (1 << flag); }
    bool isSet(T flag) const { return (flags_ >> flag) & 1; }
    bool isEmpty() const {return flags_==0;}
    bool sameAs(T flag) const {return flags_ == flag;}

private:
    int flags_;

};


class SuiteFilterItem
{
public:
	SuiteFilterItem(const std::string& name,bool present, bool filtered) :
		     name_(name), present_(present), filtered_(filtered) {}

	SuiteFilterItem(const SuiteFilterItem& other);

	std::string name_;
	bool present_;
	bool filtered_;
};

class SuiteFilter
{
public:
	SuiteFilter() : autoAddNew_(true), enabled_(false) {}

	enum ChangeFlag {AutoAddChanged=1,EnabledChanged=2,ItemChanged=4};

	SuiteFilter* clone();

	const std::vector<std::string>& filter() const {return filter_;}
	const std::vector<SuiteFilterItem> items() const {return items_;}

	void current(const std::vector<std::string>& suites);
	int count() const {return static_cast<int>(items_.size());}
	void setFiltered(int index,bool val);

	bool autoAddNewSuites() const {return autoAddNew_;}
	bool isEnabled() const {return enabled_;}

	void setAutoAddNewSuites(bool b) {autoAddNew_=b;}
	void setEnabled(bool b) {enabled_=b;}
	void selectAll();
	void unselectAll();

	bool update(SuiteFilter*);
	void setLoaded(const std::vector<std::string>& loaded);
	const FlagSet<ChangeFlag>& changeFlags() {return changeFlags_;}

private:
	void adjust();

    std::vector<std::string> loaded_;
    std::vector<std::string> defs_;
	std::vector<std::string> filter_;
	std::vector<SuiteFilterItem> items_;
	bool autoAddNew_;
	bool enabled_;
	FlagSet<ChangeFlag> changeFlags_;
};


#endif
