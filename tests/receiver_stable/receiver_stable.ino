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

/* If reliable send success then return true ,otherwise return false */
bool nrf_reliable_send(uint8_t *data, uint32_t length = 32, uint32_t max_fail_time = 100)
{
  uint8_t tx_status = 0x00;
  uint8_t failed_times = 0;

  while( nrf_carrier_detect() );
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
  nrf_set_tx_addr((uint8_t *)"mac00");
  nrf_set_rx_addr((uint8_t *)"mac01");
  nrf_chip_config(12, 32); // Set channel and payload
  nrf_set_retry_times(5);
  nrf_set_retry_durtion(750);
  nrf_set_channel(100);
  Serial.println("Begining!");
  enable_rx();
}

uint32_t comm_rate = 0;
uint32_t comm_sum = 0;
uint32_t last_check_sum = 0;
uint32_t last_check_time = 0;

uint8_t last_seq_num = 0;
uint8_t data[32];
uint32_t print_count = 0;

#define RATE_SAMPLE_TIME 128
#define RATE_SAMPLE_TIME_SHIFT 7

#define MAX_RETRY_TIME 10


void loop()
{
  if (nrf_data_ready()) {
    nrf_get_data(data);
    if (millis() - last_check_time > RATE_SAMPLE_TIME) {
      comm_rate = (1000 * (comm_sum - last_check_sum)) >> RATE_SAMPLE_TIME_SHIFT;
      last_check_sum = comm_sum;
      last_check_time = millis();
    }
    //if(print_count ++ % 50 == 0)
      printf("Recv[%3u]->%s [%u B/s]\n", (uint8_t)data[31], (char *)data, comm_rate);
    if ((uint8_t)data[31] == last_seq_num)
      printf("Repeat seq num!\n");
    else if ((uint8_t)data[31] != ((last_seq_num + 1) % 256))
      printf("##Lost frame! cur=%u last=%u\n", (uint8_t)data[31], last_seq_num);
    last_seq_num = data[31];
    
    comm_sum += 32;

  }
  
}

