#include <dht.h>
#include <lxc_nrf24l01.h>

#define DHT11_PIN 5
dht DHT;

uint8_t my_mac_addr = '1';

void dhtsensor_functional_handler(String type,String content,uint8_t senderId=0)
{
  String packet;
  if (type == "get")//getVal1
  {
    if (content == "status")
    {
      construct_format(packet, "status", "online");
      nrf_send(packet.c_str());
    }
    else if (content == "humidity")//time and val1
    {
      DHT.read11(DHT11_PIN);
      int humidityVal = DHT.humidity;
      construct_format(packet, "humidity", String(humidityVal));
      nrf_send(packet.c_str());
    }
    else if (content == "temperature")
    {
      DHT.read11(DHT11_PIN);
      int temperatureVal = DHT.temperature;
      construct_format(packet, "temperature", String(temperatureVal));
      nrf_send(packet.c_str());
    }
  }
}

void device_init()
{

}

void setup()
{
  Serial.begin(250000);
  Serial.println("Begin config!");
  handle_func_register(dhtsensor_functional_handler);
  nrf_gpio_init(8, 9);
  set_tx_addr((uint8_t *)"mac00");
  set_mac_addr(&my_mac_addr);
  nrf_chip_config(12, 32);
  Serial.println("Begining!");
}

void loop()
{
   char data[32];
   if (data_ready()) {
      get_data(data);
      Serial.print("Got packet->");
      Serial.println(data);
      String str_data = data;
      handle_packet(str_data);
  }
}
