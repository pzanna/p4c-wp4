/*
Copyright 2020 Paul Zanna.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "wp4-Model.h"
#include "wp4-Parser.h"
#include "wp4-Type.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"

namespace WP4 {

namespace {
class StateTranslationVisitor : public CodeGenInspector {
    bool hasDefault;
    P4::P4CoreLibrary& p4lib;
    const WP4ParserState* state;

    void emitCheckPacketLength(const IR::Expression* expr, const char * varname, unsigned width);
    void emitCheckPacketLength(const IR::Expression* expr)
    { emitCheckPacketLength(expr, nullptr, 0); }
    void emitCheckPacketLength(unsigned width)
    { emitCheckPacketLength(nullptr, nullptr, width); }
    void emitCheckPacketLength(const char * varname)
    { emitCheckPacketLength(nullptr, varname, 0); }

    void compileExtractField(const IR::Expression* expr, cstring name, unsigned alignment, WP4Type* type);
    void compileExtract(const IR::Expression* destination);
    void compileLookahead(const IR::Expression* destination);

 public:
    explicit StateTranslationVisitor(const WP4ParserState* state) :
            CodeGenInspector(state->parser->program->refMap, state->parser->program->typeMap),
            hasDefault(false), p4lib(P4::P4CoreLibrary::instance), state(state) {}
    bool preorder(const IR::ParserState* state) override;
    bool preorder(const IR::SelectCase* selectCase) override;
    bool preorder(const IR::SelectExpression* expression) override;
    bool preorder(const IR::Member* expression) override;
    bool preorder(const IR::MethodCallExpression* expression) override;
    bool preorder(const IR::MethodCallStatement* stat) override
    { visit(stat->methodCall); return false; }
    bool preorder(const IR::AssignmentStatement* stat) override;
};
}  // namespace

// if expr is nullprt, width is used instead
void StateTranslationVisitor::emitCheckPacketLength(const IR::Expression* expr, const char * varname, unsigned width) {
    auto program = state->parser->program;

    builder->emitIndent();
    if (expr != nullptr) {
        builder->appendFormat("if (%s < BYTES(%s + ", program->inPacketLengthVar.c_str(), program->offsetVar.c_str()); 
        visit(expr);
        builder->append(")) ");
    } else if (varname != nullptr) {
        builder->appendFormat("if (%s < BYTES(%s + %s)) ", program->inPacketLengthVar.c_str(), program->offsetVar.c_str(), varname);
    } else {
        builder->appendFormat("if (%s < BYTES(%s + %d)) ", program->inPacketLengthVar.c_str(), program->offsetVar.c_str(), width);
    }

    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("goto %s;", IR::ParserState::reject.c_str());
    builder->newline();
    builder->blockEnd(true);

    builder->emitIndent();
    builder->newline();
}

void StateTranslationVisitor::compileLookahead(const IR::Expression* destination) {
    builder->emitIndent();
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("%s_save = %s", state->parser->program->offsetVar.c_str(), state->parser->program->offsetVar.c_str());
    builder->endOfStatement(true);
    compileExtract(destination);
    builder->emitIndent();
    builder->appendFormat("%s = %s_save", state->parser->program->offsetVar.c_str(), state->parser->program->offsetVar.c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);
}

bool StateTranslationVisitor::preorder(const IR::AssignmentStatement* statement) {
    if (auto mce = statement->right->to<IR::MethodCallExpression>()) {
        auto mi = P4::MethodInstance::resolve(mce, state->parser->program->refMap, state->parser->program->typeMap);
        auto extMethod = mi->to<P4::ExternMethod>();
        if (extMethod == nullptr)
            BUG("Unhandled method %1%", mce);

        auto decl = extMethod->object;
        if (decl == state->parser->packet) {
            if (extMethod->method->name.name == p4lib.packetIn.lookahead.name) {
                compileLookahead(statement->left);
                return false;
            }
        }
        ::error("Unexpected method call in parser %1%", statement->right);
    }

    CodeGenInspector::visit(statement);
    return false;
}

bool StateTranslationVisitor::preorder(const IR::ParserState* parserState) {
    if (parserState->isBuiltin()) return false;

    builder->emitIndent();
    builder->append(parserState->name.name);
    builder->append(":");
    builder->spc();
    builder->blockStart();

    visit(parserState->components, "components");
    if (parserState->selectExpression == nullptr) {
        builder->emitIndent();
        builder->append("goto ");
        builder->append(IR::ParserState::reject);
        builder->endOfStatement(true);
    } else if (parserState->selectExpression->is<IR::SelectExpression>()) {
        visit(parserState->selectExpression);
    } else {
        // must be a PathExpression which is a state name
        if (!parserState->selectExpression->is<IR::PathExpression>())
            BUG("Expected a PathExpression, got a %1%", parserState->selectExpression);
        builder->emitIndent();
        builder->append("goto ");
        visit(parserState->selectExpression);
        builder->endOfStatement(true);
    }

    builder->blockEnd(true);
    return false;
}

bool StateTranslationVisitor::preorder(const IR::SelectExpression* expression) {
    hasDefault = false;
    if (expression->select->components.size() != 1) {
        // TODO: this does not handle correctly tuples
        ::error("%1%: only supporting a single argument for select", expression->select);
        return false;
    }
    builder->emitIndent();
    builder->append("switch (");
    visit(expression->select);
    builder->append(") ");
    builder->blockStart();

    for (auto e : expression->selectCases)
        visit(e);

    if (!hasDefault) {
        builder->emitIndent();
        builder->appendFormat("default: goto %s;", IR::ParserState::reject.c_str());
        builder->newline();
    }

    builder->blockEnd(true);
    return false;
}

bool StateTranslationVisitor::preorder(const IR::SelectCase* selectCase) {
    builder->emitIndent();
    if (selectCase->keyset->is<IR::DefaultExpression>()) {
        hasDefault = true;
        builder->append("default: ");
    } else {
        builder->append("case ");
        visit(selectCase->keyset);      //32w
        builder->append(": ");
    }
    builder->append("goto ");
    visit(selectCase->state);
    builder->endOfStatement(true);
    return false;
}

void StateTranslationVisitor::compileExtractField(const IR::Expression* expr, cstring field, unsigned alignment, WP4Type* type) {
    unsigned widthToExtract = dynamic_cast<IHasWidth*>(type)->widthInBits();
    auto program = state->parser->program;
    const char* prefix = nullptr;
    prefix = dynamic_cast<WP4ScalarType*>(type)->getSignAsString();

    unsigned lastBitIndex = widthToExtract + alignment - 1;
    unsigned lastWordIndex = lastBitIndex / 8;
    unsigned wordsToRead = lastWordIndex + 1;
    unsigned loadSize;

    const char* helper = nullptr;
    if (wordsToRead <= 1) {
        helper = "";
        loadSize = 8;
    } else if (widthToExtract <= 16)  {
        helper = "htons";
        loadSize = 16;
    } else if (widthToExtract <= 32) {
        helper = "htonl";
        loadSize = 32;
    } else {
        if (widthToExtract > 64) BUG("Unexpected width %d", widthToExtract);
        helper = "htonll";
        loadSize = 64;
    }

    if (strcmp(prefix,"u")) helper = "";    // Don't do hton if signed
    
    unsigned shift = alignment;
    builder->emitIndent();

    builder->appendFormat("memcpy(&");
    visit(expr);
    builder->appendFormat(".%s, %s + BYTES(%s), BYTES(%d));", field.c_str(), program->packetStartVar.c_str(), program->offsetVar.c_str(), loadSize);

    if (loadSize > 8){
        builder->newline();
        builder->emitIndent();
        visit(expr);
        builder->appendFormat(".%s = %s(", field.c_str(), helper);
        visit(expr);
        builder->appendFormat(".%s)", field.c_str());
        if (shift != 0)
            builder->appendFormat(" >> %d", shift);
        builder->appendFormat(";");
    }

    builder->appendFormat("     // ** %d - %d - %d - %s **", loadSize, alignment, widthToExtract, prefix);

    if (loadSize <= 8 && shift != 0) {
        builder->newline();
        builder->emitIndent();
        visit(expr);
        builder->appendFormat(".%s >>= %d;", field.c_str(), shift);
    }

    if (widthToExtract == 48){
        builder->newline();
        builder->emitIndent();
        visit(expr);
        builder->appendFormat(".%s >>= 16;", field.c_str());
    }

    if (widthToExtract != loadSize) {
        builder->newline();
        builder->emitIndent();
        visit(expr);
        builder->appendFormat(".%s &= WP4_MASK(", field.c_str());
        type->emit(builder);
        builder->appendFormat(", %d);", widthToExtract);
    }

    builder->newline();
    builder->emitIndent();
    builder->appendFormat("%s += %d;", program->offsetVar.c_str(), widthToExtract);

    /* Printk headers values that are 8 bytes or less */
    if (loadSize <= 8) {
        builder->newline();
        builder->emitIndent();
        builder->appendFormat("printk(\"** WP4: %s = %%d **\\n\", ", field.c_str());
        visit(expr);
        builder->appendFormat(".%s);", field.c_str());
    }

    //builder->endOfStatement(false);
    builder->newline();
    builder->newline();
}

void StateTranslationVisitor::compileExtract(const IR::Expression* destination) {
    auto type = state->parser->typeMap->getType(destination);
    auto ht = type->to<IR::Type_StructLike>();
    if (ht == nullptr) {
        ::error("Cannot extract to a non-struct type %1%", destination);
        return;
    }

    unsigned width = ht->width_bits();
    emitCheckPacketLength(width);

    unsigned alignment = 0;
    for (auto f : ht->fields) {
        auto ftype = state->parser->typeMap->getType(f);
        auto etype = WP4TypeFactory::instance->create(ftype);
        auto et = dynamic_cast<IHasWidth*>(etype);
        if (et == nullptr) {
            ::error("Only headers with fixed widths supported %1%", f);
            return;
        }
        compileExtractField(destination, f->name, alignment, etype);
        alignment += et->widthInBits();
        alignment %= 8;
    }

    if (ht->is<IR::Type_Header>()) {
        builder->emitIndent();
        visit(destination);
        builder->appendLine(".wp4_valid = 1;");
    }
}

bool StateTranslationVisitor::preorder(const IR::MethodCallExpression* expression) {
    builder->append("/* ");
    visit(expression->method);
    builder->append("(");
    bool first = true;
    for (auto a  : *expression->arguments) {
        if (!first)
            builder->append(", ");
        first = false;
        visit(a);
    }
    builder->append(")");
    builder->append("*/");
    builder->newline();

    auto mi = P4::MethodInstance::resolve(expression, state->parser->program->refMap, state->parser->program->typeMap);
    auto extMethod = mi->to<P4::ExternMethod>();
    if (extMethod != nullptr) {
        auto decl = extMethod->object;
        if (decl == state->parser->packet) {
            if (extMethod->method->name.name == p4lib.packetIn.extract.name) {
                if (expression->arguments->size() != 1) {
                    ::error("Variable-sized header fields not yet supported %1%", expression);
                    return false;
                }
                compileExtract(expression->arguments->at(0)->expression);
                return false;
            }
            BUG("Unhandled packet method %1%", expression->method);
            return false;
        }
    }

    ::error("Unexpected method call in parser %1%", expression);
    return false;
}

bool StateTranslationVisitor::preorder(const IR::Member* expression) {
    if (expression->expr->is<IR::PathExpression>()) {
        auto pe = expression->expr->to<IR::PathExpression>();
        auto decl = state->parser->program->refMap->getDeclaration(pe->path, true);
        if (decl == state->parser->packet) {
            builder->append(expression->member);
            return false;
        }
    }

    visit(expression->expr);
    builder->append(".");
    builder->append(expression->member);
    return false;
}

//////////////////////////////////////////////////////////////////

void WP4ParserState::emit(CodeBuilder* builder) {
    StateTranslationVisitor visitor(this);
    visitor.setBuilder(builder);
    state->apply(visitor);
}

WP4Parser::WP4Parser(const WP4Program* program, const IR::ParserBlock* block, const P4::TypeMap* typeMap) :
        program(program), typeMap(typeMap), parserBlock(block),
        packet(nullptr), headers(nullptr), headerType(nullptr) {}

void WP4Parser::emitDeclaration(CodeBuilder* builder, const IR::Declaration* decl) {
    if (decl->is<IR::Declaration_Variable>()) {
        auto vd = decl->to<IR::Declaration_Variable>();
        auto etype = WP4TypeFactory::instance->create(vd->type);
        builder->emitIndent();
        etype->declare(builder, vd->name, false);
        builder->endOfStatement(true);
        BUG_CHECK(vd->initializer == nullptr,
                  "%1%: declarations with initializers not supported", decl);
        return;
    }
    BUG("%1%: not yet handled", decl);
}


void WP4Parser::emit(CodeBuilder* builder) {
    for (auto l : parserBlock->container->parserLocals)
        emitDeclaration(builder, l);
    for (auto s : states)
        s->emit(builder);
    builder->newline();

    // Create a synthetic reject state
    builder->emitIndent();
    builder->appendFormat("%s: { ", IR::ParserState::reject.c_str());
    builder->newline();
    builder->emitIndent();
    builder->emitIndent();
    builder->appendLine("printk(\"** WP4: Packet Rejected!! **\\n\");");
    builder->emitIndent();
    builder->emitIndent();
    builder->appendFormat("return %s;", builder->target->abortReturnCode().c_str());
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("}");
    builder->newline();
}

bool WP4Parser::build() {
    auto pl = parserBlock->container->type->applyParams;
    if (pl->size() != 2) {
        ::error("Expected parser to have exactly 2 parameters");
        return false;
    }

    auto it = pl->parameters.begin();
    packet = *it; ++it;
    headers = *it;
    for (auto state : parserBlock->container->states) {
        auto ps = new WP4ParserState(state, this);
        states.push_back(ps);
    }

    auto ht = typeMap->getType(headers);
    if (ht == nullptr)
        return false;
    headerType = WP4TypeFactory::instance->create(ht);
    return true;
}

}  // namespace WP4
