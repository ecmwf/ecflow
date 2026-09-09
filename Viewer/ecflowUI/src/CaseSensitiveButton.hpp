/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_CaseSensitiveButton_HPP
#define ecflow_viewer_CaseSensitiveButton_HPP

#include <map>

#include <QToolButton>

class CaseSensitiveButton : public QToolButton {
    Q_OBJECT

public:
    explicit CaseSensitiveButton(QWidget* parent = nullptr);

protected Q_SLOTS:
    void slotClicked(bool);

Q_SIGNALS:
    void changed(bool);

private:
    std::map<bool, QString> tooltip_;
};

#endif /* ecflow_viewer_CaseSensitiveButton_HPP */
