/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_Palette_HPP
#define ecflow_viewer_Palette_HPP

#include <string>

#include <QColor>

class Palette {
public:
    Palette();
    static void load(const std::string& parFile);
    static void statusColours(QColor bg, QColor& bgLight, QColor& border);
    static QColor toColour(const std::string& name);
};

#endif /* ecflow_viewer_Palette_HPP */
