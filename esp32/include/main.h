#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include "ads1292.h"

#define PACKET_TX_BUFFER_NUMBER 5
#define PACKET_TX_BUFFER_SIZE   8004
#define PACKET_START_1          0xCA
#define PACKET_START_2          0xFE
#define PACKET_END              0xCD
#define PACKET_ID_OFFSET        2
#define PACKET_FIRST_OFFSET     3
#define PACKET_LAST_OFFSET    (PACKET_TX_BUFFER_SIZE - 1)
#define PACKET_INDEX_OF_ID(x) (x % PACKET_TX_BUFFER_NUMBER)

//== ADS1292 variables ===============================
ads1292 ADS1292;
byte DataPacketHeader[16];
byte data_len = 7;

volatile int32_t s32DaqVals[8];
volatile byte SPI_RX_Buff[15] ;
volatile static int SPI_RX_Buff_Count = 0;
volatile char *SPI_RX_Buff_Ptr;
volatile bool ads1292dataReceived = false;
volatile byte LeadStatus=0;
volatile bool leadoff_detected = true;
//== end ADS1292 variables ===========================

//== packet variables ================================
boolean is_packet_ready = false;
boolean is_packet_sent = true;
volatile int16_t packet_TX_Buffer[PACKET_TX_BUFFER_NUMBER][PACKET_TX_BUFFER_SIZE] = {0};
volatile uint8_t packet_Id = 0;
volatile uint16_t packet_Offset = 0;
//== end packet variables ============================

//== WIFI variables ==================================
String deviceId = "";
String networkName = "";
String networkPswd = "";
String udpAddress = "192.168.0.1";
uint16_t udpPort = 0;
boolean is_wifi_connected = false;
WiFiUDP udp;
Preferences preference;
//== END WIFI variables ===============================

//= = TASKS ============================================
void LEDTask(void *pvParameters);
void UDPTask(void *pvParameters);
void ATCommandTask(void * pvParameters);
//== END TASKS ========================================

void init_packet() {
  // Initital Empty packet
  for (byte i=0; i<PACKET_TX_BUFFER_NUMBER; i++) {
    memset((void*)&packet_TX_Buffer[i], 0, PACKET_TX_BUFFER_SIZE);
    packet_TX_Buffer[i][0] = PACKET_START_1;
    packet_TX_Buffer[i][1] = PACKET_START_2;
    packet_TX_Buffer[i][PACKET_TX_BUFFER_SIZE-1] = PACKET_END;
  }
}

static inline void fill_packet(int16_t ch1, int16_t ch2) {
  packet_TX_Buffer[PACKET_INDEX_OF_ID(packet_Id)][packet_Offset++] = ch1;
  packet_TX_Buffer[PACKET_INDEX_OF_ID(packet_Id)][packet_Offset++] = ch2;
  if (packet_Offset >= PACKET_LAST_OFFSET) {
    is_packet_ready = true;
    packet_Id++;
    packet_TX_Buffer[PACKET_INDEX_OF_ID(packet_Id)][PACKET_ID_OFFSET] = packet_Id;
  }
}

// wifi event handler
void WiFiEvent(WiFiEvent_t event) {
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      //When connected set 
      Serial.print("WiFi connected! IP address: ");
      Serial.println(WiFi.localIP());  
      //initializes the UDP state
      //This initializes the transfer buffer
      udp.begin(WiFi.localIP(), udpPort);
      is_wifi_connected = true;
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      is_wifi_connected = false;
      break;
    default:
      break;
  }
}

static inline void connectToWiFi() {
  Serial.println("Connecting to WiFi network: " + networkName);
  // delete old config
  WiFi.disconnect(true);
  //register event handler
  WiFi.onEvent(WiFiEvent);
  //Initiate connection
  WiFi.begin(networkName.c_str(), networkPswd.c_str());
  Serial.println("Waiting for WIFI connection...");
}

static inline void init_preference() {
  preference.begin("ADS1292");
  deviceId = preference.getString("ID", "MU-0000");
  networkName = preference.getString("SSID", "0000");
  networkPswd = preference.getString("PWD", "");
  udpAddress = preference.getString("UDP", "");
  udpPort = preference.getShort("PORT", 0);
  preference.end();
}