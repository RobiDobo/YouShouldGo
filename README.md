# YouShouldGo - Transit Tracker (Based on Tranzy's API)
A full-stack IoT transit tracking system that notifies users when to leave to catch their bus/tram. This project integrates real-time transit data with hardware, using a Java backend for logic and C++ for firmware (you can select which station to track and display from both web and from the micrcontroller).

![Fronend web interfacet](TransitTrackerWeb.png)


## Technical Stack

### Backend & Business Logic
* **Java Spring Boot**: Powers the core API and manages the backend business logic.
* **Tranzy API Integration**: Consumes real-time transit data to calculate bus proximity to stations.
* **Data Processing**: Transforms raw transit coordinates into actionable "stations until arrival" metrics.

### IoT & Firmware
* **C++ / Arduino**: Custom firmware developed for the **T-Display-S3 (ESP32)** development board.
* **Hardware Integration**: Manages WiFi connectivity and renders live (polled every 15s) transit status directly to the LCD.
* **PlatformIO**: Used for environment configuration, library management, and flashing the microcontroller.

### Web Frontend
* **HTML5 & CSS3**: Provides a clean interface for user configuration (data not stored between sessions).
* **JavaScript**: Asynchronous communication between browser and the backend API.

## System Overview
1. **Setup**: The **Spring Boot** backend polls the Tranzy API for Agency, Route and Trip information in order to build a dictionary with all the station information(geolocation, index in a trip's station order).
2. **Data Acquisition**: The **Spring Boot** backend polls the Tranzy API for live vehicle positions.
3. **Logic Execution**: The backend calculates the "Stops Away" or "Minutes to Departure" based on processed transit data.
4. **Hardware Sync**: The **ESP32**, running optimized **C++**, fetches these updates via REST endpoints.
5. **Physical Alert**: The T-Display-S3 shows the real-time status (e.g., "3 stops away", "Next Stop", "In Station", "All vehicles have passed your station") so the user knows exactly when to depart.

I recommend using the Platform.io Visual Studio extension to manage the libraries and flashing the microcontroller. Tested on t-display-s3 esp32 development board.


You can try it out for  yourself at https://youshouldgo.onrender.com/

Link to demo video: https://drive.google.com/file/d/1rAULeCVtJAD0p6ob4Yf_5ieK7WKnjSjT/view?usp=sharing

![MicroController screen Screenshot](TransitTrackerIoT.png)

![Backend endPoints Screenshot](WebEndpoints.png)
