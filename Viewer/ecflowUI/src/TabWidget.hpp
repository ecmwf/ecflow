/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_TabWidget_HPP
#define ecflow_viewer_TabWidget_HPP

#include <QTabBar>
#include <QWidget>

class QMenu;
class QStackedWidget;
class QTabBar;
class QToolButton;
class QVBoxLayout;

class IconTabBar : public QTabBar {
public:
    explicit IconTabBar(QWidget* parent = nullptr)
        : QTabBar(parent) {}

protected:
    void paintEvent(QPaintEvent* e) override;
};

class TabWidget : public QWidget {
    Q_OBJECT

public:
    explicit TabWidget(QWidget* parent = nullptr);

    int currentIndex() const;
    int indexOfWidget(QWidget*) const;
    QWidget* widget(int) const;
    QWidget* currentWidget() const;
    void checkTabStatus();
    void addTab(QWidget*, QPixmap, QString);
    void setTabText(int, QString);
    void setTabIcon(int, QPixmap);
    void setTabToolTip(int, QString);
    void setTabWht(int, QString);
    void setTabData(int, QPixmap);
    int count() const;
    void clear();
    bool beingCleared() const { return beingCleared_; }

public Q_SLOTS:
    void removeTab(int);
    void removeOtherTabs(int);
    void setCurrentIndex(int);

private Q_SLOTS:
    void slotContextMenu(const QPoint&);
    void currentTabChanged(int index);
    void tabMoved(int from, int to);
    void slotTabList();

Q_SIGNALS:
    void currentIndexChanged(int);
    void newTabRequested();
    void tabRemoved();

protected:
    // virtual MvQContextItemSet* cmSet()=0;
    virtual void tabBarCommand(QString, int) = 0;

private:
#if 0
    QSize maxIconSize() const;
#endif
    QTabBar* bar_;
    QStackedWidget* stacked_;
    QToolButton* addTb_;
    QToolButton* tabListTb_;
    bool beingCleared_{false};
};

#endif /* ecflow_viewer_TabWidget_HPP */
