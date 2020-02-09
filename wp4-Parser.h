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

#ifndef _BACKENDS_WP4_WP4PARSER_H_
#define _BACKENDS_WP4_WP4PARSER_H_

#include "ir/ir.h"
#include "wp4-Object.h"
#include "wp4-Program.h"

namespace WP4 {

class WP4Parser;

class WP4ParserState : public WP4Object {
 public:
    const IR::ParserState* state;
    const WP4Parser* parser;

    WP4ParserState(const IR::ParserState* state, WP4Parser* parser) :
            state(state), parser(parser) {}
    void emit(CodeBuilder* builder);
};

class WP4Parser : public WP4Object {
 public:
    const WP4Program*            program;
    const P4::TypeMap*                typeMap;
    const IR::ParserBlock*            parserBlock;
    std::vector<WP4ParserState*> states;
    const IR::Parameter*              packet;
    const IR::Parameter*              headers;
    WP4Type*                     headerType;

    explicit WP4Parser(const WP4Program* program, const IR::ParserBlock* block, const P4::TypeMap* typeMap);
    void emitDeclaration(CodeBuilder* builder, const IR::Declaration* decl);
    void emit(CodeBuilder* builder);
    bool build();
};

}  // namespace WP4

#endif /* _BACKENDS_WP4_WP4PARSER_H_ */
