/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_ComboMulti_HPP
#define ecflow_viewer_ComboMulti_HPP

#include <QComboBox>
#include <QItemDelegate>

class ComboMulti : public QComboBox {
    Q_OBJECT

public:
    enum Mode { BasicMode, FilterMode };

    enum CustomItemRole { SelectRole = Qt::UserRole + 1 };

    explicit ComboMulti(QWidget* widget = nullptr);
    ~ComboMulti() override;
    bool eventFilter(QObject* object, QEvent* event) override;
    void paintEvent(QPaintEvent*) override;
    void setDisplayText(QString text);
    QString displayText() const;
    bool hasSelection() const { return !selection_.isEmpty(); }
    QStringList selection() const { return selection_; }
    QStringList all() const;
    QStringList selectionData() const;
    void selectSoleItem();
    void setSelection(QStringList);
    void setSelectionByData(QStringList);
    void setMode(Mode);

public Q_SLOTS:
    void slotChecked();
    void clearSelection();

Q_SIGNALS:
    void selectionChanged();

private:
    Mode mode_{BasicMode};
    bool elide_{false};
    QString dpyText_;
    QStringList selection_;
};

class ComboMultiDelegate : public QItemDelegate {
    Q_OBJECT

public:
    explicit ComboMultiDelegate(QObject* parent);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void
    updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

protected Q_SLOTS:
    void slotEdited(int);

Q_SIGNALS:
    void itemChecked() const;
};

#endif /* ecflow_viewer_ComboMulti_HPP */
