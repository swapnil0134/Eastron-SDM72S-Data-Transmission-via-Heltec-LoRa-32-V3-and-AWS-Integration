#include "Arduino.h"
#include "WiFi.h"
#include "LoRaWan_APP.h"
#include <Wire.h>
#include "HT_SSD1306Wire.h"

/********************************* lora  *********************************************/
#define RF_FREQUENCY                                868000000 // Hz
#define TX_OUTPUT_POWER                             22        // dBm
#define LORA_BANDWIDTH                              0         // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30 // Define the payload size here

char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);

typedef enum
{
    LOWPOWER,
    STATE_RX
} States_t;

States_t state;
int16_t Rssi, rxSize;

bool receiveflag = false; // software flag for LoRa receiver, received data makes it true.
int16_t rxNumber = 0; // counter for the number of messages received

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
  rxNumber++;
  Rssi = rssi;
  rxSize = size;
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';
  Radio.Sleep();
  Serial.printf(rxpacket);
  receiveflag = true;
  state = STATE_RX;
}

void lora_init(void)
{
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  Rssi = 0;
  rxSize = 0;
  RadioEvents.RxDone = OnRxDone;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);

  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);
  state = STATE_RX;
}

/********************************* lora  *********************************************/

SSD1306Wire oledDisplay(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr, freq, i2c group, resolution, rst

void WIFISetUp(void)
{
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin("SSID", ""); // fill in "Your WiFi SSID", "Your Password"
  delay(100);

  byte count = 0;
  while (WiFi.status() != WL_CONNECTED && count < 10)
  {
    count++;
    delay(500);
    oledDisplay.drawString(0, 0, "Connecting...");
    oledDisplay.display();
  }

  oledDisplay.clear();
  if (WiFi.status() == WL_CONNECTED)
  {
    oledDisplay.drawString(0, 0, "Connecting...OK.");
  }
  else
  {
    oledDisplay.drawString(0, 0, "Connecting...Failed");
  }
  oledDisplay.drawString(0, 10, "WIFI Setup done");
  oledDisplay.display();
  delay(500);
}

void setup()
{
  Serial.begin(115200);
  oledDisplay.init();
  oledDisplay.clear();
  oledDisplay.display();

  WIFISetUp();
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  delay(100);

  lora_init();
  oledDisplay.drawString(0, 30, "Waiting for LoRa data...");
  oledDisplay.display();
  delay(100);
  oledDisplay.clear();
}

void loop()
{
  if (receiveflag && (state == LOWPOWER))
  {
    receiveflag = false;
    char packet[128], packSize[128], numMessages[128];
    snprintf(packet, sizeof(packet), "R_data: %s", rxpacket);
    snprintf(packSize, sizeof(packSize), "R_Size: %d R_rssi: %d", rxSize, Rssi);
    snprintf(numMessages, sizeof(numMessages), "Messages: %d", rxNumber);

    oledDisplay.drawString(0, 20, packet);
    oledDisplay.drawString(0, 40, packSize);
    oledDisplay.drawString(0, 50, numMessages);
    oledDisplay.display();
    delay(1000);
    oledDisplay.clear();
  }

  switch (state)
  {
    case STATE_RX:
      Radio.Rx(0);
      state = LOWPOWER;
      break;
    case LOWPOWER:
      Radio.IrqProcess();
      break;
    default:
      break;
  }
}
