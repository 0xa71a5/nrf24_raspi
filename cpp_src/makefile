nrf24.so: nrf24l01.cpp
	g++ -o nrf24.so -shared -fPIC nrf24l01.cpp -I. -lwiringPi -fpermissive -w

nrf24_test: nrf24l01.o nrf24_test.cpp
	g++ nrf24l01.o nrf24_test.cpp -o nrf24_test -I. -lwiringPi -fpermissive -w

nrf24l01.o: nrf24l01.cpp nrf24l01.h
	g++ -c nrf24l01.cpp -I. -lwiringPi -fpermissive -w

regtool: regtool.cpp nrf24l01.o
	g++ nrf24l01.o regtool.cpp -o regtool -I. -lwiringPi -fpermissive -w

clean:
	rm -rf *.o *.so
