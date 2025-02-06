/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/ClientSuiteMgr.hpp"

#include <algorithm> // for sort, remove_if
#include <sstream>   // for std::stringstream
#include <stdexcept>

#include "ecflow/core/Ecf.hpp"

using namespace ecf;
using namespace std;

// #define DEBUG_HANDLE 1

ClientSuiteMgr::ClientSuiteMgr(Defs* defs) : defs_(defs) {
}

unsigned int ClientSuiteMgr::create_client_suite(bool auto_add_new_suites,
                                                 const std::vector<std::string>& suites,
                                                 const std::string& the_user) {
    // The handle must be unique. If there are holes, re-use them, i.e
    //     1,2,3,4,5     user deletes handle 3 ===>  1,2,4,5
    // Hence re-use handle 3
    bool found_hole         = false;
    unsigned int new_handle = 1;
    for (auto& clientSuite : clientSuites_) {
        if (clientSuite.handle() == new_handle) {
            new_handle++;
        }
        else {
            found_hole = true;
            break;
        }
    }
    if (!found_hole)
        new_handle = clientSuites_.size() + 1;

    clientSuites_.emplace_back(defs_, new_handle, auto_add_new_suites, suites, the_user);

    // aesthetics only
    std::sort(clientSuites_.begin(), clientSuites_.end(), [](const ecf::ClientSuites& a, const ecf::ClientSuites& b) {
        return a.handle() < b.handle();
    });

    // make sure all suites in ClientSuites are in same order as  Defs suites
    update_suite_order();

#ifdef DEBUG_HANDLE
    std::cout << "ClientSuiteMgr::create_client_suite: " << dump() << "\n";
#endif
    return new_handle;
}

void ClientSuiteMgr::remove_client_suite(unsigned int client_handle) {
    size_t before = clientSuites_.size();
    clientSuites_.erase(
        std::remove_if(clientSuites_.begin(),
                       clientSuites_.end(),
                       [client_handle](const ecf::ClientSuites& cs) { return cs.handle() == client_handle; }),
        clientSuites_.end());

    if (before == clientSuites_.size()) {
        std::stringstream ss;
        ss << "ClientSuiteMgr::remove_client_suite: handle(" << client_handle
           << ") does not exist. Handle dropped? Please refresh GUI/re-register suites";
        throw std::runtime_error(ss.str());
    }
#ifdef DEBUG_HANDLE
    std::cout << "ClientSuiteMgr::remove_client_suite: handle(" << client_handle << ") " << dump() << "\n";
#endif
}

void ClientSuiteMgr::remove_client_suites(const std::string& user_to_drop) {
    size_t before = clientSuites_.size();
    clientSuites_.erase(
        std::remove_if(clientSuites_.begin(),
                       clientSuites_.end(),
                       [&user_to_drop](const ecf::ClientSuites& cs) { return cs.user() == user_to_drop; }),
        clientSuites_.end());

    if (before == clientSuites_.size()) {
        std::stringstream ss;
        ss << "ClientSuiteMgr::remove_client_suites: user(" << user_to_drop
           << ") has no registered handles. User dropped? Please refresh GUI/re-register suites";
        throw std::runtime_error(ss.str());
    }

#ifdef DEBUG_HANDLE
    std::cout << "ClientSuiteMgr::remove_client_suites: user_to_drop(" << user_to_drop << ") " << dump() << "\n";
#endif
}

void ClientSuiteMgr::add_suites(unsigned int client_handle, const std::vector<std::string>& suites) {
    size_t client_suites_size = clientSuites_.size();
    for (size_t i = 0; i < client_suites_size; i++) {
        if (clientSuites_[i].handle() == client_handle) {
            for (const auto& suite : suites) {
                clientSuites_[i].add_suite(suite);
            }
#ifdef DEBUG_HANDLE
            std::cout << "ClientSuiteMgr::add_suites: client_handle(" << client_handle << ") " << dump() << "\n";
#endif
            update_suite_order();
            return;
        }
    }
    std::stringstream ss;
    ss << "ClientSuiteMgr::add_suites: handle(" << client_handle
       << ") does not exist. Handle dropped? Please refresh GUI/re-register suites";
    throw std::runtime_error(ss.str());
}

void ClientSuiteMgr::remove_suites(unsigned int client_handle, const std::vector<std::string>& suites) {
    size_t client_suites_size = clientSuites_.size();
    for (size_t i = 0; i < client_suites_size; i++) {
        if (clientSuites_[i].handle() == client_handle) {
            for (const auto& suite : suites) {
                clientSuites_[i].remove_suite(suite);
            }
#ifdef DEBUG_HANDLE
            std::cout << "ClientSuiteMgr::remove_suites: client_handle(" << client_handle << ") " << dump() << "\n";
#endif
            return;
        }
    }
    std::stringstream ss;
    ss << "ClientSuiteMgr::remove_suites: handle(" << client_handle
       << ") does not exist. Handle dropped? Please refresh GUI/re-register suites";
    throw std::runtime_error(ss.str());
}

void ClientSuiteMgr::auto_add_new_suites(unsigned int client_handle, bool auto_add_new_suites) {
    size_t client_suites_size = clientSuites_.size();
    for (size_t i = 0; i < client_suites_size; i++) {
        if (clientSuites_[i].handle() == client_handle) {
            clientSuites_[i].add_new_suite(auto_add_new_suites);

#ifdef DEBUG_HANDLE
            std::cout << "ClientSuiteMgr::auto_add_new_suites: client_handle(" << client_handle
                      << ") auto_add_new_suites(" << auto_add_new_suites << ") " << dump() << "\n";
#endif
            return;
        }
    }
    std::stringstream ss;
    ss << "ClientSuiteMgr::auto_add_new_suites: handle(" << client_handle
       << ") does not exist. Handle dropped? Please refresh GUI/re-register suites";
    throw std::runtime_error(ss.str());
}

bool ClientSuiteMgr::valid_handle(unsigned int client_handle) const {
    size_t client_suites_size = clientSuites_.size();
    for (size_t i = 0; i < client_suites_size; i++) {
        if (clientSuites_[i].handle() == client_handle) {
            return true;
        }
    }
    return false;
}

/// returns true if the handle was created, or suites added or removed from it
bool ClientSuiteMgr::handle_changed(unsigned int client_handle) {
    size_t client_suites_size = clientSuites_.size();
    for (size_t i = 0; i < client_suites_size; i++) {
        if (clientSuites_[i].handle() == client_handle) {
            return clientSuites_[i].handle_changed();
        }
    }
    return false;
}

void ClientSuiteMgr::collateChanges(unsigned int client_handle, DefsDelta& changes) const {
    // collate changes over the suites that match the client handle
    size_t client_suites_size = clientSuites_.size();
    for (size_t i = 0; i < client_suites_size; i++) {
        if (clientSuites_[i].handle() == client_handle) {
            clientSuites_[i].collateChanges(changes);
            return;
        }
    }
}

void ClientSuiteMgr::suites(unsigned int client_handle, std::vector<std::string>& names) const {
    size_t client_suites_size = clientSuites_.size();
    for (size_t i = 0; i < client_suites_size; i++) {
        if (clientSuites_[i].handle() == client_handle) {
            clientSuites_[i].suites(names);
            return;
        }
    }
}

defs_ptr ClientSuiteMgr::create_defs(unsigned int client_handle, defs_ptr server_defs) const {
    size_t client_suites_size = clientSuites_.size();
    for (size_t i = 0; i < client_suites_size; i++) {
        if (clientSuites_[i].handle() == client_handle) {
            return clientSuites_[i].create_defs(server_defs);
        }
    }
    return defs_ptr();
}

void ClientSuiteMgr::max_change_no(unsigned int client_handle,
                                   unsigned int& max_state_change_no,
                                   unsigned int& max_modify_change_no) {
    size_t client_suites_size = clientSuites_.size();
    for (size_t i = 0; i < client_suites_size; i++) {
        if (clientSuites_[i].handle() == client_handle) {
            clientSuites_[i].max_change_no(max_state_change_no, max_modify_change_no);
            return;
        }
    }
    std::stringstream ss;
    ss << "ClientSuiteMgr::max_change_no: handle(" << client_handle
       << ") does not exist in server. Handle dropped or Server may have died? Please refresh GUI/re-register suites";
    throw std::runtime_error(ss.str());
}

void ClientSuiteMgr::suite_added_in_defs(suite_ptr suite) {
    size_t client_suites_size = clientSuites_.size();
    for (size_t i = 0; i < client_suites_size; i++) {
        clientSuites_[i].suite_added_in_defs(suite);
        clientSuites_[i].update_suite_order();
    }
}

void ClientSuiteMgr::suite_replaced_in_defs(suite_ptr suite) {
    size_t client_suites_size = clientSuites_.size();
    for (size_t i = 0; i < client_suites_size; i++) {
        clientSuites_[i].suite_replaced_in_defs(suite);
        clientSuites_[i].update_suite_order();
    }
}

void ClientSuiteMgr::suite_deleted_in_defs(suite_ptr suite) {
    size_t client_suites_size = clientSuites_.size();
    for (size_t i = 0; i < client_suites_size; i++) {
        clientSuites_[i].suite_deleted_in_defs(suite);
    }
}

void ClientSuiteMgr::update_suite_order() {
    size_t client_suites_size = clientSuites_.size();
    for (size_t i = 0; i < client_suites_size; i++) {
        clientSuites_[i].update_suite_order();
    }
}

std::string ClientSuiteMgr::dump_max_change_no() const {
    std::stringstream ss;
    ss << "ClientSuiteMgr::dump_max_change_no: ECF:(" << Ecf::state_change_no() << "," << Ecf::modify_change_no()
       << ")\n";
    size_t client_suites_size = clientSuites_.size();
    for (size_t i = 0; i < client_suites_size; i++) {
        unsigned int max_state_change_no  = 0;
        unsigned int max_modify_change_no = 0;
        clientSuites_[i].max_change_no(max_state_change_no, max_modify_change_no);
        ss << "handle: " << clientSuites_[i].handle() << " max(" << max_state_change_no << "," << max_modify_change_no
           << ")\n";
    }
    return ss.str();
}

/// For debug dumps
std::string ClientSuiteMgr::dump() const {
    size_t client_suites_size = clientSuites_.size();
    std::stringstream ss;
    ss << "ECF:(" << Ecf::state_change_no() << "," << Ecf::modify_change_no() << ") clientSuites_.size("
       << client_suites_size << ")\n";
    for (size_t i = 0; i < client_suites_size; i++) {
        ss << clientSuites_[i].dump() << "\n";
    }
    return ss.str();
}
