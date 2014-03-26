CXX = g++
CXXFLAGS = -g -O2 -Wall -fPIC
LD = g++
LDFLAGS =

all: libagsteam.so

clean:
	rm -f libagsteam.so *.o test

runtest: test
	LD_LIBRARY_PATH=. ./$<

test: libagsteam.so test.cpp
	$(LD) $(LDFLAGS) -o $@ -L. -lagsteam $^

libagsteam.so: agsteam.o
	$(LD) $(LDFLAGS) -shared -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $^
