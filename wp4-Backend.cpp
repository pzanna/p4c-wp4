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

#include "lib/error.h"
#include "lib/nullstream.h"
#include "frontends/p4/evaluator/evaluator.h"

#include "wp4-Backend.h"
#include "wp4-Target.h"
#include "wp4-Type.h"
#include "wp4-Program.h"

namespace WP4 {

void run_wp4_backend(const WP4Options& options, const IR::ToplevelBlock* toplevel, P4::ReferenceMap* refMap, P4::TypeMap* typeMap) {
    if (toplevel == nullptr)
        return;

    auto main = toplevel->getMain();
    if (main == nullptr) {
        ::warning(ErrorType::WARN_MISSING, "Could not locate top-level block; is there a %1% module?", IR::P4Program::main);
        return;
    }

    Target* target;
    if (options.target.isNullOrEmpty() || options.target == "wp4") {
            target = new wp4Target();
    } else {
        ::error("Unknown target %s; legal choice is 'wp4'", options.target);
        return;
    }

    WP4TypeFactory::createFactory(typeMap);
    auto wp4prog = new WP4Program(options, toplevel->getProgram(), refMap, typeMap, toplevel);
    if (!wp4prog->build())
        return;

    if (options.outputFile.isNullOrEmpty())
        return;

    cstring cfile = options.outputFile;
    auto cstream = openFile(cfile, false);
    if (cstream == nullptr)
        return;

    cstring hfile;
    const char* dot = cfile.findlast('.');
    if (dot == nullptr)
        hfile = cfile + ".h";
    else
        hfile = cfile.before(dot) + ".h";
    auto hstream = openFile(hfile, false);
    if (hstream == nullptr)
        return;
    
    CodeBuilder c(target);
    CodeBuilder h(target);

    wp4prog->emitH(&h, hfile);
    wp4prog->emitC(&c, hfile);
    
    *cstream << c.toString();
    *hstream << h.toString();
    cstream->flush();
    hstream->flush();
}

}  // namespace WP4
