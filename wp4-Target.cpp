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
         "#include <asf.h>\n"
         "#include <string.h>\n"
         "#include <stdlib.h>\n"
         "#include \"common.h\"\n"
         "#include \"switch.h\"\n"
         "\n");
}

void wp4Target::emitMain(Util::SourceCodeBuilder* builder, cstring functionName, cstring argName, cstring packetSize) const {
     builder->appendFormat("void %s(uint8_t *%s, uint16_t %s, uint8_t port)", functionName.c_str(), argName.c_str(), packetSize);
}

}  // namespace WP4
