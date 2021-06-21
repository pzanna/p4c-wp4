/*
Copyright 2019 Paul Zanna.

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

#ifndef _BACKENDS_WP4_WP4CONTROL_H_
#define _BACKENDS_WP4_WP4CONTROL_H_

#include "wp4-Object.h"
#include "wp4-Table.h"

namespace WP4 {

class WP4Control;
class WP4Deparser;

class ControlBodyTranslator : public CodeGenInspector {
    const WP4Control* control;
    std::set<const IR::Parameter*> toDereference;
    std::vector<cstring> saveAction;
    P4::P4CoreLibrary& p4lib;
 public:
    explicit ControlBodyTranslator(const WP4Control* control);

    // handle the packet_out.emit method
    virtual void compileEmitField(const IR::Expression* expr, cstring field, unsigned alignment, WP4Type* type);
    virtual void compileEmit(const IR::Vector<IR::Argument>* args);
    virtual void processApply(const P4::ApplyMethod* method);
    virtual void processFunction(const P4::ExternFunction* function);

    bool preorder(const IR::PathExpression* expression) override;
    bool preorder(const IR::MethodCallExpression* expression) override;
    bool preorder(const IR::ExitStatement*) override;
    bool preorder(const IR::ReturnStatement*) override;
    bool preorder(const IR::IfStatement* statement) override;
    bool preorder(const IR::SwitchStatement* statement) override;
};

class WP4Control : public WP4Object {
 public:
    const WP4Program*      program;
    const IR::ControlBlock* controlBlock;
    const IR::Parameter*    headers;
    const IR::Parameter*    accept;
    const IR::Parameter*    parserHeaders;
    const IR::Parameter*    inputMeta;
    const IR::Parameter*    outputMeta;
    // replace references to headers with references to parserHeaders
    cstring                 hitVariable;
    ControlBodyTranslator*  codeGen;

    std::set<const IR::Parameter*> toDereference;
    std::map<cstring, WP4Table*>  tables;

    WP4Control(const WP4Program* program, const IR::ControlBlock* block, const IR::Parameter* parserHeaders);
    virtual void emit(CodeBuilder* builder);
    void emitDeclaration(CodeBuilder* builder, const IR::Declaration* decl);
    void emitTableTypes(CodeBuilder* builder);
    void emitTableInitializers(CodeBuilder* builder);
    void emitTableInstances(CodeBuilder* builder);
    void emitLookup(CodeBuilder* builder);
    virtual bool build();
    WP4Table* getTable(cstring name) const {
        auto result = ::get(tables, name);
        BUG_CHECK(result != nullptr, "No table named %1%", name);
        return result; }

 protected:
    void scanConstants();
};

class WP4Deparser : public WP4Control {
 public:
    const IR::Parameter*    packet;

    WP4Deparser(const WP4Program* program, const IR::ControlBlock* block, const IR::Parameter* parserHeaders);
    bool build() override;
    void emit(CodeBuilder* builder) override;
    const WP4Program* getProgram() const
    { return dynamic_cast<const WP4Program*>(program); }
};

}  // namespace WP4

#endif /* _BACKENDS_WP4_WP4CONTROL_H_ */
