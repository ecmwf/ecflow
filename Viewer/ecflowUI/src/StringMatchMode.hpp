/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_StringMatchMode_HPP
#define ecflow_viewer_StringMatchMode_HPP

#include <map>
#include <string>

class StringMatchMode {
public:
    enum Mode { InvalidMatch = -1, ContainsMatch = 0, WildcardMatch = 1, RegexpMatch = 2 };

    StringMatchMode();

    explicit StringMatchMode(Mode m);
    explicit StringMatchMode(int);
    StringMatchMode(const StringMatchMode& r)            = default;
    StringMatchMode& operator=(const StringMatchMode& r) = default;

    Mode mode() const { return mode_; }
    void setMode(Mode m) { mode_ = m; }
    const std::string& matchOperator() const;
    int toInt() const { return static_cast<int>(mode_); }

    static Mode operToMode(const std::string&);

private:
    void init();

    Mode mode_{WildcardMatch};
    static std::map<Mode, std::string> matchOper_;
};

#endif /* ecflow_viewer_StringMatchMode_HPP */
