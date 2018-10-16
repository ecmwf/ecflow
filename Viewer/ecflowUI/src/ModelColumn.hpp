//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef MODELCOLUMN_H
#define MODELCOLUMN_H

class VProperty;
class DiagData;

#include <cassert>

#include <QList>
#include <QObject>
#include <QString>

class ModelColumnItem
{
friend class ModelColumn;
public:
    explicit ModelColumnItem(const std::string& id,bool extra=false);
    bool isExtra() const {return extra_;}
    bool isEditable() const {return editable_;}

protected:
	QString label_;
	QString id_;
	int index_;
	QString icon_;
	QString tooltip_;
    bool extra_;
    bool editable_;
};

class ModelColumn : public QObject
{
    Q_OBJECT
public:
	explicit ModelColumn(const std::string& id);

	int count() const {return items_.size();}
    int indexOf(QString) const;
	QString id(int i) const {assert(i>=0 && i < count()); return items_.at(i)->id_;}
	QString label(int i) const {assert(i>=0 && i < count()); return items_.at(i)->label_;}
	QString tooltip(int i) const {assert(i>=0 && i < count()); return items_.at(i)->tooltip_;}
    bool isExtra(int i) const {assert(i>=0 && i < count()); return items_.at(i)->isExtra();}
    bool isEditable(int i) const {assert(i>=0 && i < count()); return items_.at(i)->isEditable();}
    bool hasDiag() const {return diagStart_ >=0 && diagEnd_ > diagStart_;}
    int diagStartIndex() const {return diagStart_;}
    int diagEndIndex() const {return diagEnd_;}

    void addExtraItem(QString,QString);
    void changeExtraItem(int,QString,QString);
    void removeExtraItem(QString);

    void setDiagData(DiagData*);

	static ModelColumn* def(const std::string& id);
    static ModelColumn* tableModelColumn();

	//Called from VConfigLoader
	static void load(VProperty* group);

    //Called from VSettingsLoader
    static void loadSettings();

Q_SIGNALS:
    void appendItemBegin();
    void appendItemEnd();
    void addItemsBegin(int,int);
    void addItemsEnd(int,int);
    void changeItemBegin(int);
    void changeItemEnd(int);
    void removeItemsBegin(int,int);
    void removeItemsEnd(int,int);

protected:
    void save();
    void loadItem(VProperty*);
    void loadExtraItem(QString,QString);
    void loadDiagItem(QString,QString);
    void loadUserSettings();
    bool isSameDiag(DiagData *diag) const;

	std::string id_;
    QList<ModelColumnItem*> items_;
    std::string configPath_;
    int diagStart_;
    int diagEnd_;
};

#endif
