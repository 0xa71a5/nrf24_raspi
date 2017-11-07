# nrf24_raspi
<br>
This is a project for using nrf24l01 in raspberry pi (2 or 3)
You can build wireless communication with my code whether in c ,c++ or python.
<br>
To begin with python,you can follow the way which test2.py does.
To begin with c or c++,you can follow the way which testTx.cpp does.
<br>
Wire connection:
<br>
Nrf24L01      RaspberryPi    
CE             GPIO6
CSN            ce0 (GPIO10)
MOSI           MOSI(GPIO12)
MISO           MISO(GPIO13)
CSK            SCK (GPIO14)