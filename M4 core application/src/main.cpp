
/**
 * M4 core programm
 */

#include "Wiring.h"
#include "mt3620-intercore.h"

BufferHeader *outbound, *inbound;
uint32_t sharedBufSize = 0;
static const size_t payloadStart = 20;
int lectura;
unsigned long sampletime_ms = 30000;


float getCorriente(int i)
{
  float voltajeSensor;
  float corriente=0;
  float Sumatoria=0;
  long tiempo=millis();
  int N=0;

  while(millis()-tiempo<500)    // 0.5 seconds(25 cycles of a 50Hz signal
  { 
    voltajeSensor = (analogRead(i) * (3.3 / 4096.0));////Sensor voltage
    corriente=voltajeSensor*30.0; //Current Conversion (30A/1V)
    Sumatoria=Sumatoria+sq(corriente);//Square Sum
    N=N+1;
    delay(1);
  }
  Sumatoria=Sumatoria*2;//Negativ cycles are remove by the op amplifier .
  corriente=sqrt((Sumatoria)/N); //RMS
  return(corriente);

}

void setup()
{
    
    pinMode(16, OUTPUT); //GPIO Initialization
    pinMode(17, OUTPUT);
    Serial.begin(); 
    if (GetIntercoreBuffers(&outbound, &inbound, &sharedBufSize) == -1) {
        for (;;) {
            // empty.
        }
    }
   lectura=0; // Use to select each input
}


void loop()
{
    uint8_t buf[256];
    int j=0;
    uint32_t dataSize = sizeof(buf);
    float adc;
    
    union Analog_data
	{
		uint32_t u32;
		uint8_t u8[4];
	} analog_data;

    int r = DequeueData(outbound, inbound, sharedBufSize, buf, &dataSize); // M7 ask for data

      
    if (r == 0)
    {
            lectura++;
            switch (lectura)
            {
            case 1:
                digitalWrite(16,0);
                digitalWrite(17,0);
                adc = getCorriente(1);                
                break;
            case 2:
                digitalWrite(16,1);
                digitalWrite(17,0);
                adc = getCorriente(1);
                break;
            case 3:
                digitalWrite(16,0);
                digitalWrite(17,1);
                adc = getCorriente(1);
                break;
            case 4:
                digitalWrite(16,1);
                digitalWrite(17,1);
                adc = getCorriente(1);
                break;
            case 5:
                adc = getCorriente(0);
                lectura=0;
                break;
            default:
                Serial.printf("Read Error");
                break;

            }
    
     
            analog_data.u32 = adc*100;
            
            for (int i = payloadStart; i < payloadStart + 4; i++)
            {
                // Put ADC data to buffer
                buf[i] = analog_data.u8[j++];
            }
    
            EnqueueData(inbound, outbound, sharedBufSize, buf, payloadStart + 4); // SEnd data to M7

    }
    else
    {
        Serial.println("without read");
    }
 
}
