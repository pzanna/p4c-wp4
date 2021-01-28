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

#ifndef _BACKENDS_WP4_WP4OBJECT_H_
#define _BACKENDS_WP4_WP4OBJECT_H_

#include "wp4-Target.h"
#include "wp4-Model.h"
#include "ir/ir.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "wp4-CodeGen.h"

namespace WP4 {

// Base class for WP4 objects
class WP4Object {
 public:
    virtual ~WP4Object() {}
    template<typename T> bool is() const { return to<T>() != nullptr; }
    template<typename T> const T* to() const {
        return dynamic_cast<const T*>(this); }
    template<typename T> T* to() {
        return dynamic_cast<T*>(this); }

    static cstring externalName(const IR::IDeclaration* declaration) {
        cstring name = declaration->externalName();
        return name.replace('.', '_');
    }
};

}  // namespace WP4

#endif /* _BACKENDS_WP4_WP4OBJECT_H_ */
