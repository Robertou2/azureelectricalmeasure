/**
 * Send Data from M4 core
 * 
 */

#include "Wiring.h"
//#include <ctype.h>
//#include <stddef.h>
//#include <stdbool.h>
//#include <stdint.h>
//#include <errno.h>

//#include "mt3620-baremetal.h"
//#include <mt3620-uart-poll.h>
//#include "mt3620-adc.h"
#include "mt3620-intercore.h"

//#include <Stepper.h>

BufferHeader *outbound, *inbound;
uint32_t sharedBufSize = 0;
static const size_t payloadStart = 20;
int lectura;


void setup()
{
    pinMode(16, OUTPUT);
    pinMode(17, OUTPUT);
    Serial.begin(); // 115200-8-N-1 ... need more info about uart
    Serial.println("\nPass Adc information to M7");
    Serial.printf("SETUP %d\n", 42);
    
    
    if (GetIntercoreBuffers(&outbound, &inbound, &sharedBufSize) == -1) {
        for (;;) {
            // empty.
        }
    }
   
    lectura=0;
}


void loop()
{
    uint8_t buf[256];
    int j=0;
    uint32_t dataSize = sizeof(buf);
    float mv=0;
    
    union Analog_data
	{
		uint32_t u32;
		uint8_t u8[4];
	} analog_data;

    lectura++;
    int r = DequeueData(outbound, inbound, sharedBufSize, buf, &dataSize);
    //Serial.println(buf);
    uint32_t adc = analogRead(1);
     
     digitalWrite(16,0);
     digitalWrite(17,0);
     adc = analogRead(1);
     Serial.printf("lectura 0 = %u\n", adc);
     delay(5000); 

     digitalWrite(16,1);
     digitalWrite(17,1);
     adc = analogRead(1);
     Serial.printf("lectura 3 = %u\n", adc);
     delay(5000); 


    if (lectura < 6)
    {
     lectura = lectura++;
     if (lectura==5)
     {
     adc = analogRead(0);
     lectura=0;
        //Serial.printf("analogRead(0) = %u.%u\n", analog_data.u32 / 1000, adc); 
    }
    }
    uint32_t mV = (adc * 2500) / 0xFFF;
    
    
    analog_data.u32 = adc;
    //Serial.printf("analogRead() = %u.%u\n", analog_data.u32 / 1000, adc);
    
    for (int i = payloadStart; i < payloadStart + 4; i++)
    {
			// Put ADC data to buffer
			buf[i] = analog_data.u8[j++];
	}
    
    
        
    EnqueueData(inbound, outbound, sharedBufSize, buf, payloadStart + 4);
//    digitalWrite(32,0);
    delay(1000);  //  digitalWrite(32,1);



}