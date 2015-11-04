//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================
#ifndef VIEWER_SRC_ATTRIBUTESEARCHPANEL_HPP_
#define VIEWER_SRC_ATTRIBUTESEARCHPANEL_HPP_

#include <QMap>
#include <QWidget>

class QGridLayout;
class QStandardItemModel;
class AttrGroupDesc;

class AttributeSearchPanel : public QWidget
{
    Q_OBJECT

public:
    explicit AttributeSearchPanel(QWidget *parent = 0);
    ~AttributeSearchPanel();
    QString query() const {return query_;}
    QStringList groupNames() const;

public Q_SLOTS:
    void buildQuery();
    void setSelection(QStringList);
    void clearSelection();

protected Q_SLOTS:
	void slotTextEdited(QString);

Q_SIGNALS:
	void queryChanged();

private:
	void addStringLine(QString labelTxt,QString text,QString group);

	QString query_;
	QMap<QString,AttrGroupDesc*> groups_;
	QGridLayout* grid_;
	QStringList selection_;

};

#endif /* VIEWER_SRC_ATTRIBUTESEARCHPANEL_HPP_ */
