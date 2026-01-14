# Eastron-SDM72S-Data-Transmission-via-Heltec LoRa 32 (V3)-and-AWS-Integration

**Objective:**

The project aims to read data from an SDM72S energy meter using Modbus protocol, transmit this data over LoRa using Heltec LoRa v3 modules, and then handle the data with a Raspberry Pi 4 by saving it locally and forwarding it to AWS for further processing and storage.

**Components Used:**

- SDM72S Energy Meter: Provides electrical measurement data.
- Heltec LoRa v3 Modules: Facilitate wireless communication over long distances using LoRa technology.
- Raspberry Pi 4: Acts as the central hub for data reception, local storage, and AWS integration.
- AWS (Amazon Web Services): Cloud platform for data storage and processing.

**Process:**

1.  **Data Reading:** The SDM72S energy meter is connected to a Heltec LoRa v3 module configured as a Modbus master to read measurement data.
2.  **Data Transmission:** The SDM72S data is transmitted wirelessly over LoRa from the first Heltec LoRa v3 module to a second Heltec LoRa v3 module.
3.  **Data Reception:** The second Heltec LoRa v3 module receives the data and sends it to a Raspberry Pi 4.
4.  **Local Storage:** The Raspberry Pi 4 stores the received data locally on its filesystem.
5.  **AWS Integration:** Periodically or based on triggers, the Raspberry Pi 4 uploads the locally stored data to AWS for cloud-based storage and further analysis.

**Outcome:**

This project enables efficient long-range wireless transmission of energy meter data and integrates local data handling with cloud services, providing both immediate access and long-term storage for energy consumption analytics.

## Repository Structure

The repository is organized as follows:

```
.
├── docs
│   └── images
│       ├── Blockdiagram.pdf
│       ├── Communication.png
│       ├── Raspberry PI.png
│       ├── Receiver Architecture.png
│       └── Transmitter.png
├── src
│   ├── mqtt_broker
│   │   └── Raspberry_Pi_Broker.py
│   ├── receiver
│   │   ├── Energy_meter_RX.ino
│   │   └── images.h
│   └── transmitter
│       ├── images.h
│       └── Transmitter.ino
└── README.md
```

-   `src/`: Contains the source code for the project.
    -   `mqtt_broker/`: Contains the Python script for the MQTT broker running on the Raspberry Pi.
    -   `receiver/`: Contains the Arduino sketch for the Heltec LoRa v3 module acting as the receiver.
    -   `transmitter/`: Contains the Arduino sketch for the Heltec LoRa v3 module acting as the transmitter.
-   `docs/`: Contains documentation and diagrams.
    -   `images/`: Contains images, diagrams, and other visual assets.

## Getting Started

### Prerequisites

-   Hardware:
    -   Eastron SDM72S Energy Meter
    -   2 x Heltec LoRa 32 (V3) modules
    -   Raspberry Pi 4
    -   Necessary wiring and power supplies
-   Software:
    -   Arduino IDE
    -   Python 3
    -   Paho MQTT library for Python (`pip install paho-mqtt`)
    -   An AWS account

### Installation

1.  **Heltec LoRa Modules:**
    -   Install the Arduino IDE and the necessary board definitions for the Heltec LoRa 32 (V3).
    -   Install the required libraries for LoRa and Modbus communication.
    -   Upload the `Transmitter.ino` sketch to one Heltec LoRa module and the `Energy_meter_RX.ino` sketch to the other.

2.  **Raspberry Pi:**
    -   Set up your Raspberry Pi with Raspberry Pi OS.
    -   Install Python 3 and the Paho MQTT library:
        ```bash
        sudo apt-get update
        sudo apt-get install python3
        pip install paho-mqtt
        ```

## Usage

1.  **Hardware Setup:**
    -   Connect the SDM72S energy meter to the transmitter Heltec LoRa module.
    -   Connect the receiver Heltec LoRa module to the Raspberry Pi.
    -   Power on all components.

2.  **Run the MQTT Broker:**
    -   On the Raspberry Pi, run the `Raspberry_Pi_Broker.py` script:
        ```bash
        python3 src/mqtt_broker/Raspberry_Pi_Broker.py
        ```

3.  **Monitor Data:**
    -   The Raspberry Pi will start receiving data from the receiver module and logging it to a local file.
    -   The script will also forward the data to your configured AWS IoT endpoint.

## Contributing

Contributions are welcome! Please follow these steps to contribute:

1.  **Fork the repository.**
2.  **Create a new branch** (`git checkout -b feature/your-feature`).
3.  **Make your changes.**
4.  **Commit your changes** (`git commit -am 'Add some feature'`).
5.  **Push to the branch** (`git push origin feature/your-feature`).
6.  **Create a new Pull Request.**

Please make sure to update tests as appropriate.
