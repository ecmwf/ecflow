// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/node/permissions/Allowed.hpp"

namespace ecf {

std::string allowed_to_string(Allowed allowed) {
    std::string s;
    if ((allowed & Allowed::READ) != Allowed::NONE) {
        s += "r";
    }
    if ((allowed & Allowed::WRITE) != Allowed::NONE) {
        s += "w";
    }
    if ((allowed & Allowed::EXECUTE) != Allowed::NONE) {
        s += "x";
    }
    if ((allowed & Allowed::OWNER) != Allowed::NONE) {
        s += "o";
    }
    if ((allowed & Allowed::STICKY) != Allowed::NONE) {
        s += "s";
    }
    return s;
}

Allowed allowed_from_string(const std::string& s) {
    Allowed allowed = Allowed::NONE;
    for (char c : s) {
        switch (c) {
            case ' ':
                break; // Ignore spaces
            case 'r':
            case 'R':
                allowed |= Allowed::READ;
                break;
            case 'w':
            case 'W':
                allowed |= Allowed::WRITE;
                break;
            case 'x':
            case 'X':
                allowed |= Allowed::EXECUTE;
                break;
            case 'o':
            case 'O':
                allowed |= Allowed::OWNER;
                break;
            case 's':
            case 'S':
                allowed |= Allowed::STICKY;
                break;
            default:
                throw InvalidPermissionValue("Invalid permission character: " + std::string(1, c) +
                                             ". Expected one of: [r, w, x, o, s] (regardless of case)");
        }
    }
    return allowed;
}

} // namespace ecf
