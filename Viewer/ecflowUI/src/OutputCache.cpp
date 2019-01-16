//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "OutputCache.hpp"

#include <QDebug>

#include "UiLog.hpp"

#define _UI_OUTPUTCACHE_DEBUG

OutputCacheItem::OutputCacheItem(QString id, VFile_ptr file) :
      id_(id), file_(file), used_(false)
{
}

bool OutputCacheItem::isAttached() const
{
    return used_;
}

void OutputCacheItem::attach()
{
    used_=true;
}

void OutputCacheItem::detach()
{
    used_=false;
    inTimeOut_.start();
}

std::ostream& operator<<(std::ostream& stream,const OutputCacheItem& item)
{
    stream << "id:" << item.id_.toStdString() << " tmp:" <<
                            item.file_->path() << " countdown:" <<
                            (!item.isAttached());
    return stream;
}

//========================================================
//
// OutputCache
//
//========================================================

OutputCache::OutputCache(QObject *parent) :
    QObject(parent),
    timeOut_(1000*15), //we check the state every 15 sec
    maxAttachedPeriod_(1000*120) //cache retention period is 2 min
{
    timer_=new QTimer(this);
    connect(timer_,SIGNAL(timeout()),
            this,SLOT(slotTimeOut()));
}

OutputCache::~OutputCache()
{
   clear();
}

void OutputCache::adjustTimer()
{
    if(items_.isEmpty())
        stopTimer();
    else
        startTimer();
}

void OutputCache::startTimer()
{
    if(!timer_->isActive())
        timer_->start(timeOut_);
}

void OutputCache::stopTimer()
{
    timer_->stop();
}

void OutputCache::clear()
{
    Q_FOREACH(OutputCacheItem* item,items_.values())
    {
        delete item;
    }
    stopTimer();
}

OutputCacheItem* OutputCache::add(VInfo_ptr info,const std::string& sourcePath,VFile_ptr file)
{
    if(!file)
        return nullptr;

#ifdef _UI_OUTPUTCACHE_DEBUG
    UI_FUNCTION_LOG
    file->print();
    //print();
#endif

    if(info && file  && info->isNode() && info->server())
    {
        //The key we would store for the item in the map
        QString id=QString::fromStdString(info->path() + ":" + sourcePath);

        //The item exists
        QMap<QString, OutputCacheItem*>::iterator it = items_.find(id);
        if(it != items_.end())
        {
            it.value()->attach();
            it.value()->file_=file;
            startTimer();
            return it.value();
        }
        //The item not yet exists
        else
        {
            OutputCacheItem* item=new OutputCacheItem(id,file);
            items_[id]=item;
            item->attach();
#ifdef _UI_OUTPUTCACHE_DEBUG
            UiLog().dbg() << "  add " << *item;
            //print();
#endif
            startTimer();
            return item;
        }
    }

    return nullptr;
}    

//Attach one item and detach all the others
OutputCacheItem* OutputCache::attachOne(VInfo_ptr info,const std::string& fileName)
{
    OutputCacheItem* attachedItem=nullptr;
    if(info && info->isNode() && info->server())
    {
        QString inPath=QString::fromStdString(info->path() + ":");
        QString inFileName=QString::fromStdString(fileName);

        QMap<QString, OutputCacheItem*>::iterator it=items_.begin();
        while (it != items_.end() )
        {
            if(it.key().startsWith(inPath) && it.key().endsWith(inFileName))
            {
                it.value()->attach();
                attachedItem=it.value();
            }
            else
            {
                it.value()->detach();
            }
            ++it;
        }
    }
    adjustTimer();
    return attachedItem;
}

//Detach all the items
void OutputCache::detach()
{
    QMap<QString, OutputCacheItem*>::iterator it=items_.begin();
    while(it != items_.end())
    {
#ifdef _UI_OUTPUTCACHE_DEBUG
        UI_FUNCTION_LOG
        //print();
        UiLog().dbg() << "  detach -> " << *(it.value());
#endif
        it.value()->detach();
        Q_ASSERT(!it.value()->isAttached());

#ifdef _UI_OUTPUTCACHE_DEBUG
        //print();
        UiLog().dbg() << "  detached -> " << *(it.value());
#endif
        ++it;
    }

    adjustTimer();
}

void OutputCache::slotTimeOut()
{
#ifdef _UI_OUTPUTCACHE_DEBUG
UI_FUNCTION_LOG
#endif

    QMap<QString, OutputCacheItem*>::iterator it=items_.begin();
    while(it != items_.end() )
    {
        OutputCacheItem* item=it.value();
        if(!item->isAttached() &&
           item->inTimeOut_.elapsed() > maxAttachedPeriod_)
        {
#ifdef _UI_OUTPUTCACHE_DEBUG
            UiLog().dbg() << "  remove " << it.value();
            UiLog().dbg() << "    -> ref_count:" << it.value()->file_.use_count();
#endif
            it=items_.erase(it);
            delete item;
            //item->deleteLater();
#ifdef _UI_OUTPUTCACHE_DEBUG
            print();
#endif
        }
        else
        {
            ++it;
        }
    }

    adjustTimer();
}

void OutputCache::print()
{
    UI_FUNCTION_LOG
    QMap<QString, OutputCacheItem*>::iterator it = items_.begin();
    while (it != items_.end() )
    {
        UiLog().dbg() << *(it.value());
        ++it;
    }  
}
