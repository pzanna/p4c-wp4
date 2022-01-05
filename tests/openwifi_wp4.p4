/* -*- P4_16 -*- */
#include <core.p4>
#include <wp4_model.p4>

/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

typedef bit<9>  egressSpec_t;

typedef bit<48> macAddr_t;  // MAC Address

// Signed (int) header values will not be converted from network to host byte order 

header rfFeatures_t {
    int<64>     timestamp;   // Frame Timestamp
    int<16>     rssi;        // RSSI (dB)
    int<16>     blank;       // Unusedß
    int<16>     len;         // Length
    int<16>     rate_idx;    // Rate Index
    int<32>     phaseOffset; // AUX 1
    int<32>     pilotOffset; // Frequency Offset (kHz)
    int<32>     magSq;       // AUX 3
    int<32>     aux_4;       // AUX 4
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

/** Declaration of the wp4_runtime extern function. */
extern void to_cpu(in Headers_t headers);

/*************************************************************************
*********************** P A R S E R  *************************************
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
        0x00 : mac80211;
        default : reject;
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

    action Drop_action() {
        wp4out.output_action = wp4_action.DROP;
    }

    action Pass_action() {
        wp4out.output_action = wp4_action.PASS;
    }

    action CPU_action() {
        wp4out.output_action = wp4_action.CPU;
        to_cpu(headers);
    }

    table lookup_tbl {

        size = 512;

        key = {
            headers.mac80211.Addr2 : class;
            headers.frameCtrl.frameType : exact;
            headers.frameCtrl.subType : exact;
            headers.rfFeatures.rate_idx : exact;
            headers.rfFeatures.rssi : min;
            headers.rfFeatures.rssi : max;
            headers.rfFeatures.phaseOffset : min;
            headers.rfFeatures.phaseOffset : max;
            headers.rfFeatures.pilotOffset : min;
            headers.rfFeatures.pilotOffset : max;
            headers.rfFeatures.magSq : min;
            headers.rfFeatures.magSq : max;
        }

        actions = {
            Drop_action;
            Pass_action;
            CPU_action;
        }

        default_action = Pass_action;    // Action to perform for a miss in a exact match table or a predictive match in a LCS table.

    }

    /* If it is not a deauth or disassoc management frame send the headers to the CPU otherwise do a table lookup.*/
    apply {
        if(headers.frameCtrl.frameType == 0x0 && (headers.frameCtrl.subType == 0xA || headers.frameCtrl.subType == 0xC)) {    
            lookup_tbl.apply();
        } else {
            CPU_action();
        }
    }

}

control deprs(in Headers_t headers, packet_out p, in wp4_output wp4out){
    apply {
        p.emit(headers.frameCtrl);
        p.emit(headers.mac80211);
    }
}


WP4Switch(prs(), swtch(), deprs())main;
