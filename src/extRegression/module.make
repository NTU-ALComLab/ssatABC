.PHONY: test

test: all
	./$(PROG) -c "regression-test"

CXXFLAGS += -std=c++11
SRC +=	src/extRegression/regression.cc

