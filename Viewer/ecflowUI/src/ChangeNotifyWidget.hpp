/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_ChangeNotifyWidget_HPP
#define ecflow_viewer_ChangeNotifyWidget_HPP

#include <map>
#include <string>
#include <vector>

#include <QLinearGradient>
#include <QToolButton>
#include <QWidget>

class QHBoxLayout;
class QLabel;
class QSignalMapper;

class ChangeNotify;
class VProperty;
class ChangeNotifyWidget;

class ChangeNotifyButton : public QToolButton {
    Q_OBJECT

    friend class ChangeNotifyWidget;

public:
    explicit ChangeNotifyButton(QWidget* parent = nullptr);

    void setNotifier(ChangeNotify*);

public Q_SLOTS:
    void slotAppend();
    void slotRemoveRow(int);
    void slotReset();
    void slotClicked(bool);

protected:
    void updateIcon();

    ChangeNotify* notifier_{nullptr};
    QLinearGradient grad_;
};

class ChangeNotifyWidget : public QWidget {
    friend class ChangeNotify;

public:
    explicit ChangeNotifyWidget(QWidget* parent = nullptr);
    ~ChangeNotifyWidget() override;

    void updateVisibility();
    static void setEnabled(const std::string& id, bool b);
    static void updateSettings(const std::string& id);

protected:
    void addTb(ChangeNotify*);
    ChangeNotifyButton* findButton(const std::string& id);
    bool hasVisibleButton() const;

    QHBoxLayout* layout_;
    std::map<std::string, ChangeNotifyButton*> buttons_;
    static std::vector<ChangeNotifyWidget*> widgets_;
};

#endif /* ecflow_viewer_ChangeNotifyWidget_HPP */
