/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "TimelineData.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include <QDateTime>
#include <QFileInfo>
#include <QStringList>

#include "TimelineData.hpp"
#include "UIDebug.hpp"
#include "UiLog.hpp"
#include "VNState.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/File_r.hpp"
#include "ecflow/core/NodePath.hpp"
#include "ecflow/core/Str.hpp"

#ifndef SIZE_MAX
    #ifdef __SIZE_MAX__
        #define SIZE_MAX __SIZE_MAX__
    #else
        #define SIZE_MAX std::numeric_limits<size_t>::max()
    #endif
#endif

TimelineItem::TimelineItem(const std::string& path, unsigned char status, unsigned int time, Type type)
    : path_(path),
      type_(type) {
    add(status, time);
}

void TimelineItem::add(unsigned char status, unsigned int time) {
    status_.push_back(status);
    start_.push_back(time);
}

int TimelineItem::firstInPeriod(QDateTime startDt, QDateTime /*endDt*/) const {
    unsigned int start = fromQDateTime(startDt);
    unsigned int end   = fromQDateTime(startDt);
    for (size_t i = 0; i < start_.size(); i++) {
        if (start_[i] >= start && start_[i] <= end) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool TimelineItem::hasSubmittedOrActiveDuration(QDateTime startDt, QDateTime endDt) const {
    unsigned int start = fromQDateTime(startDt);
    unsigned int end   = fromQDateTime(endDt);
    for (size_t i = 0; i < start_.size(); i++) {
        if (start_[i] >= start) {
            if (VNState::isActive(status_[i])) {
                return true;
            }

            if (i < start_.size() - 1 && VNState::isSubmitted(status_[i]) && VNState::isActive(status_[i + 1])) {
                return true;
            }
        }
        else if (start_[i] >= end) {
            return false;
        }
    }
    return false;
}

// find the first submitted duration preceding the first active duration in the given period
int TimelineItem::firstSubmittedDuration(QDateTime startDt, QDateTime endDt) const {
    unsigned int start = fromQDateTime(startDt);
    unsigned int end   = fromQDateTime(endDt);
    for (size_t i = 0; i < start_.size() - 1; i++) {
        if (start_[i] >= start) {
            if (VNState::isActive(status_[i])) {
                return -1;
            }

            if (VNState::isSubmitted(status_[i]) && VNState::isActive(status_[i + 1])) {
                return start_[i + 1] - start_[i]; // secs
            }
        }
        else if (start_[i] >= end) {
            return -1;
        }
    }
    return -1;
}

// find the first active duration in the given period
int TimelineItem::firstActiveDuration(QDateTime startDt, QDateTime endDt, unsigned int tlEndTime) const {
    unsigned int start = fromQDateTime(startDt);
    unsigned int end   = fromQDateTime(endDt);
    for (size_t i = 0; i < start_.size(); i++) {
        if (start_[i] >= start) {
            if (VNState::isActive(status_[i])) {
                return ((i != start_.size() - 1) ? start_[i + 1] : tlEndTime) - start_[i]; // secs
            }
        }
        else if (start_[i] >= end) {
            return -1;
        }
    }
    return -1;
}

void TimelineItem::meanSubmittedDuration(float& meanVal, int& num, unsigned int tlEndTime) const {
    int sum = 0;
    num     = 0;
    meanVal = 0.;

    for (size_t i = 0; i < start_.size(); i++) {
        if (VNState::isSubmitted(status_[i])) {
            sum += ((i != start_.size() - 1) ? start_[i + 1] : tlEndTime) - start_[i]; // secs
            num++;
        }
    }

    if (num > 0) {
        meanVal = static_cast<float>(sum) / static_cast<float>(num);
    }
}

void TimelineItem::meanActiveDuration(float& meanVal, int& num, unsigned int tlEndTime) const {
    int sum = 0;
    num     = 0;
    meanVal = 0.;

    for (size_t i = 0; i < start_.size(); i++) {
        if (VNState::isActive(status_[i])) {
            sum += ((i != start_.size() - 1) ? start_[i + 1] : tlEndTime) - start_[i]; // secs
            num++;
        }
    }

    if (num > 0) {
        meanVal = static_cast<float>(sum) / static_cast<float>(num);
    }
}

void TimelineItem::durationStats(unsigned char statusId,
                                 int& num,
                                 float& mean,
                                 TimelineItemStats& stats,
                                 unsigned int tlEndTime) const {
    int sum = 0;
    num     = 0;

    std::vector<int> dvals;
    for (size_t i = 0; i < start_.size(); i++) {
        if (status_[i] == statusId) {
            dvals.push_back(((i != start_.size() - 1) ? start_[i + 1] : tlEndTime) - start_[i]); // secs
            sum += dvals.back();
        }
    }

    if (dvals.size() > 0) {
        std::sort(dvals.begin(), dvals.end());
        stats.total  = sum;
        stats.min    = dvals.front();
        stats.max    = dvals.back();
        stats.perc25 = dvals[dvals.size() / 4];
        stats.median = dvals[dvals.size() / 2];
        stats.perc75 = dvals[3 * dvals.size() / 4];
        num          = dvals.size();
        mean         = static_cast<float>(sum) / static_cast<float>(dvals.size());
    }
}

void TimelineItem::days(std::vector<unsigned int>& lst) const {
    unsigned int prevDay = 0;
    for (unsigned int i : start_) {
        unsigned int day = i / 86400;
        if (prevDay != day) {
            prevDay = day;
            lst.push_back(day * 86400);
        }
    }
}

//====================================================================
//
// TimelineData
//
//====================================================================

void TimelineData::clear() {
    numOfRows_   = 0;
    startTime_   = 0;
    endTime_     = 0;
    maxReadSize_ = 0;
    fullRead_    = false;
    loadStatus_  = LoadNotTried;
    items_       = std::vector<TimelineItem>();
    loadedAt_    = QDateTime();
    pathHash_.clear();
    sortIndex_ = std::vector<size_t>();
}

void TimelineData::setItemType(int index, TimelineItem::Type type) {
    items_[index].type_ = type;
}

void TimelineData::setItemTreeIndex(size_t index, size_t treeIndex) {
    items_[index].treeIndex_ = treeIndex;
}

void TimelineData::loadLogFile(const std::string& logFile, size_t maxReadSize, const std::vector<std::string>& suites) {
    // Clear all collected data
    clear();

    maxReadSize_ = maxReadSize;
    fullRead_    = false;
    loadStatus_  = LoadNotTried;
    loadedAt_    = QDateTime::currentDateTime();

    loadLogFileCore(logFile, maxReadSize, suites, false);
}

void TimelineData::loadMultiLogFile(const std::string& logFile,
                                    const std::vector<std::string>& suites,
                                    int logFileIndex,
                                    bool last) {
    // Clear all collected data
    if (logFileIndex == 0) {
        clear();

        maxReadSize_ = 0;
        fullRead_    = true;
        loadStatus_  = LoadNotTried;
        loadedAt_    = QDateTime::currentDateTime();
    }

    loadLogFileCore(logFile, -1, suites, true);

    if (last) {
        sortByPath();
        loadStatus_ = LoadDone;
    }
}

void TimelineData::loadLogFileCore(const std::string& logFile,
                                   size_t /*maxReadSize*/,
                                   const std::vector<std::string>& suites,
                                   bool multi) {
    // Clear all collected data
    // clear();

    // maxReadSize_=maxReadSize;
    // fullRead_=false;
    // loadStatus_=LoadNotTried;
    // loadedAt_=QDateTime::currentDateTime();

    /// The log file can be massive > 50Mb
    ecf::File_r log_file(logFile);
    if (!log_file.ok()) {
        loadStatus_ = LoadFailed;
        UiLog().warn() << "TimelineData::loadLogFileCore: Could not open log file " << logFile;
        throw std::runtime_error("Could not open log file: " + logFile);
    }

    fullRead_ = true;
    QFileInfo fInfo(QString::fromStdString(logFile));
    size_t fSize         = fInfo.size();
    size_t progressChunk = fSize / 200;
    if (progressChunk == 0) {
        progressChunk = fSize;
    }
    size_t currentProgressChunk = 0;
    size_t percent              = 0;

    if (fSize == 0) {
        return;
    }

    if (maxReadSize_ > 0) {
        if (fSize > maxReadSize_) {
            fullRead_     = false;
            progressChunk = maxReadSize_ / 200;
            log_file.setPos(fSize - maxReadSize_);
        }
    }

    std::string line;

    while (log_file.good()) {
        log_file.getline(line); // default delimiter is /n

        std::string name;
        unsigned char statusId  = 0;
        unsigned int statusTime = 0;
        if (parseLine(line, name, statusId, statusTime)) {
            // Filter by suites
            if (!suites.empty() && name.size() > 1 && name[0] == '/') {
                std::string suite;
                std::string::size_type next_sep = name.find("/", 1);
                if (next_sep != std::string::npos) {
                    suite = name.substr(1, next_sep - 1);
                }
                else {
                    suite = name.substr(1);
                }

                if (std::find(suites.begin(), suites.end(), suite) == suites.end()) {
                    continue;
                }
            }

            if (startTime_ == 0) {
                startTime_ = statusTime;
            }

            if (statusTime > endTime_) {
                endTime_ = statusTime;
            }

            size_t idx = 0;
            // exsiting item
            if (indexOfItem(name, idx)) {
                items_[idx].add(statusId, statusTime);
                if (items_[idx].type_ == TimelineItem::UndeterminedType) {
                    items_[idx].type_ = guessNodeType(line);
                }
            }
            else {
                items_.emplace_back(name, statusId, statusTime, guessNodeType(line, name));

                pathHash_.insert(QString::fromStdString(name), items_.size() - 1);
            }

            size_t current = log_file.pos();
            if (current / progressChunk > currentProgressChunk) {
                currentProgressChunk = current / progressChunk;
                percent              = current / fSize;
                if (percent <= 100) {
                    Q_EMIT loadProgress(current, fSize);
                }
            }

            numOfRows_++;
        }
    }

    if (!multi) {
        sortByPath();
        loadStatus_ = LoadDone;
    }
}

bool TimelineData::parseLine(const std::string& line,
                             std::string& name,
                             unsigned char& statusId,
                             unsigned int& statusTime) {
    // The log file format we are interested is :
    // LOG:[22:45:30 21.4.2018]  complete: path
    // LOG:[22:45:30 21.4.2018]  submitted: path job_size:16408

    /// We are only interested in status changes (i.e LOG:)
    if (line.empty()) {
        return false;
    }

    if (line[0] != 'L') {
        return false;
    }

    std::string::size_type log_pos = line.find("LOG:");
    if (log_pos != 0) {
        return false;
    }

    /// LOG:[HH:MM:SS D.M.YYYY] status: fullname [+additional information]
    /// EXTRACT the date
    std::string::size_type first_open_bracket = line.find('[');
    if (first_open_bracket == std::string::npos) {
        return false;
    }

    std::string::size_type first_closed_bracket = line.find(']', first_open_bracket);
    if (first_closed_bracket == std::string::npos) {
        return false;
    }

    std::string time_stamp = line.substr(first_open_bracket + 1, first_closed_bracket - first_open_bracket - 1);

    /// extract the status
    std::string::size_type first_colon = line.find(':', first_closed_bracket);
    if (first_colon == std::string::npos) {
        return false;
    }

    std::string::size_type first_char = line.find_first_not_of(' ', first_closed_bracket + 1);
    if (first_char == std::string::npos) {
        return false;
    }

    std::string status = line.substr(first_char, first_colon - first_char);

    // get the status id
    // unsigned char statusId;
    if (VNState* vn = VNState::find(status)) {
        statusId = vn->ucId();
    }
    else {
        return false;
    }

    // extract the full name
    first_char = line.find_first_not_of(' ', first_colon + 1);
    if (first_char == std::string::npos) {
        return false;
    }

    std::string::size_type next_ws = line.find(' ', first_char + 1);
    if (next_ws == std::string::npos) {
        name = line.substr(first_char);
    }
    else {
        name = line.substr(first_char, next_ws - first_char);
    }

    // Convert status time into secs
    QDateTime dt = QDateTime::fromString(QString::fromStdString(time_stamp), "hh:mm:ss d.M.yyyy");
    dt.setTimeSpec(Qt::UTC);
    statusTime = dt.toMSecsSinceEpoch() / 1000;

    return true;
}

void TimelineData::guessNodeType() {
    std::vector<size_t> taskIndex;
    for (size_t i = 0; i < items_.size(); i++) {
        if (items_[i].isTask()) {
            taskIndex.push_back(i);
        }
    }

    for (auto& item : items_) {
        if (item.type_ == TimelineItem::UndeterminedType) {
            for (size_t j = 0; j < taskIndex.size(); j++) {
                if (items_[j].path_.find(item.path_) != std::string::npos) {
                    item.type_ = TimelineItem::FamilyType;
                    break;
                }
            }
        }
    }
}

TimelineItem::Type TimelineData::guessNodeType(const std::string& line, const std::string& name) const {
    if (name.find_last_of("/") == 0) {
        return TimelineItem::SuiteType;
    }

    return guessNodeType(line);
}

TimelineItem::Type TimelineData::guessNodeType(const std::string& line) const {
    // Try to figure out if it is a taks when status=submitted. If there is
    // an item with "job_size:" it must be a task.
    if (line.find("job_size:") != std::string::npos) {
        return TimelineItem::TaskType;
    }

    return TimelineItem::UndeterminedType;
}

// It has to be very fast!!!!
bool TimelineData::indexOfItem(const std::string& p, size_t& idx) {
    size_t v = pathHash_.value(QString::fromStdString(p), SIZE_MAX);
    if (v != SIZE_MAX) {
        idx = v;
        return true;
    }
    return false;
}

bool sortVecFunction(const std::pair<size_t, std::string>& a, const std::pair<size_t, std::string>& b) {
    return a.second < b.second;
}

void TimelineData::sortByPath() {
    sortIndex_ = std::vector<size_t>();
    sortIndex_.resize(items_.size(), 0);

    std::vector<std::pair<size_t, std::string>> sortVec(items_.size());
    for (size_t i = 0; i < items_.size(); i++) {
        sortVec[i] = std::make_pair(i, items_[i].path_);
    }

    std::sort(sortVec.begin(), sortVec.end(), sortVecFunction);

    for (size_t i = 0; i < items_.size(); i++) {
        int idx                = sortVec[i].first;
        items_[idx].sortIndex_ = i;
        sortIndex_[i]          = idx;
    }
}
