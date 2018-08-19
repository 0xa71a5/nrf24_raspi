#include <stdio.h>
#include <iostream>
#include <string.h>
#include <wiringPiSPI.h>
#include <unistd.h>
#include <wiringPi.h>
using namespace std;
#include <nrf24l01.h>

static uint8_t in_tx_mode = 0;
static uint8_t channel = 0;
static uint8_t payload = 32;

#ifdef DEBUG_TEST
    #define debug_printf(...) printf( __VA_ARGS__)
#else
    #define debug_printf(...)
#endif

void nrf_init() 
{   
    wiringPiSetup();
    wiringPiSPISetup(SPI_CHANNEL,500000);/* Initialize rasperi pi spi */
    pinMode(CE_PIN,OUTPUT);
    ce_low();
}


void nrf_config() 
{
    config_register(RF_CH,channel);
    /* Set length of incoming payload */
    config_register(RX_PW_P0, payload);
    config_register(RX_PW_P1, payload);
    config_register(RX_PW_P2, payload);
    /* config_register(EN_AA, 0x00);//disable shockburst mode:auto ack */
    config_register(EN_AA, 0x00);/* Enable pipe0 and pipe1 auto ack*/
    config_register(EN_RXADDR, 0x03);/* Enable pipe0 1 2 rx */

    // Start receiver 
    powerup_rx();
    flush_rx();
}

void nrf_set_broadcast_addr(uint8_t addr)
{
  config_register(RX_ADDR_P2, addr);
}

void nrf_set_retry_times(uint8_t max_retry_times)
{
    uint8_t rety_reg = 0x00;

    read_register(SETUP_RETR, &rety_reg, 1);
    rety_reg = (rety_reg & 0xf0) | (0x0f & max_retry_times);
    config_register(SETUP_RETR, rety_reg);
}

void nrf_set_retry_durtion(uint32_t micro_senconds)
{
    uint8_t rety_reg = 0x00;

    if (micro_senconds > 4000)
        micro_senconds = 4000;
    else if (micro_senconds == 0)
        micro_senconds = 1;

    micro_senconds --;
    micro_senconds /= 250;

    read_register(SETUP_RETR, &rety_reg, 1);
    rety_reg = (rety_reg & 0x0f) | ((uint8_t)micro_senconds << 4);
    config_register(SETUP_RETR, rety_reg);
}

void set_rx_addr(uint8_t * addr) /* Sets the receiving address */
{
    uint8_t reverse_addr[5];
    debug_printf("set_rx_addr=[0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]\n", 
        addr[0], addr[1], addr[2], addr[3], addr[4]);
    /* RX_ADDR_P0 must be set to the sending addr for auto ack to work. */
    for (int i = 0; i < 5; i ++) {
        reverse_addr[i] = addr[4 - i];
    }

    ce_low();
    write_register(RX_ADDR_P1, reverse_addr, 5);
    ce_high();
}

void set_tx_addr(uint8_t * addr) /* Sets the transmitting address */
{
    uint8_t reverse_addr[5];

    /* RX_ADDR_P0 must be set to the sending addr for auto ack to work. */
    for (uint8_t i = 0; i < 5; i ++) {
        reverse_addr[i] = addr[5-i-1];
    }
    write_register(RX_ADDR_P0, reverse_addr, 5);
    write_register(TX_ADDR, reverse_addr, 5);
    debug_printf("set_tx_addr=[%c %c %c %c %c]\n",
        addr[0], addr[1], addr[2], addr[3], addr[4]);
}

bool data_ready() /* Checks if data is available for reading */
{
    uint8_t status = get_status();
    if ( status & (1 << RX_DR) )
        return 1;
    
    return !rx_fifo_empty();
}

bool rx_fifo_empty(){
    uint8_t fifo_status;

    read_register(FIFO_STATUS, &fifo_status, sizeof(fifo_status));
    return (fifo_status & (1 << RX_EMPTY));
}

void get_data(uint8_t * data) /* Reads payload bytes into data array */
{
    uint8_t *temp_buffer = (uint8_t *)malloc(payload + 1);

    memset(temp_buffer,0,payload+1);

    temp_buffer[0] = R_RX_PAYLOAD;
    wiringPiSPIDataRW(SPI_CHANNEL, temp_buffer, payload + 1);
    
    memcpy(data, temp_buffer + 1, payload);
    config_register(STATUS, (1 << RX_DR));   /* Reset status register */

    free(temp_buffer);
    debug_printf("get_data=[%s]\n", data);
}

void config_register(uint8_t reg, uint8_t value)
{
    uint8_t temp_buffer[2];

    temp_buffer[0] = (W_REGISTER | (REGISTER_MASK & reg));
    temp_buffer[1] = value;

    wiringPiSPIDataRW(SPI_CHANNEL, temp_buffer, 2);
}

void read_register(uint8_t reg, uint8_t * value, uint8_t len)
/* Reads an array of bytes from the given start position in the MiRF registers. */
{
    uint8_t *temp_buffer = (uint8_t *)malloc(len + 1);

    memcpy(temp_buffer + 1, value, len);

    temp_buffer[0] = (R_REGISTER | (REGISTER_MASK & reg));

    wiringPiSPIDataRW(SPI_CHANNEL, temp_buffer, len + 1);

    memcpy(value, temp_buffer + 1, len);
    free(temp_buffer);
}

void write_register(uint8_t reg, uint8_t * value, uint8_t len) 
/* Writes an array of bytes into inte the MiRF registers. */
{
    uint8_t *temp_buffer = (uint8_t *)malloc(len+1);

    temp_buffer[0] = (W_REGISTER | (REGISTER_MASK & reg));

    memcpy(temp_buffer + 1, value, len);

    wiringPiSPIDataRW(SPI_CHANNEL, temp_buffer, len + 1);
    free(temp_buffer);
}

void nrf_send(uint8_t * value) 
/* Sends a data package to the default address.
    Make sure to send the correct*/
{
    uint8_t status;

    status = get_status();
    while (in_tx_mode) {
            status = get_status();

            if((status & ((1 << TX_DS)  | (1 << MAX_RT)))){
                    in_tx_mode = 0;
                    break;
            }
    }                  /* Wait until last paket is send */
    ce_low();
    powerup_tx();       /* Set to transmitter mode , Power up*/

    uint8_t temp = FLUSH_TX;
    wiringPiSPIDataRW(SPI_CHANNEL, &temp, 1);

    uint8_t *temp_buffer = (uint8_t *) malloc(payload + 1);
    temp_buffer[0] = W_TX_PAYLOAD;

    memcpy(temp_buffer + 1, value, payload);
    wiringPiSPIDataRW(SPI_CHANNEL, temp_buffer, payload + 1);

    ce_high();
    while(is_sending());
    debug_printf("nrf_send payload=[%s]\n", value);
}

bool is_sending(){
        uint8_t status;

        if (in_tx_mode) {
                status = get_status();
                /* if sending successful (TX_DS) or max retries exceded (MAX_RT).*/
                if((status & ((1 << TX_DS)  | (1 << MAX_RT)))){
                    powerup_rx();
                    return false; 
                }
                return true;
        }
        return false;
}

uint8_t get_status(){
        uint8_t rv;

        read_register(STATUS, &rv, 1);
        return rv;
}

void powerup_rx(){
        in_tx_mode = 0;

        ce_low();
        config_register(CONFIG, mirf_CONFIG | ( (1<<PWR_UP) | (1<<PRIM_RX) ) );
        ce_high();
        config_register(STATUS,(1 << TX_DS) | (1 << MAX_RT)); 
}

void flush_rx(){
    uint8_t temp = FLUSH_RX;

    wiringPiSPIDataRW(SPI_CHANNEL, &temp, 1);
}

void powerup_tx(){
    in_tx_mode = 1;

    config_register(CONFIG, mirf_CONFIG | ((1<<PWR_UP) | (0<<PRIM_RX)));
}

void ce_high(){
    digitalWrite(CE_PIN, 1);
}

void ce_low(){
    digitalWrite(CE_PIN, 0);
}

void power_down(){
    ce_low();
    config_register(CONFIG, mirf_CONFIG);
}


void set_channel(int channel_)
{
    channel = channel_;
    config_register(RF_CH, channel);
}
void set_payload_length(int length)
{
    payload = length;
    config_register(RX_PW_P0, payload);
    config_register(RX_PW_P1, payload);
}


extern "C"{
  void nrf24_spi_init(void)
  {
    nrf_init();
  }

  void nrf24_setup(uint8_t *my_addr, int channel)
  {
    set_channel(channel);
    nrf_init();
    set_rx_addr(my_addr);
    set_payload_length(32);
    nrf_config();
  }

  void nrf24_tx_addr(uint8_t *target_addr)
  {
    set_tx_addr(target_addr);
  }

  void nrf24_send(uint8_t *data)
  {
    nrf_send(data);
  }

  uint8_t nrf24_available()
  {
    return data_ready();
  }

  void nrf24_read(uint8_t *data)
  {
      get_data(data);
  }

  void nrf_test(uint8_t *data)
  {
    uint8_t *t = "hello";
    memcpy(data,t,strlen(t)+1);
  }

  void read_status(void)
  {
    return get_status();
  }
}
