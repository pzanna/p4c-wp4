/* -*- P4_16 -*- */
#include <core.p4>
#include <wp4_model.p4>

/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

typedef bit<9>  egressSpec_t;

typedef bit<48> macAddr_t;  // MAC Address

#define RF_FEATURE 1234

header rfFeatures_t {
    bit<64>     timestamp;   // Frame Timestamp
    bit<16>     rssi;        // RSSI (dB)
    bit<16>     blank;       // Unusedß
    bit<16>     len;         // Length
    bit<16>     rate_idx;    // Rate Index
    int<32>     phaseOffset; // AUX 1
    int<32>     pilotOffset; // Frequency Offset (kHz)
    int<32>     magSq;       // AUX 3
    bit<32>     aux_4;       // AUX 4
}

header frameCtrl_t {
    bit<2>      protoVer;   // Protocol Version 
    bit<2>      frameType; // Frame Type
    bit<4>      subType;   // Frame Subtype
    bit<1>      toDS;      // To Dist. System
    bit<1>      fromDS;    // From Dist. System
    bit<1>      moreFrag;  // More Fragments
    bit<1>      retry;     // Retransmission
    bit<1>      pwrMgmt;   // Power Management
    bit<1>      moreData;  // More Data
    bit<1>      protFrame; // Protected Frame
    bit<1>      order;     // Order Bit
}

header mac80211_t {
    bit<16>     durID;      // Duration ID
    macAddr_t   Addr1;     // MAC Address 1
    macAddr_t   Addr2;     // MAC Address 2
    macAddr_t   Addr3;     // MAC Address 3
    bit<16>     seqCtrl;   // Sequence Control
    macAddr_t   Addr4;     // MAC Address 4
}

struct Headers_t {
    rfFeatures_t    rfFeatures; // RF Features
    frameCtrl_t     frameCtrl;
    mac80211_t      mac80211;
}


/*************************************************************************
*********************** P A R S E R  ***********************************
*************************************************************************/

parser prs(packet_in p, out Headers_t headers) {

    state start {
        p.extract(headers.rfFeatures);
        transition select(headers.rfFeatures.phaseOffset) {
        default : prot_ver;
        }
    }

    state prot_ver {
        p.extract(headers.frameCtrl);
        transition select(headers.frameCtrl.protoVer) {
        0x00 : frame_type;
        default : reject;
        }
    }

    state frame_type {
        transition select(headers.frameCtrl.frameType) {
        0x00 : mac80211;
        0x02 : data;
        default : accept;
        }
    }

    state data {
        transition select(headers.frameCtrl.subType) {
        0x04 : accept;
        default : mac80211;
        }
    }

    state mac80211 {
        p.extract(headers.mac80211);
        transition select(headers.mac80211.Addr2) {
        default : accept;
        }
    }
}

control swtch(inout Headers_t headers, in wp4_input wp4in, out wp4_output wp4out){

    action Pass_action() {
        wp4out.output_action = wp4_action.PASS;
    }

    action Drop_action() {
        wp4out.output_action = wp4_action.DROP;
    }

    action CPU_action() {
        wp4out.output_action = wp4_action.CPU;
    }

    table lookup_tbl {

        size = 1024;

        key = {
            headers.mac80211.Addr2 : exact;
            headers.frameCtrl.frameType : exact;
            headers.frameCtrl.subType : exact;
        }

        actions = {
            Pass_action;
            Drop_action;
            CPU_action;
        }

    }

    apply {
        lookup_tbl.apply();
        if (wp4out.output_action == wp4_action.PASS) return;
    }

}

control deprs(in Headers_t headers, packet_out p, in wp4_output wp4out){
    apply {
        p.emit(headers.frameCtrl);
        p.emit(headers.mac80211);
    }
}


WP4Switch(prs(), swtch(), deprs())main;
