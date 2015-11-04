#ifndef COMBOMULTI_HPP_
#define COMBOMULTI_HPP_

#include <QComboBox>
#include <QItemDelegate>

class ComboMulti: public QComboBox
{
Q_OBJECT;

public:
    explicit ComboMulti(QWidget *widget = 0);
    virtual ~ComboMulti();
    bool eventFilter(QObject *object, QEvent *event);
    virtual void paintEvent(QPaintEvent *);
    void setDisplayText(QString text);
    QString displayText() const;
    bool hasSelection() const {return !selection_.isEmpty();}
    QStringList selection() const {return selection_;}
    void selectSoleItem();

public Q_SLOTS:
    void slotChecked();
    void clearSelection();

Q_SIGNALS:
	void selectionChanged();

private:
    QString dpyText_;
    QStringList selection_;
};

class ComboMultiDelegate : public QItemDelegate
{
Q_OBJECT

public:
    explicit ComboMultiDelegate(QObject *parent);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const;

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem & option,
              const QModelIndex & index ) const;

    void setEditorData(QWidget *editor,const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option, const QModelIndex &index ) const;

protected Q_SLOTS:
    void slotEdited(int);

Q_SIGNALS:
     void itemChecked() const;
};


#endif

