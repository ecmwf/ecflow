// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QTextBrowser>

class TriggerTableItem;

class TriggerTextWidget : public QTextBrowser {
public:
    explicit TriggerTextWidget(QWidget* parent = nullptr);
    void reloadItem(TriggerTableItem* item);

private:
    QString makeHtml(TriggerTableItem* ti, QString directTitle, QString modeText) const;
};
