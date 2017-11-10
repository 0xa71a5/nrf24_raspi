#encoding=utf-8
#author:oxa71a5
#date:2017/11/07
import time
import os
import lxc_nrf24
from IOT_Devices.DHT_Sensor import *
from IOT_Devices.AirconditionController import *
import json
import tornado.httpserver
import tornado.web
import tornado.ioloop
import sys
reload(sys)
sys.setdefaultencoding("utf8")

class IOT:
    def __init__(self):
        self.max_timeout = 10 #最多十次重发
        self.machine = lxc_nrf24.nrf24(my_addr = "mac00",channel = 12)
        self.machine.begin()
        while( self.machine.available() ): recv = self.machine.read_str() #Flush RX buffer

    def __init__(self,my_addr):
        self.max_timeout = 10 #最多十次重发
        self.machine = lxc_nrf24.nrf24(my_addr = my_addr,channel = 12)
        self.machine.begin()
        while( self.machine.available() ): recv = self.machine.read_str() #Flush RX buffer

    def flushRxBuffer(self):
        while( self.machine.available() ): recv = self.machine.read_str() #Flush RX buffer

    def communicateToNode(self,machineId,typeVal,contentVal): #与指定节点进行通信
        toSendPacket = "{},{}:{}".format(machineId,typeVal,contentVal)
        self.flushRxBuffer()
        timeout = 0
        while( self.machine.available() == False and timeout< self.max_timeout ):
            self.machine.send_to(toSendPacket,machineId)
            check_point = time.time() #记录发射时间点
            while( self.machine.available()==False ):
                if(time.time() - check_point > 0.01): #10ms试一次超时周期
                    timeout+=1 
                    break #超时一次
        if( self.machine.available() ):
            data = self.machine.read_str()
            sender_addr = data[0] #获取接受方的地址
            sender_payload = json.loads("{"+",".join(["\"{}\":\"{}\"".format(x.split(":")[0],x.split(":")[1]) for x in data.split(",")[1:]])+"}")
            if(sender_addr == machineId[-1]): #接收方地址与实际发送地址相符合
                return (True,sender_payload,)
        print "Communicate to node failed!"
        return (False,None)


class IndexHandler(tornado.web.RequestHandler):
    def get(self):
        self.render("index.html")

    def post(self):
        self.write("")

class ApiHandler(tornado.web.RequestHandler):
    def get(self):
        self.post()

    def post(self):
        global dhtSensors
        global airCondition1

        postdata = self.request.body.decode('utf-8')
        postdata = json.loads(postdata)
        typeVal  = postdata["Type"]
        contentVal = postdata["Content"]
        returnData = {
            "Type":"",
            "Content":{
                
            }
        }
        deviceId = 0 #目标设备ID
        #deviceId = deviceId if(deviceId< len(dhtSensors)) else len(dhtSensors)-1 #判断设备ID是否合法
        if(typeVal == "getDhtStatus"):
            status = dhtSensors[deviceId].getOnlineStatus()
            returnData["Type"] = "getDhtStatusResult"
            returnData["Content"]["status"] = status
        elif( typeVal == "getTemperature" ):
            temperature = dhtSensors[deviceId].getTemperature()
            returnData["Type"] = "getTemperatureResult"
            returnData["Content"]["temperature"] = temperature
        elif( typeVal == "getHumidity" ):
            humidity = dhtSensors[deviceId].getHumidity()
            returnData["Type"] = "getHumidityResult"
            returnData["Content"]["humidity"] = humidity
        # 下面是空调控制器
        elif(typeVal == "getAcStatus"):
            status = airCondition1.getOnlineStatus()
            returnData["Type"] = "getAcStatusResult"
            returnData["Content"]["status"] = status
        elif(typeVal == "turnOnAc"):
            status = airCondition1.turnOnAc()
            returnData["Type"] = "turnOnAcResult"
            returnData["Content"]["result"] = status
        elif(typeVal == "turnOffAc"):
            status = airCondition1.turnOffAc()
            returnData["Type"] = "turnOffAcResult"
            returnData["Content"]["result"] = status
        elif(typeVal == "setAcTemperature"):
            temperature = contentVal["temperature"]
            status = airCondition1.setAcTemperature(temperature)
            returnData["Type"] = "setAcTemperatureResult"
            returnData["Content"]["result"] = status
        self.write(json.dumps(returnData))

if __name__ == '__main__':
    global myIOT
    global dhtSensors
    global airCondition1

    myIOT = IOT("mac00")
    dhtSensor1 = DHT_Sensor( IOT_Center = myIOT,machineId = "mac01" )
    airCondition1 = AirconditionController( IOT_Center = myIOT,machineId = "mac02" )
    dhtSensors = [dhtSensor1,]


    app = tornado.web.Application([
        ('/', IndexHandler),
        ('/api', ApiHandler)
        ],cookie_secret='abcdswweww12314eqwdsgerhtrhw2!!wsws2',
        template_path=os.path.join(os.path.dirname(__file__), "."),
    )
    print "Running..."
    app.listen(80)
    tornado.ioloop.IOLoop.instance().start()