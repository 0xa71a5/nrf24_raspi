#include <stdio.h>
#include <iostream>
#include <string.h>
#include <wiringPiSPI.h>
#include <unistd.h>
#include <time.h>
#include <wiringPi.h>
#include "nrf24l01.h"
using namespace std;

void rx_test()
{
	char data[32];

	while(1)
	{
		if(data_ready())
		{
			get_data(data);
			cout<<"Get packet->"<<data<<endl;
		}
		delay(10);
	}
}

uint8_t to_send[32];
int count = 0;

void tx_test()
{
	set_tx_addr((uint8_t *)"serv1");
	int pac_count=0;

	while (1) {
		sprintf(to_send, "%d", pac_count);
		nrf_send(to_send);

		cout << "Send packet " << pac_count ++ << endl;

    delay(100);
	}
}

void setup(uint8_t *my_addr,int channel)
{
  set_channel(channel);
  nrf_init();
  set_rx_addr(my_addr);
  set_payload_length(32);
  nrf_config();
}


int main()
{
    char data[32];
    char words[32];
    
    setup("clie1", 12);
    set_tx_addr((uint8_t *)"serv1");

    get_data(data);

    while (1) {
      cin.getline(words, 32);
      nrf_send(words);
      printf("Success sended\n");

      long check_point = clock();
      while (!data_ready()) {
        if (clock() - check_point > 1000) {
          printf("break\n");
          break;
        }
      }
      
      while (data_ready()) {
        get_data(data);
        cout<<"Recv:"<<data<<endl;
      }
    } 
    return 0;
}
