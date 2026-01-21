#include <ModbusMaster.h>
#include <HardwareSerial.h>
#include "Arduino.h"
#include "LoRaWan_APP.h"
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include "images.h"

/********************************* Modbus  *********************************************/
float params[17]; // Array to hold 17 parameters

#define RS485_TX 48   // TX pin of Heltec LoRa V3 connected to DI of HW-097
#define RS485_RX 19   // RX pin of Heltec LoRa V3 connected to RO of HW-097
#define RS485_DE_RE 4 // GPIO pin controlling DE/RE of HW-097 DO NOT CONNECT DE
#define MODBUS_ADDR 1 // Modbus address of SDM72D-M

HardwareSerial SerialMod(1); // Serial1 for RS485

ModbusMaster node;

void preTransmission() {
  digitalWrite(RS485_DE_RE, HIGH); // Enable RS485 Transmit
}

void postTransmission() {
  digitalWrite(RS485_DE_RE, LOW); // Enable RS485 Receive
}

float reform_uint16_2_float32(uint16_t u1, uint16_t u2) {
  uint32_t num = ((uint32_t)u1 & 0xFFFF) << 16 | ((uint32_t)u2 & 0xFFFF);
  float numf;
  memcpy(&numf, &num, 4);
  return numf;
}

float getRTU(uint16_t m_startAddress) {
  uint8_t m_length = 2;
  uint16_t result;
  float value = 0.0;

  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  result = node.readInputRegisters(m_startAddress, m_length);
  
  if (result == node.ku8MBSuccess) {
    value = reform_uint16_2_float32(node.getResponseBuffer(0), node.getResponseBuffer(1));
  }
  
  return value;
}

/********************************* Modbus  *********************************************/

/********************************* LoRa  *********************************************/
#define RF_FREQUENCY                                868000000 // Hz
#define TX_OUTPUT_POWER                             10        // dBm
#define LORA_BANDWIDTH                              0         // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false
#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 256       // Define the payload size here

char txpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;
void OnTxDone(void);
void OnTxTimeout(void);

typedef enum {
    LOWPOWER,
    STATE_TX,
    STATE_WAIT
} States_t;

int16_t txNumber;
int16_t rxNumber;
States_t state;
bool sleepMode = false;
int16_t Rssi;

void OnTxDone(void) {
  Serial.print("TX done...");
  state = STATE_WAIT;
}

void OnTxTimeout(void) {
  Radio.Sleep();
  Serial.print("TX Timeout...");
  state = STATE_WAIT;
}

void lora_init(void) {
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  txNumber = 0;
  Rssi = 0;
  rxNumber = 0;
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;

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
  state = STATE_TX;
}
/********************************* LoRa  *********************************************/

/********************************* OLED  *********************************************/
SSD1306Wire factory_display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr, freq, i2c group, resolution, rst

void logo() {
  factory_display.clear();
  factory_display.drawXbm(0, 5, logo_width, logo_height, (const unsigned char *)logo_bits);
  factory_display.display();
}

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}
/********************************* OLED  *********************************************/

void setup() {
  Serial.begin(115200);
  VextON();
  delay(100);
  factory_display.init();
  factory_display.clear();
  factory_display.display();
  logo();
  delay(300);
  factory_display.clear();

  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);
  
  SerialMod.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX); // RS485 serial port
  delay(2000);
  Serial.println("Ready");

  node.begin(MODBUS_ADDR, SerialMod);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  lora_init();
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
}

void loop() {
  static int messageIndex = 0;
  static unsigned long lastTransmissionTime = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastTransmissionTime >= 10000 || lastTransmissionTime == 0) {
    // Read all 17 parameters
    params[0] = getRTU(0x0000);   // Phase 1 line to neutral volts
    params[1] = getRTU(0x0002);   // Phase 2 line to neutral volts
    params[2] = getRTU(0x0004);   // Phase 3 line to neutral volts

    params[3] = getRTU(0x0006);   // Phase 1 current
    params[4] = getRTU(0x0008);   // Phase 2 current
    params[5] = getRTU(0x000A);   // Phase 3 current

    params[6] = getRTU(0x000C);   // Phase 1 reactive power
    params[7] = getRTU(0x000E);   // Phase 2 reactive power
    params[8] = getRTU(0x0010);   // Phase 3 reactive power

    params[9] = getRTU(0x001E);  // Phase 1 power factor
    params[10] = getRTU(0x0020);  // Phase 2 power factor
    params[11] = getRTU(0x0022);  // Phase 3 power factor

    params[12] = getRTU(0x00C8);  // Line 1 to Line 2 volts
    params[13] = getRTU(0x00CA);  // Line 2 to Line 3 volts
    params[14] = getRTU(0x00CC);  // Line 3 to Line 1 volts

    params[15] = getRTU(0x00E0);  // Neutral current
    params[16] = getRTU(0x0046);  // Frequency of supply voltages

    // Create payload for the current message
    String payload = "";
    payload.reserve(80); // Pre-allocate memory
    bool firstParam = true;
    for (int i = 0; i < 3; i++) {
      int paramIndex = messageIndex * 3 + i;
      if (paramIndex < 17) {
        if (!firstParam) {
          payload += ','; // Append comma if not the first parameter
        }
        payload += 'P';
        payload += (paramIndex + 1);
        payload += ':';
        payload += String(params[paramIndex], 2); // Append float with 2 decimal places
        firstParam = false;
      }
    }

    Serial.println("Sending payload: " + payload);
    payload.toCharArray(txpacket, BUFFER_SIZE);

    // Display the payload on the OLED
    factory_display.clear();
    factory_display.drawString(0, 0, "Smart Manufacturing");
    factory_display.drawString(0, 10, "Data Hub");
    factory_display.drawString(0, 40, "Sending:");
    factory_display.drawString(0, 50, payload);
    factory_display.display();
    delay(1000);
    factory_display.clear();
    
    txNumber++;
    Serial.printf("\r\nsending packet \"%s\" , length %d\r\n", txpacket, strlen(txpacket));
    Radio.Send((uint8_t *)txpacket, strlen(txpacket));
    state = LOWPOWER;
    lastTransmissionTime = currentTime;
    messageIndex = (messageIndex + 1) % 6; // Increment message index
  }

  switch(state) {
    case LOWPOWER:
      Radio.IrqProcess();
      break;
    case STATE_WAIT:
      state = STATE_TX;
      break;
    default:
      break;
  }
}
