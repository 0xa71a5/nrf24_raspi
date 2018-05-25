#include <lxc_nrf24l01.h>

char my_mac_addr = '2' ; 
#define PWM_PIN 5

#define TIMER_DISABLE_INTR  (TIMSK2 = 0)
#define SYSCLOCK  16000000 
#define TIMER_CONFIG_KHZ(val) ({ \
  const uint8_t pwmval = SYSCLOCK / 2000 / (val); \
  TCCR2A               = _BV(WGM20); \
  TCCR2B               = _BV(WGM22) | _BV(CS20); \
  OCR2A                = pwmval; \
  OCR2B                = pwmval / 3; \
})
#define TIMER_ENABLE_PWM    (TCCR2A |= _BV(COM2B1))
#define TIMER_DISABLE_PWM   (TCCR2A &= ~(_BV(COM2B1)))

void  mark(unsigned int time)
{
  TIMER_ENABLE_PWM; // Enable pin 3 PWM output
  if (time > 0) custom_delay_usec(time);
}

void  space(unsigned int time)
{
  TIMER_DISABLE_PWM; // Disable pin 3 PWM output
  if (time > 0) custom_delay_usec(time);
}

void custom_delay_usec(unsigned long uSecs) 
{
  if (uSecs > 4) {
    unsigned long start = micros();
    unsigned long endMicros = start + uSecs - 4;
    if (endMicros < start) { // Check if overflow
      while ( micros() > start ) {} 
    }
    while ( micros() < endMicros ) {} 
  } 
}

void sendpresumable()
{
  mark(9000);
  space(4500);
}   

void send0()
{
  mark(560);
  space(565);
}

void send1()
{
  mark(560);
  space(1690);
}

void send_gree(byte ircode, byte len)
{
  byte mask = 0x01;
  for (int i = 0; i < len; i++)
  {
    if (ircode & mask)
      send1();
    else
      send0();
    mask <<= 1;
  }
}


void sendCode(uint8_t x0,uint8_t x1,uint8_t x2)
{
   TIMER_DISABLE_INTR; //Timer2 Overflow Interrupt
  pinMode(3, OUTPUT);//PWM端口
  digitalWrite(3, LOW); //PWM端口
  digitalWrite(4, LOW);  
  TIMER_CONFIG_KHZ(38);//PWM端口频率KHz
  sendpresumable();
  //send_gree(0x31, 8);//000自动 100制冷 010除湿 110送风 001制热； 1开；00自动 10小风 01中风 11大风；0扫风；0睡眠//关
  send_gree(x0, 8);//000自动 100制冷 010除湿 110送风 001制热； 1开；00自动 10小风 01中风 11大风；0扫风；0睡眠//开
  send_gree(x1, 8);//温度0000-16度 1000-17度 1100-18度.... 0111-30度 低位在后
  send_gree(0x20, 8);
  send_gree(0x50, 8);
  send_gree(0x02, 3);
  mark(560);
  space(10000);
  space(10000);
  send_gree(0x00, 8);
  send_gree(0x21, 8);
  send_gree(0x00, 8);
  send_gree(x2<<4, 8);
  mark(560);
  space(0);
  digitalWrite(4, HIGH); 
}

void _turnOnAC()
{
  sendCode(0x0c,0x0b,0x03);
}

void turnOnAC()
{
  analogWrite(PWM_PIN, 127);
}

void _turnOffAC()
{
  sendCode(0x04,0x0b,0x0b);
}

void turnOffAC()
{
  analogWrite(PWM_PIN, 0);
}

void setTemperature(uint32_t t)
{
  if (t > 30)
    t = 30;
  if (t < 16)
    t = 16;

  t = (t - 16) * 255 / (30 - 16);
  analogWrite(PWM_PIN, t);
}

void _setTemperature(int t)
{
  if(t<16)t=16;
  if(t>30)t=30;
  uint8_t code2 = t-16;
  uint8_t code3 = t-24;
  sendCode(0x0c, code2 ,code3);
}

void ac_functional_handler(String type,String content,uint8_t senderId=0)
{
  String packet;
  if (type == "get")//getVal1
  {
    if(content == "status")
    {
      construct_format(packet, "status", "online");
      nrf_send(packet.c_str());
    }
  }
  else if (type == "changeAcTemp")
  {
    String newTemp = content;
    int newTempInt = atoi(newTemp.c_str());

    construct_format(packet, "changeAcTempResult", "suc");
    nrf_send(packet.c_str());
    setTemperature(newTempInt);
  }
  else if (type == "turnOnAc")
  {
    construct_format(packet, "turnOnAcResult", "suc");
    nrf_send(packet.c_str());
    turnOnAC();
  }
  else if (type == "turnOffAc")
  {
    construct_format(packet, "turnOffAcResult", "suc");
    nrf_send(packet.c_str());
    turnOffAC();
  }
}

void device_init()
{
  /*
  pinMode(4,OUTPUT);
  digitalWrite(4, HIGH);
  turnOnAC();
  delay(600);
  turnOffAC();
  */
  //pinMode(LED_PIN, OUTPUT);
  turnOffAC();
}

void setup()
{
  Serial.begin(2000000);
  Serial.println("Begin config!");
  handle_func_register(ac_functional_handler);
  nrf_gpio_init(8, 9);
  set_tx_addr((uint8_t *)"mac00");
  my_mac_addr = '2' ; 
  set_mac_addr(&my_mac_addr);
  nrf_chip_config(12, 32);
  Serial.println("Device ac is running!");
  device_init();
}

void loop()
{
   char data[32];
   if (data_ready()){
      get_data(data);
      Serial.print(" Got packet->");
      Serial.println(data);
      String str_data = data;
      handle_packet(str_data);
  }
}

