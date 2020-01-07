//============================================================================
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef VFILEUNCOMPRESS_HPP
#define VFILEUNCOMPRESS_HPP

#include <QString>

#include "VFile.hpp"

class VFileUncompress
{
public:
    static bool isCompressed(QString sourceFile);
    static VFile_ptr uncompress(QString sourceFile, QString &errStr);
};

#endif // VFILEUNCOMPRESS_HPP
