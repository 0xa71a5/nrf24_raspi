#encoding=utf8
import thread
import time

class Doorlock: #门禁控制器
    def __init__(self,IOT_Center,machineId):
        self.IOT = IOT_Center #获取通信接口
        self.machineId = machineId #获取本控制器的通信地址

    def getOnlineStatus(self): #获取控制器的状态
        result,data = self.IOT.communicateToNode(self.machineId,"get","status")
        return data["status"] if(result) else "offline" #如果获取到通信结果  那么返回当前状态

    def getSwitchStatus(self): #获取物理开关的状态
        result,data = self.IOT.communicateToNode(self.machineId,"get","switchState")
        return data["status"] if(result) else "off" #如果获取到通信结果  那么返回当前状态       

    def turnOnPower(self, delay = 0): #允许开关
        time.sleep(delay)
        result,data = self.IOT.communicateToNode(self.machineId,"power","on")
        thread.start_new_thread(self.turnOffPower, (5,))
        return data["result"] if(result) else "" 

    def turnOffPower(self, delay = 0): #禁止开关
        time.sleep(delay)
        result,data = self.IOT.communicateToNode(self.machineId,"power","off")
        return data["result"] if(result) else ""  
