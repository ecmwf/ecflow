/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_PrintStyle_HPP
#define ecflow_core_PrintStyle_HPP

#include <string>

class PrintStyle {
public:
    // Use to control the print defs/node output to string or file
    // Please note: with PrintStyle::NET is used to transfer Defs between client <-> server
    //              as such there is no need extensive checking on recreating defs.
    //              i.e. valid name, duplicate nodes, etc
    enum Type_t {
        // Does nothing
        NOTHING = 0,

        // Output the definition that is fully parse-able                       -> On reload *CHECK* everything
        DEFS = 1,

        // Output definition that includes Node state, and AST, fully parseable -> On reload *CHECK* everything
        STATE = 2,

        // Output the definition that is fully parse-able & includes state      -> On reload *CHECK* everything
        MIGRATE = 3,

        // Output the definition that is fully parse-able & includes state      -> On reload relax checking
        NET = 4
    };

    // Disable default construction
    PrintStyle() = delete;
    // Disable copy (and move) semantics
    PrintStyle(const PrintStyle&)            = delete;
    PrintStyle& operator=(const PrintStyle&) = delete;

    static bool is_persist_style(Type_t);

    static std::string to_string(PrintStyle::Type_t);
};

class PrintStyleHolder {
public:
    using holding_t = PrintStyle::Type_t;

    explicit PrintStyleHolder(holding_t t) : old_style_(getStyle()) { setStyle(t); }

    // Disable default construction
    PrintStyleHolder() = delete;
    // Disable copy (and move) semantics
    PrintStyleHolder(const PrintStyleHolder&)            = delete;
    PrintStyleHolder& operator=(const PrintStyleHolder&) = delete;

    ~PrintStyleHolder() { setStyle(old_style_); }

    /// We want to control the output, so that we can dump in old style defs format
    /// or choose to dump for debug.
    static holding_t getStyle() { return current_style_; }
    static void setStyle(holding_t style) { current_style_ = style; };

private:
    holding_t old_style_;

    inline static holding_t current_style_ = PrintStyle::NOTHING;
};

#endif /* ecflow_core_PrintStyle_HPP */
