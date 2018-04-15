#encoding = utf-8
#author: 0xa71a5
#date:2018/03/11
import Machine_Id

class Iot_Interface:
    def __init__(self):
        self.SUCCESS = 0
        self.ERROR   = 1
        pass

    def turn_on_light(self,machine_id=0):
        return (self.SUCCESS,())

    def turn_off_light(self,machine_id=0):
        pass
        return (self.SUCCESS,())

    def unlock_door(self,machine_id=0):
        pass
        return (self.SUCCESS,())

    def lock_door(self,machine_id=0):
        pass
        return (self.SUCCESS,())

    def turn_on_ac(self,machine_id=0):
        pass
        return (self.SUCCESS,())

    def turn_off_ac(self,machine_id=0):
        pass
        return (self.SUCCESS,())

    def query_temperature(self,machine_id=0):
        pass
        return (self.SUCCESS,(-1,))

    def query_humidity(self,machine_id=0):
        pass 
        return (self.SUCCESS,(-1,))

    def turn_on_ec(self,machine_id=0):
        pass 
        return (self.SUCCESS,(-1,))

    def turn_off_ec(self,machine_id=0):
        pass
        return (self.SUCCESS,(-1,))

if __name__ == "__main__":
    interface = Iot_Interface()
    interface.turn_on_light( Machine_Id.Desklamp0 )
    interface.turn_off_light( Machine_Id.Desklamp0 )
    interface.turn_off_ac()
    interface.query_temperature( Machine_Id.Temperature0 )










