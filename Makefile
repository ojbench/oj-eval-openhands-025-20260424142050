CXXFLAGS=-g -O2 -MMD -std=c++17
BINS=eval cheat anticheat code
SRCS=lang.cpp eval.cpp cheat.cpp anticheat.cpp main.cpp
SUBMITFILES=cheat-submit.cpp anticheat-submit.cpp
OBJS=$(SRCS:.cpp=.o)
DEPS=$(SRCS:.cpp=.d)

all: $(BINS) $(SUBMITFILES)
eval: eval.o lang.o
	$(CXX) -o $@ $^
cheat: cheat.o lang.o
	$(CXX) -o $@ $^
anticheat: anticheat.o lang.o
	$(CXX) -o $@ $^
code: main.o lang.o
	$(CXX) -o $@ $^

$(SUBMITFILES): %-submit.cpp: %.cpp %
	rm -f $@
	echo '#include <bits/stdc++.h>' > $@
	cpp -imacros bits/stdc++.h -include lang.cpp $< | sed -E 's/^#.+$$//;/^\s*$$/d' >> $@

.PHONY: clean
clean:
	rm -f $(BINS) $(OBJS) $(DEPS) $(SUBMITFILES)

-include *.d
