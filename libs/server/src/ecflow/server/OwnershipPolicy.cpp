// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/server/OwnershipPolicy.hpp"

#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include "ecflow/attribute/OwnerAttr.hpp"
#include "ecflow/core/Identity.hpp"
#include "ecflow/node/Node.hpp"

namespace ecf {

namespace {

[[noreturn]] void refuse(const char* operation, const Node& node, const std::string& reason) {
    throw std::runtime_error(std::string("Ownership ") + operation + " refused on " + node.absNodePath() + ": " +
                             reason);
}

} // namespace

OwnershipPolicy::OwnershipPolicy(std::shared_ptr<const os::AccountDatabase> accounts, uid_t server_uid)
    : accounts_(std::move(accounts)),
      server_uid_(server_uid) {
    if (!accounts_) {
        throw std::invalid_argument("OwnershipPolicy: an account database is required");
    }
}

void OwnershipPolicy::check_create(const Identity& requester, const Node& node, const OwnerAttr& value) const {
    if (!requester.is_user()) {
        refuse("create", node, "only users can manage Owners");
    }
    if (node.isAlias()) {
        refuse("create", node, "an alias cannot have an Owner (it inherits the Owner of its task)");
    }
    if (node.get_owner()) {
        refuse("create", node, "the node already has an Owner");
    }
    if (!is_administrator(requester) && requester.username().value() != value.user()) {
        refuse("create",
               node,
               "users can only create an Owner naming themselves, but '" + requester.username().value() +
                   "' requested '" + value.value() + "'");
    }
    check_eligible(value, node, "create");
}

void OwnershipPolicy::check_update(const Identity& requester, const Node& node, const OwnerAttr& value) const {
    check_requester_is_current_owner(requester, node, "update");
    check_eligible(value, node, "update");
}

void OwnershipPolicy::check_delete(const Identity& requester, const Node& node) const {
    check_requester_is_current_owner(requester, node, "delete");
}

void OwnershipPolicy::check_eligible(const OwnerAttr& value, const Node& node, const char* operation) const {
    std::optional<Result<os::OsIdentity>> eligible;
    try {
        eligible.emplace(os::OwnerEligibility(*accounts_, server_uid_).check(value.user(), value.group()));
    }
    catch (const std::exception& e) {
        // Fail closed, with the context of the operation
        refuse(operation, node, "could not check the Owner '" + value.value() + "': " + e.what());
    }
    if (!eligible->ok()) {
        refuse(operation, node, eligible->reason());
    }
}

void OwnershipPolicy::check_requester_is_current_owner(const Identity& requester,
                                                       const Node& node,
                                                       const char* operation) const {
    if (!requester.is_user()) {
        refuse(operation, node, "only users can manage Owners");
    }
    const auto* current = node.get_owner();
    if (!current) {
        refuse(operation, node, "the node has no Owner");
    }
    if (!is_administrator(requester) && requester.username().value() != current->user()) {
        refuse(operation,
               node,
               "only the current owner '" + current->user() + "' can change it, not '" + requester.username().value() +
                   "'");
    }
}

} // namespace ecf
