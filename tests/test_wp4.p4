/* -*- P4_16 -*- */
#include <core.p4>
#include <wp4_model.p4>

/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

typedef bit<9>  egressSpec_t;

typedef bit<48> macAddr_t;  // MAC Address

header rfFeatures_t {
    bit<64>     timestamp;   // Frame Timestamp
    bit<16>     rssi;        // RSSI (dB)
    bit<16>     blank;       // Unusedß
    bit<16>     len;         // Length
    bit<16>     rate_idx;    // Rate Index
    bit<32>     aux_1;      // AUX 1
    int<32>     freqOffset; // Frequency Offset (kHz)
    bit<32>     aux_3;      // AUX 3
    bit<32>     aux_4;      // AUX 4
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
        transition select(headers.rfFeatures.aux_1) {
        0xCCCCDDDD : mac;
        default : accept;
        }
    }

    state mac {
        p.extract(headers.frameCtrl);
        transition select(headers.frameCtrl.frameType) {
        0x00 : mgnt;
        default : accept;
        }
    }

    state mgnt {
        p.extract(headers.mac80211);
        transition accept;
    }
}

control swtch(inout Headers_t headers, in wp4_input wp4in, out wp4_output wp4out){
    apply {

    }
}
control deprs(in Headers_t headers, packet_out p, in wp4_output wp4out){
    apply {
    
    }
}

WP4Switch(prs(), swtch(), deprs())main;
