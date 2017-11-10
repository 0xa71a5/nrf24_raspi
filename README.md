# nrf24_raspi
#### Project Description
This is a project for using nrf24l01 in raspberry pi (2 or 3)  
You can build wireless communication with my code whether in c ,c++ or python.  
To begin with python,you can follow the way which test2.py does.  
To begin with c or c++,you can follow the way which testTx.cpp does.  
<br>
<br>

#### 项目介绍  
<pre>
本项目是为了在树莓派上提供nrf24l01无线芯片控制功能而设计。
你可以使用c、c++或者Python来调用本项目的API接口以实现在树莓派上使用Nrf24L01进行无线通信。
test2.py是使用Python进行芯片控制的一个demo  
testTx.cpp是使用c或c++ 进行芯片控制的一个demo

服务器端入口文件是:IOT_Server.py
每一个物联网节点的类实现在 IOT_Devices文件夹中
</pre>

#### Wire connection:
<pre>
Nrf24L01      RaspberryPi      
CE             GPIO6  
CSN            ce0 (GPIO10)  
MOSI           MOSI(GPIO12)  
MISO           MISO(GPIO13)  
CSK            SCK (GPIO14)  
</pre>
#### Nrf24L01 GPIO
![image](https://raw.githubusercontent.com/at1a5-lxc/nrf24_raspi/master/nrf24l01GPIO.jpg)

#### Raspberry Pi 2(3) GPIO
![image](https://raw.githubusercontent.com/at1a5-lxc/nrf24_raspi/master/raspberrypiGPIO.jpg)








