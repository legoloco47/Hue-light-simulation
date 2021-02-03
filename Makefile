# MAKEFILE

# C++ Compiler (Default: g++)
CXX = g++

# Librarys
INCLUDE = -Iusr/local/include
LDFLAGS = -Lusr/local/lib 
LDLIBS = -lcurl

# Details
SOURCES = HUELightSimulator.cpp
OUT = test

# HUELightSimulation: HUELightSimulator.o
# 	g++ HUELightSimulator.o -o HUELightSimulation

HUELightSimulator: HUELightSimulator.cpp HUELightSimulator.h
	g++ -o  HUELightSimulation -std=c++11 $(INCLUDE) $(LDFLAGS) $(LDLIBS) HUELightSimulator.cpp 

clean: 
	rm *.o HUELightSimulation
