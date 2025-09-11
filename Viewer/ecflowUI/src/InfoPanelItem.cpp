/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "InfoPanelItem.hpp"

#include <map>

#include "InfoPanel.hpp"
#include "InfoProvider.hpp"
#include "ServerHandler.hpp"
#include "VNode.hpp"

static std::map<std::string, InfoPanelItemFactory*>* makers = nullptr;

InfoPanelItemFactory::InfoPanelItemFactory(const std::string& name) {
    if (makers == nullptr) {
        makers = new std::map<std::string, InfoPanelItemFactory*>;
    }

    // Put in reverse order...
    (*makers)[name] = this;
}

InfoPanelItem* InfoPanelItemFactory::create(const std::string& name) {
    auto j = makers->find(name);
    if (j != makers->end()) {
        return (*j).second->make();
    }

    return nullptr;
}

//=======================================================
//
// InfoPanelItem
//
//=======================================================

InfoPanelItem::~InfoPanelItem() {
    clear();
}

void InfoPanelItem::setOwner(InfoPanel* owner) {
    assert(!owner_);
    owner_ = owner;
}

// Set the new VInfo object.
// We also we need to manage the node observers. The InfoItem
// will be the observer of the server of the object stored in
// the new VInfo
void InfoPanelItem::adjust(VInfo_ptr info) {
    // Check if there is data in info
    if (info) {
        ServerHandler* server = info->server();

        bool sameServer = (info_) ? (info_->server() == server) : false;

        // Handle observers
        if (!sameServer) {
            if (info_ && info_->server()) {
                info_->server()->removeNodeObserver(this);
            }
            info->server()->addNodeObserver(this);
        }
    }
    // If the there is no data we clean everything and return
    else {
        if (info_ && info_->server()) {
            info_->server()->removeNodeObserver(this);
        }
    }

    // Set the info
    info_ = info;
}

void InfoPanelItem::clear() {
    if (info_ && info_->server()) {
        // info_->server()->removeServerObserver(this);
        info_->server()->removeNodeObserver(this);
    }

    info_.reset();

    for (auto& infoProvider : infoProviders_) {
        infoProvider->clear();
    }
}

// This function is called when the infopanel
//  is being reset. The info_ might be unset.
void InfoPanelItem::setActive(bool active) {
    active_ = active;

    if (active_) {
        // Enable the infoProviders
        for (auto& infoProvider : infoProviders_) {
            infoProvider->setActive(true);
        }
    }
    else {
        clearContents();

        selected_  = false;
        suspended_ = false;

        // Disable the info provider
        for (auto& infoProvider : infoProviders_) {
            // This will clear the providers again
            infoProvider->setActive(false);
        }
    }

    // updateWidgetState();
}

void InfoPanelItem::setSelected(bool selected, VInfo_ptr info) {
    if (selected_ == selected) {
        return;
    }

    ChangeFlags flags(SelectedChanged);
    selected_ = selected;

    assert(active_);

    if (selected_) {
        // Suspend
        if (suspended_) {}
        // Resume
        else {
            if (unselectedFlags_.isSet(KeepContents)) {
                if (!info_) {
                    reload(info);
                    return;
                }
            }
            else {
                reload(info);
                return;
            }
        }
    }
    else {
        if (!frozen_ && !unselectedFlags_.isSet(KeepContents)) {
            // This will also clear the providers
            clearContents();
            return;
        }
    }

    // We update the derived class
    updateState(flags);
}

void InfoPanelItem::setSuspended(bool suspended, VInfo_ptr info) {
    if (suspended_ == suspended) {
        return;
    }

    suspended_ = suspended;
    ChangeFlags flags(SuspendedChanged);

    if (!active_) {
        return;
    }

    // Suspend
    if (suspended_) {}
    // Resume
    else {
        if (selected_ && !info_) {
            reload(info);
            return;
        }
    }

    // We update the derived class
    updateState(flags);
}

void InfoPanelItem::setFrozen(bool b) {
    frozen_ = b;
    if (!active_) {
        return;
    }

    // We update the derived class
    updateState(ChangeFlags(FrozenChanged));
}

void InfoPanelItem::setDetached(bool b, bool update) {
    detached_ = b;

    if (update) {
        if (!active_) {
            return;
        }

        // We update the derived class
        updateState(ChangeFlags(DetachedChanged));
    }
}

bool InfoPanelItem::hasSameContents(VInfo_ptr) {
    return false;
}

void InfoPanelItem::linkSelected(const std::string& path) {
    if (!suspended_) {
        VInfo_ptr info = VInfo::createFromPath(info_->server(), path);

        if (info) {
            assert(owner_);
            owner_->linkSelected(info);
        }
    }
}

void InfoPanelItem::linkSelected(VInfo_ptr info) {
    if (!suspended_) {
        if (info) {
            assert(owner_);
            owner_->linkSelected(info);
        }
    }
}

void InfoPanelItem::relayInfoPanelCommand(VInfo_ptr info, QString cmd) {
    if (info) {
        assert(owner_);
        owner_->relayInfoPanelCommand(info, cmd);
    }
}

void InfoPanelItem::relayDashboardCommand(VInfo_ptr info, QString cmd) {
    if (info) {
        assert(owner_);
        owner_->relayDashboardCommand(info, cmd);
    }
}

// From NodeObserver
void InfoPanelItem::notifyBeginNodeChange(const VNode* node,
                                          const std::vector<ecf::Aspect::Type>& aspect,
                                          const VNodeChange&) {
    if (!node || frozen_ || !active_ || suspended_) {
        return;
    }

    // Check if there is data in info
    if (info_) {
        if (info_->isNode()) {
            // Check if updates are handled when unselected
            if (!selected_ && !unselectedFlags_.isSet(KeepContents)) {
                return;
            }

            // Check if the updated node is handled by the item
            if (handleAnyChange_ || info_->node() == node || (useAncestors_ && info_->node()->isAncestor(node))) {
                // We call the method implemented in the concrete class
                // to handle the changes
                nodeChanged(node, aspect);
                return;
            }
        }
    }
}
