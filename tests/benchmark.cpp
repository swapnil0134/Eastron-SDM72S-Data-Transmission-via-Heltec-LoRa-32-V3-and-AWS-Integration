#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <iomanip>

// Mock ModbusMaster
class ModbusMaster {
public:
    static const uint8_t ku8MBSuccess = 0x00;

    int transactionCount = 0;
    int totalBytesTransferred = 0;
    std::vector<uint16_t> buffer;

    void begin(uint8_t, int) {}
    void preTransmission(void (*)()) {}
    void postTransmission(void (*)()) {}

    uint8_t readInputRegisters(uint16_t u16ReadAddress, uint8_t u16ReadQty) {
        transactionCount++;
        // Request: Addr(1) + Func(1) + Start(2) + Count(2) + CRC(2) = 8 bytes
        // Response: Addr(1) + Func(1) + Bytes(1) + Data(2*N) + CRC(2) = 5 + 2*N bytes
        totalBytesTransferred += 8 + (5 + 2 * u16ReadQty);

        buffer.clear();
        for (int i = 0; i < u16ReadQty; i++) {
            buffer.push_back(0); // Mock data
        }
        return ku8MBSuccess;
    }

    uint16_t getResponseBuffer(uint8_t u8Index) {
        if (u8Index < buffer.size()) return buffer[u8Index];
        return 0;
    }
};

ModbusMaster node;

void preTransmission() {}
void postTransmission() {}

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

float params[17];

void run_original() {
  node.transactionCount = 0;
  node.totalBytesTransferred = 0;

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

  std::cout << "Original Logic:" << std::endl;
  std::cout << "  Transactions: " << node.transactionCount << std::endl;
  std::cout << "  Total Bytes: " << node.totalBytesTransferred << std::endl;
}

// Proposed Optimization Logic
void readModbusBlock(uint16_t startAddress, uint8_t numRegisters, int paramStartIndex) {
  uint16_t result;

  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  result = node.readInputRegisters(startAddress, numRegisters);

  if (result == node.ku8MBSuccess) {
    for (int i = 0; i < numRegisters / 2; i++) {
        if (paramStartIndex + i < 17) {
            params[paramStartIndex + i] = reform_uint16_2_float32(
                node.getResponseBuffer(i * 2),
                node.getResponseBuffer(i * 2 + 1)
            );
        }
    }
  } else {
    for (int i = 0; i < numRegisters / 2; i++) {
        if (paramStartIndex + i < 17) {
            params[paramStartIndex + i] = 0.0;
        }
    }
  }
}

void run_optimized() {
  node.transactionCount = 0;
  node.totalBytesTransferred = 0;

  // Block 1: 0x0000 to 0x0011 (18 registers) -> params[0] to params[8]
  readModbusBlock(0x0000, 18, 0);

  // Block 2: 0x001E to 0x0023 (6 registers) -> params[9] to params[11]
  readModbusBlock(0x001E, 6, 9);

  // Block 3: 0x00C8 to 0x00CD (6 registers) -> params[12] to params[14]
  readModbusBlock(0x00C8, 6, 12);

  // Standalone
  params[15] = getRTU(0x00E0);
  params[16] = getRTU(0x0046);

  std::cout << "Optimized Logic:" << std::endl;
  std::cout << "  Transactions: " << node.transactionCount << std::endl;
  std::cout << "  Total Bytes: " << node.totalBytesTransferred << std::endl;
}

int main() {
    run_original();
    std::cout << "----------------" << std::endl;
    run_optimized();
    return 0;
}
