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

#ifndef _BACKENDS_WP4_WP4TABLE_H_
#define _BACKENDS_WP4_WP4TABLE_H_

#include "wp4-Object.h"
#include "wp4-Program.h"
#include "frontends/p4/methodInstance.h"

namespace WP4 {
class WP4TableBase : public WP4Object {
 public:
    const WP4Program* program;

    cstring instanceName;
    cstring keyTypeName;
    cstring valueTypeName;
    const IR::Type *keyType{};
    const IR::Type *valueType{};
    cstring dataMapName;
    size_t size{};
    CodeGenInspector* codeGen;

 protected:
    WP4TableBase(const WP4Program* program, cstring instanceName,
                  CodeGenInspector* codeGen) :
            program(program), instanceName(instanceName), codeGen(codeGen) {
        CHECK_NULL(codeGen); CHECK_NULL(program);
        keyTypeName = program->refMap->newName(instanceName + "_key");
        valueTypeName = program->refMap->newName(instanceName + "_value");
        dataMapName = instanceName;
    }
};

class WP4Table final : public WP4TableBase {
private:
    void setTableSize(const IR::TableBlock *table);
    
 public:
    const IR::Key*            keyGenerator;
    const IR::ActionList*     actionList;
    const IR::TableBlock*    table;
    cstring     defaultActionMapName;
    cstring     actionEnumName;
    cstring     noActionName;
    std::map<const IR::KeyElement*, cstring> keyFieldNames;
    std::map<const IR::KeyElement*, WP4Type*> keyTypes;

    WP4Table(const WP4Program* program, const IR::TableBlock* table, CodeGenInspector* codeGen);
    cstring generateActionName(const IR::P4Action *action);
    void emitTypes(CodeBuilder* builder);
    void emitInstance(CodeBuilder* builder);
    void emitActionArguments(CodeBuilder* builder, const IR::P4Action* action, cstring name);
    void emitKeyType(CodeBuilder* builder);
    void emitValueType(CodeBuilder* builder);
    void emitKey(CodeBuilder* builder, cstring keyName);
    void emitAction(CodeBuilder* builder, cstring valueName);
    void emitInitializer(CodeBuilder* builder);
    void emitLookupFunc(CodeBuilder* builder);
};

}  // namespace WP4

#endif /* _BACKENDS_WP4_WP4TABLE_H_ */
