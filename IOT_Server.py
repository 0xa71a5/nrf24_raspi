#encoding=utf-8
#author:oxa71a5
#date:2017/11/07
import time
import os
import lxc_nrf24
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
        return (False,None)


class DHT_Sensor: #温湿度传感器
    def __init__(self,IOT_Center,machineId):
        self.IOT = IOT_Center #获取通信接口
        self.machineId = machineId #获取本传感器的通信地址

    def getOnlineStatus(self): #获取传感器的在线状态
        result,data = self.IOT.communicateToNode(self.machineId,"get","status")
        return data["status"] if(result) else "offline" #如果获取到通信结果  那么返回当前状态

    def getHumidity(self): #获取传感器的湿度数值
        result,data = self.IOT.communicateToNode(self.machineId,"get","humidity")
        return data["humidity"] if(result) else ""   

    def getTemperature(self):
        result,data = self.IOT.communicateToNode(self.machineId,"get","temperature")
        return data["temperature"] if(result) else ""


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
        postdata = self.request.body.decode('utf-8')
        postdata = json.loads(postdata)
        typeVal  = postdata["Type"]
        contentVal = postdata["Content"]
        returnData = {
            "Type":"",
            "Content":{
                
            }
        }
        deviceId = int(contentVal["deviceId"]) #目标设备ID
        deviceId = deviceId if(deviceId< len(dhtSensors)) else len(dhtSensors)-1 #判断设备ID是否合法
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
        self.write(json.dumps(returnData))

if __name__ == '__main__':
    global myIOT
    global dhtSensors


    myIOT = IOT("mac00")
    dhtSensor1 = DHT_Sensor( IOT_Center = myIOT,machineId = "mac01" )
    dhtSensor2 = DHT_Sensor( IOT_Center = myIOT,machineId = "mac02" )
    dhtSensors = [dhtSensor1,dhtSensor2]


    app = tornado.web.Application([
        ('/', IndexHandler),
        ('/api', ApiHandler)
        ],cookie_secret='abcdswweww12314eqwdsgerhtrhw2!!wsws2',
        template_path=os.path.join(os.path.dirname(__file__), "."),
    )
    print "Running..."
    app.listen(80)
    tornado.ioloop.IOLoop.instance().start()