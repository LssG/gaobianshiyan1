.PHONY:all clean

all: fuwuqi kehuduan

fuwuqi:01test2_khd.cpp
	g++ -o $@ 02test2_fwq.cpp

kehuduan:02test2_fwq.cpp
	g++ -o $@ 01test2_khd.cpp

clean:
	rm fuwuqi kehuduan
	