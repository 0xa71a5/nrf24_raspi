#include <stdio.h>
#include <iostream>
#include <string.h>
#include <wiringPiSPI.h>
#include <unistd.h>
#include <wiringPi.h>
#include <nrf24l01.h>
using namespace std;
 
void setup()
{
  setChannel(12);
  nrf_init();
  setRADDR((uint8_t *)"clie1");
  setPayloadLength(32);
  nrf_config();
  printf("Begining!\n");
}

int count = 0 ;
uint8_t toSend[32];

void loop()
{
    setTADDR((uint8_t *)"serv1");
    sprintf(toSend,"Hello,this is packet %d",count++);
    nrf_send(toSend);
    while(isSending()){
    }
    printf("Finished sending->%s\n",toSend);
    delay(100);
}

int main()
{
    setup();
	setTADDR((uint8_t *)"serv1");
    char words[32];
	while(1)
    {
       // loop();
		cin.getline(words,32);
		nrf_send(words);
		while(isSending());
		printf("Success sended\n");
    }
    return 0;
}
