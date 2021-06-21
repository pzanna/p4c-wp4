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

#ifndef _WP4Switch_MODEL_P4_
#define _WP4Switch_MODEL_P4_

#include <core.p4>

enum wp4_action {
    DROP,
    PASS,
    CPU
}

match_kind {
    max,      // Lower than the value specified.
    min    // Greater than the value specified.
}

/* architectural model for WP4Switch packet switch target architecture */
struct wp4_input {
    bit<32> input_port;// input port of the packet
}

struct wp4_output {
    wp4_action output_action;  // output action for packet
}

parser wp4_parse<H>(packet_in packet, out H headers);
control wp4_switch<H>(inout H headers, in wp4_input imd, out wp4_output omd);
control wp4_deparse<H>(in H headers, packet_out packet, in wp4_output omd);

package WP4Switch<H>(wp4_parse<H> prs, wp4_switch<H> swtch, wp4_deparse<H> deprs);

#endif
