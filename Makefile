# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -pthread
TFLAGS = -fgnu-tm

# Target executables
TARGETS = cuckoo_seq cuckoo_seq_v2 cuckoo_con cuckoo_con_v2 cuckoo_trans

all: $(TARGETS)

cuckoo_seq: cuckoo_seq.cpp
	$(CXX) $(CXXFLAGS) cuckoo_seq.cpp -o cuckoo_seq

cuckoo_seq_v2: cuckoo_seq_v2.cpp
	$(CXX) $(CXXFLAGS) cuckoo_seq_v2.cpp -o cuckoo_seq_v2

cuckoo_con: cuckoo_con.cpp
	$(CXX) $(CXXFLAGS) cuckoo_con.cpp -o cuckoo_con

cuckoo_con_v2: cuckoo_con_v2.cpp
	$(CXX) $(CXXFLAGS) cuckoo_con_v2.cpp -o cuckoo_con_v2

cuckoo_trans: cuckoo_trans.cpp
	$(CXX) $(CXXFLAGS) $(TFLAGS) cuckoo_trans.cpp -o cuckoo_trans

# Run selected executables
run: $(TARGETS)
	./cuckoo_seq
	./cuckoo_con_v3
	./cuckoo_trans

# Clean up generated files
clean:
	rm -f $(TARGETS) cuckoo_trans