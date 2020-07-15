/*
Copyright 2019 Northbound Networks.

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

#ifndef _BACKENDS_WP4_WP4PROGRAM_H_
#define _BACKENDS_WP4_WP4PROGRAM_H_

#include "wp4-Target.h"
#include "wp4-Model.h"
#include "wp4-Object.h"
#include "ir/ir.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/common/options.h"
#include "wp4-CodeGen.h"

namespace WP4 {

class WP4Program;
class WP4Parser;
class WP4Control;
class WP4Deparser;
class WP4Table;
class WP4Type;

class WP4Program : public WP4Object {
 public:
    const CompilerOptions& options;
    const IR::P4Program* program;
    const IR::ToplevelBlock*  toplevel;
    P4::ReferenceMap*    refMap;
    P4::TypeMap*         typeMap;
    WP4Parser*      parser;
    WP4Deparser*    deparser;
    WP4Control*     control;
    WP4Model        &model;

    cstring endLabel, offsetVar, lengthVar;
    cstring zeroKey, functionName, errorVar;
    cstring packetStartVar, byteVar;
    cstring errorEnum;
    cstring license = "GPL";  // TODO: this should be a compiler option probably
    cstring arrayIndexType = "u32";
    cstring inPacketLengthVar, outHeaderLengthVar;

    virtual bool build();  // return 'true' on success

    WP4Program(const CompilerOptions &options, const IR::P4Program* program,
                P4::ReferenceMap* refMap, P4::TypeMap* typeMap, const IR::ToplevelBlock* toplevel) :
            options(options), program(program), toplevel(toplevel),
            refMap(refMap), typeMap(typeMap),
            parser(nullptr), control(nullptr), model(WP4Model::instance) {
        offsetVar = WP4Model::reserved("packetOffsetInBits");
        packetStartVar = WP4Model::reserved("packetStart");
        zeroKey = WP4Model::reserved("zero");
        functionName = WP4Model::reserved("wp4_switch");
        byteVar = WP4Model::reserved("byte");
        inPacketLengthVar = WP4Model::reserved("ul_size");
        outHeaderLengthVar = WP4Model::reserved("outHeaderLength");
        endLabel = WP4Model::reserved("end");
    }

    virtual void emitGeneratedComment(CodeBuilder* builder);
    virtual void emitPreamble(CodeBuilder* builder);
    virtual void emitTypes(CodeBuilder* builder);
    virtual void emitHeaderInstances(CodeBuilder* builder);
    virtual void emitLocalVariables(CodeBuilder* builder);
    virtual void emitPipeline(CodeBuilder* builder);
    virtual void emitH(CodeBuilder* builder, cstring headerFile);  // emits C headers
    virtual void emitC(CodeBuilder* builder, cstring headerFile);  // emits C program
    WP4Control* getSwitch() const;
};

}  // namespace WP4

#endif /* _BACKENDS_WP4_WP4PROGRAM_H_ */
