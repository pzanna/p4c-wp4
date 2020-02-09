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

#ifndef _BACKENDS_WP4_WP4MODEL_H_
#define _BACKENDS_WP4_WP4MODEL_H_

#include "frontends/common/model.h"
#include "frontends/p4/coreLibrary.h"
#include "ir/ir.h"
#include "lib/cstring.h"

namespace WP4 {

struct TableImpl_Model : public ::Model::Extern_Model {
    explicit TableImpl_Model(cstring name) :
            Extern_Model(name),
            size("size") {}
    ::Model::Elem size;
};

struct Switch_Model : public ::Model::Elem {
    Switch_Model() : Elem("wp4"),
                     zfx_parser("prs"), zfx_switch("swtch"), zfx_deparser("deprs") {}
    ::Model::Elem zfx_parser;
    ::Model::Elem zfx_switch;
    ::Model::Elem zfx_deparser;
};

struct InputMetadataModel : public ::Model::Type_Model {
    InputMetadataModel() : ::Model::Type_Model("wp4_input"),
        inputPort("input_port"), inputPortType(IR::Type_Bits::get(32))
    {}

    ::Model::Elem inputPort;
    const IR::Type* inputPortType;
};

struct OutputMetadataModel : public ::Model::Type_Model {
    OutputMetadataModel() : ::Model::Type_Model("wp4_output"),
            outputPort("output_port"), outputPortType(IR::Type_Bits::get(32)),
                            output_action("output_action")
    {}

    ::Model::Elem outputPort;
    const IR::Type* outputPortType;
    ::Model::Elem output_action;
};

// Keep this in sync with wp4_model.p4
class WP4Model : public ::Model::Model {
 protected:
    WP4Model() : Model("0.1"),
                  hash_table("hash_table"),
                  tableImplProperty("implementation"),
                  CPacketName("p_uc_data"),
                  packet("packet", P4::P4CoreLibrary::instance.packetIn, 0),
                  inputMetadataModel(), outputMetadataModel(),
                  zfx_switch(), counterIndexType("uint32_t"), counterValueType("uint32_t")
    {}

 public:
    static WP4Model instance;
    static cstring reservedPrefix;
    TableImpl_Model        hash_table;
    ::Model::Elem          tableImplProperty;
    ::Model::Elem          CPacketName;
    ::Model::Param_Model   packet;
    InputMetadataModel inputMetadataModel;
    OutputMetadataModel outputMetadataModel;
    Switch_Model           zfx_switch;
    cstring counterIndexType;
    cstring counterValueType;

    static cstring reserved(cstring name)
    { return reservedPrefix + name; }
};

}  // namespace WP4

#endif /* _BACKENDS_WP4_WP4MODEL_H_ */
