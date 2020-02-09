/*
Copyright 2019 Paul Zanna.

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

#ifndef _BACKENDS_WP4_WP4BACKEND_H_
#define _BACKENDS_WP4_WP4BACKEND_H_

#include "wp4-Options.h"
#include "wp4-Object.h"
#include "ir/ir.h"
#include "frontends/p4/evaluator/evaluator.h"

namespace WP4 {

void run_wp4_backend(const WP4Options& options, const IR::ToplevelBlock* toplevel,
                      P4::ReferenceMap* refMap, P4::TypeMap* typeMap);

}  // namespace WP4

#endif /* _BACKENDS_WP4_WP4BACKEND_H_ */
