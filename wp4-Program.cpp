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

#include <chrono>
#include <ctime>

#include "wp4-Program.h"
#include "wp4-Type.h"
#include "wp4-Control.h"
#include "wp4-Parser.h"
#include "wp4-Table.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/common/options.h"

namespace WP4 {

bool WP4Program::build() {
    auto pack = toplevel->getMain();
    if (pack->type->name != "WP4Switch")
        ::warning(ErrorType::WARN_INVALID, "%1%: the main wp4 package should be called WP4Switch"
                  "; are you using the wrong architecture?", pack->type->name);

    if (pack->getConstructorParameters()->size() != 3) {
        ::error("Expected toplevel package %1% to have 3 parameters", pack->type);
        return false;
    }

    auto pb = pack->getParameterValue(model.wp4_switch.wp4_parser.name)->to<IR::ParserBlock>();
    BUG_CHECK(pb != nullptr, "No parser block found");
    parser = new WP4Parser(this, pb, typeMap);
    bool success = parser->build();
    if (!success)
        return success;

    auto cb = pack->getParameterValue(model.wp4_switch.wp4_switch.name)->to<IR::ControlBlock>();
    BUG_CHECK(cb != nullptr, "No control block found");
    control = new WP4Control(this, cb, parser->headers);
    success = control->build();
    if (!success)
        return success;

    auto db = pack->getParameterValue(model.wp4_switch.wp4_deparser.name)->to<IR::ControlBlock>();
    BUG_CHECK(db != nullptr, "No deparser block found");
    deparser = new WP4Deparser(this, db, parser->headers);
    success = deparser->build();
    if (!success)
        return success;

    return true;
}

void WP4Program::emitC(CodeBuilder* builder, cstring header) {
    emitGeneratedComment(builder);

    builder->target->emitIncludes(builder);     // Add C header files
    builder->newline();

    builder->appendFormat("#include \"%s\"", header.c_str());
    builder->newline();
    builder->newline();

    emitPreamble(builder);

    builder->emitIndent();
    control->emitTableInstances(builder);
    builder->newline();

    builder->emitIndent();
    builder->appendFormat("struct wp4_output %s;", outputVar.c_str());
    builder->newline();

    builder->newline();
    builder->target->emitModule(builder);       // Add kernel module config
    builder->target->emitMain(builder, "wp4_packet_in", model.CPacketName.str(), "wp4_ul_size");
    builder->blockStart();

    builder->newline();
    emitHeaderInstances(builder);
    builder->append(" = ");
    parser->headerType->emitInitializer(builder);
    builder->endOfStatement(true);

    emitLocalVariables(builder);
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("goto %s;", IR::ParserState::start.c_str());
    builder->newline();

    builder->appendFormat("\n// Start of Parser\n");
    parser->emit(builder);

    builder->appendFormat("\n// Start of Pipeline\n");
    emitPipeline(builder);

    builder->appendFormat("\n// Start of Deparser\n");
    deparser->emit(builder);
    builder->appendFormat("    return 0;");
    builder->newline();
    builder->blockEnd(true);  // end of function
    builder->appendFormat("\n// Kernel module functions\n");
    builder->append(
        "EXPORT_SYMBOL(wp4_packet_in);\n"
        "\n"
        "module_init(wp4_init);\n"
        "module_exit(wp4_exit);\n"
        "\n"
        "MODULE_LICENSE(\"GPL\");\n"
        "MODULE_AUTHOR(\"");
    builder->append(options.file);
    builder->append(
        "\");\n"
        "MODULE_DESCRIPTION(\"WP4\");\n"
        "MODULE_VERSION(\"0.1\");\n"
        "\n");
    builder->target->emitLicense(builder, license);
}

void WP4Program::emitH(CodeBuilder* builder, cstring) {
    emitGeneratedComment(builder);
    builder->appendLine("#ifndef _P4_GEN_HEADER_");
    builder->appendLine("#define _P4_GEN_HEADER_");
    builder->newline();
    builder->appendLine("#define htonll(x) ((((uint64_t)htonl(x)) << 32) + htonl((x) >> 32))");
    builder->newline();
    builder->appendLine("#define PACKET_BUFFER_SIZE 1024");
    builder->newline();
    builder->appendLine("#include <linux/types.h>");
    builder->newline();
    builder->appendLine("int wp4_packet_in(u8 *p_uc_data, u16 wp4_ul_size, u8 port);");
    builder->newline();
    emitTypes(builder);
    //emitBufferDefinition(builder);
    builder->newline();    
    control->emitTableTypes(builder);
    builder->newline();
    builder->appendLine("#endif");
}

void WP4Program::emitGeneratedComment(CodeBuilder* builder) {
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    builder->append("/* Automatically generated by ");
    builder->append(options.exe_name);
    builder->append(" from ");
    builder->append(options.file);
    builder->append(" on ");
    builder->append(std::ctime(&time));
    builder->append(" */");
    builder->newline();
}

void WP4Program::emitTypes(CodeBuilder* builder) {
    for (auto d : program->objects) {
        if (d->is<IR::Type>() && !d->is<IR::IContainer>() &&
            !d->is<IR::Type_Extern>() && !d->is<IR::Type_Parser>() &&
            !d->is<IR::Type_Control>() && !d->is<IR::Type_Typedef>() &&
            !d->is<IR::Type_Error>()) {
            auto type = WP4TypeFactory::instance->create(d->to<IR::Type>());
            if (type == nullptr)
                continue;
            type->emit(builder);
        }
    }
}

    void WP4Program::emitBufferDefinition(CodeBuilder *builder) {

        // definition of Packet Buffer
        builder->append("struct ");
        builder->append("packet_buffer");
        builder->spc();
        builder->blockStart();

        builder->emitIndent();
        builder->append("u8 reason;");
        builder->newline();

        builder->emitIndent();
        builder->append("u16 size;");
        builder->newline();

        builder->emitIndent();
        builder->append("u8 flow;");
        builder->newline();

        builder->emitIndent();
        builder->append("u8 buffer[PACKET_BUFFER_SIZE];");
        builder->newline();

        builder->blockEnd(false);
        builder->endOfStatement(true);
    }

void WP4Program::emitPreamble(CodeBuilder* builder) {
    builder->emitIndent();
    builder->appendLine("#define WP4_MASK(t, w) ((((t)(1)) << (w)) - (t)1)");
    builder->appendLine("#define BYTES(w) ((w) / 8)");
    builder->newline();
}

void WP4Program::emitLocalVariables(CodeBuilder* builder) {
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("u16 %s = 0;", offsetVar); 
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("u8 *%s = %s;", packetStartVar, model.CPacketName.str());
    builder->newline();
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("dump_rx_packet(%s);", packetStartVar);
    builder->newline();
    builder->emitIndent();
    builder->appendLine("printk(\"** WP4: Packet Received, size = %d **\\n\", wp4_ul_size);");
}

void WP4Program::emitHeaderInstances(CodeBuilder* builder) {
    builder->emitIndent();
    parser->headerType->declare(builder, parser->headers->name.name, false);
}

void WP4Program::emitPipeline(CodeBuilder* builder) {
    builder->emitIndent();
    builder->append(IR::ParserState::accept);
    builder->append(":");
    builder->newline();
    builder->emitIndent();
    builder->blockStart();
    control->emit(builder);
    builder->blockEnd(true);
}

WP4Control* WP4Program::getSwitch() const {
    return dynamic_cast<WP4Control*>(control);
}

}  // namespace WP4
