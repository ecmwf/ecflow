//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef OUTPUTCHACHE_HPP_
#define OUTPUTCHACHE_HPP_

#include <QMap>
#include <QTimer>

#include "OutputClient.hpp"
#include "VFile.hpp"
#include "VInfo.hpp"

class OutputCache;

struct OutputCacheItem : public QTimer
{
  friend class OutputCache;

public:
    OutputCacheItem(const std::string& id,VFile_ptr file,QObject *parent=0);
    VFile_ptr file() const {return file_;}
    bool sameAs(VInfo_ptr info,const std::string& sourcePath);

protected:
    void attach();
    void detach();
    void keepIt();

    std::string id_;
    VFile_ptr file_;
    int timeout_;
    int cnt_;
};

class OutputCache:  public QObject
{
    Q_OBJECT

public:   
    OutputCacheItem* add(VInfo_ptr info,const std::string& sourcePath,VFile_ptr file);
    void detach(OutputCacheItem*);
    OutputCacheItem* use(VInfo_ptr info,const std::string& sourcePath);


    /*void markForRemove(VFile_ptr file,bool forget=false);
    void setCurrent(VInfo_ptr,const std::string&);
    bool isCurrent(VInfo_ptr,const std::string&);

    VFile_ptr find(VInfo_ptr,const std::string&);*/

    void print();
    static OutputCache* instance();
    void clear();

protected Q_SLOTS:
    void removeItem(); 

protected:
    OutputCache() {}
    ~OutputCache();

private:
    OutputCache(const OutputClient&);
    OutputCache& operator=(const OutputCache&);

    static OutputCache* instance_;
    QMap<std::string,OutputCacheItem*> items_;

};

#endif // OUTPUTCHACHE_HPP

