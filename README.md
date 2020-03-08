# WP4
Compiler extension for creating Wireless P4 Data Planes for OpenWRT.

## Installation

### Ubuntu dependencies

Most dependencies can be installed using `apt-get install`:

`sudo apt-get install cmake g++ git automake libtool libgc-dev bison flex
libfl-dev libgmp-dev libboost-dev libboost-iostreams-dev
libboost-graph-dev llvm pkg-config python python-scapy python-ipaddr python-ply
tcpdump`

For documentation building:
`sudo apt-get install -y doxygen graphviz texlive-full`

### Install protobuf 3.2.0
```bash
git clone https://github.com/google/protobuf.git`
git checkout v3.2.0`
./autogen.sh`
./configure`
make`
make check`
sudo make install`
sudo ldconfig # refresh shared library cache.`
```

### P4-16 Compiler
First you need to follow the installation guide of [P4-16](https://github.com/p4lang/p4c/)
When you have P4-16 compiler, then add this project as an extension.
Assuming you have P4-16 at your dir  ~/p4c/, to setup p4c-wp4:
```bash
cd ~/p4c/
mkdir extensions
cd extensions
git clone https://github.com/pzanna/p4c-wp4.git
ln -s ~/p4c p4c-wp4/p4c
```
Now that you have cloned p4c-WP4 at ~/p4c/extensions/p4c-wp4, the next step is to
recompile p4c:
```bash
cd ~/p4c/
mkdir -p build
cd build/
cmake ..
make
```
This generates a p4c-wp4 binary in ~/p4c/build/extensions/p4c-wp4.
To build the test P4 program:
```bash
cd ~/p4c/extensions/p4c-wp4/tests
~/p4c/build/extensions/p4c-wp4/p4c-wp4 test_wp4.p4 -o wp4-p4.c
```

### Current Status
This is the first version of this extension and very much a work in progress to don't expect too much at the start with, more functionality will be added over time. On that note, any assistance would be extremely welcome.