/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "TimelineModel.hpp"

#include <QDebug>

#include "TimelineData.hpp"
#include "ViewerUtil.hpp"

TimelineModel::TimelineModel(QObject* parent) : QAbstractItemModel(parent), data_(nullptr) {
}

TimelineModel::~TimelineModel() = default;

void TimelineModel::resetData(TimelineData* data) {
    beginResetModel();
    data_ = data;
    endResetModel();
}

void TimelineModel::clearData() {
    beginResetModel();
    data_ = nullptr;
    endResetModel();
}

bool TimelineModel::hasData() const {
    return data_;
}

int TimelineModel::columnCount(const QModelIndex& /*parent */) const {
    return 4;
}

int TimelineModel::rowCount(const QModelIndex& parent) const {
    if (!hasData()) {
        return 0;
    }

    // Parent is the root:
    if (!parent.isValid()) {
        return static_cast<int>(data_->size());
    }

    return 0;
}

Qt::ItemFlags TimelineModel::flags(const QModelIndex& /*index*/) const {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant TimelineModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || !hasData()) {
        return {};
    }

    int row = index.row();
    if (row < 0 || row >= static_cast<int>(data_->size())) {
        return {};
    }

    if (role == Qt::DisplayRole) {
        if (index.column() == PathColumn) {
            return QString::fromStdString(data_->items()[row].path());
        }
        else if (index.column() == TimelineColumn) {
            return row;
        }
        else if (index.column() == SubmittedDurationColumn) {
            return data_->items()[row].firstSubmittedDuration(startDate_, endDate_);
        }
        else if (index.column() == ActiveDurationColumn) {
            return data_->items()[row].firstActiveDuration(startDate_, endDate_, data_->endTime());
        }
        else {
            return row;
        }
    }

    // sort roles
    else if (role == PathSortRole) {
        if (index.column() == PathColumn) {
            return static_cast<qint64>(data_->items()[row].sortIndex());
        }
        else {
            return {};
        }
    }

    // sort roles
    else if (role == TimeSortRole) {
        unsigned int start = TimelineItem::fromQDateTime(startDate_);
        unsigned int end   = TimelineItem::fromQDateTime(endDate_);
        for (size_t i = 0; i <= data_->items()[row].size(); i++) {
            unsigned int val = data_->items()[row].start_[i];
            if (val >= start) {
                if (val <= end) {
                    return val;
                }
                else {
                    return end + 2;
                }
            }
        }
        return end + 1;
    }

    // sort roles
    else if (role == TreeSortRole) {
        if (index.column() == PathColumn) {
            return static_cast<qint64>(data_->items()[row].treeIndex());
        }
        else {
            return {};
        }
    }

    // Qt sort roles
    else if (role == QtSortRole) {
        if (index.column() == PathColumn) {
            return static_cast<qint64>(data_->items()[row].sortIndex());
        }
        else if (index.column() == SubmittedDurationColumn) {
            if (useMeanDuration_) {
                float meanVal = 0.;
                int num       = 0;
                data_->items()[row].meanSubmittedDuration(meanVal, num, data_->endTime());
                return meanVal;
            }
            else {
                return data_->items()[row].firstSubmittedDuration(startDate_, endDate_);
            }
        }
        else if (index.column() == ActiveDurationColumn) {
            if (useMeanDuration_) {
                float meanVal = 0.;
                int num       = 0;
                data_->items()[row].meanActiveDuration(meanVal, num, data_->endTime());
                return meanVal;
            }
            else {
                return data_->items()[row].firstActiveDuration(startDate_, endDate_, data_->endTime());
            }
        }
        return {};
    }

    // task filter
    else if (role == Qt::UserRole) {
        if (index.column() == PathColumn) {
            return data_->items()[row].isTask();
        }
        else {
            return {};
        }
    }

    // filter = unchanged in period
    else if (role == UnchangedRole) {
        unsigned int start = TimelineItem::fromQDateTime(startDate_);
        unsigned int end   = TimelineItem::fromQDateTime(endDate_);
        for (size_t i = 0; i <= data_->items()[row].size(); i++) {
            unsigned int val = data_->items()[row].start_[i];
            if (val >= start && val <= end) {
                return false;
            }
        }
        return true;
    }

    // filter = unchanged in period
    else if (role == DurationUnchangedRole) {
        return !data_->items()[row].hasSubmittedOrActiveDuration(startDate_, endDate_);
    }

    // duration of first submitted task in period preceding the first active state
    else if (role == MeanDurationRole) {
        int num       = 0;
        float meanVal = -1.;
        QVariantList vals;
        if (index.column() == SubmittedDurationColumn) {
            data_->items()[row].meanSubmittedDuration(meanVal, num, data_->endTime());
            vals << meanVal << num;
        }
        else if (index.column() == ActiveDurationColumn) {
            data_->items()[row].meanActiveDuration(meanVal, num, data_->endTime());
            vals << meanVal << num;
        }
        return vals;
    }

    // duration of first submitted task in period preceding the first active state
    else if (role == DurationStatRole) {}

    return {};
}

QVariant TimelineModel::headerData(const int section, const Qt::Orientation orient, const int role) const {
    if (orient != Qt::Horizontal || (role != Qt::DisplayRole && role != Qt::UserRole)) {
        return QAbstractItemModel::headerData(section, orient, role);
    }

    if (role == Qt::DisplayRole) {
        switch (section) {
            case PathColumn:
                return tr("Path");
            case TimelineColumn:
                return "";
            case SubmittedDurationColumn:
                if (useMeanDuration_) {
                    return tr("Mean submitted duration");
                }
                else {
                    return tr("First submitted duration in period");
                }
            case ActiveDurationColumn:
                if (useMeanDuration_) {
                    return tr("Mean active duration");
                }
                else {
                    return tr("First active duration in period");
                }
            default:
                return {};
        }
    }

    return {};
}

QModelIndex TimelineModel::index(int row, int column, const QModelIndex& parent) const {
    if (!hasData() || row < 0 || column < 0) {
        return {};
    }

    // When parent is the root this index refers to a node or server
    if (!parent.isValid()) {
        return createIndex(row, column);
    }

    return {};
}

QModelIndex TimelineModel::parent(const QModelIndex& /*child*/) const {
    return {};
}

void TimelineModel::setPeriod(QDateTime t1, QDateTime t2) {
    startDate_ = t1;
    endDate_   = t2;
    Q_EMIT periodChanged();
}

void TimelineModel::setStartDate(QDateTime t) {
    startDate_ = t;
    Q_EMIT periodChanged();
}

void TimelineModel::setEndDate(QDateTime t) {
    endDate_ = t;
    Q_EMIT periodChanged();
}

//===========================================
//
// TimelineSortModel
//
//===========================================

TimelineSortModel::TimelineSortModel(TimelineModel* tlModel, QObject* parent)
    : QSortFilterProxyModel(parent),
      tlModel_(tlModel),
      skipSort_(false),
      sortMode_(PathSortMode),
      ascending_(true),
      pathMatchMode_(StringMatchMode::WildcardMatch),
      taskFilter_(false),
      changeFilterMode_(NoChangeFilterMode) {
    Q_ASSERT(tlModel_);

    connect(tlModel_, SIGNAL(periodChanged()), this, SLOT(slotPeriodChanged()));

    QSortFilterProxyModel::setSourceModel(tlModel_);

    setDynamicSortFilter(true);
}

TimelineSortModel::~TimelineSortModel() = default;

void TimelineSortModel::sortAgain() {
    invalidate();
    Q_EMIT invalidateCalled();
}

void TimelineSortModel::slotPeriodChanged() {
    if (sortMode_ == TimeSortMode || changeFilterMode_ != NoChangeFilterMode) {
        invalidate();
        Q_EMIT invalidateCalled();
    }
}

void TimelineSortModel::setSortMode(SortMode mode) {
    if (sortMode_ != mode) {
        sortMode_ = mode;
        invalidate();
        Q_EMIT invalidateCalled();
    }
}

void TimelineSortModel::setSortDirection(bool ascending) {
    Q_ASSERT(sortMode_ != QtSortMode);

    if (sortMode_ != QtSortMode) {
        ascending_ = ascending;
        sort(0, (ascending_) ? Qt::AscendingOrder : Qt::DescendingOrder);
    }
}

void TimelineSortModel::setPathMatchMode(const StringMatchMode& matchMode) {
    pathMatchMode_ = matchMode;
    setPathFilter(pathFilter_);
}

void TimelineSortModel::setPathFilter(QString pathFilter) {
    pathFilter_ = pathFilter;

    if (pathMatchMode_.mode() == StringMatchMode::WildcardMatch ||
        pathMatchMode_.mode() == StringMatchMode::RegexpMatch) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
        pathFilterRx_ = QRegularExpression();
        if (pathMatchMode_.mode() == StringMatchMode::WildcardMatch) {
            // pathFilterRx_.setPattern(QRegularExpression::wildcardToRegularExpression(pathFilter_));
            pathFilterRx_.setPattern(ViewerUtil::wildcardToRegex(pathFilter_));
        }
        else {
            pathFilterRx_.setPattern(pathFilter_);
        }
        pathFilterRx_.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
#else

        pathFilterRx_ = QRegExp(pathFilter_);

        if (pathMatchMode_.mode() == StringMatchMode::WildcardMatch) {
            pathFilterRx_.setPatternSyntax(QRegExp::Wildcard);
        }
        else {
            pathFilterRx_.setPatternSyntax(QRegExp::RegExp);
        }
        pathFilterRx_.setCaseSensitivity(Qt::CaseInsensitive);
#endif
    }
    invalidate();
    Q_EMIT invalidateCalled();
}

void TimelineSortModel::setTaskFilter(bool taskFilter) {
    taskFilter_ = taskFilter;
    invalidate();
    Q_EMIT invalidateCalled();
}

void TimelineSortModel::setRootNodeFilter(QString rootNodeFilter) {
    if (rootNodeFilter != rootNodeFilter_) {
        rootNodeFilter_ = rootNodeFilter;
        invalidate();
        Q_EMIT invalidateCalled();
    }
}

void TimelineSortModel::setChangeFilterMode(ChangeFilterMode m) {
    if (changeFilterMode_ != m) {
        changeFilterMode_ = m;
        invalidate();
        Q_EMIT invalidateCalled();
    }
}

bool TimelineSortModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
    if (skipSort_) {
        return true;
    }

    if (sortMode_ == PathSortMode) {
        return tlModel_->data(left, TimelineModel::PathSortRole).toInt() <
               tlModel_->data(right, TimelineModel::PathSortRole).toInt();
    }
    else if (sortMode_ == TimeSortMode) {
        return tlModel_->data(left, TimelineModel::TimeSortRole).toUInt() <
               tlModel_->data(right, TimelineModel::TimeSortRole).toUInt();
    }
    else if (sortMode_ == TreeSortMode) {
        return tlModel_->data(left, TimelineModel::TreeSortRole).toUInt() <
               tlModel_->data(right, TimelineModel::TreeSortRole).toUInt();
    }
    else if (sortMode_ == QtSortMode) {
        return tlModel_->data(left, TimelineModel::QtSortRole).toInt() <
               tlModel_->data(right, TimelineModel::QtSortRole).toInt();
    }

    return true;
}

bool TimelineSortModel::filterAcceptsRow(int sourceRow, const QModelIndex& /*sourceParent*/) const {
    bool matched = true;

    if (!rootNodeFilter_.isEmpty()) {
        matched = tlModel_->data(tlModel_->index(sourceRow, 0)).toString().startsWith(rootNodeFilter_);
    }

    if (matched && !pathFilter_.isEmpty()) {
        if (pathMatchMode_.mode() != StringMatchMode::ContainsMatch) {
            matched = tlModel_->data(tlModel_->index(sourceRow, 0)).toString().contains(pathFilterRx_);
        }
        else {
            matched = tlModel_->data(tlModel_->index(sourceRow, 0)).toString().contains(pathFilter_);
        }
    }

    if (matched && taskFilter_) {
        matched = tlModel_->data(tlModel_->index(sourceRow, 0), Qt::UserRole).toBool();
    }

    if (matched) {
        if (changeFilterMode_ == TimelineChangeFilterMode) {
            matched = (tlModel_->data(tlModel_->index(sourceRow, 0), TimelineModel::UnchangedRole).toBool() == false);
        }
        else if (changeFilterMode_ == DurationChangeFilterMode) {
            matched =
                (tlModel_->data(tlModel_->index(sourceRow, 0), TimelineModel::DurationUnchangedRole).toBool() == false);
        }
    }

    return matched;
}
