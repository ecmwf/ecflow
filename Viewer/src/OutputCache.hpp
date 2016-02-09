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

#include "OutputClient.hpp"
#include "VFile.hpp"
#include "VInfo.hpp"

class OutputCacheItem;

class OutputCache:  public QObject
{
    Q_OBJECT

public:   
    void add(VInfo_ptr info,const std::string& sourcePath,VFile_ptr file);
    void markForRemove(VFile_ptr);
    VFile_ptr find(VInfo_ptr,const std::string&);
    void print();
    static OutputCache* instance();
    
protected Q_SLOTS:
    void removeItem(); 

protected:
    OutputCache() {}
      
private:
    OutputCache(const OutputClient&);
    OutputCache& operator=(const OutputCache&);

    static OutputCache* instance_;
    QMap<std::string,OutputCacheItem*> items_;

};

#endif // OUTPUTCHACHE_HPP

