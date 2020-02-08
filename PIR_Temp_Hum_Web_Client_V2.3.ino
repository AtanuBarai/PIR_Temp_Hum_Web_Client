/*  Author: Atanu Barai <atanu.bd@outlook.com>
 Description: Sends PIR and DS18B20 reading to WebServer
 */

#include "OneWire.h"
#include "DHT.h"
#include <SPI.h>
#include <avr/wdt.h>
#include "EthernetV2_0.h"

#define DEBUG_ATANU 1

#define MAX_NUM_ONEWIRE_DEVICE 2

#define USE_DFROBOT_W5200

#ifdef USE_DFROBOT_W5200
#define SS 10
#define nRST  8
#define nPWDN 9
#define nINT 3
#endif

#define DHTPIN 7 // what digital pin we're connected to
#define DHTTYPE DHT22

byte mac[] = { 
    0xB8, 0x27, 0xEB, 0x5D, 0x78, 0xD0 };

char server[] = "www.test.com";    // name address for Google (using DNS)

// Set the static IP address to use if the DHCP fails to assign
/*
IPAddress ip (192, 168, 15, 105);
 IPAddress subnet(255, 255, 255, 0);
 IPAddress gateway(192, 168, 15, 1);
 IPAddress dnServer(8, 8, 8, 8);
 */

EthernetClient client;

OneWire  ds(6);
DHT dht(DHTPIN, DHTTYPE);

float result_in_celcius[] = {0.0, 1.1, 2.2};
int addi_temp_cel[6] = {0};
unsigned int deci0, deci1, deci2, frac0, frac1, frac2, humidity;
volatile bool PIR_reading[] = {0, 0};
uint8_t num_devices;
byte mydelay_index;
//Use Post Method
void postPage(){
    int inChar;
    char outBuf[64];
    char thisData[90];
    if (client.connected()){
        sprintf(outBuf, "POST /post.php HTTP/1.1");
        client.println(outBuf);
        sprintf(outBuf, "Host: www.test.com");
        client.println(outBuf);
        client.println(F("Connection: close\r\nContent-Type: application/x-www-form-urlencoded"));

        deci0 = result_in_celcius[0];
        frac0 = (result_in_celcius[0] - deci0) * 100;
        deci1 = result_in_celcius[1];
        frac1 = (result_in_celcius[1] - deci1) * 100;
        deci2 = result_in_celcius[2];
        frac2 = (result_in_celcius[2] - deci2) * 100;
		
#ifdef DEBUG_ATANU
        Serial.print("Pir1 = ");
        Serial.println(PIR_reading[0]);
        Serial.print("Pir2 = ");
        Serial.println(PIR_reading[1]);
#endif
        sprintf(thisData, "pir1=%d&pir2=%d&temp1=%u.%u&temp2=%u.%u&hum=%u&temp_hum=%u.%u&pass=hmp582", PIR_reading[0], PIR_reading[1], deci0, frac0, deci1, frac1, deci2, frac2, humidity );
        sprintf(outBuf, "Content-Length: %u\r\n", strlen(thisData));
        client.println(outBuf);
        // send the body (variables)
        client.println(thisData);
        delay(1000);
        Serial.println(" **************** START sending Additional Sensor Info to web");
	sprintf(outBuf, "POST /post2.php HTTP/1.1");
        client.println(outBuf);
        Serial.println(outBuf);
        sprintf(outBuf, "Host: www.test.com");
        client.println(outBuf);
        Serial.println(outBuf);
        client.println(F("Connection: close\r\nContent-Type: application/x-www-form-urlencoded"));
        Serial.println(F("Connection: close\r\nContent-Type: application/x-www-form-urlencoded"));
	sprintf(thisData, "adt1=%d&adt2=%d&adt3=%d&adt4=%d&adt5=%d&adt6=%d&pass=hmp582", addi_temp_cel[0], addi_temp_cel[1], addi_temp_cel[2], addi_temp_cel[3], addi_temp_cel[4], addi_temp_cel[5]);
        sprintf(outBuf, "Content-Length: %u\r\n", strlen(thisData));
        client.println(outBuf);
        Serial.println(outBuf);
        // send the body (variables)
        client.println(thisData);
        Serial.println(thisData);
        Serial.println(" **************** END printing Additional Sensor Info ");
        PIR_reading[0] = 0;
        PIR_reading[1] = 0;
        delay(1000);
    }
}

void read_switch(){
    char c;
    if (client.connected()){    
        client.println("GET /switch.php?q=arduino HTTP/1.0");
        client.println();
    }
    delay(1000);
    while (client.available()) {
        c = client.read();
#ifdef DEBUG_ATANU
        Serial.print(c);
#endif
    }
}

void init_eth(){
#ifdef DEBUG_ATANU
    Serial.println("\nInitializing Ethernet");
#endif
    Ethernet.begin(mac);
    //  Ethernet.begin(mac, ip, dnServer, gateway, subnet);
    delay(1000); // give the Ethernet shield a second to initialize:
}

void InitialiseInterrupt(){
    cli();		// switch interrupts off while messing with their settings  
    PCICR =0x02;          // Enable PCINT1 interrupt
    PCMSK1 = 0b00000011;
    sei();		// turn interrupts back on
}

ISR(PCINT1_vect){    // Interrupt service routine. Every single PCINT8..14 (=ADC0..5) change
    // will generate an interrupt: but this will always be the same interrupt routine
    cli();
    if (digitalRead(A0)==1) {
        if(PIR_reading[0] == 0) {            
            Serial.println("PIR 1 Signal Received");
            PIR_reading[0] = 1;
        }
    }
    if (digitalRead(A1)==1) {
        if(PIR_reading[1] == 0) {   
            PIR_reading[1] = 1;
            Serial.println("PIR 2 Signal Received");
        }
    }
    sei();
}

void setup(){
#ifdef USE_DFROBOT_W5200
    pinMode(SS,OUTPUT);
    pinMode(nRST,OUTPUT);
    pinMode(nPWDN,OUTPUT);
    pinMode(nINT,INPUT);
    pinMode(A0, INPUT);	   // Pin A0 is input to which a PIR is connected
    digitalWrite(A0, HIGH);   // Configure internal pull-up resistor
    pinMode(A1, INPUT);	   // Pin A1 is input to which a PIR is connected
    digitalWrite(A1, HIGH);   // Configure internal pull-up resistor
    //Setup Ethernet
    digitalWrite(nPWDN,LOW);  //enable power
    digitalWrite(nRST,LOW);  //Reset W5200
    delay(10);
    digitalWrite(nRST,HIGH);  
    delay(200);       // wait W5200 work
#endif
    Serial.begin(9600);
    InitialiseInterrupt();
    init_eth();
    dht.begin();
}

void Ethernet_Send_Data() {
#ifdef DEBUG_ATANU
    Serial.println("\nEntered Into Ethernet_Send_Data");
#endif
    if (!client.connected()) {
#ifdef DEBUG_ATANU
        Serial.println();
        Serial.println("Client not connected, disconnecting.");
#endif
        client.stop();
		
#ifdef DEBUG_ATANU
        Serial.println("connecting...");
        if (client.connect(server, 80)) {
            Serial.println("connected");
        }
        else {
            Serial.println("Connection failed");
        }
#endif
    }
    postPage();
    while (client.available()) {
        char c = client.read();
#ifdef DEBUG_ATANU
        Serial.print(c);
#endif
    }
    
//    read_switch();
    
#ifdef DEBUG_ATANU
    Serial.println();
#endif
}

void DS_Temp_Sensor() {
    num_devices = 0;
#ifdef DEBUG_ATANU
    Serial.println("Entered Into DS_Temp_Sensor");
#endif
    while (1) {
        byte i;
        byte present = 0;
        byte type_s;
        byte data[12];
        byte addr[8]; 

        if ( !ds.search(addr)) {
#ifdef DEBUG_ATANU
            Serial.println("No more addresses. Returning");
            Serial.print("Total sensors read ");
            Serial.println(num_devices);            
#endif
            ds.reset_search();
            delay(250);
            return;
        }
#ifdef DEBUG_ATANU
        Serial.print("ROM =");
        for ( i = 0; i < 8; i++) {
            Serial.write(' ');
            Serial.print(addr[i], HEX);
        }
#endif
        if (OneWire::crc8(addr, 7) != addr[7]) {
#ifdef DEBUG_ATANU
            Serial.println("CRC is not valid!");
#endif
            return;
        }
#ifdef DEBUG_ATANU
        Serial.println();
#endif
        // the first ROM byte indicates which chip
        switch (addr[0]) {
        case 0x10:
            type_s = 1; //Chip = DS18S20
            break;
        case 0x28:
            type_s = 0; //Chip = DS18B20
            break;
        case 0x22:
            type_s = 0; //Chip = DS1822")
            break;
        default:
#ifdef DEBUG_ATANU
            Serial.println("Device is not a DS18x20 family device.");
#endif
            return;
        }

        ds.reset();
        ds.select(addr);
        ds.write(0x44, 0);        // start conversion, normal mode

        delay(1000);     // maybe 750ms is enough, maybe not
        // we might do a ds.depower() here, but the reset will take care of it.
        present = ds.reset();
        ds.select(addr);
        ds.write(0xBE);
#ifdef DEBUG_ATANU
        Serial.print("  Data = ");
        Serial.print(present, HEX);
        Serial.print(" ");
#endif
        for ( i = 0; i < 9; i++) {           // we need 9 bytes
            data[i] = ds.read();
#ifdef DEBUG_ATANU
            Serial.print(data[i], HEX);
            Serial.print(" ");
#endif
        }
#ifdef DEBUG_ATANU
        Serial.print(" CRC=");
        Serial.print(OneWire::crc8(data, 8), HEX);
        Serial.println();
#endif
        // Convert the data to actual temperature
        // because the result is a 16 bit signed integer, it should
        // be stored to an "int16_t" type, which is always 16 bits
        // even when compiled on a 32 bit processor.
        int16_t raw = (data[1] << 8) | data[0];
        if (type_s) {
            raw = raw << 3; // 9 bit resolution default
            if (data[7] == 0x10) {
                raw = (raw & 0xFFF0) + 12 - data[6]; // "count remain" gives full 12 bit resolution
            }
        }
        else {
            byte cfg = (data[4] & 0x60);
            if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
            else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
            else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
        }
#ifdef DEBUG_ATANU
            Serial.print("  Temperature Sensor Number ");
            Serial.print(num_devices);
            Serial.print("  Reading : ");
#endif
	if(num_devices < 2)
	{		
	    result_in_celcius[num_devices] = (float)raw / 16.0;
#ifdef DEBUG_ATANU
	    Serial.print(result_in_celcius[num_devices]);
#endif
        }
	else
	{
	    addi_temp_cel[num_devices - 2] = raw / 16.0;
#ifdef DEBUG_ATANU
	    Serial.print(addi_temp_cel[num_devices - 2]);
#endif
	}
#ifdef DEBUG_ATANU
		Serial.println(" Celsius ");
#endif
        num_devices++;
    }
}

void loop(){
    //Used for DHCP
    wdt_enable(WDTO_8S);
    Ethernet.maintain();
    DS_Temp_Sensor();
    wdt_disable();
    wdt_enable(WDTO_8S);
    humidity = dht.readHumidity();    
    result_in_celcius[2] = dht.readTemperature();
#ifdef DEBUG_ATANU
    Serial.print("  Outdoor Temperature = ");
    Serial.print(result_in_celcius[2]);
    Serial.println(" Celsius, ");
    Serial.print("  Humidity = ");
    Serial.println(humidity);
#endif
    wdt_disable();
    for(mydelay_index=0; mydelay_index<2; mydelay_index++)
        delay(30000);
    wdt_enable(WDTO_8S);
    Ethernet_Send_Data();
    wdt_disable();
}
