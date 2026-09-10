/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VFileUncompress_HPP
#define ecflow_viewer_VFileUncompress_HPP

#include <QString>

#include "VFile.hpp"

class VFileUncompress {
public:
    static bool isCompressed(QString sourceFile);
    static VFile_ptr uncompress(QString sourceFile, QString& errStr);
};

#endif /* ecflow_viewer_VFileUncompress_HPP */
