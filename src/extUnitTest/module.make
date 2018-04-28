.PHONY: test

test: all
	./$(PROG) -q "utest"

CXXFLAGS += -std=c++11
SRC +=	src/extUnitTest/main_test.cc

