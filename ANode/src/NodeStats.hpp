#ifndef NODE_STATS_HPP_
#define NODE_STATS_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #251 $
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <string>

struct NodeStats
{
    NodeStats() = default;

    std::string print() const;

    size_t suites_{0};
    size_t family_{0};
    size_t task_{0};
    size_t alias_{0};
    size_t nodes_{0};

    size_t edit_history_nodes_{0};
    size_t edit_history_paths_{0};

    size_t vars_{0};
    size_t c_trigger_{0};
    size_t trigger_{0};
    size_t meters_{0};
    size_t events_{0};
    size_t labels_{0};

    size_t times_{0};
    size_t todays_{0};
    size_t crons_{0};
    size_t dates_{0};
    size_t days_{0};

    size_t late_{0};
    size_t zombies_{0};
    size_t verifys_{0};
    size_t queues_{0};
    size_t generics_{0};

    size_t repeats_{0};
    size_t limits_{0};
    size_t inlimits_{0};

    size_t auto_cancel_{0};
    size_t auto_archive_{0};
    size_t auto_restore_{0};
};

#endif
