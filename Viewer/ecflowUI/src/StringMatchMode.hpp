/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
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
