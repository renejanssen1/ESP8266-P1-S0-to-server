#include <ESP8266WiFi.h>
#include <time.h>
#include "CronAlarms.h"
const char* ssid = "Janssen"; 
const char* password = "wtawhY7xdjaa"; 
const char* getHost = "192.168.178.100"; 
const int httpGetPort = 80;
String getReceiverURL = "/received.php";
const unsigned int MAX_MESSAGE_LENGTH = 52;
long pulsetime = 0;
#define interruptPin 5 // GPIO 5 = D1 on a Wemos D1 mini v.3.0
int interruptCounter = 0;
int NA = 0; //kWh night consumption
int DA = 0; //kWh day consumption
int NT = 0; //kWh night deliverd
int DT = 0; //kWh day deliverd
int AF = 0; //Actual consumption
int TE = 0; //Actual deliverd
int F1V = 0; // Fase 1 voltage
int F2V = 0; // Fase 2 voltage
int F3V = 0; // Fase 3 voltage
int F1A = 0; // Fase 1 amperage
int F2A = 0; // Fase 2 amperage
int F3A = 0; // Fase 3 amperage
int F1 = 0; //Fase 1 kWh consumption
int F2 = 0; //Fase 2 kWh consumption
int F3 = 0; //Fase 3 kWh consumption
int F1T = 0; //Fase 1 kWh deliverd
int F2T = 0; //Fase 2 kWh deliverd
int F3T = 0; //Fase 3 kWh deliverd
int GAS = 0; //Meter reading Gas
char tGAS[14];  // time stamp gas
  static unsigned long last_interrupt_time = 0;
  
void ICACHE_RAM_ATTR handleInterrupt() {
//	static unsigned long last_interrupt_time = 0;
	unsigned long interrupt_time = millis();
	if (interrupt_time - last_interrupt_time > 1000){
		interruptCounter++;
		pulsetime = interrupt_time - last_interrupt_time;
		Serial.print("<<< Interrupt counter value:");
		Serial.print(interruptCounter);
		Serial.print("   Pulstijd:");
		Serial.println(pulsetime);
		last_interrupt_time = interrupt_time;		
	}
//	last_interrupt_time = interrupt_time;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
	Serial.begin(115200);
	while (!Serial);
		Serial.println("Starting setup...");
		Cron.create("0 */5 * * * *", Repeats, false); // repeat every 5 minutes
		Serial.println("Ending setup...");	
	Serial.println('\n');
	WiFi.hostname("ESPboard-counter");
	WiFi.begin(ssid, password);
	Serial.print("Connecting to ");
	Serial.print(ssid); Serial.println(" ...");
	int i = 0;
	while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW);
		Cron.delay(500);
     digitalWrite(LED_BUILTIN, HIGH);
    Cron.delay(500);     
		Serial.print(++i); Serial.print(' ');
	}
	Serial.println('\n');
	Serial.println("Connection established!");
	Serial.print("IP address:\t");
	Serial.println(WiFi.localIP()); // Send the IP address of the ESP8266 to the computer
	// Get time from a server
	configTime(2 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // 2 = timezone
	Serial.println("\nWaiting for time");
	while (!time(nullptr)) {
		Serial.print(".");
		Cron.delay(500);
	}
	Serial.println("");
	attachInterrupt(interruptPin, handleInterrupt, FALLING);
}

void postData() {
	WiFiClient clientGet;
	char sValue[255];
	int vint = interruptCounter;
	int ptijd = pulsetime;
	sprintf(sValue, "%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d", NA, DA, NT, DT, AF, TE, F1, F2, F3, F1V, F2V, F3V, F1A, F2A, F3A, F1T, F2T, F3T, GAS, vint, ptijd);
	Serial.print("waardes zijn: ");
	Serial.println(sValue);
	String getReceiverURLtemp = getReceiverURL + "?data=" + sValue;
	Serial.println("-------------------------------");
	Serial.print(">>> Connecting to host: ");
	Serial.println(getHost);
	if (!clientGet.connect(getHost, httpGetPort)) {
		Serial.print("Connection failed: ");
		Serial.print(getHost);
	} else {
		clientGet.println("GET " + getReceiverURLtemp + " HTTP/1.1");
		clientGet.print("Host: ");
		clientGet.println(getHost);
		clientGet.println("User-Agent: ESP8266/1.0");
		clientGet.println("Authorization: Basic c21hcnRtZXRlcjprbm9lcGll=");
		clientGet.println("Connection: close\r\n\r\n");
		unsigned long timeoutP = millis();
		while (clientGet.available() == 0) {
			if (millis() - timeoutP > 10000) {
				Serial.print(">>> Client Timeout: ");
				Serial.println(getHost);
				clientGet.stop();
				return;
			}
		}
		while(clientGet.available()){
			String retLine = clientGet.readStringUntil('\r');
			Serial.print(">>> Host returned: ");
			Serial.println(retLine);
			// reset counter if successully connected
			if (retLine == "HTTP/1.1 200 OK") {
				Serial.println(">>> Communication successful");
				interruptCounter -= vint;
				Serial.print(">>> Changed interrupt counter to: ");
				Serial.println(interruptCounter);
				last_interrupt_time = millis(); 
			} else {
				Serial.println(">>> Communication failed!!!");
			}
			break;
		}
	}
	Serial.print(">>> Closing host: ");
	Serial.println(getHost);
	digitalWrite(LED_BUILTIN, HIGH);
	clientGet.stop();
}

void readTelegram() {
	while (Serial.available() > 0) {
		static char message[MAX_MESSAGE_LENGTH];
		static unsigned int message_pos = 0;
		char inByte = Serial.read();
		if ( inByte != '\n' && (message_pos < MAX_MESSAGE_LENGTH - 1) ){
			message[message_pos] = inByte;
			message_pos++;
		}else{
			message[message_pos] = '\0';
			Serial.println(message);
			long tl  = 0;
			long tld = 0;
			if (sscanf(message, "1-0:1.8.1(%ld.%ld%*s" , &tl, &tld) == 2 ) {
				NA = tl * 1000 + tld; Serial.print("na: ");}
			if (sscanf(message, "1-0:1.8.2(%ld.%ld%*s" , &tl, &tld) == 2 ) {
				DA = tl * 1000 + tld;  }
			if (sscanf(message, "1-0:2.8.1(%ld.%ld%*s" , &tl, &tld) == 2 ) {
				NT = tl * 1000 + tld;  }
			if (sscanf(message, "1-0:2.8.2(%ld.%ld%*s" , &tl, &tld) == 2 ) {
				DT = tl * 1000 + tld;  }
			if (sscanf(message, "1-0:1.7.0(%ld.%ld%*s" , &tl, &tld) == 2 ) {
				AF = tl * 1000 + tld;  }
			if (sscanf(message, "1-0:2.7.0(%ld.%ld%*s" , &tl, &tld) == 2 ) {
				TE = tl * 1000 + tld;  }
			if (sscanf(message, "1-0:21.7.0(%ld.%ld%*s" , &tl, &tld) == 2 ) {
				F1 = tl * 1000 + tld;  }
			if (sscanf(message, "1-0:41.7.0(%ld.%ld%*s" , &tl, &tld) == 2 ) {
				F2 = tl * 1000 + tld;  }
			if (sscanf(message, "1-0:61.7.0(%ld.%ld%*s" , &tl, &tld) == 2 ) {
				F3 = tl * 1000 + tld;  }
			if (sscanf(message, "1-0:32.7.0(%ld.%ld%*s" , &tl, &tld) == 2 ) {
				F1V = tl + tld;  }
			if (sscanf(message, "1-0:52.7.0(%ld.%ld%*s" , &tl, &tld) == 2 ) {
				F2V = tl + tld;  }
			if (sscanf(message, "1-0:72.7.0(%ld.%ld%*s" , &tl, &tld) == 2 ) {
				F3V = tl + tld;  }    
			if (sscanf(message, "1-0:31.7.0(%ld.%ld%*s" , &tl, &tld) == 1 ) {
				F1A = tl + tld;  }
			if (sscanf(message, "1-0:51.7.0(%ld.%ld%*s" , &tl, &tld) == 1 ) {
				F2A = tl + tld;  }
			if (sscanf(message, "1-0:71.7.0(%ld.%ld%*s" , &tl, &tld) == 1 ) {
				F3A = tl + tld;  }  
			if (sscanf(message, "1-0:22.7.0(%ld.%ld%*s" , &tl, &tld) == 2 ) {
				F1T = tl * 1000 + tld;  }
			if (sscanf(message, "1-0:42.7.0(%ld.%ld%*s" , &tl, &tld) == 2 ) {
				F2T = tl * 1000 + tld;  }
			if (sscanf(message, "1-0:62.7.0(%ld.%ld%*s" , &tl, &tld) == 2 ) {
				F3T = tl * 1000 + tld;  } 
			if (sscanf(message, "0-1:24.2.1(%13s)(%ld.%ld%*s" , tGAS, &tl, &tld) == 3 ) {
				GAS = tl * 1000 + tld;  }
			message_pos = 0;
		}
	}
}

void loop() {
	time_t now = time(nullptr);
//  Serial.println(ctime(&now));
	Cron.delay(1000);
	readTelegram();
}

void Repeats() {
	digitalWrite(LED_BUILTIN, LOW);
	postData();
}
