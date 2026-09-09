/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_FlagSet_HPP
#define ecflow_viewer_FlagSet_HPP

template <class T>
class FlagSet {
public:
    FlagSet() = default;
    explicit FlagSet(T t) { set(t); }

    void clear() { flags_ = 0; }
    void set(T flag) { flags_ |= (1 << flag); }
    void unset(T flag) { flags_ &= ~(1 << flag); }
    bool isSet(T flag) const { return (flags_ >> flag) & 1; }
    bool isEmpty() const { return flags_ == 0; }
    bool sameAs(T flag) const { return flags_ == flag; }

private:
    int flags_{0};
};

#endif /* ecflow_viewer_FlagSet_HPP */
