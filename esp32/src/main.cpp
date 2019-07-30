#include "main.h"

void setup() {
  // setup pin
  pinMode(ADS1292_DRDY_PIN, INPUT);
  pinMode(ADS1292_START_PIN, OUTPUT);
  pinMode(ADS1292_CS_PIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(ADS1292_DRDY_PIN), DRDYHandler, FALLING);
  Serial.begin(115200);
  // inititial packet
  init_preference();
  init_packet();
  //register event handler
  WiFi.onEvent(WiFiEvent);
  // create RTOS task
  vTaskPrioritySet(NULL, 1);
  loop_task = xTaskGetCurrentTaskHandle();
  xTaskCreate(LEDTask, "LED", 1000, NULL, 4, NULL);
  xTaskCreate(ATCommandTask, "ATCom", 4096, NULL, 3, NULL);
  xTaskCreate(UDPTask, "UDP", 4096, NULL, 2, NULL);
  // ADS init
  ADS1292.spi_Init(18, 19, 23, 4);
  ADS1292.ads1292_Init(&is_ads1292_init);  //initalize ADS1292 slave
}

uint8_t i,j;
uint32_t uecgtemp, DRDY_notify = 0;
int32_t secgtemp, status_byte;
void loop()
{
  DRDY_notify = ulTaskNotifyTake(pdTRUE, 10 / portTICK_PERIOD_MS);
  if (DRDY_notify > 0) {
    SPI_RX_Buff_Ptr = ADS1292.ads1292_Read_Data(); // Read the data,point the data to a pointer
    for (i = 0; i < 9; i++)
    {
      SPI_RX_Buff[SPI_RX_Buff_Count++] = *(SPI_RX_Buff_Ptr + i);  // store the result data in array
    }
    ads1292dataReceived = true;
    DRDY_counter++;
  }

  if (ads1292dataReceived && (DRDY_counter % 4) == 3) {
    j = 0;
    for (i = 3; i < 9; i += 3)         // data outputs is (24 status bits + 24 bits Respiration data +  24 bits ECG data)
    {

      uecgtemp = (unsigned long) (  ((unsigned long)SPI_RX_Buff[i + 0] << 16) | ( (unsigned long) SPI_RX_Buff[i + 1] << 8) |  (unsigned long) SPI_RX_Buff[i + 2]);
      uecgtemp = (unsigned long) (uecgtemp << 8);
      secgtemp = (signed long) (uecgtemp);
      secgtemp = (signed long) (secgtemp >> 8);

      s32DaqVals[j++] = secgtemp;  //s32DaqVals[0] is Channel 1 and s32DaqVals[1] is Channel 2
    }

    status_byte = (long)((long)SPI_RX_Buff[2] | ((long) SPI_RX_Buff[1]) <<8 | ((long) SPI_RX_Buff[0])<<16); // First 3 bytes represents the status
    status_byte  = (status_byte & 0x0f8000) >> 15;  // bit15 gives the lead status
    LeadStatus = (unsigned char ) status_byte ;  

    if(!((LeadStatus & 0x1f) == 0 ))
      leadoff_detected  = true; 
    else
      leadoff_detected  = false;
       
    if (leadoff_detected == false) {
      raw_ecg[0] = (int16_t)(s32DaqVals[0] >> 8); //ignore the lower 8 bits out of 24bits (24bit to 16bit conversion)
      raw_ecg[1] = (int16_t)(s32DaqVals[1] >> 8);
      filtered_ecg[0] = raw_ecg[0]; filtered_ecg[1] = raw_ecg[1];
      //ECG_ProcessCurrSample(&raw_ecg[0], &filtered_ecg[0]);   // filter out the line noise @40Hz cutoff 161 order
      //ECG_ProcessCurrSample(&raw_ecg[1], &filtered_ecg[1]);
    } else {
      filtered_ecg[0] = 0;
      filtered_ecg[1] = 0;
    }
    // create packet from filtered_ecg
    fill_packet(filtered_ecg[0], filtered_ecg[1]);
    // clear received flag and buffer count
    ads1292dataReceived = false;
    SPI_RX_Buff_Count = 0;
  }
}

void IRAM_ATTR DRDYHandler(void)
{
  if (!is_ads1292_init) return;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  configASSERT(loop_task != NULL);
  vTaskNotifyGiveFromISR(loop_task, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR();
}

void LEDTask(void *pvParameters) {
  pinMode(LED_BUILTIN, OUTPUT);
  for (;;) {
    digitalWrite(LED_BUILTIN, HIGH);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    digitalWrite(LED_BUILTIN, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void UDPTask(void *pvParameters) {
  connectToWiFi();
  while(true) {
    //only send data when connected
    if(is_wifi_connected){
      send_packet();
    } else if (!is_wifi_connecting) {
      // Retry to connect WIFI
      while (!is_wifi_connected) {
        connectToWiFi();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
      }
    }
    //Wait for 1 second
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void ATCommandTask(void *pvParameters)
{
  int state = -1, index_of_equal;
  bool isReboot = false;
  int8_t b;
  String tempStr;
  for(;;) {
    while ((b = Serial.read()) != -1) {
      if (b == 'A') state = 0;
      else if (state == 0 && b =='T') {
        Serial.println("OK");
        // Read String after AT until \n
        String command = Serial.readStringUntil('\n');
        // Find position of = in command string
        index_of_equal = command.indexOf('=');
        if (index_of_equal > 0) {
          // if '=' is found, parse and prepare the input value
          tempStr = command.substring(command.indexOf('=')+1);
          tempStr.replace('\r', '\0');
        }
        preference.begin("ADS1292");
        // ATID, ATID=xxxx 
        // Set device id to xxxx
        if (command.startsWith("ID")) {
          if (index_of_equal > 0 && tempStr.length() > 0) {
            preference.putUChar("ID", (uint8_t) tempStr.toInt());
            isReboot = true;
          } else {
            Serial.printf("%u\r\n", preference.getUChar("ID", 0));
          }
        } 
        // ATSSID, ATSSID=xxxx
        // Set WIFI SSID that want to connect
        else if (command.startsWith("SSID")) {
          if (index_of_equal > 0 && tempStr.length() > 0) {
            preference.putString("SSID", tempStr);
            isReboot = true;
          } else {
            Serial.println(preference.getString("SSID", ""));
          }
        }
        // ATPWD, ATPWD=xxxx
        // Set WIFI Password
        else if (command.startsWith("PWD")) {
          if (index_of_equal > 0 && tempStr.length() > 0) {
            preference.putString("PWD", tempStr);
            isReboot = true;
          } else {
            String pwd = preference.getString("PWD", "");
            for (int i=0; i<pwd.length(); i++) {
              if (i>1 && i!=pwd.length()-1) Serial.print('*');
              else Serial.print(pwd[i]);
            }
            Serial.print("\r\n");
          }
        }
        // ATUDP, ATUDP=0.0.0.0
        // Set IP address of target UDP server
        else if (command.startsWith("UDP")) {
          if (index_of_equal > 0 && tempStr.length() > 0) {
            preference.putString("UDP", tempStr);
            isReboot = true;
          } else {
            Serial.println(preference.getString("UDP", ""));
          }
        }
        // ATPORT, ATPORT=0000
        // Set port of target UDP server
        else if (command.startsWith("PORT")) {
          if (index_of_equal > 0 && tempStr.length() > 0) {
            preference.putUShort("PORT", (uint16_t) tempStr.toInt());
            isReboot = true;
          } else {
            Serial.println(preference.getUShort("PORT", 0));
          }
        }
        preference.end();
        if (command.startsWith("TEST")) {
          Serial.println("TEST MODE");
          ADS1292.ads1292_Test_Mode();
        }
        if (isReboot) ESP.restart();
      } else {
        state = -1;
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}