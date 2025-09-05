/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/parser/AutoArchiveParser.hpp"

#include <stdexcept>

#include "ecflow/attribute/AutoArchiveAttr.hpp"
#include "ecflow/core/Extract.hpp"
#include "ecflow/core/TimeSeries.hpp"
#include "ecflow/node/Node.hpp"

using namespace ecf;
using namespace std;

// The -i is the last token before the comments start.
static bool has_idle_flag(std::vector<std::string>& lineTokens) {
    return (lineTokens.size() >= 3 && lineTokens[2] == "-i");
}

bool AutoArchiveParser::doParse(const std::string& line, std::vector<std::string>& lineTokens) {
    // autoarchive +01:00    # archive one hour after complete
    // autoarchive 01:00     # archive at 1 am in morning after complete
    // autoarchive 10        # archive 10 days after complete
    // autoarchive 0         # archive immediately after complete

    // autoarchive +01:00 -i   # archive one hour after complete,queued,aborted
    // autoarchive 01:00  -i   # archive at 1 am in morning after complete,queued,aborted
    // autoarchive 10     -i   # archive 10 days after complete,queued,aborted
    // autoarchive 0      -i   # archive immediately after complete,queued,aborted

    if (lineTokens.size() < 2) {
        throw std::runtime_error("AutoArchiveParser::doParse: Invalid autoarchive :" + line);
    }
    if (nodeStack().empty()) {
        throw std::runtime_error(
            "AutoArchiveParser::doParse: Could not add autoarchive as node stack is empty at line: " + line);
    }

    if (lineTokens[1].find_first_of(':') == string::npos) {
        // Must be of the form:
        // autoarchive 10        # archive 10 days after complete
        // autoarchive 0         # archive immediately after complete
        int days = Extract::theInt(lineTokens[1], "invalid autoarchive " + line);

        nodeStack_top()->add_autoarchive(AutoArchiveAttr(days, has_idle_flag(lineTokens)));
    }
    else {
        // Must be of the form:
        // autoarchive +01:00    # archive one hour after complete
        // autoarchive 01:00     # archive at 1 am in morning after complete
        int hour      = 0;
        int min       = 0;
        bool relative = TimeSeries::getTime(lineTokens[1], hour, min);

        nodeStack_top()->add_autoarchive(AutoArchiveAttr(TimeSlot(hour, min), relative, has_idle_flag(lineTokens)));
    }
    return true;
}
