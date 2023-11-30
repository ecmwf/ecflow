/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_MessageLabel_HPP
#define ecflow_viewer_MessageLabel_HPP

#include <QWidget>

class QHBoxLayout;
class QLabel;
class QProgressBar;
class QToolButton;
class QTimer;

class DelayedProgressDef {
public:
    DelayedProgressDef() = default;
    DelayedProgressDef(QString text, int max = 0) : infoText_(text), max_(max) {}
    void clear() {
        infoText_.clear();
        progText_.clear();
        max_     = 0;
        progVal_ = 0;
    }
    QString infoText_;
    QString progText_;
    int max_{0};
    int progVal_{0};
};

class MessageLabelProgWidget : public QWidget {
    Q_OBJECT

public:
    MessageLabelProgWidget(QWidget* parent);

    void startProgress(int max);
    void showProgressCancelButton(bool);
    void stopProgress();
    void progress(QString text, int value);
    void startDelayedProgress(QString text, int max = 0);

protected Q_SLOTS:
    void cancelButtonClicked();
    void showDelayedProgress();

Q_SIGNALS:
    void showInfoRequested(QString);
    void stoppedByButton();

private:
    QLabel* progLabel_{nullptr};
    QProgressBar* progBar_{nullptr};
    QToolButton* progCancelTb_{nullptr};
    QTimer* delayedProgressTimer_{nullptr};
    DelayedProgressDef delayedProgressDef_;
    int delayInMs_{500}; // ms
};

class MessageLabelLoadWidget : public QWidget {
    Q_OBJECT

public:
    MessageLabelLoadWidget(QWidget* parent);
    void startLoadLabel(bool showCacelButton);
    void stopLoadLabel();

protected Q_SLOTS:
    void cancelButtonClicked();

Q_SIGNALS:
    void stoppedByButton();

private:
    QLabel* loadIconLabel_{nullptr};
    QLabel* loadTextLabel_{nullptr};
    QToolButton* loadCancelTb_{nullptr};
};

class MessageLabel : public QWidget {
    Q_OBJECT
public:
    explicit MessageLabel(QWidget* parent = nullptr);

    enum Type { NoType, InfoType, WarningType, ErrorType, TipType };

    void showWarning(QString);
    void showError(QString);
    void showTip(QString);
    void appendInfo(QString);
    void appendWarning(QString);
    void appendError(QString);
    void appendTip(QString);
    void startLoadLabel(bool showCacelButton = false);
    void stopLoadLabel();

    void startProgress(int max = 0);
    void stopProgress();
    void progress(QString text, int value);
    void startDelayedProgress(QString text, int max = 0);

    void setShowTypeTitle(bool);
    void clear();
    void setNarrowMode(bool);

public Q_SLOTS:
    void showInfo(QString);

Q_SIGNALS:
    void loadStoppedByButton();
    void progressStoppedByButton();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void showMessage(const Type&, QString);
    void appendMessage(const Type&, QString);

    bool showTypeTitle_{true};
    bool narrowMode_{false};
    Type currentType_{NoType};
    QHBoxLayout* layout_;
    QLabel* pixLabel_{nullptr};
    QLabel* msgLabel_{nullptr};
    MessageLabelLoadWidget* loadWidget_{nullptr};
    MessageLabelProgWidget* progWidget_{nullptr};
    QString message_;
};

#endif /* ecflow_viewer_MessageLabel_HPP */
