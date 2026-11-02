# Host-side unit tests for the PCR classes:  make test
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror
SRC := pcr_control/Thermistor.cpp pcr_control/PidController.cpp pcr_control/HeaterDriver.cpp pcr_control/CycleSequencer.cpp

test: $(SRC) test/test_pcr.cpp
	$(CXX) $(CXXFLAGS) $(SRC) test/test_pcr.cpp -o test_pcr
	./test_pcr

clean:
	rm -f test_pcr
.PHONY: test clean
