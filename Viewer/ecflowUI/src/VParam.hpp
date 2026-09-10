/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_viewer_VParam_HPP
#define ecflow_viewer_VParam_HPP

#include <QColor>

#include "VProperty.hpp"

class VParam : public VPropertyObserver {
public:
    explicit VParam(const std::string& name);
    ~VParam() override;

    unsigned int id() const { return id_; }
    QString name() const { return qName_; }
    const std::string& strName() const { return name_; }

    QString label() const { return label_; }
    QColor colour() const { return colour_; }
    QColor fontColour() const { return fontColour_; }
    QColor typeColour() const { return typeColour_; }

    void setProperty(VProperty*);
    void notifyChange(VProperty*) override;

    /*
    int number(const std::string&) const;
    QColor colour(const std::string&) const;
    std::string text(const std::string&) const;
    QFont font(const std::string&) const;

    static QColor toColour(const std::string&) ;
    static QFont  toFont(const std::string&);
    static int    toNumber(const std::string&);
    static bool isColour(const std::string&);
    static bool isFont(const std::string&);
    static bool isNumber(const std::string&);*/

protected:
    // void addAttributes(const std::map<std::string,std::string>& attr);
    // static void init(const std::string& parFile,const std::string
    // id,std::map<std::string,std::map<std::string,std::string> >& vals);

    std::string name_;
    QString qName_;
    QString label_;

    // Cached information
    QColor colour_;
    QColor fontColour_;
    QColor typeColour_;

    /*std::map<std::string,int> numberMap_;
            std::map<std::string,std::string> textMap_;
            std::map<std::string,QColor> colourMap_;
            std::map<std::string,QFont> fontMap_;*/

    VProperty* prop_;
    QString colourPropName_;
    QString fontColourPropName_;
    QString typeColourPropName_;
    int id_;
};

#endif /* ecflow_viewer_VParam_HPP */
