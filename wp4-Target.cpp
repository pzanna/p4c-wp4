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

#include "wp4-Target.h"
#include "wp4-Type.h"

namespace WP4 {


void wp4Target::emitTableLookup(Util::SourceCodeBuilder* builder, cstring tblName, cstring key, cstring value) const {
    //builder->appendFormat("%s = %s.lookup(&%s)", value.c_str(), tblName.c_str(), key.c_str());
    builder->appendFormat("%s = %s.lookup(&%s)", value.c_str(), tblName.c_str(), key.c_str());
}

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
         "\n");
}

void wp4Target::emitModule(Util::SourceCodeBuilder* builder) const {
     builder->append(
         "static int __init wp4_init(void) {\n"
         "   printk(KERN_INFO \"WP4: Loading WP4 LKM!\\n\");\n"
         "   return 0;\n"
         "}\n"
         "\n"
         "static void __exit wp4_exit(void) {\n"
         "   printk(KERN_INFO \"WP4: Removing WP4 LKM!\\n\");\n"
         "}\n"
         "\n");
}

void wp4Target::emitMain(Util::SourceCodeBuilder* builder, cstring functionName, cstring argName, cstring packetSize) const {
     builder->appendFormat("int %s(u8 *%s, u16 %s, u8 port)", functionName.c_str(), argName.c_str(), packetSize);
}

}  // namespace WP4






