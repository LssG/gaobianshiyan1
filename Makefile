.PHONY:all clean

all: fuwuqi kehuduan

fuwuqi:fwq.cpp
	g++ -o $@ fwq.cpp -lpthread

kehuduan:khd.cpp
	g++ -o $@ khd.cpp -lpthread

clean:
	rm fuwuqi kehuduan
	