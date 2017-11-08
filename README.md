# nrf24_raspi
This is a project for using nrf24l01 in raspberry pi (2 or 3)  
You can build wireless communication with my code whether in c ,c++ or python.  
To begin with python,you can follow the way which test2.py does.  
To begin with c or c++,you can follow the way which testTx.cpp does.  
<br>
<br>
### Wire connection:
<pre>
Nrf24L01      RaspberryPi      
CE             GPIO6  
CSN            ce0 (GPIO10)  
MOSI           MOSI(GPIO12)  
MISO           MISO(GPIO13)  
CSK            SCK (GPIO14)  
</pre>
