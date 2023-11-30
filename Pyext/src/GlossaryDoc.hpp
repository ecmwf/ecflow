/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_python_GlossaryDoc_HPP
#define ecflow_python_GlossaryDoc_HPP

#include <boost/core/noncopyable.hpp>

// ===========================================================================
// IMPORTANT: These appear as python doc strings.
//            Additionally they are auto documented using sphinx-poco
//            Hence the doc strings use reStructuredText markup.
// ===========================================================================
class GlossaryDoc : private boost::noncopyable {
public:
    static const char* list();

private:
    GlossaryDoc() = default;
};

#endif /* ecflow_python_GlossaryDoc_HPP */
