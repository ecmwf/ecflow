// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <ctime>

#include <QFileInfo>

class VFileInfo : public QFileInfo {
public:
    explicit VFileInfo(const QString& file)
        : QFileInfo(file) {}

    QString formatSize() const;
    QString formatModDate() const;
    QString formatPermissions() const;

    static QString formatSize(unsigned int size);
    static QString formatDate(const std::time_t& t);
    static QString formatDateAgo(const std::time_t& t);
};
