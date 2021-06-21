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

#include "wp4-Target.h"
#include "wp4-Type.h"

namespace WP4 {

void wp4Target::emitIncludes(Util::SourceCodeBuilder* builder) const {
    builder->append(
        "#include <linux/module.h>    // included for all kernel modules\n"
        "#include <linux/kernel.h>    // included for KERN_INFO\n"
        "#include <linux/init.h>      // included for __init and __exit macros\n"
        "#include <linux/fs.h>\n"
        "#include <linux/debugfs.h>\n"
        "#include <linux/slab.h>\n"
        "#include <linux/mm.h>\n"  
        "#include <linux/skbuff.h>\n"
        "#include <linux/netdevice.h>\n"
        "#include \"wp4_runtime.h\"\n"
        "\n");
}

void wp4Target::emitModule(Util::SourceCodeBuilder* builder) const {
    builder->newline();
    builder->append(
         "static void __exit wp4_exit(void) {\n"
         "   printk(KERN_INFO \"WP4: Removing WP4 LKM!\\n\");\n"
         "   table_exit();\n"
         "}\n"
         "\n");
}

void wp4Target::emitMain(Util::SourceCodeBuilder* builder, cstring functionName, cstring argName, cstring packetSize) const {
    builder->appendFormat("int %s(u8 *%s, u16 %s, u8 port)", functionName.c_str(), argName.c_str(), packetSize);
}

void wp4Target::emitTableLookup(Util::SourceCodeBuilder* builder, cstring key, UNUSED cstring value) const {
    builder->appendFormat("wp4_table_lookup(&%s)", key.c_str());
}

void wp4Target::emitTableDecl(Util::SourceCodeBuilder *builder, cstring tblName, cstring keyType, cstring valueType, unsigned size) const {
    builder->append("extern struct wp4_map_def *flow_table;");
    builder->newline();
    builder->newline();

    builder->append("static int __init wp4_init(void) ");
    builder->blockStart();
    builder->emitIndent();
    builder->append("printk(KERN_INFO \"WP4: Loading WP4 LKM!\\n\");\n");
    builder->emitIndent();
    builder->append("table_init();\n");

    builder->emitIndent();
    builder->appendFormat("flow_table->key_size = sizeof(%s);", keyType);
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("flow_table->value_size = sizeof(%s);", valueType);
    builder->newline();
    builder->emitIndent();
    builder->appendFormat("flow_table->max_entries = %d;", size);
    builder->newline();
    builder->emitIndent();
    builder->append("flow_table->last_entry = 0;");
    builder->newline();
    builder->emitIndent();
    builder->append("return 0;");
    builder->newline();
    builder->blockEnd(false);
    }


}  // namespace WP4






