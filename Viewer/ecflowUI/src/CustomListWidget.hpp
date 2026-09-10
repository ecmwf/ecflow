/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_CustomListWidget_HPP
#define ecflow_viewer_CustomListWidget_HPP

#include <QListWidget>

class CustomListWidget : public QListWidget {
    Q_OBJECT

public:
    explicit CustomListWidget(QWidget* parent = nullptr);

    void addItems(QStringList lst, bool checkState);
    void addItems(QStringList lst, bool checkState, QList<QColor>);
    QStringList selection() const;
    bool hasSelection() const;
    void setSelectionWithList(QStringList sel);

public Q_SLOTS:
    void clearSelection();

protected Q_SLOTS:
    void slotItemChanged(QListWidgetItem*);

Q_SIGNALS:
    void itemSelectionChanged();
};

#endif /* ecflow_viewer_CustomListWidget_HPP */
