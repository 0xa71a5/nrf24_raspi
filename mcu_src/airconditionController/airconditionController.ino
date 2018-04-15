#include <SPI.h>
#include <stdlib.h>
#define CONFIG      0x00
#define EN_AA       0x01
#define EN_RXADDR   0x02
#define SETUP_AW    0x03
#define SETUP_RETR  0x04
#define RF_CH       0x05
#define RF_SETUP    0x06
#define STATUS      0x07
#define OBSERVE_TX  0x08
#define CD          0x09
#define RX_ADDR_P0  0x0A
#define RX_ADDR_P1  0x0B
#define RX_ADDR_P2  0x0C
#define RX_ADDR_P3  0x0D
#define RX_ADDR_P4  0x0E
#define RX_ADDR_P5  0x0F
#define TX_ADDR     0x10
#define RX_PW_P0    0x11
#define RX_PW_P1    0x12
#define RX_PW_P2    0x13
#define RX_PW_P3    0x14
#define RX_PW_P4    0x15
#define RX_PW_P5    0x16
#define FIFO_STATUS 0x17

/* Bit Mnemonics */
#define MASK_RX_DR  6
#define MASK_TX_DS  5
#define MASK_MAX_RT 4
#define EN_CRC      3
#define CRCO        2
#define PWR_UP      1
#define PRIM_RX     0
#define ENAA_P5     5
#define ENAA_P4     4
#define ENAA_P3     3
#define ENAA_P2     2
#define ENAA_P1     1
#define ENAA_P0     0
#define ERX_P5      5
#define ERX_P4      4
#define ERX_P3      3
#define ERX_P2      2
#define ERX_P1      1
#define ERX_P0      0
#define AW          0
#define ARD         4
#define ARC         0
#define PLL_LOCK    4
#define RF_DR       3
#define RF_PWR      1
#define LNA_HCURR   0        
#define RX_DR       6
#define TX_DS       5
#define MAX_RT      4
#define RX_P_NO     1
#define TX_FULL     0
#define PLOS_CNT    4
#define ARC_CNT     0
#define TX_REUSE    6
#define FIFO_FULL   5
#define TX_EMPTY    4
#define RX_FULL     1
#define RX_EMPTY    0

/* Instruction Mnemonics */
#define R_REGISTER    0x00
#define W_REGISTER    0x20
#define REGISTER_MASK 0x1F
#define R_RX_PAYLOAD  0x61
#define W_TX_PAYLOAD  0xA0
#define FLUSH_TX      0xE1
#define FLUSH_RX      0xE2
#define REUSE_TX_PL   0xE3
#define NOP           0xFF

#define mirf_ADDR_LEN  5
#define mirf_CONFIG ((1<<EN_CRC) | (0<<CRCO) )

void nrf_init();
void nrf_config();
void nrf_send(uint8_t *value);
void setRADDR(uint8_t * adr);
void setTADDR(uint8_t * adr);
bool dataReady();
bool isSending();
bool rxFifoEmpty();
bool txFifoEmpty();
void getData(uint8_t * data);
uint8_t getStatus();
void transmitSync(uint8_t *dataout,uint8_t len);
void transferSync(uint8_t *dataout,uint8_t *datain,uint8_t len);
void configRegister(uint8_t reg, uint8_t value);
void readRegister(uint8_t reg, uint8_t * value, uint8_t len);
void writeRegister(uint8_t reg, uint8_t * value, uint8_t len);
void powerUpRx();
void powerUpTx();
void powerDown();
void csnHi();
void csnLow();
void ceHi();
void ceLow();
void flushRx();
uint8_t spi_transfer(uint8_t data);



uint8_t PTX=0;
uint8_t cePin=13;
uint8_t csnPin=9;
uint8_t channel=0;
uint8_t payload=32;

char myAddessByte = '2' ; 

uint8_t spi_transfer(uint8_t data)
{
    return SPI.transfer(data);
}

//This is a new test function for spi transfer
void transferSync(uint8_t *dataout,uint8_t *datain,uint8_t len){
        uint8_t i;
        for(i = 0;i < len;i++){
                datain[i] = spi_transfer(dataout[i]);
        }
}

//This is a new test function for spi transfer
void transmitSync(uint8_t *dataout,uint8_t len){
        uint8_t i;
        for(i = 0;i < len;i++){
                spi_transfer(dataout[i]);
        }
}

void nrf_init() 
{   
    pinMode(cePin,OUTPUT);
    pinMode(csnPin,OUTPUT);

    ceLow();
    csnHi();
    SPI.begin();
}


void nrf_config() 
{
    configRegister(RF_CH,channel);
    // Set length of incoming payload 
    configRegister(RX_PW_P0, payload);
    configRegister(RX_PW_P1, payload);
    configRegister(EN_AA, 0x00);//disable shockburst mode:auto ack
    // Start receiver 
    powerUpRx();
    flushRx();
}

void setRADDR(uint8_t * adr) // Sets the receiving address
{
        ceLow();
        writeRegister(RX_ADDR_P1,adr,5);
        ceHi();
}

void setTADDR(uint8_t * adr) // Sets the transmitting address
{
    /*
     * RX_ADDR_P0 must be set to the sending addr for auto ack to work.
     */
    writeRegister(RX_ADDR_P0,adr,5);
    writeRegister(TX_ADDR,adr,5);
}

extern bool dataReady() // Checks if data is available for reading
{
    // See note in getData() function - just checking RX_DR isn't good enough
        uint8_t status = getStatus();
    // We can short circuit on RX_DR, but if it's not set, we still need
    // to check the FIFO for any pending packets
    if ( status & (1 << RX_DR) ) return 1;
    return !rxFifoEmpty();
}

extern bool rxFifoEmpty(){
        uint8_t fifoStatus;
        readRegister(FIFO_STATUS,&fifoStatus,sizeof(fifoStatus));
        return (fifoStatus & (1 << RX_EMPTY));
}

extern void getData(uint8_t * data) // Reads payload bytes into data array
{
    csnLow();                               // Pull down chip select
    spi_transfer( R_RX_PAYLOAD );
    transferSync(data,data,payload); // Read payload
    csnHi();                               // Pull up chip select
    // NVI: per product spec, p 67, note c:
    //  "The RX_DR IRQ is asserted by a new packet arrival event. The procedure
    //  for handling this interrupt should be: 1) read payload through SPI,
    //  2) clear RX_DR IRQ, 3) read FIFO_STATUS to check if there are more 
    //  payloads available in RX FIFO, 4) if there are more data in RX FIFO,
    //  repeat from step 1)."
    // So if we're going to clear RX_DR here, we need to check the RX FIFO
    // in the dataReady() function
    configRegister(STATUS,(1<<RX_DR));   // Reset status register
}

void configRegister(uint8_t reg, uint8_t value)
// Clocks only one byte into the given MiRF register
{
    csnLow();
    spi_transfer(W_REGISTER | (REGISTER_MASK & reg));
    spi_transfer(value);
    csnHi();
}

void readRegister(uint8_t reg, uint8_t * value, uint8_t len)// Reads an array of bytes from the given start position in the MiRF registers.
{
    csnLow();
    spi_transfer(R_REGISTER | (REGISTER_MASK & reg));
    transferSync(value,value,len);
    csnHi();
}

void writeRegister(uint8_t reg, uint8_t * value, uint8_t len) // Writes an array of bytes into inte the MiRF registers.
{
    csnLow();
    spi_transfer(W_REGISTER | (REGISTER_MASK & reg));
    transmitSync(value,len);
    csnHi();
}

void nrf_send(uint8_t * value) // Sends a data package to the default address. Be sure to send the correct
{
// amount of bytes as configured as payload on the receiver.
    uint8_t status;
    status = getStatus();
    while (PTX) {
            status = getStatus();

            if((status & ((1 << TX_DS)  | (1 << MAX_RT)))){
                    PTX = 0;
                    break;
            }
    }                  // Wait until last paket is send
    ceLow();
    powerUpTx();       // Set to transmitter mode , Power up
    csnLow();                    // Pull down chip select
    spi_transfer(FLUSH_TX);
    csnHi();                    // Pull up chip select
    csnLow();                    // Pull down chip select
    spi_transfer( W_TX_PAYLOAD );
    transmitSync(value,payload);   // Write payload
    csnHi();                    // Pull up chip select
    ceHi();                     // Start transmission
    while(isSending());
}
bool isSending(){
        uint8_t status;
        if(PTX){
                status = getStatus();
                /*
                 *  if sending successful (TX_DS) or max retries exceded (MAX_RT).
                 */
                if((status & ((1 << TX_DS)  | (1 << MAX_RT)))){
                        powerUpRx();
                        return false; 
                }
                return true;
        }
        return false;
}

uint8_t getStatus(){
        uint8_t rv;
        readRegister(STATUS,&rv,1);
        return rv;
}

void powerUpRx(){
        PTX = 0;
        ceLow();
        configRegister(CONFIG, mirf_CONFIG | ( (1<<PWR_UP) | (1<<PRIM_RX) ) );
        ceHi();
        configRegister(STATUS,(1 << TX_DS) | (1 << MAX_RT)); 
}

void flushRx(){
    csnLow();
    spi_transfer(FLUSH_RX);
    csnHi();
}

void powerUpTx(){
        PTX = 1;
        configRegister(CONFIG, mirf_CONFIG | ( (1<<PWR_UP) | (0<<PRIM_RX) ) );
}

void ceHi(){
        digitalWrite(cePin,HIGH);
}

void ceLow(){
        digitalWrite(cePin,LOW);
}

void csnHi(){
        digitalWrite(csnPin,HIGH);
}

void csnLow(){
        digitalWrite(csnPin,LOW);
}

void powerDown(){
        ceLow();
        configRegister(CONFIG, mirf_CONFIG );
}


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

void  mark (unsigned int time)
{
  TIMER_ENABLE_PWM; // Enable pin 3 PWM output
  if (time > 0) custom_delay_usec(time);
}

void  space (unsigned int time)
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

void sendGree(byte ircode, byte len)
{
  byte mask = 0x01;
  for(int i = 0;i < len;i++)
  {
    if (ircode & mask)
    {   send1();   }
    else
    {   send0();   }
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
  //sendGree(0x31, 8);//000自动 100制冷 010除湿 110送风 001制热； 1开；00自动 10小风 01中风 11大风；0扫风；0睡眠//关
  sendGree(x0, 8);//000自动 100制冷 010除湿 110送风 001制热； 1开；00自动 10小风 01中风 11大风；0扫风；0睡眠//开
  sendGree(x1, 8);//温度0000-16度 1000-17度 1100-18度.... 0111-30度 低位在后
  sendGree(0x20, 8);
  sendGree(0x50, 8);
  sendGree(0x02, 3);
  mark(560);
  space(10000);
  space(10000);
  sendGree(0x00, 8);
  sendGree(0x21, 8);
  sendGree(0x00, 8);
  sendGree(x2<<4, 8);
  mark(560);
  space(0);
  digitalWrite(4, HIGH); 
}

void turnOnAC()
{
  sendCode(0x0c,0x0b,0x03);
}

void turnOffAC()
{
  sendCode(0x04,0x0b,0x0b);
}

void setTemperature(int t)
{
  if(t<16)t=16;
  if(t>30)t=30;
  uint8_t code2 = t-16;
  uint8_t code3 = t-24;
  sendCode(0x0c, code2 ,code3);
}


void constructFormat(String &raw,String type,String value)
{
    if(raw.length()==0)
    {
      char temp[2]={0x00,0x00};
      temp[0] = myAddessByte;
      raw += temp;
    }
    raw += ","+type+":"+value;
}


void parsePack(String type,String content,uint8_t senderId=0)
{
  String packet;
  if(type=="get")//getVal1
  {
    if(content=="status")
    {
      constructFormat(packet,"status","online");
      nrf_send(packet.c_str());
    }    
  }
  else if(type=="changeAcTemp")
  {
    String newTemp = content;
    int newTempInt = atoi(newTemp.c_str());
    
    constructFormat(packet,"changeAcTempResult","suc");
    nrf_send(packet.c_str());
    setTemperature(newTempInt);
  }
  else if(type=="turnOnAc")
  {
    constructFormat(packet,"turnOnAcResult","suc");
    nrf_send(packet.c_str());
    turnOnAC();
  }
  else if(type=="turnOffAc")
  {
    constructFormat(packet,"turnOffAcResult","suc");
    nrf_send(packet.c_str());
    turnOffAC();
  }
}
void handlePacket(String input)
{
  if(input.length()<3)return;
  uint8_t sendId = input[0];
  String type,content;
  uint8_t state = 0;
  for(int i=2;i<input.length();i++)
  {
    char cur = input[i];
    if(cur==':')
    {
      state = 2;
    }
    else if(cur == ',')
    {
      state = 0;
      parsePack(type,content,sendId);
      type="";
      content="";
    }
    else
    {
      if(state == 0)
      {
        type += cur;
      }
      else if (state ==2)
      {
        content += cur;
      }
    }
  }
  parsePack(type,content,sendId);
}
void setup()
{
  pinMode(4,OUTPUT);
  digitalWrite(4, HIGH);
  turnOnAC();
  delay(1000);
  turnOffAC();
  
  char myaddr[5]="mac0";
  Serial.begin(250000);
  Serial.println("Begin config!");
  cePin = 8;
  csnPin = 9;
  channel = 12;
  nrf_init();
  setTADDR((byte *)"mac00");
  myaddr[4] = myAddessByte;
  setRADDR((byte *)myaddr);
  payload = 32;
  nrf_config();
  Serial.println("Begining!");
  
}


char data[32];
void loop()
{
  
   if(dataReady()){
      getData(data);
      Serial.print("Got packet->");
      Serial.println(data);
      String sData = data;
      handlePacket(sData);
  }
}

