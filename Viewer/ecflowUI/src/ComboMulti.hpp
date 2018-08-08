#ifndef COMBOMULTI_HPP_
#define COMBOMULTI_HPP_

#include <QComboBox>
#include <QItemDelegate>

class ComboMulti: public QComboBox
{
Q_OBJECT;

public:
    enum Mode {BasicMode,FilterMode};

    enum CustomItemRole {SelectRole = Qt::UserRole+1};

	explicit ComboMulti(QWidget *widget = nullptr);
    ~ComboMulti() override;
    bool eventFilter(QObject *object, QEvent *event) override;
    void paintEvent(QPaintEvent *) override;
    void setDisplayText(QString text);
    QString displayText() const;
    bool hasSelection() const {return !selection_.isEmpty();}
    QStringList selection() const {return selection_;}
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
    Mode mode_;
    bool elide_;
	QString dpyText_;
    QStringList selection_;
};

class ComboMultiDelegate : public QItemDelegate
{
Q_OBJECT

public:
    explicit ComboMultiDelegate(QObject *parent);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem & option,
              const QModelIndex & index ) const override;

    void setEditorData(QWidget *editor,const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option, const QModelIndex &index ) const override;

protected Q_SLOTS:
    void slotEdited(int);

Q_SIGNALS:
     void itemChecked() const;
};


#endif

