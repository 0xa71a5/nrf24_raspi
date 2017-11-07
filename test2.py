#encoding=utf-8
#author:oxa71a5
#date:2017/11/07
import time
import lxc_nrf24

machine = lxc_nrf24.nrf24(my_addr = "clie1",channel = 12)
machine.begin()
#machine.set_target_addr("serv1")
while True:
    words = raw_input("Enter:")
    if(words == "exit" ):break
    machine.send_to(words,"serv1")
    check_point = time.time()
    while( machine.available()==False ):
        if(time.time() - check_point > 0.5):break
    while( machine.available() ):
        recv = machine.read_str()
        print "Recv:",recv