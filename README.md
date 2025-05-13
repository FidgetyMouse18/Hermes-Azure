# Hermes - Azure

**Authors:** Walter Nedov, Joshua Pinti, Cheng Bin Saw

**Topic C: Trusted Environmental Supply Chain Monitoring**

## Overview

Hermes-Azure is a trusted environmental monitoring system designed for supply chains handling sensitive goods such as pharmaceuticals and food. It uses distributed mobile sensor nodes and secure data transmission to ensure product integrity across all stages of the supply chain.

## System Architecture

* **Mobile Nodes (e.g. Nordic Thingy:52):**
  Deployed at various points along the supply chain, these nodes monitor environmental parameters such as:

  * Temperature
  * Humidity
  * Gas levels
  * CO₂ concentration
  * Magnetic interference

* **Data Transmission:**
  Mobile nodes send sensor data via Bluetooth Low Energy (BLE) to a nearby **Base Node**.

* **Base Node (e.g. ESP32):**
  Receives BLE packets and uploads the data to a **secure blockchain** over Wi-Fi, ensuring data immutability and transparency.

* **M5Core2 Display Unit:**
  Can access the data via Wi-Fi or directly receive BLE packets. It displays real-time graphs representing the environmental snapshot.

* **Desktop Dashboard:**
  Provides a more detailed interface for visualizing historical and live data from across the entire supply chain.

## Key Features

* Distributed environmental monitoring
* Real-time BLE communication
* Secure data logging using blockchain technology
* Multi-platform visualization (M5Core2, Desktop)
