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

#ifndef _BACKENDS_WP4_WP4MODEL_H_
#define _BACKENDS_WP4_WP4MODEL_H_

#include "frontends/common/model.h"
#include "frontends/p4/coreLibrary.h"
#include "ir/ir.h"
#include "lib/cstring.h"

namespace WP4 {

struct Switch_Model : public ::Model::Elem {
    Switch_Model() : Elem("wp4"),
                     wp4_parser("prs"), wp4_switch("swtch"), wp4_deparser("deprs") {}
    ::Model::Elem wp4_parser;
    ::Model::Elem wp4_switch;
    ::Model::Elem wp4_deparser;
};

// Keep this in sync with wp4_model.p4
class WP4Model : public ::Model::Model {
 protected:
    WP4Model() : Model("0.1"),
                  CPacketName("p_uc_data"),
                  packet("packet", P4::P4CoreLibrary::instance.packetIn, 0),
                  wp4_switch(),
                  counterIndexType("u32"),
                  counterValueType("u32")
    {}

 public:
    static WP4Model instance;
    static cstring reservedPrefix;
    ::Model::Elem          CPacketName;
    ::Model::Param_Model   packet;
    Switch_Model           wp4_switch;
    cstring counterIndexType;
    cstring counterValueType;

    static cstring reserved(cstring name)
    { return reservedPrefix + name; }
};

}  // namespace WP4

#endif /* _BACKENDS_WP4_WP4MODEL_H_ */
