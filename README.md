# ESP32 AirChat (Offline Captive Portal)
(demo.png)

A completely self-contained, local messaging system running on an ESP32 microcontroller. When users connect to the "AirChat" Wi-Fi network, a captive portal automatically opens on their device, allowing them to send and receive messages in real-time. No internet, no apps, and no external servers required.

# Features
*   **Zero Setup for Users:** Connects instantly via a Wi-Fi Captive Portal (like hotel or airport Wi-Fi).
*   **100% Offline:** Runs entirely on the ESP32 hardware without needing an internet connection.
*   **Real-Time Updates:** The frontend uses JavaScript polling to fetch new messages every 2 seconds.

# Tech Stack
*   **Hardware:** ESP32 (e.g., ESP32 Dev Module)
*   **Backend:** C++ (Arduino IDE), `WebServer.h`, `DNSServer.h`
*   **Frontend:** HTML, CSS, JavaScript (Fetch API)
*   **Networking:** Wi-Fi Access Point (SoftAP), DNS Interception.

# How it Works
1. The ESP32 creates an open Wi-Fi network called **AirChat**.
2. A DNS server on the ESP32 intercepts all requests (e.g., if a phone tries to load google.com) and redirects them back to the ESP32's local IP (`192.168.4.1`).
3. The ESP32 hosts a lightweight web server serving an HTML/JS chat page.
4. Messages are sent to the ESP32 via HTTP GET requests and stored in the ESP32's RAM.
5. The webpage fetches new messages as JSON and updates the UI.

# Known Limitation
*   **Memory Limit:** The ESP32 only stores a maximum of 20 messages in RAM to prevent memory overflow. Rebooting the ESP32 clears the chat history.
