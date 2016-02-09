//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "OutputCache.hpp"

#include <QTimer>

#include "UserMessage.hpp"

OutputCache* OutputCache::instance_=NULL;

#define _UI_OUTPUTCACHE_DEBUG

struct OutputCacheItem : public QTimer 
{
    OutputCacheItem(VFile_ptr,QObject *parent=0);
    VFile_ptr file_;
    int timeout_;
    
    inline void markForRemove()
    {
        start(timeout_);      
    }
    
    inline void keepIt()
    {
        stop();
    }
};    

OutputCacheItem::OutputCacheItem(VFile_ptr file,QObject* parent) : 
    QTimer(parent), file_(file), timeout_(15*1000) //180*1000
{
}    
  
OutputCache* OutputCache::instance()
{
    if(!instance_)
        instance_= new OutputCache();
    
    return instance_;
}
    
void OutputCache::add(VInfo_ptr info,const std::string& sourcePath,VFile_ptr file)
{
    if(!file)
        return;

#ifdef _UI_OUTPUTCACHE_DEBUG
    UserMessage::debug("OutputCache::add --> file");
    file->print();
    print();
#endif

    if(info && file  && info->isNode() && info->server())
    {
        std::string id=info->path() + ":" + sourcePath;        
        if(items_.find(id) == items_.end())
        {
            OutputCacheItem* item=new OutputCacheItem(file);
            connect(item,SIGNAL(timeout()),
                    this,SLOT(removeItem()));
            
            items_[id]=item; 
#ifdef _UI_OUTPUTCACHE_DEBUG
            UserMessage::debug("  add item:" + id);
#endif
        }    
    }
#ifdef _UI_OUTPUTCACHE_DEBUG
    UserMessage::debug("<-- OutputCache::add");
#endif
}    
    
void OutputCache::markForRemove(VFile_ptr file)
{
    if(!file)
        return;
#ifdef _UI_OUTPUTCACHE_DEBUG
    UserMessage::debug("OutputCache::markForRemove --> ref_count: " +
                       QString::number(file.use_count()).toStdString());
    file->print();
    print();
#endif

    QMap<std::string, OutputCacheItem*>::iterator it = items_.begin();
    while (it != items_.end() ) 
    {
       if(it.value()->file_ == file)
       {    
           it.value()->markForRemove();
#ifdef _UI_OUTPUTCACHE_DEBUG
           UserMessage::debug("  item to remove: " + it.key());
           print();
#endif
           return;
       }    
       
       ++it;
    }    
}

void OutputCache::removeItem()
{
    if(OutputCacheItem* item=static_cast<OutputCacheItem*>(sender()))
    {
#ifdef _UI_OUTPUTCACHE_DEBUG
        UserMessage::debug("OutputCache::removeItem --> file");
        item->file_->print();
        print();
#endif
        QMap<std::string, OutputCacheItem*>::iterator it = items_.begin();
        while (it != items_.end() ) 
        {
            if(it.value() == item)
            {            
#ifdef _UI_OUTPUTCACHE_DEBUG
                UserMessage::message(UserMessage::DBG,false,"  remove item --> ref_count:" +
                                     QString::number(it.value()->file_.use_count()).toStdString() +
                                     " item:" + it.key());
#endif
                assert(item->file_.use_count() == 1);
                items_.erase(it);
                item->deleteLater();
#ifdef _UI_OUTPUTCACHE_DEBUG
                print();
                UserMessage::debug("<-- OutputCache::removeItem");
#endif
                return;
            }

            ++it;
        }
    }        
}

VFile_ptr OutputCache::find(VInfo_ptr info,const std::string& sourcePath)
{
    if(info && info->isNode() && info->server())
    {
        std::string id=info->path() + ":" + sourcePath;        
        QMap<std::string, OutputCacheItem*>::iterator it = items_.find(id);       
        if(it != items_.end())
        {
            it.value()->keepIt();
            return it.value()->file_;
        }
    }
    
    
    return VFile_ptr();
}    


void OutputCache::print()
{
    UserMessage::debug("  OutputCache contents -->");
    QMap<std::string, OutputCacheItem*>::iterator it = items_.begin();
    while (it != items_.end() )
    {
        UserMessage::debug("  item:" + it.key() + " tmp:" +
                             it.value()->file_->path() + " countdown:" +
                             ((it.value()->isActive())?"on":"off"));
        ++it;
    }
    UserMessage::debug("  <-- OutputCache contents");
}
