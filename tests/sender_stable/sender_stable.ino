#include <lxc_nrf24l01.h>

int serial_putc( char c, struct __file * )
{
  Serial.write( c );
  return c;
}
void printf_begin(void)
{
  fdevopen( &serial_putc, 0 );
}

/* 
 * If reliable send success then return true ,otherwise return false 
 * Reliable send shall consider complex wireless environment
 * and retreat when neccessary.
*/
bool nrf_reliable_send(uint8_t *data, uint32_t length = 32, uint32_t max_fail_time = 100)
{
  uint8_t tx_status = 0x00;
  uint8_t failed_times = 0;
  uint16_t retreat_time = 1;
  uint16_t max_retreat_time = 20000; /* 20ms */
  uint32_t max_block_time = 100000; /* We can only blocked within this time*/
  uint32_t check_time = millis();
  
  while (nrf_carrier_detect()) {
    delayMicroseconds(retreat_time + random(retreat_time>>2));
    if (retreat_time < max_retreat_time)
      retreat_time <<= 1;
    if (millis() - check_time > max_block_time)
      break; /* Consider of bad communication condition,we may just return failed */
  }

  do {
        tx_status = nrf_send(data);
        failed_times ++;
  }
  while (tx_status != TX_REACH_DST && failed_times < max_fail_time);
  return tx_status == TX_REACH_DST;
}

void setup()
{
  Serial.begin(115200);
  printf_begin();
  Serial.println("Begin config!");
  nrf_gpio_init(8, 9); //Set ce pin and csn pin
  nrf_set_tx_addr((uint8_t *)"mac01");
  nrf_set_rx_addr((uint8_t *)"mac00");
  nrf_chip_config(12, 32); // Set channel and payload
  nrf_set_retry_times(5);
  nrf_set_retry_durtion(1250);
  nrf_set_channel(100);
  randomSeed(analogRead(A0)^analogRead(A1));
}

int count = 0;
int start = 0;
uint8_t seq_num = 0;
uint8_t failed_times = 0;

#define MAX_RETRY_TIME 10
void loop()
{
   uint8_t data[32];
   uint8_t tx_status = 0x00;
   sprintf((char *)data, "Packet %d", count++);
   data[31] = seq_num ++;
   printf("Send->%s\n", (char *)data);
   nrf_reliable_send(data);
   //delay(100);
   if (tx_status != TX_REACH_DST)
      printf("Send failed with too many retries\n");

   if (nrf_data_ready()) {
      nrf_get_data(data);
      printf("Recv->%s\n\n", data);
  }
  
}

