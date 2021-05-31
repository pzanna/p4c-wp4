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

#include "wp4-Table.h"
#include "wp4-Type.h"
#include "ir/ir.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"

namespace WP4 {

namespace {
class ActionTranslationVisitor : public CodeGenInspector {
 protected:
    const WP4Program*  program;
    const IR::P4Action* action;
    cstring             valueName;

 public:
    ActionTranslationVisitor(cstring valueName, const WP4Program* program):
            CodeGenInspector(program->refMap, program->typeMap), program(program),
            action(nullptr), valueName(valueName)
    { CHECK_NULL(program); }

    bool preorder(const IR::PathExpression* expression) override {
        auto decl = program->refMap->getDeclaration(expression->path, true);
        if (decl->is<IR::Parameter>()) {
            auto param = decl->to<IR::Parameter>();
            bool isParam = action->parameters->getParameter(param->name) == param;
            if (isParam) {
                builder->append(valueName);
                builder->append("->u.");
                cstring name = WP4Object::externalName(action);
                builder->append(name);
                builder->append(".");
                builder->append(expression->path->toString());  // original name
                return false;
            }
        }
        visit(expression->path);
        return false;
    }

    void convertActionBody(const IR::Vector<IR::StatOrDecl> *body) {
        for (auto s : *body) {
            if (!s->is<IR::Statement>()) {
                continue;
            } else if (auto block = s->to<IR::BlockStatement>()) {
                convertActionBody(&block->components);
                continue;
            } else if (s->is<IR::ReturnStatement>()) {
                break;
            } else if (s->is<IR::ExitStatement>()) {
                break;
            } else if (s->is<IR::EmptyStatement>()) {
                continue;
            } else if (s->is<IR::MethodCallStatement>()) {
                visit(s);
                builder->newline();
            } else {
                visit(s);
            }
        }
    }

    void convertAction() {
        builder->append("");
        convertActionBody(&action->body->components);
    }

    bool preorder(const IR::P4Action* act) override {
        action = act;
        convertAction();    // Insert action - builder->append("****");
        return false;
    }
};  // ActionTranslationVisitor
}  // namespace

////////////////////////////////////////////////////////////////

WP4Table::WP4Table(const WP4Program* program, const IR::TableBlock* table,
                     CodeGenInspector* codeGen) :
        WP4TableBase(program, WP4Object::externalName(table->container), codeGen), table(table) {
    cstring base = instanceName + "_defaultAction";
    defaultActionMapName = program->refMap->newName(base);

    base = table->container->name.name + "_actions";
    actionEnumName = program->refMap->newName(base);

    base = instanceName + "_NoAction";
    noActionName = program->refMap->newName(base);

    keyGenerator = table->container->getKey();
    actionList = table->container->getActionList();

    keyType = new IR::Type_Struct(IR::ID(keyTypeName));
    valueType = new IR::Type_Struct(IR::ID(valueTypeName));

    setTableSize(table);
}

void WP4Table::setTableSize(const IR::TableBlock *table) {
    auto properties = table->container->properties->properties;
    this->size = UINT16_MAX;  // Default value 2^16. Next power is too big for ubpf vm.
                              // For instance, 2^17 causes error while loading program.

    auto sz = table->container->getSizeProperty();
    if (sz == nullptr)
        return;

    auto pConstant = sz->to<IR::Constant>();
    if (pConstant->asInt() <= 0) {
        ::error(ErrorType::ERR_INVALID, "negative size", pConstant);
        return;
    }

    this->size = pConstant->asInt();
    if (this->size > UINT16_MAX) {
        ::error(ErrorType::ERR_UNSUPPORTED, "size too large. Using default value (%2%).",
                pConstant, UINT16_MAX);
        return;
    }
}

void WP4Table::emitKeyType(CodeBuilder* builder) {

    builder->emitIndent();
    builder->appendFormat("struct %s ", keyTypeName.c_str());
    builder->blockStart();

    CodeGenInspector commentGen(program->refMap, program->typeMap);
    commentGen.setBuilder(builder);

    if (keyGenerator != nullptr) {
        // Use this to order elements by size
        std::multimap<size_t, const IR::KeyElement*> ordered;
        unsigned fieldNumber = 0;
        for (auto c : keyGenerator->keyElements) {
            auto type = program->typeMap->getType(c->expression);
            auto wp4Type = WP4TypeFactory::instance->create(type);
            cstring fieldName = c->expression->toString().replace('.', '_');
            if (!wp4Type->is<IHasWidth>()) {
                ::error("%1%: illegal type %2% for key field", c, type);
                return;
            }
            unsigned width = wp4Type->to<IHasWidth>()->widthInBits();
            ordered.emplace(width, c);
            keyTypes.emplace(c, wp4Type);
            keyFieldNames.emplace(c, fieldName);
            fieldNumber++;
        }

        // Emit key in decreasing order size - this way there will be no gaps
        for (auto it = ordered.rbegin(); it != ordered.rend(); ++it) {
            auto c = it->second;

            auto wp4Type = ::get(keyTypes, c);
            builder->emitIndent();
            cstring fieldName = ::get(keyFieldNames, c);
            wp4Type->declare(builder, fieldName, false);
            builder->append("; /* ");
            c->expression->apply(commentGen);
            builder->append(" */");
            builder->newline();

            auto mtdecl = program->refMap->getDeclaration(c->matchType->path, true);
            auto matchType = mtdecl->getNode()->to<IR::Declaration_ID>();
            if (matchType->name.name != P4::P4CoreLibrary::instance.exactMatch.name)
                ::error("Match of type %1% not supported", c->matchType);
        }
    }
    builder->blockEnd(false);
    builder->endOfStatement(true);

    // definition of map
    builder->newline();
    builder->append("struct ");
    builder->append("wp4_map_def");
    builder->spc();
    builder->blockStart();

    builder->emitIndent();
    builder->append("u16 key_size;");
    builder->newline();

    builder->emitIndent();
    builder->append("u16 value_size;");
    builder->newline();

    builder->emitIndent();
    builder->append("u16 max_entries;");
    builder->newline();

    builder->emitIndent();
    builder->append("u16 last_entry;");
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("struct %s lookup_key[%d];", keyTypeName.c_str(), size);
    builder->newline();

    builder->blockEnd(false);
    builder->endOfStatement(true);
}

void WP4Table::emitActionArguments(CodeBuilder* builder, const IR::P4Action* action, cstring name) {
    builder->emitIndent();
    builder->append("struct ");
    builder->blockStart();

    for (auto p : *action->parameters->getEnumerator()) {
        builder->emitIndent();
        auto type = WP4TypeFactory::instance->create(p->type);
        type->declare(builder, p->name.name, false);
        builder->endOfStatement(true);
    }

    builder->blockEnd(false);
    builder->spc();
    builder->append(name);
    builder->endOfStatement(true);
}

cstring WP4Table::generateActionName(const IR::P4Action *action) {
    if (action->getName().originalName == P4::P4CoreLibrary::instance.noAction.name) {
        return this->noActionName;
    } else {
        return WP4::WP4Object::externalName(action);
    }
}

void WP4Table::emitValueType(CodeBuilder* builder) {
    // create an enum with tags for all actions
    builder->emitIndent();
    builder->newline();
    builder->append("enum ");
    builder->append(actionEnumName);
    builder->spc();
    builder->blockStart();

    for (auto a : actionList->actionList) {
        auto adecl = program->refMap->getDeclaration(a->getPath(), true);
        auto action = adecl->getNode()->to<IR::P4Action>();
        cstring name = generateActionName(action);
        builder->emitIndent();
        builder->append(name);
        builder->append(",");
        builder->newline();
    }

    builder->blockEnd(false);
    builder->endOfStatement(true);
/*
    // a type-safe union: a struct with a tag and an union
    builder->emitIndent();
    builder->newline();
    builder->appendFormat("struct %s ", valueTypeName.c_str());
    builder->blockStart();

    builder->emitIndent();
    builder->appendFormat("enum %s action;", actionEnumName.c_str());
    builder->newline();

    builder->emitIndent();
    builder->append("union ");
    builder->blockStart();

    for (auto a : actionList->actionList) {
        auto adecl = program->refMap->getDeclaration(a->getPath(), true);
        auto action = adecl->getNode()->to<IR::P4Action>();
        cstring name = generateActionName(action);
        emitActionArguments(builder, action, name);
    }

    builder->blockEnd(false);
    builder->spc();
    builder->appendLine("u;");
    builder->blockEnd(false);
    builder->endOfStatement(true);
*/
}

void WP4Table::emitTypes(CodeBuilder* builder) {
    emitKeyType(builder);
    emitValueType(builder);
}

void WP4Table::emitInstance(CodeBuilder* builder) {
    BUG_CHECK(keyType != nullptr, "Key type of %1% is not set", instanceName);
    BUG_CHECK(valueType != nullptr, "Value type of %1% is not set", instanceName);

    cstring keyTypeStr;
    if (keyType->is<IR::Type_Bits>()) {
        auto tb = keyType->to<IR::Type_Bits>();
        auto scalar = new WP4ScalarType(tb);
        keyTypeStr = scalar->getAsString();
    } else if (keyType->is<IR::Type_StructLike>()) {
        keyTypeStr = cstring("struct ") + keyTypeName.c_str();;
    }
    // Key type is not null, but we didn't handle it
    BUG_CHECK(!keyTypeStr.isNullOrEmpty(), "Key type %1% not supported", keyType->toString());

    cstring valueTypeStr;
    if (valueType->is<IR::Type_Bits>()) {
        auto tb = valueType->to<IR::Type_Bits>();
        auto scalar = new WP4ScalarType(tb);
        valueTypeStr = scalar->getAsString();
    } else if (valueType->is<IR::Type_StructLike>()) {
        //valueTypeStr = cstring("struct ") + valueTypeName.c_str();
        valueTypeStr = cstring("enum ") + actionEnumName.c_str();
    }
    // Value type is not null, but we didn't handle it
    BUG_CHECK(!valueTypeStr.isNullOrEmpty(), "Value type %1% not supported", valueType->toString());

    builder->target->emitTableDecl(builder, dataMapName, keyTypeStr, valueTypeStr, size);
}

void WP4Table::emitKey(CodeBuilder* builder, cstring keyName) {
    if (keyGenerator == nullptr)
        return;
    for (auto c : keyGenerator->keyElements) {
        auto wp4Type = ::get(keyTypes, c);
        cstring fieldName = ::get(keyFieldNames, c);
        CHECK_NULL(fieldName);
        bool memcpy = false;
        WP4ScalarType* scalar = nullptr;
        unsigned width = 0;
        if (wp4Type->is<WP4ScalarType>()) {
            scalar = wp4Type->to<WP4ScalarType>();
            width = scalar->implementationWidthInBits();
            memcpy = !WP4ScalarType::generatesScalar(width);
        }

        builder->emitIndent();
        if (memcpy) {
            builder->appendFormat("memcpy(&%s.%s, &", keyName.c_str(), fieldName.c_str());
            codeGen->visit(c->expression);
            builder->appendFormat(", %d)", scalar->bytesRequired());
        } else {
            builder->appendFormat("%s.%s = ", keyName.c_str(), fieldName.c_str());
            codeGen->visit(c->expression);
        }
        builder->endOfStatement(true);
    }
}

void WP4Table::emitAction(CodeBuilder* builder, cstring valueName) {
    builder->emitIndent();
    builder->appendFormat("switch (%s) ", valueName.c_str());
    builder->blockStart();

    for (auto a : actionList->actionList) {
        auto adecl = program->refMap->getDeclaration(a->getPath(), true);
        auto action = adecl->getNode()->to<IR::P4Action>();
        builder->emitIndent();
        cstring name = generateActionName(action);
        builder->appendFormat("case %s: ", name.c_str());
        builder->newline();
        builder->emitIndent();
        builder->blockStart();
        builder->emitIndent();

        ActionTranslationVisitor visitor(valueName, program);
        visitor.setBuilder(builder);
        visitor.copySubstitutions(codeGen);

        action->apply(visitor);
        builder->newline();
        builder->blockEnd(true);
        builder->emitIndent();
        builder->appendLine("break;");
        builder->newline();
    }

    builder->emitIndent();
    builder->appendFormat("default: return %s", builder->target->abortReturnCode().c_str());
    builder->endOfStatement(true);
    builder->blockEnd(true);
}

void WP4Table::emitInitializer(CodeBuilder* builder) {
    // emit code to initialize the default action
    const IR::P4Table* t = table->container;
    const IR::Expression* defaultAction = t->getDefaultAction();
    BUG_CHECK(defaultAction->is<IR::MethodCallExpression>(),
              "%1%: expected an action call", defaultAction);
    auto mce = defaultAction->to<IR::MethodCallExpression>();
    auto mi = P4::MethodInstance::resolve(mce, program->refMap, program->typeMap);

    auto defact = t->properties->getProperty(IR::TableProperties::defaultActionPropertyName);
    // uBPF does not support setting default action at compile time.
    // Default action must be set from a control plane and 'const' qualifier
    // does not permit to modify default action by a control plane.
    if (defact->isConstant) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "%1%: WP4 target does not allow 'const default_action'. "
                "Use `default_action` instead.", defact);
    }
    auto ac = mi->to<P4::ActionCall>();
    BUG_CHECK(ac != nullptr, "%1%: expected an action call", mce);
    auto action = ac->action;

    cstring name = generateActionName(action);
    cstring defaultTable = defaultActionMapName;
    cstring value = name + "_value";

    builder->emitIndent();
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat("struct %s %s = ", valueTypeName.c_str(), value.c_str());
    builder->blockStart();
    builder->emitIndent();
    builder->appendFormat(".action = %s,", name.c_str());
    builder->newline();
    WP4::CodeGenInspector cg(program->refMap, program->typeMap);
    cg.setBuilder(builder);
    builder->emitIndent();
    builder->appendFormat(".u = {.%s = {", name.c_str());
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        auto arg = mi->substitution.lookup(p);
        arg->apply(cg);
        builder->append(",");
    }
    builder->append("}},\n");
    builder->blockEnd(false);
    builder->endOfStatement(true);

    builder->emitIndent();
    builder->appendFormat("INIT_WP4_TABLE(\"%s\", sizeof(%s), sizeof(%s));", defaultTable,
            program->zeroKey.c_str(), value);
    builder->newline();

    builder->emitIndent();
    //builder->target->emitTableUpdate(builder, defaultTable, program->zeroKey, "&" + value);
    builder->endOfStatement(true);
    builder->blockEnd(true);


    // Check if there are const entries.
    auto entries = t->getEntries();
    if (entries != nullptr) {
        ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                "%1%: Immutable table entries cannot be configured by the WP4 target "
                "and should not be used.",
                entries);
    }
}

}  // namespace WP4
