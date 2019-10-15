#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <applibs/application.h>

#include <ArduinoJson.h>

#include "google.h"
#include <curlClient.h>
#include <CloudIoTCore.h>
#include <jwt.h>
#include <Base64.h>


Adafruit_SSD1306 display;
static int iter;
float electrical_measure0;
float electrical_measure1;
float electrical_measure2;
float electrical_measure3;
float light_sensor;

CloudIoTCoreDevice goo(GOOGLE_PROJECT, GOOGLE_LOCATION, GOOGLE_REGISTRY, GOOGLE_DEVICE, PRIVATE_KEY);
curlClient client;


//Send Information to GOOGLE IoT Core
void send(int port, const char *url, const char *message)
{
  Log_Debug("\n[REST] Begin\n");
  String jwt = goo.createJWT(utc(),1000);
 
Log_Debug("[SEND] %s\n", message);
  client.begin(url);
  client.CURL_SETOPT(CURLOPT_PORT, port);
  client.CURL_SETOPT(CURLOPT_POSTFIELDS, message);
  client.CURL_SETOPT(CURLOPT_CONNECTTIMEOUT, 30);
  client.CURL_SETOPT(CURLOPT_SSL_VERIFYPEER, 1);
  client.CURL_SETOPT(CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);

  struct curl_slist *chunk = NULL;
  char authorization[1024];
  sprintf(authorization, "authorization: Bearer %s", jwt.c_str());
  chunk = curl_slist_append(chunk, authorization);
  chunk = curl_slist_append(chunk, "content-type: application/json");
  chunk = curl_slist_append(chunk, "cache-control: no-cache");

  client.CURL_SETOPT(CURLOPT_HTTPHEADER, chunk);

  char *path;

  path = Storage_GetAbsolutePathInImagePackage(GOOGLE_CA_LIST);
  client.CURL_SETOPT(CURLOPT_CAINFO, path);

  MemoryBlock *response;
  
  client.run(&response);
  if (response)
    Log_Debug("[GOOGLE][%d] %.*s\n", response->size, response->size, response->data);
  else
    Log_Debug("[ERROR] NO RESPONSE\n");
end:
  client.end();
  //Log_Debug("[REST] End\n");
}

//OLED MANAGER
void actualizar_oled()
{

  display.begin(SSD1306_SWITCHCAPVCC, 0x78 >> 1); // 7-bits
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(" MT3620");
  display.setTextColor(WHITE,BLACK);
  display.setTextSize(1);
  display.print(" ME0: ");
  display.println(electrical_measure0);
  display.print(" ME1: ");
  display.println(electrical_measure1);
  display.print(" ME2: ");
  display.println(electrical_measure2);
  display.print(" ME3: ");
  display.println(electrical_measure3);
  display.print(" ADC0: ");
  display.println(light_sensor);
  display.println();
  display.setTextColor(WHITE, BLACK);
  display.display();

}




static const char rtAppComponentId[] = "005180bc-402f-4cb3-a662-72937dbcde47";
static int sockFd = -1;
uint8_t RTCore_status;



static void SendMessageToRTCore(void)
{
	static int iter = 0;

	// Send "Read-ADC-%d" message to real-time capable application.
	static char txMessage[32];
	sprintf(txMessage, "Read-ADC-%d", iter++);
	Log_Debug("Sending: %s\n", txMessage);

	int bytesSent = send(sockFd, txMessage, strlen(txMessage), 0);
	if (bytesSent == -1)
	{
		Log_Debug("ERROR: Unable to send message: %d (%s)\n", errno, strerror(errno));
		//terminationRequired = true;
		return;
	}
}


// This function is for using only two decimals
float truncate(float val, byte dec)
{
    float x = val * pow(10, dec);
    float y = round(x);
    float z = x - y;
    if ((int)z == 5)
    {
        y++;
    } else {}
    x = y / pow(10, dec);
    return x;
}


void setup()
{
  bool outIsNetworkingReady = 0;

  Serial.begin(115200);
  Serial.println("\nPrueba oled");

  sockFd = Application_Socket(rtAppComponentId);
	if (sockFd == -1) 
	{
		Log_Debug("ERROR: Unable to create socket: %d (%s)\n", errno, strerror(errno));
		Serial.println("Real Time Core disabled or Component Id is not correct.\n");
		Serial.println("The program will continue without showing light sensor data.\n");
		// Communication with RT core error
		RTCore_status = 1;
		//return -1;
	}
	else
	{
		// Communication with RT core success
		RTCore_status = 0;
		// Set timeout, to handle case where real-time capable application does not respond.
		static const struct timeval recvTimeout = { .tv_sec = 5,.tv_usec = 0 };
		int result = setsockopt(sockFd, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout));
		if (result == -1)
		{
			Serial.println("ERROR: Unable to set socket timeout");
		}
  }
  /* wait wifi */
  if (Networking_IsNetworkingReady(&outIsNetworkingReady) < 0 && 0 == outIsNetworkingReady)
  {
    Serial.print(".");
    delay(100);
  }
  
	iter=0;
	Serial.redirect(stderr);
 }


void loop()
{
	char message[1024]="";
  	String payload="";
	StaticJsonDocument <200> root;
	char buffer[100]="";
    const float factor =100;
	static int t = 0;
	char rxBuf[32];
	union Analog_data
	{
		uint32_t u32;
		uint8_t u8[4];
	} analog_data;

	SendMessageToRTCore(); //Ask for information to M4Core
	int bytesReceived = recv(sockFd, rxBuf, sizeof(rxBuf), 0); //Information from M4 Core

	if (bytesReceived == -1) {
		Serial.println("ERROR: Unable to receive message");
	}
	else
	{

		iter++;
		if (iter > 4)
			iter = 0;

		for (int i = 0; i < sizeof(analog_data); i++)
		{
			analog_data.u8[i] = rxBuf[i];
		}
		
		if (iter== 1)
		{
			electrical_measure0 = analog_data.u32/factor;
			electrical_measure0=truncate(electrical_measure0,2);
		}

		else if (iter==2)
		{
			electrical_measure1 = analog_data.u32/factor;
			electrical_measure1=truncate(electrical_measure1,2);

		}
		else if (iter== 3)
		{
			electrical_measure2 = analog_data.u32/factor;
			electrical_measure2=truncate(electrical_measure2,2);
		}
		else if (iter==4)
		{
			electrical_measure3 = analog_data.u32/factor;
			electrical_measure3=truncate(electrical_measure3,2);
		}
		else if (iter == 0)
		{
			light_sensor = ((float)analog_data.u32 * 2.5 / 4095) * 10000 / (3650 * 0.1428);
			light_sensor=truncate(light_sensor,2);
		}
  
		//Json  to send to google

		root["me0"] = electrical_measure0;
    	root["me1"] = electrical_measure1;
    	root["me2"] = electrical_measure2;
		root["me3"] = electrical_measure3;
		root["la"] = light_sensor;
	    serializeJson(root,buffer);
	}
	
    actualizar_oled();


  
  int inputStringLength = sizeof(buffer);
  int encodedLength = Base64.encodedLength(inputStringLength);
  char encodedString[encodedLength];

  //To send telemetry events to the Cloud using the HTTP bridge, 
  //you have to send a POST request containing base64 encoded data to a given URL
  Base64.encode(encodedString, buffer, inputStringLength);
  sprintf(message, "{\"binary_data\":\"%s\"}",encodedString); // Base64
  Send(API_PORT, GOOGLE_URL, message);
  sleep(30);
}
