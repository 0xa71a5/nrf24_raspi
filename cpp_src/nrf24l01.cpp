#include <stdio.h>
#include <iostream>
#include <string.h>
#include <wiringPiSPI.h>
#include <unistd.h>
#include <wiringPi.h>
using namespace std;
#include <nrf24l01.h>

uint8_t PTX=0;
uint8_t channel=0;
uint8_t payload=32;


void nrf_init() 
{   
    wiringPiSetup();
    wiringPiSPISetup(SPI_CHANNEL,500000);//Initialize spi
    pinMode(CE_PIN,OUTPUT);
    ceLow();
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
    writeRegister(RX_ADDR_P0,adr,5);
    writeRegister(TX_ADDR,adr,5);
}

extern bool dataReady() // Checks if data is available for reading
{
    uint8_t status = getStatus();
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
    uint8_t *temp_buffer = (uint8_t *)malloc(payload+1);
    memset(temp_buffer,0,payload+1);
    temp_buffer[0] = R_RX_PAYLOAD;
    wiringPiSPIDataRW(SPI_CHANNEL,temp_buffer,payload+1);
    memcpy(data,temp_buffer+1,payload);
    configRegister(STATUS,(1<<RX_DR));   // Reset status register
    free(temp_buffer);
}

void configRegister(uint8_t reg, uint8_t value)
{
    uint8_t temp_buffer[2];
    temp_buffer[0] = (W_REGISTER | (REGISTER_MASK & reg));
    temp_buffer[1] = value;
    wiringPiSPIDataRW(SPI_CHANNEL,temp_buffer,2);
}

void readRegister(uint8_t reg, uint8_t * value, uint8_t len)// Reads an array of bytes from the given start position in the MiRF registers.
{
    uint8_t *temp_buffer = (uint8_t *)malloc(len+1);
    memcpy(temp_buffer+1,value,len);
    temp_buffer[0] = (R_REGISTER | (REGISTER_MASK & reg));
    wiringPiSPIDataRW(SPI_CHANNEL,temp_buffer,len+1);
    memcpy(value,temp_buffer+1,len);
    free(temp_buffer);
}

void writeRegister(uint8_t reg, uint8_t * value, uint8_t len) // Writes an array of bytes into inte the MiRF registers.
{
    uint8_t *temp_buffer = (uint8_t *)malloc(len+1);
    temp_buffer[0] = (W_REGISTER | (REGISTER_MASK & reg));
    memcpy(temp_buffer+1,value,len);
    wiringPiSPIDataRW(SPI_CHANNEL,temp_buffer,len+1);
    free(temp_buffer);
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

    uint8_t temp = FLUSH_TX;
    wiringPiSPIDataRW(SPI_CHANNEL,&temp,1);

    uint8_t *temp_buffer = (uint8_t *) malloc(payload+1);
    temp_buffer[0] = W_TX_PAYLOAD;
    memcpy ( temp_buffer+1 , value , payload);
    wiringPiSPIDataRW(SPI_CHANNEL,temp_buffer,payload+1);

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
    //csnLow();
    //spi_transfer(FLUSH_RX);
    //csnHi();
    uint8_t temp = FLUSH_RX;
    wiringPiSPIDataRW(SPI_CHANNEL,&temp,1);
}

void powerUpTx(){
        PTX = 1;
        configRegister(CONFIG, mirf_CONFIG | ( (1<<PWR_UP) | (0<<PRIM_RX) ) );
}

void ceHi(){
        digitalWrite(CE_PIN,1);
}

void ceLow(){
        digitalWrite(CE_PIN,0);
}

void powerDown(){
        ceLow();
        configRegister(CONFIG, mirf_CONFIG );
}


void setChannel(int channel_)
{
    channel = channel_;
    configRegister(RF_CH,channel);
}
void setPayloadLength(int length)
{
    payload = length;
    configRegister(RX_PW_P0, payload);
    configRegister(RX_PW_P1, payload);
}

extern "C"{

  void nrf24_setup(uint8_t *my_addr,int channel)
  {
    setChannel(channel);
    nrf_init();
    setRADDR(my_addr);
    setPayloadLength(32);
    nrf_config();
  }

  void nrf24_tx_addr(uint8_t *target_addr)
  {
    setTADDR(target_addr);
  }

  void nrf24_send(uint8_t *data)
  {
    nrf_send(data);
  }

  uint8_t nrf24_available()
  {
    return dataReady();
  }

  void nrf24_read(uint8_t *data)
  {
      getData(data);
  }

  void nrf_test(uint8_t *data)
  {
    uint8_t *t = "hello";
    memcpy(data,t,strlen(t)+1);
  }

}