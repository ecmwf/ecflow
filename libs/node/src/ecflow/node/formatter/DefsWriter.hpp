/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_formatter_DefsWriter_HPP
#define ecflow_node_formatter_DefsWriter_HPP

#include "ecflow/attribute/AutoArchiveAttr.hpp"
#include "ecflow/attribute/AutoCancelAttr.hpp"
#include "ecflow/attribute/LateAttr.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/Version.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/AutoRestoreAttr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/MiscAttrs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/Format.hpp"

namespace ecf {

/* ************************************************************************** */
/* *** Context ************************************************************** */
/* ************************************************************************** */

struct Style
{
    Style(PrintStyle::Type_t s) : selected_(s) {}

    PrintStyle::Type_t selected() { return selected_; }

    template <PrintStyle::Type_t... Args>
    bool is_one_of() const {
        return ((Args == selected_) || ...);
    }
    template <PrintStyle::Type_t... Args>
    bool is_not_one_of() const {
        return !is_one_of<Args...>();
    }

private:
    PrintStyle::Type_t selected_;
};

struct Format
{
    bool indenting           = true;
    int8_t indentation_width = 2;
    int8_t indentation_level = 0;

    bool is_indenting() const { return indenting; }
    void increase_indentation() { indentation_level++; }
    void decrease_indentation() { indentation_level = std::max(0, indentation_level - 1); }

    uint32_t indentation_spaces() const { return indentation_width * indentation_level; }
};

struct Context
{
    Style style;
    Format format;

    static Context make_for(PrintStyle::Type_t style) {
        switch (style) {
            case PrintStyle::DEFS:
                return Context{Style{PrintStyle::DEFS}, Format{true, 2, 0}};
            case PrintStyle::STATE:
                return Context{Style{PrintStyle::STATE}, Format{false, 0, 0}};
            case PrintStyle::NET:
                return Context{Style{PrintStyle::NET}, Format{false, 0, 0}};
            case PrintStyle::MIGRATE:
                return Context{Style{PrintStyle::MIGRATE}, Format{false, 0, 0}};
            default:
                return Context{Style{PrintStyle::NOTHING}, Format{false, 0, 0}};
        }
    }
};

struct Indent
{
    Indent(Context& ctx) : ctx_(ctx) { ctx_.format.increase_indentation(); }
    ~Indent() { ctx_.format.decrease_indentation(); }

    template <typename Stream>
    void write(Stream& output) const {
        output << (ctx_.format.is_indenting() ? std::string(ctx_.format.indentation_spaces(), ' ') : std::string(""));
    }

    template <typename Stream>
    friend Stream& operator<<(Stream& output, const Indent& indent) {
        indent.write(output);
        return output;
    }

private:
    Context& ctx_;
};

/* ************************************************************************** */
/* *** Writers ************************************************************** */
/* ************************************************************************** */

namespace implementation {

template <typename T, typename Stream>
struct Writer
{
};

/* ************************************************************************** */
/* *** Writers : Expression/AST ********************************************* */
/* ************************************************************************** */

namespace detail {

template <typename Stream, typename T>
bool write_ast_derived_type(Stream& output, const Ast* root, Context& ctx) {
    if (auto x = dynamic_cast<const T*>(root); x) {
        Writer<T, Stream>::write(output, *x, ctx);
        return true;
    }
    return false;
}

template <typename Stream, typename... T>
void write_ast_derived_types(Stream& output, const Ast* root, Context& ctx) {
    (write_ast_derived_type<Stream, T>(output, root, ctx) || ...);
}

template <typename Stream>
void write_ast_type(Stream& output, const Ast* root, Context& ctx) {
    write_ast_derived_types<Stream,
                            AstNot,
                            AstPlus,
                            AstMinus,
                            AstDivide,
                            AstMultiply,
                            AstModulo,
                            AstAnd,
                            AstOr,
                            AstEqual,
                            AstNotEqual,
                            AstLessEqual,
                            AstGreaterEqual,
                            AstGreaterThan,
                            AstLessThan,
                            AstFunction,
                            AstInteger,
                            AstInstant,
                            AstNodeState,
                            AstEventState,
                            AstNode,
                            AstFlag,
                            AstVariable,
                            AstParentVariable>(output, root, ctx);
}

} // namespace detail

template <typename Stream>
struct Writer<Ast, Stream>
{
    static void write(Stream& output, const Ast& item, Context& ctx) { detail::write_ast_type(output, &item, ctx); }
};

template <typename Stream>
struct Writer<AstTop, Stream>
{
    static void write(Stream& output, const AstTop& item, Context& ctx) {
        // taken from AstTop::print(std::string& os) const

        Indent l1(ctx);
        Indent l2(ctx);

        output << l2;

        output << "# Trigger Evaluation Tree\n";
        if (const auto* root = item.left(); root) {
            detail::write_ast_type<Stream>(output, root, ctx);
        }
    }
};

template <typename Stream>
struct Writer<AstRoot, Stream>
{
    static void write(Stream& output, const AstRoot& item, Context& ctx) {
        // taken from AstRoot::print(std::string& os) const

        if (const auto* left = item.left(); left) {
            detail::write_ast_type(output, left, ctx);
        }

        if (const auto* right = item.right(); right) { // right_ is empty for Not
            detail::write_ast_type(output, right, ctx);
        }
    }
};

template <typename Stream>
struct Writer<AstNot, Stream>
{
    static void write(Stream& output, const AstNot& item, Context& ctx) {
        // taken from AstNot::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";

        Writer<AstRoot, Stream>::write(output, item, ctx);
    }

    static void writeln(Stream& output, const AstNot& item) {
        output << "# NOT (";
        output << item.evaluate_str();
        output << ")";
        if (const auto* right = item.right(); right) {
            output << " # ERROR has right_";
        }
    }
};

template <typename Stream>
struct Writer<AstPlus, Stream>
{
    static void write(Stream& output, const AstPlus& item, Context& ctx) {
        // taken from AstPlus::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";

        Writer<AstRoot, Stream>::write(output, item, ctx);
    }

    static void writeln(Stream& output, const AstPlus& item) {
        output << "# PLUS value(";
        output << item.value();
        output << ")";
        if (const auto* left = item.left(); !left) {
            output << " # ERROR has no left_";
        }
        if (const auto* right = item.right(); !right) {
            output << " # ERROR has no right_";
        }
    }
};

template <typename Stream>
struct Writer<AstMinus, Stream>
{
    static void write(Stream& output, const AstMinus& item, Context& ctx) {
        // taken from AstMinus::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";

        Writer<AstRoot, Stream>::write(output, item, ctx);
    }

    static void writeln(Stream& output, const AstMinus& item) {
        output << "# MINUS value(";
        output << item.value();
        output << ")";
        if (const auto* left = item.left(); !left) {
            output << " # ERROR has no left_";
        }
        if (const auto* right = item.right(); !right) {
            output << " # ERROR has no right_";
        }
    }
};

template <typename Stream>
struct Writer<AstDivide, Stream>
{
    static void write(Stream& output, const AstDivide& item, Context& ctx) {
        // taken from AstDivide::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";

        Writer<AstRoot, Stream>::write(output, item, ctx);
    }

    static void writeln(Stream& output, const AstDivide& item) {
        output << "# DIVIDE value(";
        output << item.value();
        output << ")";
        if (const auto* left = item.left(); !left) {
            output << " # ERROR has no left_";
        }
        if (const auto* right = item.right(); !right) {
            output << " # ERROR has no right_";
        }
    }
};

template <typename Stream>
struct Writer<AstMultiply, Stream>
{
    static void write(Stream& output, const AstMultiply& item, Context& ctx) {
        // taken from AstMultiply::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";

        Writer<AstRoot, Stream>::write(output, item, ctx);
    }

    static void writeln(Stream& output, const AstMultiply& item) {
        output << "# MULTIPLY value(";
        output << item.value();
        output << ")";
        if (const auto* left = item.left(); !left) {
            output << " # ERROR has no left_";
        }
        if (const auto* right = item.right(); !right) {
            output << " # ERROR has no right_";
        }
    }
};

template <typename Stream>
struct Writer<AstModulo, Stream>
{
    static void write(Stream& output, const AstModulo& item, Context& ctx) {
        // taken from AstModulo::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";

        Writer<AstRoot, Stream>::write(output, item, ctx);
    }

    static void writeln(Stream& output, const AstModulo& item) {
        output << "# Modulo value(";
        output << item.value();
        output << ")";
        if (const auto* left = item.left(); !left) {
            output << " # ERROR has no left_";
        }
        if (const auto* right = item.right(); !right) {
            output << " # ERROR has no right_";
        }
    }
};

template <typename Stream>
struct Writer<AstAnd, Stream>
{
    static void write(Stream& output, const AstAnd& item, Context& ctx) {
        // taken from AstAnd::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";

        Writer<AstRoot, Stream>::write(output, item, ctx);
    }

    static void writeln(Stream& output, const AstAnd& item) {
        output << "# AND (";
        output << item.evaluate_str();
        output << ")";
        if (const auto* left = item.left(); !left) {
            output << " # ERROR has no left_";
        }
        if (const auto* right = item.right(); !right) {
            output << " # ERROR has no right_";
        }
    }
};

template <typename Stream>
struct Writer<AstOr, Stream>
{
    static void write(Stream& output, const AstOr& item, Context& ctx) {
        // taken from AstOr::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";

        Writer<AstRoot, Stream>::write(output, item, ctx);
    }

    static void writeln(Stream& output, const AstOr& item) {
        output << "# OR (";
        output << item.evaluate_str();
        output << ")";
        if (const auto* left_ = item.left(); !left_) {
            output << " # ERROR has no left_";
        }
        if (const auto* right_ = item.right(); !right_) {
            output << " # ERROR has no right_";
        }
    }
};

template <typename Stream>
struct Writer<AstEqual, Stream>
{
    static void write(Stream& output, const AstEqual& item, Context& ctx) {
        // taken from AstEqual::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";

        Writer<AstRoot, Stream>::write(output, item, ctx);
    }

    static void writeln(Stream& output, const AstEqual& item) {
        output << "# EQUAL (";
        output << item.evaluate_str();
        output << ")";
        if (const auto* left = item.left(); !left) {
            output << " # ERROR has no left_";
        }
        if (const auto* right = item.right(); !right) {
            output << " # ERROR has no right_";
        }
    }
};

template <typename Stream>
struct Writer<AstNotEqual, Stream>
{
    static void write(Stream& output, const AstNotEqual& item, Context& ctx) {
        // taken from AstNotEqual::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";

        Writer<AstRoot, Stream>::write(output, item, ctx);
    }

    static void writeln(Stream& output, const AstNotEqual& item) {
        output << "# NOT_EQUAL (";
        output << item.evaluate_str();
        output << ")";
        if (const auto* left = item.left(); !left) {
            output << " # ERROR has no left_";
        }
        if (const auto* right = item.right(); !right) {
            output << " # ERROR has no right_";
        }
    }
};

template <typename Stream>
struct Writer<AstLessEqual, Stream>
{
    static void write(Stream& output, const AstLessEqual& item, Context& ctx) {
        // taken from AstLessEqual::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";

        Writer<AstRoot, Stream>::write(output, item, ctx);
    }

    static void writeln(Stream& output, const AstLessEqual& item) {
        output << "# LESS_EQUAL (";
        output << item.evaluate_str();
        output << ")";
        if (const auto* left_ = item.left(); !left_) {
            output << " # ERROR has no left_";
        }

        if (const auto* right_ = item.right(); !right_) {
            output << " # ERROR has no right_";
        }
    }
};

template <typename Stream>
struct Writer<AstGreaterEqual, Stream>
{
    static void write(Stream& output, const AstGreaterEqual& item, Context& ctx) {
        // taken from AstGreaterEqual::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";

        Writer<AstRoot, Stream>::write(output, item, ctx);
    }

    static void writeln(Stream& output, const AstGreaterEqual& item) {
        output << "# GREATER_EQUAL (";
        output << item.evaluate_str();
        output << ")";
        if (const auto* left = item.left(); !left) {
            output << " # ERROR has no left_";
        }
        if (const auto* right = item.right(); !right) {
            output << " # ERROR has no right_";
        }
    }
};

template <typename Stream>
struct Writer<AstGreaterThan, Stream>
{
    static void write(Stream& output, const AstGreaterThan& item, Context& ctx) {
        // taken from AstGreaterThan::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";

        Writer<AstRoot, Stream>::write(output, item, ctx);
    }

    static void writeln(Stream& output, const AstGreaterThan& item) {
        output << "# GREATER_THAN (";
        output << item.evaluate_str();
        output << ")";
        if (const auto* left = item.left(); !left) {
            output << " # ERROR has no left_";
        }
        if (const auto* right = item.right(); !right) {
            output << " # ERROR has no right_";
        }
    }
};

template <typename Stream>
struct Writer<AstLessThan, Stream>
{
    static void write(Stream& output, const AstLessThan& item, Context& ctx) {
        // taken from AstLessThan::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";

        Writer<AstRoot, Stream>::write(output, item, ctx);
    }

    static void writeln(Stream& output, const AstLessThan& item) {
        output << "# LESS_THAN (";
        output << item.evaluate_str();
        output << ")";
        if (const auto* left = item.left(); !left) {
            output << " # ERROR has no left_";
        }
        if (const auto* right = item.right(); !right) {
            output << " # ERROR has no right_";
        }
    }
};

template <typename Stream>
struct Writer<AstFunction, Stream>
{
    static void write(Stream& output, const AstFunction& item, Context& ctx) {
        // taken from AstFunction::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";
    }

    static void writeln(Stream& output, const AstFunction& item) {
        switch (item.ft()) {
            case AstFunction::DATE_TO_JULIAN:
                output << "# DATE_TO_JULIAN ";
                output << item.value();
                break;
            case AstFunction::JULIAN_TO_DATE:
                output << "# JULIAN_TO_DATE ";
                output << item.value();
                break;
            default:
                assert(false);
        }
    }
};

template <typename Stream>
struct Writer<AstInteger, Stream>
{
    static void write(Stream& output, const AstInteger& item, Context& ctx) {
        // taken from AstInteger::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";
    }

    static void writeln(Stream& output, const AstInteger& item) {
        output << "# INTEGER ";
        output << item.value();
    }
};

template <typename Stream>
struct Writer<AstInstant, Stream>
{
    static void write(Stream& output, const AstInstant& item, Context& ctx) {
        // taken from AstInstant::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";
    }

    static void writeln(Stream& output, const AstInstant& item) {
        output << "# Instant ";
        output << item.value();
    }
};

template <typename Stream>
struct Writer<AstNodeState, Stream>
{
    static void write(Stream& output, const AstNodeState& item, Context& ctx) {
        // taken from AstNodeState::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";
    }

    static void writeln(Stream& output, const AstNodeState& item) {
        output << "# NODE_STATE ";
        output << DState::toString(item.state());
        output << "(";
        output << item.value();
        output << ")";
    }
};

template <typename Stream>
struct Writer<AstEventState, Stream>
{
    static void write(Stream& output, const AstEventState& item, Context& ctx) {
        // taken from AstEventState::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";
    }

    static void writeln(Stream& output, const AstEventState& item) {
        output << "# EVENT_STATE ";
        output << item.value();
    }
};

template <typename Stream>
struct Writer<AstNode, Stream>
{
    static void write(Stream& output, const AstNode& item, Context& ctx) {
        // taken from AstNode::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";
    }

    static void writeln(Stream& output, const AstNode& item) {
        if (const Node* refNode = item.referencedNode(); refNode) {
            output << "# NODE ";
            output << item.nodePath();
            output << " ";
            output << DState::toString(refNode->dstate());
            output << "(";
            output << static_cast<int>(refNode->dstate());
            output << ")";
        }
        else {
            output << "# NODE node(?not-found?) ";
            output << item.nodePath();
            output << " ";
            output << DState::toString(DState::UNKNOWN);
            output << "(";
            output << static_cast<int>(DState::UNKNOWN);
            output << ") # check suite filter";
        }
    }
};

template <typename Stream>
struct Writer<AstFlag, Stream>
{
    static void write(Stream& output, const AstFlag& item, Context& ctx) {
        // taken from AstFlag::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";
    }

    static void writeln(Stream& output, const AstFlag& item) {
        if (const Node* refNode = item.referencedNode(); refNode) {
            output << "# FLAG_NODE ";
            output << item.nodePath();
            output << " ";
            output << ecf::Flag::enum_to_string(item.flag());
            output << "(";
            output << static_cast<int>(refNode->get_flag().is_set(item.flag()));
            output << ")";
        }
        else {
            output << "# FLAG_NODE node(?not-found?) ";
            output << item.nodePath();
            output << " ";
            output << ecf::Flag::enum_to_string(item.flag());
            output << "(0) # check suite filter";
        }
    }
};

template <typename Stream>
struct Writer<AstVariable, Stream>
{
    static void write(Stream& output, const AstVariable& item, Context& ctx) {
        // taken from AstVariable::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";
    }

    static void writeln(Stream& output, const AstVariable& item) {
        output << "# ";
        output << item.nodePath();
        output << Str::COLON();
        output << item.name();

        std::string error;
        if (const auto* refNode = item.referencedNode(error); refNode) {
            output << " node(";
            output << refNode->name();
            output << ") ";
            std::ostringstream os;
            refNode->findExprVariableAndPrint(item.name(), os);
            output << os.str();
        }
        else {
            output << " node(?not-found?) ";
            output << item.nodePath();
            output << " value(0) # check suite filter";
        }
    }
};

template <typename Stream>
struct Writer<AstParentVariable, Stream>
{
    static void write(Stream& output, const AstParentVariable& item, Context& ctx) {
        // taken from AstParentVariable::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";
    }

    static void writeln(Stream& output, const AstParentVariable& item) {
        output << "# ";
        output << Str::COLON();
        output << item.name();

        if (const auto* ref_node = item.find_node_which_references_variable(); ref_node) {
            output << " node(";
            output << ref_node->name();
            output << ") ";
            std::ostringstream os;
            ref_node->findExprVariableAndPrint(item.name(), os);
            output << os.str();
            output << "\n";
            return;
        }
        output << " node(?not-found?) value(0)";
        output << " # check suite filter";
    }
};

template <typename Stream>
struct Writer<PartExpression, Stream>
{
    static void
    write(Stream& output, const PartExpression& item, Context& ctx, const std::string& expression_type, bool is_free) {
        // taken from PartExpression::print(std::string& os, const std::string& exprType, bool isFree) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item, ctx, expression_type, is_free);

        output << "\n";
    }

    static void writeln(Stream& output,
                        const PartExpression& item,
                        Context& ctx,
                        const std::string& expression_type,
                        bool is_free) {
        output << expression_type;
        switch (item.expr_type()) {
            case PartExpression::FIRST:
                output << " ";
                break;
            case PartExpression::AND:
                output << " -a ";
                break;
            case PartExpression::OR:
                output << " -o ";
                break;
            default:
                assert(false);
                break;
        }
        output << item.expression();

        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            if (item.expr_type() == PartExpression::FIRST) {
                if (is_free)
                    output << " # free";
            }
        }
    }
};

template <typename Stream>
struct Writer<Expression, Stream>
{
    static void write(Stream& output, const Expression& item, Context& ctx, const std::string& expression_type) {
        // taken from Expression::print(std::string& os, const std::string& exprType) const

        for (const auto& part : item.expr()) {
            Writer<PartExpression, Stream>::write(output, part, ctx, expression_type, item.isFree());
        }
    }
};

/* ************************************************************************** */
/* *** Writer : Attributes ************************************************** */
/* ************************************************************************** */

template <typename Stream>
struct Writer<AutoArchiveAttr, Stream>
{
    static void write(Stream& output, const AutoArchiveAttr& item, Context& ctx) {
        // taken from AutoArchiveAttr::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";
    }

    static void writeln(Stream& output, const AutoArchiveAttr& item) { item.write(output.buf); }
};

template <typename Stream>
struct Writer<AutoCancelAttr, Stream>
{
    static void write(Stream& output, const AutoCancelAttr& item, Context& ctx) {
        // taken from AutoCancelAttr::print(std::string& os) const

        Indent l1(ctx);
        output << l1;

        writeln(output, item);

        output << "\n";
    }

    static void writeln(Stream& output, const AutoCancelAttr& item) { item.write(output.buf); }
};

template <typename Stream>
struct Writer<AutoRestoreAttr, Stream>
{
    static void write(Stream& output, const AutoRestoreAttr& item, Context& ctx) {
        // taken from AutoRestoreAttr::print(std::string& os) const

        Indent l1(ctx);
        output << l1;

        writeln(output, item);

        output << "\n";
    }

    static void writeln(Stream& output, const AutoRestoreAttr& item) { item.write(output.buf); }
};

template <typename Stream>
struct Writer<AvisoAttr, Stream>
{
    static void write(Stream& output, const AvisoAttr& item, Context& ctx) {
        // taken from AvisoAttr::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        ecf::format_as_defs(item, output);

        output << "\n";
    }

    static void writeln(Stream& output, const AvisoAttr& item) { ecf::format_as_defs(item, output); }
};

template <typename Stream>
struct Writer<ClockAttr, Stream>
{
    static void write(Stream& output, const ClockAttr& item, Context& ctx) {
        // taken from ClockAttr::print(std::string& os) const

        Indent l1(ctx);
        output << l1;

        writeln(output, item);

        output << "\n";
    }

    static void writeln(Stream& output, const ClockAttr& item) { item.write(output.buf); }
};

template <typename Stream>
struct Writer<CronAttr, Stream>
{
    static void write(Stream& output, const CronAttr& item, Context& ctx) {
        // taken from CronAttr::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item, ctx);

        output << "\n";
    }

    static void writeln(Stream& output, const CronAttr& item, Context& ctx) {
        item.write(output.buf);
        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            item.time_series().write_state(output.buf, item.isSetFree());
        }
    }
};

template <typename Stream>
struct Writer<DateAttr, Stream>
{
    static void write(Stream& output, const DateAttr& item, Context& ctx) {
        // taken from DayAttr::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item, ctx);

        output << "\n";
    }

    static void writeln(Stream& output, const DateAttr& item, Context& ctx) {
        item.write(output.buf);
        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            if (item.isSetFree()) {
                output << " # free";
            }
        }
    }
};

template <typename Stream>
struct Writer<DayAttr, Stream>
{
    static void write(Stream& output, const DayAttr& item, Context& ctx) {
        // taken from DayAttr::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item, ctx);

        output << "\n";
    }

    static void writeln(Stream& output, const DayAttr& item, Context& ctx) {
        item.write(output.buf);
        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            bool added_hash = false;
            if (item.isSetFree()) {
                output << " # free";
                added_hash = true;
            }
            if (item.expired()) {
                if (added_hash)
                    output << " expired";
                else
                    output << " # expired";
                added_hash = true;
            }

            if (!added_hash) {
                output << " #";
            }
            output << " date:";
            output << item.as_simple_string();
        }
    }
};

template <typename Stream>
struct Writer<DState::State, Stream>
{
    static void write(Stream& output, const DState::State& item, Context& ctx) {

        if (item != DState::default_state()) {
            Indent l1(ctx);

            output << l1;

            writeln(output, item);

            output << "\n";
        }
    }

    static void writeln(Stream& output, const DState::State& item) {
        output << "defstatus ";
        output << DState::toString(item);
    }
};

template <typename Stream>
struct Writer<Event, Stream>
{
    static void write(Stream& output, const Event& item, Context& ctx) {
        // taken from Event::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item, ctx);

        output << "\n";
    }

    static void writeln(Stream& output, const Event& item, Context& ctx) {
        item.write(output.buf);
        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            if (item.initial_value() != item.value()) { // initial value and value differ
                output << " # ";
                if (item.value()) {
                    output << Event::SET();
                }
                else {
                    output << Event::CLEAR();
                }
            }
        }
    }
};

template <typename Stream>
struct Writer<GenericAttr, Stream>
{
    static void write(Stream& output, const GenericAttr& item, Context& ctx) {
        // taken from GenericAttr::print(std::string& os) const

        Indent l1(ctx);
        output << l1;

        writeln(output, item);

        output << "\n";
    }

    static void writeln(Stream& output, const GenericAttr& item) { item.write(output.buf); }
};

template <typename Stream>
struct Writer<InLimit, Stream>
{
    static void write(Stream& output, const InLimit& item, Context& ctx) {
        // taken from InLimit::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item, ctx);

        output << "\n";
    }

    static void writeln(Stream& output, const InLimit& item, const Context& ctx) {
        item.write(output.buf);
        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {

            // write state; See InlimitParser::doParse for read state part
            if (item.incremented())
                output << " # incremented:1";

            if (ctx.style.is_one_of<PrintStyle::STATE>()) {
                Limit* the_limit = item.limit();
                if (the_limit) {
                    output << " # referenced limit(value) ";
                    output << ecf::convert_to<std::string>(the_limit->theLimit());
                    output << "(";
                    output << ecf::convert_to<std::string>(the_limit->value());
                    output << ")";
                }
            }
        }
    }
};

template <typename Stream>
struct Writer<Label, Stream>
{
    static void write(Stream& output, const Label& item, Context& ctx) {
        // taken from Meter::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item, ctx);

        output << "\n";
    }

    static void writeln(Stream& output, const Label& item, const Context& ctx) {
        item.write(output.buf);
        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            if (!item.new_value().empty()) {
                if (item.new_value().find("\n") == std::string::npos) {
                    output << " # \"";
                    output << item.new_value();
                    output << "\"";
                }
                else {
                    std::string value = item.new_value();
                    Str::replaceall(value, "\n", "\\n");
                    output << " # \"";
                    output << value;
                    output << "\"";
                }
            }
        }
    }
};

template <typename Stream>
struct Writer<LateAttr, Stream>
{
    static void write(Stream& output, const LateAttr& item, Context& ctx) {
        // taken from LateAttr::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item, ctx);

        output << "\n";
    }

    static void writeln(Stream& output, const LateAttr& item, const Context& ctx) {
        item.write(output.buf);
        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            if (item.isLate()) {
                output << " # late";
            }
        }
    }
};

template <typename Stream>
struct Writer<Limit, Stream>
{
    static void write(Stream& output, const Limit& item, Context& ctx) {
        // taken from Limit::print(std::string& os) const

        Indent l1(ctx);
        output << l1;

        writeln(output, item, ctx);

        output << "\n";
    }

    static void writeln(Stream& output, const Limit& item, const Context& ctx) {
        item.write(output.buf);

        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            if (item.value() != 0) {
                output << " # ";
                output << ecf::convert_to<std::string>(item.value());
                for (const auto& path : item.paths()) {
                    output << " ";
                    output << path;
                }
            }
        }
    }
};

template <typename Stream>
struct Writer<Meter, Stream>
{
    static void write(Stream& output, const Meter& item, Context& ctx) {
        // taken from Meter::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item, ctx);

        output << "\n";
    }

    static void writeln(Stream& output, const Meter& item, const Context& ctx) {
        item.write(output.buf);
        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            if (item.value() != item.min()) {
                output << " # ";
                output << ecf::convert_to<std::string>(item.value());
            }
        }
    }
};

template <typename Stream>
struct Writer<MirrorAttr, Stream>
{
    static void write(Stream& output, const MirrorAttr& item, Context& ctx) {
        // taken from MirrorAttr::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";
    }

    static void writeln(Stream& output, const MirrorAttr& item) { ecf::format_as_defs(item, output); }
};

template <typename Stream>
struct Writer<QueueAttr, Stream>
{
    static void write(Stream& output, const QueueAttr& item, Context& ctx) {
        // taken from QueueAttr::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item, ctx);

        output << "\n";
    }

    static void writeln(Stream& output, const QueueAttr& item, const Context& ctx) {
        item.write(output.buf);
        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            output << " # ";
            output << ecf::convert_to<std::string>(item.index());
            for (auto i : item.state_vec()) {
                output << " ";
                output << NState::toString(i);
            }
        }
    }
};

template <typename Stream>
struct Writer<RepeatInteger, Stream>
{
    static void write(Stream& output, const RepeatInteger& item, Context& ctx) {
        // taken from RepeatInteger::write(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item, ctx);

        output << "\n";
    }

    static void writeln(Stream& output, const RepeatInteger& item, const Context& ctx) {
        output << "repeat integer ";
        output << item.name();
        output << " ";
        output << ecf::convert_to<std::string>(item.start());
        output << " ";
        output << ecf::convert_to<std::string>(item.end());
        if (auto dt = item.delta(); dt != 1) {
            output << " ";
            output << ecf::convert_to<std::string>(dt);
        }

        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>() && (item.value() != item.start())) {
            output << " # ";
            output << ecf::convert_to<std::string>(item.value());
        }
    }
};

template <typename Stream>
struct Writer<RepeatDate, Stream>
{
    static void write(Stream& output, const RepeatDate& item, Context& ctx) {
        // taken from RepeatDate::write(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item, ctx);

        output << "\n";
    }

    static void writeln(Stream& output, const RepeatDate& item, const Context& ctx) {
        output << "repeat date ";
        output << item.name();
        output << " ";
        output << ecf::convert_to<std::string>(item.start());
        output << " ";
        output << ecf::convert_to<std::string>(item.end());
        output << " ";
        output << ecf::convert_to<std::string>(item.delta());

        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>() && (item.value() != item.start())) {
            output << " # ";
            output << ecf::convert_to<std::string>(item.value());
        }
    }
};

template <typename Stream>
struct Writer<RepeatDateList, Stream>
{
    static void write(Stream& output, const RepeatDateList& item, Context& ctx) {
        // taken from RepeatDateList::write(std::string& os) const

        Indent l1(ctx);
        output << l1;

        writeln(output, item, ctx);

        output << "\n";
    }

    static void writeln(Stream& output, const RepeatDateList& item, const Context& ctx) {
        output << "repeat datelist ";
        output << item.name();
        for (auto date : item.values()) {
            output << " \"";
            output << ecf::convert_to<std::string>(date);
            output << "\"";
        }
        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>() && (item.index_or_value() != 0)) {
            output << " # ";
            output << ecf::convert_to<std::string>(item.index_or_value());
        }
    }
};

template <typename Stream>
struct Writer<RepeatDateTime, Stream>
{
    static void write(Stream& output, const RepeatDateTime& item, Context& ctx) {
        // taken from RepeatDateTime::write(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item, ctx);

        output << "\n";
    }

    static void writeln(Stream& output, const RepeatDateTime& item, Context& ctx) {
        output << "repeat datetime ";
        output << item.name();
        output << " ";
        output << boost::lexical_cast<std::string>(item.start_instant());
        output << " ";
        output << boost::lexical_cast<std::string>(item.end_instant());
        output << " ";
        output << boost::lexical_cast<std::string>(item.step_duration());

        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>() &&
            (item.value_instant() != item.start_instant())) {
            output << " # ";
            output << boost::lexical_cast<std::string>(item.value_instant());
        }
    }
};

template <typename Stream>
struct Writer<RepeatEnumerated, Stream>
{
    /* RepeatEnumerated */
    static void write(Stream& output, const RepeatEnumerated& item, Context& ctx) {
        // taken from RepeatEnumerated::write(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item, ctx);

        output << "\n";
    }

    static void writeln(Stream& output, const RepeatEnumerated& item, const Context& ctx) {
        output << "repeat enumerated ";
        output << item.name();
        for (const auto& value : item.values()) {
            output << " \"";
            output << value;
            output << "\"";
        }

        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>() && (item.index_or_value() != 0)) {
            output << " # ";
            output << ecf::convert_to<std::string>(item.index_or_value());
        }
    }
};

template <typename Stream>
struct Writer<RepeatString, Stream>
{
    static void write(Stream& output, const RepeatString& item, Context& ctx) {
        // taken from RepeatString::write(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item, ctx);

        output << "\n";
    }

    static void writeln(Stream& output, const RepeatString& item, const Context& ctx) {
        output << "repeat string ";
        output << item.name();
        for (const std::string& s : item.values()) {
            output << " \"";
            output << s;
            output << "\"";
        }

        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>() && (item.value() != item.start())) {
            output << " # ";
            output << ecf::convert_to<std::string>(item.value());
        }
    }
};

template <typename Stream>
struct Writer<RepeatDay, Stream>
{
    static void write(Stream& output, const RepeatDay& item, Context& ctx) {
        // taken from RepeatDay::write(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";
    }

    static void writeln(Stream& output, const RepeatDay& item) {
        output << "repeat day ";
        output << ecf::convert_to<std::string>(item.step());
    }
};

template <typename Stream>
struct Writer<RepeatBase, Stream>
{
    static void write(Stream& output, const RepeatBase& item, Context& ctx) {
        // taken from Repeat::print(std::string& os) const

        if (auto r = dynamic_cast<const RepeatInteger*>(&item)) {
            Writer<RepeatInteger, Stream>::write(output, *r, ctx);
        }
        else if (auto r = dynamic_cast<const RepeatDate*>(&item)) {
            Writer<RepeatDate, Stream>::write(output, *r, ctx);
        }
        else if (auto r = dynamic_cast<const RepeatDateList*>(&item)) {
            Writer<RepeatDateList, Stream>::write(output, *r, ctx);
        }
        else if (auto r = dynamic_cast<const RepeatDateTime*>(&item)) {
            Writer<RepeatDateTime, Stream>::write(output, *r, ctx);
        }
        else if (auto r = dynamic_cast<const RepeatEnumerated*>(&item)) {
            Writer<RepeatEnumerated, Stream>::write(output, *r, ctx);
        }
        else if (auto r = dynamic_cast<const RepeatString*>(&item)) {
            Writer<RepeatString, Stream>::write(output, *r, ctx);
        }
        else if (auto r = dynamic_cast<const RepeatDay*>(&item)) {
            Writer<RepeatDay, Stream>::write(output, *r, ctx);
        }
        else {
            assert(false);
        }
    }
};

template <typename Stream>
struct Writer<Repeat, Stream>
{
    static void write(Stream& output, const Repeat& item, Context& ctx) {
        // taken from Repeat::print(std::string& os) const

        const RepeatBase* base = item.repeatBase();
        if (base) {
            Writer<RepeatBase, Stream>::write(output, *base, ctx);
        }
    }
};

template <typename Stream>
struct Writer<TimeAttr, Stream>
{
    static void write(Stream& output, const TimeAttr& item, Context& ctx) {
        // taken from TimeAttr::print(std::string& os) const

        Indent l1(ctx);
        output << l1;

        writeln(output, item, ctx);

        output << "\n";
    }

    static void writeln(Stream& output, const TimeAttr& item, const Context& ctx) {
        item.write(output.buf);
        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            item.time_series().write_state(output.buf, item.isSetFree());
        }
    }
};

template <typename Stream>
struct Writer<TodayAttr, Stream>
{
    static void write(Stream& output, const TodayAttr& item, Context& ctx) {
        // taken from TodayAttr::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item, ctx);

        output << "\n";
    }

    static void writeln(Stream& output, const TodayAttr& item, const Context& ctx) {
        item.write(output.buf);
        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            item.time_series().write_state(output.buf, item.isSetFree());
        }
    }
};

template <typename Stream>
struct Writer<Variable, Stream>
{
    // The suffix is used to distinguish between:
    //  - a normal variable ( suffix = "" )
    //  - a generated variable ( suffix = "#" )
    //  - a server variable ( suffix = "# server" )

    inline static std::string empty                  = "";
    inline static std::string generated_prefix       = "# ";
    inline static std::string server_variable_suffix = " # server";

    static void write(Stream& output,
                      const Variable& item,
                      Context& ctx,
                      const std::string& prefix = empty,
                      const std::string& suffix = empty) {
        // taken from Variable::print(std::string& os) const
        //   and from Variable::print_generated(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item, prefix, suffix);

        output << "\n";
    }

    static void writeln(Stream& output,
                        const Variable& item,
                        const std::string& prefix = empty,
                        const std::string& suffix = empty) {
        output << prefix;
        item.write(output.buf);
        output << suffix;
    }
};

template <typename Stream>
struct Writer<VerifyAttr, Stream>
{
    static void write(Stream& output, const VerifyAttr& item, Context& ctx) {
        // taken from VerifyAttr::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item, ctx);

        output << "\n";
    }

    static void writeln(Stream& output, const VerifyAttr& item, const Context& ctx) {
        output << item.toString();
        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            output << " # ";
            output << ecf::convert_to<std::string>(item.actual());
        }
    }
};

template <typename Stream>
struct Writer<ZombieAttr, Stream>
{
    static void write(Stream& output, const ZombieAttr& item, Context& ctx) {
        // taken from ZombieAttr::print(std::string& os) const

        Indent l1(ctx);

        output << l1;

        writeln(output, item);

        output << "\n";
    }

    static void writeln(Stream& output, const ZombieAttr& item) { item.write(output.buf); }
};

/* ************************************************************************** */
/* *** Writer : Nodes ******************************************************* */
/* ************************************************************************** */

template <typename Stream>
struct Writer<Alias, Stream>
{
    static void write(Stream& output, const Alias& item, Context& ctx) {
        // taken from Alias::print(std::string& os) const

        Indent l1(ctx);

        // Write the alias header
        output << l1;

        output << "alias ";
        output << item.name();
        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            bool added_comment_char = false;
            item.write_state(output.buf, added_comment_char);
        }

        output << "\n";

        // Write the node information
        Writer<Node, Stream>::write(output, item, ctx);
    }
};

template <typename Stream>
struct Writer<Family, Stream>
{
    static void write(Stream& output, const Family& item, Context& ctx) {
        // taken from Family::print(std::string& os) const

        Indent l1(ctx);

        // Write the family header
        output << l1;

        output << "family ";
        output << item.name();
        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            bool added_comment_char = false;
            item.write_state(output.buf, added_comment_char);
        }

        output << "\n";

        // Write the node information
        Writer<Node, Stream>::write(output, item, ctx);

        // Write the children
        Writer<NodeContainer, Stream>::write(output, item, ctx);

        // Write footer
        l1.write(output);
        output << "endfamily\n";
    }
};

template <typename Stream>
struct Writer<Task, Stream>
{
    static void write(Stream& output, const Task& item, Context& ctx) {
        // taken from Task::print(std::string& os) const

        Indent l1(ctx);

        // Write the task header
        output << l1;

        output << "task ";
        output << item.name();
        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            bool added_comment_char = false;
            item.write_state(output.buf, added_comment_char);
        }

        output << "\n";

        // Write the node information
        Writer<Node, Stream>::write(output, item, ctx);

        // Although Aliases are not printed, they can part of _checkpoint_ files.
        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            Indent l2(ctx);
            for (auto alias : item.aliases()) {
                Writer<Alias, Stream>::write(output, *alias, ctx);
            }
            Indent l3(ctx);
            if (!item.aliases().empty()) {
                l3.write(output);
                output << "endalias";
                output << "\n";
            }
        }
    }
};

template <typename Stream>
struct Writer<const Node*, Stream>
{
    static void write(Stream& output, const Node* item, Context& ctx) {

        // Note: here we must use dynamic_cast to determine the type of the node
        // This is because we are using a templated Writer, and we need to call the correct
        // Writer specialisation based on the actual type of the node.

        if (auto n = dynamic_cast<const Alias*>(item)) {
            Writer<Alias, Stream>::write(output, *n, ctx);
        }
        else if (auto n = dynamic_cast<const Family*>(item)) {
            Writer<Family, Stream>::write(output, *n, ctx);
        }
        else if (auto n = dynamic_cast<const Task*>(item)) {
            Writer<Task, Stream>::write(output, *n, ctx);
        }
        else if (auto n = dynamic_cast<const Suite*>(item)) {
            Writer<Suite, Stream>::write(output, *n, ctx);
        }
        else {
            assert(false && "Unknown Node type");
        }
    }
};

template <typename Stream>
struct Writer<Node*, Stream>
{
    static void write(Stream& output, Node* item, Context& ctx) {
        return Writer<const Node*, Stream>::write(output, item, ctx);
    }
};

template <typename Stream>
struct Writer<std::shared_ptr<Node>, Stream>
{
    static void write(Stream& output, const std::shared_ptr<Node>& item, Context& ctx) {
        return Writer<const Node*, Stream>::write(output, item.get(), ctx);
    }
};

template <typename Stream>
struct Writer<Node, Stream>
{
    static void write(Stream& output, const Node& item, Context& ctx) {
        // taken from Node::print(std::string& os) const

        Writer<DState::State, Stream>::write(output, item.defStatus(), ctx);
        // Notice: if item.defStatus() != DState::default_state(), the above doesn't generate any output line

        if (auto late = item.get_late(); late) {
            Writer<LateAttr, Stream>::write(output, *late, ctx);
        }

        if (const auto* c_expr = item.get_complete(); c_expr) {
            Writer<Expression, Stream>::write(output, *c_expr, ctx, "complete");
            if (ctx.style.is_one_of<PrintStyle::STATE>()) {
                if (c_expr->isFree()) {
                    Indent l1(ctx);
                    l1.write(output);
                    output << "# (free)\n";
                }
                if (const auto* ast = item.completeAst(); ast) {
                    if (!item.defs()) {
                        // Full defs is required for extern checking, and finding absolute node paths
                        // Hence print will with no defs can give inaccurate information
                        Indent l2(ctx);
                        l2.write(output);
                        output << "# Warning: Full/correct AST evaluation requires the definition\n";
                    }
                    Writer<AstTop, Stream>::write(output, *ast, ctx);
                }
            }
        }
        if (const auto* t_expr_ = item.get_trigger(); t_expr_) {
            Writer<Expression, Stream>::write(output, *t_expr_, ctx, "trigger");
            if (ctx.style.is_one_of<PrintStyle::STATE>()) {
                if (t_expr_->isFree()) {
                    Indent l1(ctx);
                    l1.write(output);
                    output << "# (free)\n";
                }
                if (const auto* ast = item.triggerAst(); ast) {
                    if (!item.defs()) {
                        Indent l1(ctx);
                        l1.write(output);
                        output << "# Warning: Full/correct AST evaluation requires the definition\n";
                    }
                    Writer<AstTop, Stream>::write(output, *ast, ctx);
                }
            }
        }

        Writer<Repeat, Stream>::write(output, item.repeat(), ctx);
        // Notice: if repeat.empty(), the above doesn't generate any output line

        for (const auto& v : item.variables()) {
            Writer<Variable, Stream>::write(output, v, ctx);
        }

        if (ctx.style.is_one_of<PrintStyle::STATE>()) {
            std::vector<Variable> gvec;
            item.gen_variables(gvec);
            for (const auto& v : gvec) {
                using writer_t = Writer<Variable, Stream>;
                writer_t::write(output, v, ctx, writer_t::generated_prefix);
            }
        }

        for (const auto& l : item.limits()) {
            Writer<Limit, Stream>::write(output, *l, ctx);
        }

        for (const auto& l : item.inlimits()) {
            Writer<InLimit, Stream>::write(output, l, ctx);
        }

        for (const auto& l : item.labels()) {
            Writer<Label, Stream>::write(output, l, ctx);
        }
        for (const auto& m : item.meters()) {
            Writer<Meter, Stream>::write(output, m, ctx);
        }
        for (const auto& e : item.events()) {
            Writer<Event, Stream>::write(output, e, ctx);
        }

        for (const auto& t : item.timeVec()) {
            Writer<TimeAttr, Stream>::write(output, t, ctx);
        }
        for (const auto& t : item.todayVec()) {
            Writer<TodayAttr, Stream>::write(output, t, ctx);
        }
        for (const auto& d : item.dates()) {
            Writer<DateAttr, Stream>::write(output, d, ctx);
        }
        for (const auto& d : item.days()) {
            Writer<DayAttr, Stream>::write(output, d, ctx);
        }
        for (const auto& c : item.crons()) {
            Writer<CronAttr, Stream>::write(output, c, ctx);
        }
        for (const auto& a : item.avisos()) {
            Writer<AvisoAttr, Stream>::write(output, a, ctx);
        }
        for (const auto& m : item.mirrors()) {
            Writer<MirrorAttr, Stream>::write(output, m, ctx);
        }

        if (auto a = item.get_autocancel(); a) {
            Writer<AutoCancelAttr, Stream>::write(output, *a, ctx);
        }
        if (auto a = item.get_autoarchive(); a) {
            Writer<AutoArchiveAttr, Stream>::write(output, *a, ctx);
        }
        if (auto a = item.get_autorestore(); a) {
            Writer<AutoRestoreAttr, Stream>::write(output, *a, ctx);
        }

        if (auto a = item.get_misc_attrs(); a) {
            for (const auto& zombie : a->zombies()) {
                Writer<ZombieAttr, Stream>::write(output, zombie, ctx);
            }
            for (const auto& verify : a->verifys()) {
                Writer<VerifyAttr, Stream>::write(output, verify, ctx);
            }
            for (const auto& queue : a->queues()) {
                Writer<QueueAttr, Stream>::write(output, queue, ctx);
            }
            for (const auto& generic : a->generics()) {
                Writer<GenericAttr, Stream>::write(output, generic, ctx);
            }
        }
    }
};

template <typename Stream>
struct Writer<NodeContainer, Stream>
{
    static void write(Stream& output, const NodeContainer& item, Context& ctx) {
        // taken from NodeContainer::print(std::string& os) const

        for (const auto& node : item.nodeVec()) {

            // Note: here we must use dynamic_cast to determine the type of the node
            // This is because we are using a templated Writer, and we need to call the correct
            // Writer specialisation based on the actual type of the node.

            if (auto family = dynamic_cast<const Family*>(node.get()); family) {
                Writer<Family, Stream>::write(output, *family, ctx);
            }
            else if (auto task = dynamic_cast<const Task*>(node.get()); task) {
                Writer<Task, Stream>::write(output, *task, ctx);
            }
            else if (auto alias = dynamic_cast<const Alias*>(node.get()); alias) {
                Writer<Alias, Stream>::write(output, *alias, ctx);
            }
            else if (auto suite = dynamic_cast<const Suite*>(node.get()); suite) {
                Writer<Suite, Stream>::write(output, *suite, ctx);
            }
            else {
                assert(false); // Should never happen
            }
        }
    }
};

template <typename Stream>
struct Writer<Suite, Stream>
{
    static void write(Stream& output, const Suite& item, Context& ctx) {
        // taken from Suite::print(std::string& os) const

        // Write the suite header
        output << "suite ";
        output << item.name();
        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            bool added_comment_char = false;
            item.write_state(output.buf, added_comment_char);
        }

        output << "\n";

        // Write the node information
        Writer<Node, Stream>::write(output, item, ctx);

        // Write Clock attributes
        if (auto attr = item.clockAttr(); attr) {
            Writer<ClockAttr, Stream>::write(output, *attr, ctx);
        }
        if (auto attr = item.clock_end_attr(); attr) {
            Writer<ClockAttr, Stream>::write(output, *attr, ctx);
        }

        // Write Calendar attribute
        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            if (!item.calendar().is_special()) {
                Indent l2(ctx);
                l2.write(output);
                output << "calendar";
                item.calendar().write_state(output.buf);
                output << "\n";
            }
        }

        // Write the children
        Writer<NodeContainer, Stream>::write(output, item, ctx);

        // Write footer
        output << "endsuite";

        output << "\n";
    }
};

/* ************************************************************************** */
/* *** Writer : Defs ******************************************************** */
/* ************************************************************************** */

template <typename Stream>
struct Writer<Defs, Stream>
{
    static void write(Stream& output, const Defs& item, Context& ctx) {
        // taken from Defs::print(std::string& os) const

        output << "#";
        output << ecf::Version::full();
        output << "\n";

        if (ctx.style.is_not_one_of<PrintStyle::DEFS, PrintStyle::NOTHING>()) {
            Writer<Defs, Stream>::write_state(output, item, ctx);
        }

        // Write the server state
        if (ctx.style.is_one_of<PrintStyle::STATE>()) {
            output << "# server state: ";
            output << SState::to_string(item.server_state().get_state());
            output << "\n";
        }

        // Write externs
        if (ctx.style.is_not_one_of<PrintStyle::MIGRATE, PrintStyle::NET>()) {
            // We do NOT persist the externs in these cases (+matches boost serialisation)
            for (const auto& e : item.externs()) {
                output << "extern ";
                output << e;
                output << "\n";
            }
        }

        // Write the children (i.e. suites)
        for (const auto& s : item.suiteVec()) {
            Writer<Suite, Stream>::write(output, *s, ctx);
        }

        // Write footer
        output << "# enddef";
        output << "\n";
    }

private:
    static void write_state(Stream& output, const Defs& item, Context& ctx) {
        // taken from Defs::write_state(std::string& os) const

        // *IMPORTANT* we *CANT* use ';' character, since is used in the parser, when we have
        //             multiple statement on a single line i.e.
        //                 task a; task b;
        // *IMPORTANT* make sure name are unique, i.e. can't have state: and server_state:
        // Otherwise read_state() will mess up
        output << "defs_state ";
        output << PrintStyle::to_string(ctx.style.selected());

        if (item.state() != NState::UNKNOWN) {
            output << " state>:";
            output << NState::toString(item.state());
        } // make <state> is unique
        if (auto flag = item.get_flag(); flag.flag() != 0) {
            output << " flag:";
            flag.write(output.buf);
        }
        if (item.state_change_no() != 0) {
            output << " state_change:";
            output << ecf::convert_to<std::string>(item.state_change_no());
        }
        if (item.modify_change_no() != 0) {
            output << " modify_change:";
            output << ecf::convert_to<std::string>(item.modify_change_no());
        }
        if (item.server_state().get_state() != ServerState::default_state()) {
            output << " server_state:";
            output << SState::to_string(item.server_state().get_state());
        }

        // This only works when the full defs is requested, otherwise zero as defs is fabricated for handles
        output << " cal_count:";
        output << ecf::convert_to<std::string>(item.updateCalendarCount());
        output << "\n";

        // Write Server level variables, defined by the User
        for (const auto& variable : item.server_state().user_variables()) {
            using writer_t = Writer<Variable, Stream>;
            writer_t::write(output, variable, ctx);
        }

        // Write Server level variables
        for (const auto& variable : item.server_state().server_variables()) {
            using writer_t = Writer<Variable, Stream>;
            writer_t::write(output, variable, ctx, writer_t::empty, writer_t::server_variable_suffix);
        }

        // READ by Defs::read_history()
        // We need to define a separator for the message, will to allow it to be re-read
        // This separator cannot be :
        // ' ' space, used in the messages
        // %  Used in job submission
        // :  Used in time, and name (:ma0)
        // [] Used in time
        // integers used in the time.
        // -  Used in commands
        if (item.get_save_edit_history()) {

            for (const auto& i : item.get_edit_history()) {
                Indent l(ctx);
                l.write(output);

                output << "history ";
                output << i.first;
                output << " ";                                  // node path
                const std::vector<std::string>& vec = i.second; // list of requests
                for (const auto& c : vec) {

                    // We expect to output a single newline.
                    // If there are additional new lines (e.g. during alter change label/value the user added new
                    // lines), any future parsing attempt will fail.
                    // To avoid this, we replace any '\n' with '\\n' and ensure we only output a single '\n' at the end.
                    if (c.find("\n") == std::string::npos) {
                        output << "\b";
                        output << c;
                    }
                    else {
                        std::string h = c;
                        Str::replaceall(h, "\n", "\\n");
                        output << "\b";
                        output << h;
                    }
                }
                output << "\n";
            }
            item.save_edit_history(false);
        }
    }
};

} // namespace implementation

/* ************************************************************************** */
/* *** Write entry point **************************************************** */
/* ************************************************************************** */

template <typename T>
inline void write_t(std::string& buffer, const T& item, Context& ctx) {
    buffer.reserve(1024 * 4); // Should be using a sensible default size for the buffer
    stringstreambuf output{buffer};
    implementation::Writer<T, stringstreambuf>::write(output, item, ctx);
}

template <typename Stream, typename T>
inline void write_t(Stream& output, const T& item, Context& ctx) {
    std::string buffer;
    write_t(buffer, item, ctx);
    output << buffer;
}

template <typename T>
inline std::string as_string(const T& item, Context& ctx) {
    std::string buffer;
    write_t(buffer, item, ctx);
    return buffer;
}

/* ************************************************************************** */
/* *** Write entry point (with PrintStyle) ********************************** */
/* ************************************************************************** */

inline void write_t(std::string& buffer, const Expression& item, PrintStyle::Type_t style, const std::string& type) {
    buffer.reserve(1024 * 4); // Should be using a sensible default size for the buffer
    stringstreambuf output{buffer};
    auto ctx = Context::make_for(style);
    implementation::Writer<Expression, stringstreambuf>::write(output, item, ctx, type);
}

template <typename T>
inline void write_t(std::string& buffer, const T& item, PrintStyle::Type_t style) {
    buffer.reserve(1024 * 4); // Should be using a sensible default size for the buffer
    stringstreambuf output{buffer};
    auto ctx = Context::make_for(style);
    implementation::Writer<T, stringstreambuf>::write(output, item, ctx);
}

template <typename Stream, typename T>
inline void write_t(Stream& output, const T& item, PrintStyle::Type_t style) {
    std::string buffer;
    write_t(buffer, item, style);
    output << buffer;
}

template <typename T>
inline std::string as_string(const T& item, PrintStyle::Type_t style) {
    std::string buffer;
    write_t(buffer, item, style);
    return buffer;
}

} // namespace ecf

#endif /* ecflow_node_formatter_DefsWriter_HPP */
