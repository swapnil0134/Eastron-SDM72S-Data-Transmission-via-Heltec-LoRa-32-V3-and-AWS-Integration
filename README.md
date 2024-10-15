# Eastron-SDM72S-Data-Transmission-via-LoRa-and-AWS-Integration
**Objective:**

    The project aims to read data from an SDM72S energy meter using Modbus protocol, transmit this data over LoRa using Heltec LoRa v3 modules, and then handle the data with a Raspberry Pi 4 by saving it locally and forwarding it to AWS for further processing and storage.

**Components Used:**

    SDM72S Energy Meter: Provides electrical measurement data.
    Heltec LoRa v3 Modules: Facilitate wireless communication over long distances using LoRa technology.
    Raspberry Pi 4: Acts as the central hub for data reception, local storage, and AWS integration.
    AWS (Amazon Web Services): Cloud platform for data storage and processing.

**Process:**

    Data Reading: The SDM72S energy meter is connected to a Heltec LoRa v3 module configured as a Modbus master to read measurement data.
    Data Transmission: The SDM72S data is transmitted wirelessly over LoRa from the first Heltec LoRa v3 module to a second Heltec LoRa v3 module.
    Data Reception: The second Heltec LoRa v3 module receives the data and sends it to a Raspberry Pi 4.
    Local Storage: The Raspberry Pi 4 stores the received data locally on its filesystem.
    AWS Integration: Periodically or based on triggers, the Raspberry Pi 4 uploads the locally stored data to AWS for cloud-based storage and further analysis.

**Outcome:**

    This project enables efficient long-range wireless transmission of energy meter data and integrates local data handling with cloud services, providing both immediate access and long-term storage for energy consumption analytics.
