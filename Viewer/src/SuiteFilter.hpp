//============================================================================
// Copyright 2016 ECMWF.
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

#include "FlagSet.hpp"

class SuiteFilter;
class SuiteFilterObserver;
class VSettings;

#if 0
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

#endif

class SuiteFilterItem
{
   friend class SuiteFilter;
public:
    SuiteFilterItem(const std::string& name,bool loaded, bool filtered) :
             name_(name), loaded_(loaded), filtered_(filtered) {}

	SuiteFilterItem(const SuiteFilterItem& other);

    bool operator!=(const SuiteFilterItem& rhs) const {return name_ != rhs.name_ || loaded_ != rhs.loaded_ ||
                filtered_ != rhs.filtered_;}

    const std::string& name() const {return name_;}
    bool loaded() const {return loaded_;}
    bool filtered() const {return filtered_;}

protected:
	std::string name_;
    bool loaded_;
	bool filtered_;
};

class SuiteFilter
{
public:
    SuiteFilter() : autoAddNew_(false), enabled_(false), loadedInitialised_(false) {}
	~SuiteFilter();

	enum ChangeFlag {AutoAddChanged=1,EnabledChanged=2,ItemChanged=4};

	SuiteFilter* clone();

    std::vector<std::string> filter() const;
    std::vector<std::string> loaded() const;
    const std::vector<SuiteFilterItem>& items() const {return items_;}

	void current(const std::vector<std::string>& suites);
	int count() const {return static_cast<int>(items_.size());}
	void setFiltered(int index,bool val);
    bool isLoadedInitialised() const {return loadedInitialised_;}
	bool autoAddNewSuites() const {return autoAddNew_;}
	bool isEnabled() const {return enabled_;}

	void setAutoAddNewSuites(bool b) {autoAddNew_=b;}
	void setEnabled(bool b) {enabled_=b;}
	void selectAll();
	void unselectAll();
    bool removeUnloaded();
    bool hasUnloaded() const;

    bool sameAs(const SuiteFilter*) const;
	bool update(SuiteFilter*);
	bool setLoaded(const std::vector<std::string>& loaded,bool checkDiff=true);
	bool loadedSameAs(const std::vector<std::string>& loaded) const;
	const FlagSet<ChangeFlag>& changeFlags() {return changeFlags_;}

	bool hasObserver() const {return !observers_.empty();}
	void addObserver(SuiteFilterObserver*);
	void removeObserver(SuiteFilterObserver*);

	void readSettings(VSettings *vs);
	void writeSettings(VSettings *vs);

    static const std::string dummySuite() {return dummySuite_;}

private:
	void clear();
	void adjust();
    void broadcastChange();
    void adjustLoaded(const std::vector<std::string>& loaded);
    void adjustFiltered(const std::vector<std::string>& filtered);

    std::vector<SuiteFilterItem> items_;
    bool autoAddNew_;
	bool enabled_;
    bool loadedInitialised_;
	FlagSet<ChangeFlag> changeFlags_;
	std::vector<SuiteFilterObserver*> observers_;
    static std::string dummySuite_;
};


#endif
