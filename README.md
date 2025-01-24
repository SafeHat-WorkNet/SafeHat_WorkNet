# MeshNet: A Smart Security System with Self-Healing Mesh Network and Local AI/ML Capabilities

![IoT](https://img.shields.io/badge/IoT-Security%20System-blue)  
![ESP32](https://img.shields.io/badge/ESP32-CAM-orange)  
![AI/ML](https://img.shields.io/badge/AI%2FML-Edge%20Computing-green)  
![Mesh Network](https://img.shields.io/badge/Network-Self%20Healing-red)

MeshNet is a smart security system designed for the **TAMUhack 2025 Hardware IoT Track**. It leverages multiple ESP32 devices with cameras to create a self-healing mesh network with local AI/ML capabilities for real-time analysis. This project showcases the potential of combining IoT, AI, and distributed systems to build a cost-effective, scalable, and resilient security system.

---

## Features

### 1. **ESP32 Devices with Cameras**
- Each ESP32 is equipped with a camera module (e.g., ESP32-CAM) to capture video or images.
- Camera data is processed locally or streamed to other nodes or a central system.
- Devices are strategically placed for full coverage of the monitored area.

### 2. **Self-Healing Mesh Network**
- Multiple ESP32 devices form a mesh network for communication without relying on a central router.
- If one device fails or goes offline, the network automatically reroutes data through alternate paths, ensuring uninterrupted operation.
- The mesh network distributes workload and enhances fault tolerance.

### 3. **Local AI/ML Model**
- AI/ML models are deployed locally on each ESP32 (or groups of ESP32s) to perform real-time tasks like:
  - Object detection (e.g., recognizing intruders, distinguishing between humans and animals).
  - Facial recognition for access control.
  - Anomaly detection to identify unusual activities.
- The ML model is optimized for lightweight, edge-based computing to fit within the resource constraints of ESP32s.

### 4. **Parallel Computing for the ML Model**
- The project leverages parallel computing by distributing model inference tasks across multiple ESP32 devices.
- For computationally intensive operations, tasks are divided, processed by different devices, and results are aggregated.
- This approach maximizes processing power while maintaining energy efficiency.

### 5. **Security System Capabilities**
- Event-based alerts: ESP32 devices trigger alerts (e.g., send notifications or alarms) based on detected events like movement or specific objects.
- Local data processing: Reduces the need for cloud dependency, improving privacy and response time.
- Low latency: Critical for real-time analysis and decision-making.
- Energy efficiency: Essential for long-term operation, especially in battery-powered setups.

### 6. **Expandable and Scalable Design**
- The system is designed to easily add new ESP32 nodes to the network for extended coverage.
- Supports modular updates to AI/ML models for improving detection accuracy or adding new features.

### 7. **Self-Healing Mechanism**
- Mesh nodes automatically detect and compensate for network failures by redistributing tasks.
- Ensures continuous coverage and robust performance even during partial system outages.

---

## Applications

- **Home Security**: Detects and alerts for unauthorized access or unusual activities.
- **Industrial Monitoring**: Monitors equipment or restricted areas in factories or warehouses.
- **Agricultural Surveillance**: Tracks movement in fields and detects potential threats to crops or livestock.
- **Disaster Recovery**: Rapidly deployable surveillance system in disaster zones where infrastructure is damaged.

---

## Challenges and Considerations

- **Computational Load**: AI/ML models need to be highly optimized for ESP32's limited resources.
- **Power Management**: Efficient power consumption is crucial for long-term deployment.
- **Mesh Network Optimization**: Ensuring reliable data transmission and low latency across the network.
- **Security**: Implementing encryption and secure communication protocols to protect sensitive data.
- **Integration**: Balancing local processing with potential cloud integration for heavy computations or storage.

---

## Parts List

For a detailed list of components used in this project, check out our **[Parts List](https://docs.google.com/spreadsheets/d/1McdBwdMqhl6GKV4Jj4hYkLqE92k9_GQFfjGTEWFoEXA/edit?gid=0#gid=0)**.

---

## Getting Started

To set up MeshNet on your own, follow these steps:

1. **Hardware Setup**: Assemble the ESP32-CAM modules and connect them to power sources.
2. **Mesh Network Configuration**: Use the provided firmware to establish a self-healing mesh network.
3. **AI/ML Model Deployment**: Load the optimized AI/ML models onto the ESP32 devices.
4. **Testing and Calibration**: Test the system in your desired environment and calibrate the cameras and sensors for optimal performance.
5. **Expand and Scale**: Add more ESP32 nodes to the network as needed.

---

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---

## Acknowledgments

- **TAMUhack 2025** for providing the platform to showcase this project.
- **ESP32 Community** for their extensive documentation and support.
- **OpenCV** and **TensorFlow Lite** for enabling lightweight AI/ML on edge devices.

---
