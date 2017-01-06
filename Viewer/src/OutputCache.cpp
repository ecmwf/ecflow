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

#include "UiLog.hpp"

OutputCache* OutputCache::instance_=NULL;

#define _UI_OUTPUTCACHE_DEBUG

OutputCacheItem::OutputCacheItem(const std::string& id, VFile_ptr file,QObject *parent) :
      QTimer(parent), id_(id), file_(file), timeout_(120*1000), cnt_(0)
{
    attach();
}

bool OutputCacheItem::sameAs(VInfo_ptr info,const std::string& sourcePath)
{
    if(info && info->isNode() && info->server())
    {
        std::string id=info->path() + ":" + sourcePath;
        return (id == id_);
    }
    return false;
}

void OutputCacheItem::attach()
{
    cnt_++;
    stop();
}

void OutputCacheItem::detach()
{
    cnt_--;
    if(cnt_<=0)
    {
        cnt_=0;
        start(timeout_);
    }
}

//========================================================
//
// OutputCache
//
//========================================================

OutputCache::~OutputCache()
{
   clear();
}

OutputCache* OutputCache::instance()
{
    if(!instance_)
        instance_= new OutputCache();
    
    return instance_;
}

void OutputCache::clear()
{
    Q_FOREACH(OutputCacheItem* item,items_.values())
    {
        delete item;
    }
}

OutputCacheItem* OutputCache::add(VInfo_ptr info,const std::string& sourcePath,VFile_ptr file)
{
    if(!file)
        return NULL;

#ifdef _UI_OUTPUTCACHE_DEBUG
    UiLog().dbg() << "OutputCache::add --> file";
    file->print();
    print();
#endif

    if(info && file  && info->isNode() && info->server())
    {
        std::string id=info->path() + ":" + sourcePath;        
        QMap<std::string, OutputCacheItem*>::iterator it = items_.find(id);
        if(it != items_.end())
        {
            it.value()->attach();
            it.value()->file_=file;
            return it.value();
        }
        else
        {
            OutputCacheItem* item=new OutputCacheItem(id,file);
            connect(item,SIGNAL(timeout()),
                    this,SLOT(removeItem()));
            
            items_[id]=item;
#ifdef _UI_OUTPUTCACHE_DEBUG
            UiLog().dbg() << "  add item:" << id;
            print();
            UiLog().dbg() << "<-- OutputCache::add";
#endif
            return item;
        }
    }
#ifdef _UI_OUTPUTCACHE_DEBUG
    UiLog().dbg() << "<-- OutputCache::add";
#endif

    return NULL;
}    

void OutputCache::detach(OutputCacheItem* item)
{
    if(item)
    {
#ifdef _UI_OUTPUTCACHE_DEBUG
        UiLog().dbg() << "OutputCache::detach -->";
        print();
        UiLog().dbg() << "  detach item: " << item->id_;
        item->detach();
        print();
        UiLog().dbg() << "<-- detach";
#endif
    }
}

void OutputCache::removeItem()
{
    if(OutputCacheItem* item=static_cast<OutputCacheItem*>(sender()))
    {
#ifdef _UI_OUTPUTCACHE_DEBUG
        UiLog().dbg() << "OutputCache::removeItem --> file";
        item->file_->print();
        print();
#endif
        QMap<std::string, OutputCacheItem*>::iterator it = items_.begin();
        while (it != items_.end() ) 
        {
            if(it.value() == item)
            {            
#ifdef _UI_OUTPUTCACHE_DEBUG
                UiLog().dbg() << "  remove item --> ref_count:" <<
                                     it.value()->file_.use_count() <<
                                     " item:" << it.key();
#endif
                //assert(item->file_.use_count() == 1);
                //The ref count is not necessarily 1 here
                items_.erase(it);
                item->deleteLater();
#ifdef _UI_OUTPUTCACHE_DEBUG
                print();
                UiLog().dbg() << "<-- OutputCache::removeItem";
#endif
                return;
            }

            ++it;
        }
    }        
}

OutputCacheItem* OutputCache::use(VInfo_ptr info,const std::string& sourcePath)
{
    if(info && info->isNode() && info->server())
    {
        std::string id=info->path() + ":" + sourcePath;
        QMap<std::string, OutputCacheItem*>::iterator it = items_.find(id);
        if(it != items_.end())
        {
            it.value()->attach();
            return it.value();

        }
    }

    return NULL;
}

void OutputCache::print()
{
    UiLog().dbg() << "  OutputCache contents -->";
    QMap<std::string, OutputCacheItem*>::iterator it = items_.begin();
    while (it != items_.end() )
    {
        UiLog().dbg() << "  item:" + it.key() << " tmp:" <<
                             it.value()->file_->path() << " countdown:" <<
                             ((it.value()->isActive())?"on":"off");
        ++it;
    }
    UiLog().dbg() << "  <-- OutputCache contents";
}
