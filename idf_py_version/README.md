# SafeHat WorkNet: A Connected Safety Solution with Mesh Networking and Real-Time Monitoring

![IoT](https://img.shields.io/badge/IoT-Safety%20Helmet-blue)  
![ESP32](https://img.shields.io/badge/ESP32-Mesh%20Network-orange)  
![Sensors](https://img.shields.io/badge/Sensors-Multifunctional-green)  
![Safety](https://img.shields.io/badge/Alerts-Real%20Time-red)

SafeHat WorkNet is a smart safety system designed to revolutionize workplace safety. This project integrates advanced sensors, a self-healing mesh network, and real-time alert mechanisms into a hard hat, providing comprehensive monitoring and protection for industrial and construction workers.

---

## Features

### 1. **Advanced Environmental and Positional Sensing**
- **Temperature and Humidity Sensors**: Monitor worksite conditions to prevent heat stress and maintain comfort.
- **Gas Sensor**: Detect hazardous gas levels and provide immediate warnings.
- **Light Sensor**: Adjust for ambient light conditions to optimize visibility.
- **GPS Module**: Track worker location for safety and coordination.
- **Accelerometer and Gyroscope**: Detect falls, impacts, or irregular movements.

### 2. **Self-Healing Mesh Network**
- Employs **ESP32 microcontrollers** to create a robust and fault-tolerant network.
- Enables seamless communication between workers’ helmets, ensuring consistent data flow.
- Automatically reroutes data in case of device failure, maintaining network integrity.

### 3. **Onboard Display**
- Displays critical real-time metrics, including environmental readings, location, and alerts, directly on the helmet.

### 4. **Camera with Event Buffer**
- Integrated camera records the **previous 20 seconds** of footage upon detecting impacts or falls.
- **MicroSD card support** for secure, local storage of video data.

### 5. **Real-Time Alerts**
- **Piezo buzzer** provides immediate alerts for hazardous gas levels or other critical events.

### 6. **Battery Health Monitoring**
- Continuously tracks battery status to ensure uninterrupted operation and alerts workers to low battery levels.

---

## Applications

- **Industrial and Construction Worksites**: Enhances worker safety and environmental monitoring.
- **Hazardous Environments**: Provides real-time alerts and monitoring in mines, chemical plants, and similar high-risk areas.
- **Incident Analysis**: Post-incident video and data review for improved safety protocols.
- **Remote Monitoring**: Facilitates centralized oversight of multiple workers through mesh networking.

---

## Challenges and Solutions

- **Sensor Integration**: Ensuring compatibility and seamless communication among diverse sensors.
- **Power Management**: Optimized for low power consumption to maximize operational time.
- **Mesh Network Stability**: Designed to handle dynamic environments with minimal latency and reliable rerouting.
- **Scalability**: Easily add nodes to the network without compromising performance.

---

## Parts List

For a detailed list of components used in this project, refer to our **[Parts List](https://docs.google.com/spreadsheets/d/example-link/edit#gid=0)**.

---

## Getting Started

To build and deploy your own **SafeHat WorkNet**, follow these steps:

1. **Hardware Assembly**: Install sensors, camera, and ESP32 modules on the helmet. Connect to a secure power source.
2. **Firmware Installation**: Use the provided firmware to initialize sensors and establish the mesh network.
3. **Calibration and Testing**: Calibrate sensors for accuracy and test the network in your worksite environment.
4. **Deploy and Expand**: Deploy helmets to workers and expand the network by adding more nodes as needed.

---

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for more details.

---

## Acknowledgments

- **ESP32 Community** for providing the foundation for mesh networking.
- **Open-Source Sensor Libraries** for enabling easy integration of diverse sensors.
- The dedicated team behind SafeHat WorkNet for their vision and innovation.
