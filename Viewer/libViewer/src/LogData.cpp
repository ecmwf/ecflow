/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "LogData.hpp"

#include <stdexcept>

#include <QFile>
#include <QFileInfo>
#include <QStringList>

#include "UIDebug.hpp"
#include "UiLog.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/File_r.hpp"
#include "ecflow/core/NodePath.hpp"
#include "ecflow/core/Str.hpp"

LogDataItem::LogDataItem(const std::string& line, qint64& refTimeInMs) : type_(NoType) {
    // Format is as follows:
    // MSG:[06:46:44 23.4.2018] chd:complete .....

    if (line.size() == 0) {
        return;
    }

    std::string::size_type pos = line.find("[");
    if (pos == std::string::npos) {
        return;
    }

    std::string::size_type pos1 = line.find("]", pos);
    if (pos1 == std::string::npos || pos1 == pos + 1) {
        return;
    }

    std::string t = line.substr(0, pos);
    std::string d = line.substr(pos + 1, pos1 - pos - 1);

    if (pos1 + 1 < line.size()) {
        entry_ = line.substr(pos1 + 1);
    }

    QDateTime dt = QDateTime::fromString(QString::fromStdString(d), "hh:mm:ss d.M.yyyy");
    dt.setTimeSpec(Qt::UTC);

    if (refTimeInMs == 0) {
        time_       = 0;
        refTimeInMs = dt.toMSecsSinceEpoch();
    }
    else {
        time_ = (dt.toMSecsSinceEpoch() - refTimeInMs) / 1000;
    }

    if (t == "MSG:") {
        type_ = MessageType;
    }
    else if (t == "LOG:") {
        type_ = LogType;
    }
    else if (t == "ERR:") {
        type_ = ErrorType;
    }
    else if (t == "WAR:") {
        type_ = WarningType;
    }
    else if (t == "DBG:") {
        type_ = DebugType;
    }
}

qint64 LogDataItem::getTimeInMs(const std::string& line) {
    if (line.size() == 0) {
        return 0;
    }

    std::string::size_type pos = line.find("[");
    if (pos == std::string::npos) {
        return 0;
    }

    std::string::size_type pos1 = line.find("]", pos);
    if (pos1 == std::string::npos || pos1 == pos + 1) {
        return 0;
    }

    QString d    = QString::fromStdString(line.substr(pos + 1, pos1 - pos - 1));
    QDateTime dt = QDateTime::fromString(d, "hh:mm:ss d.M.yyyy");
    dt.setTimeSpec(Qt::UTC);
    return dt.toMSecsSinceEpoch();
}

void LogData::loadFromFile(const std::string& logFile, size_t startPos) {
    data_.clear();

    /// The log file can be massive > 50Mb
    ecf::File_r log_file(logFile);
    if (!log_file.ok()) {
        throw std::runtime_error("LogData::loadFromFile: Could not open log file " + logFile);
    }

    log_file.setPos(startPos);

    std::string line;

    while (log_file.good()) {
        log_file.getline(line); // default delimiter is /n
        appendFromText(line);
    }
}

void LogData::loadFromText(const std::string& txt) {
    data_.clear();

    QString in = QString::fromStdString(txt);
    Q_FOREACH (QString s, in.split("\n")) {
        if (!s.simplified().isEmpty()) {
            data_.emplace_back(s.toStdString(), refTimeInMs_);
        }
    }
}

void LogData::loadFromText(const std::vector<std::string>& txtVec) {
    data_.clear();
    appendFromText(txtVec);
}

void LogData::addLogLine(const std::string& txt) {
    appendFromText(txt);
}

void LogData::appendFromText(const std::string& txt) {
    if (txt.size() < 4 || txt.substr(0, 4) != "MSG:") {
        return;
    }

    QString s = QString::fromStdString(txt);
    if (!s.simplified().isEmpty()) {
        data_.emplace_back(s.toStdString(), refTimeInMs_);
    }
}

void LogData::appendFromText(const std::vector<std::string>& txtVec) {
    for (const auto& i : txtVec) {
        QString s = QString::fromStdString(i);
        if (!s.simplified().isEmpty()) {
            data_.emplace_back(s.toStdString(), refTimeInMs_);
        }
    }
}

bool LogData::indexOfPeriod(qint64 start, qint64 end, size_t& idxStart, size_t& idxEnd, qint64 toleranceInMs) {
    unsigned int startTime = (start - refTimeInMs_) / 1000;
    unsigned int endTime   = (end - refTimeInMs_) / 1000;

    bool hasStart    = false;
    qint64 tolerance = toleranceInMs / 1000;

    if (tolerance == 0) {
        for (size_t i = 0; i < data_.size(); i++) {
            if (data_[i].time_ >= startTime) {
                idxStart = i;
                hasStart = true;
                break;
            }
        }

        if (!hasStart) {
            return false;
        }

        for (size_t i = idxStart; i < data_.size(); i++) {
            if (data_[i].time_ > endTime) {
                idxEnd = (i == 0) ? 0 : (i - 1);
                return true;
            }
        }

        return false;
    }

    for (size_t i = 0; i < data_.size(); i++) {
        if (startTime - tolerance <= data_[i].time_ && data_[i].time_ < startTime) {
            idxStart = i;
            idxEnd   = i;
            hasStart = true;
        }
        else if (startTime == data_[i].time_) {
            idxStart = i;
            idxEnd   = i;
            hasStart = true;
            break;
        }
        else if (!hasStart && startTime + tolerance < data_[i].time_) {
            idxStart = i;
            idxEnd   = i;
            hasStart = true;
            break;
        }
        else if (data_[i].time_ >= startTime) {
            break;
        }
    }

    if (!hasStart) {
        return false;
    }

    for (size_t i = idxStart; i < data_.size(); i++) {
        if (data_[i].time_ > endTime) {
            idxEnd = (i == 0) ? 0 : (i - 1);
            if (idxEnd < idxStart) {
                idxEnd = idxStart;
            }
            return true;
        }
    }

    return false;
}
