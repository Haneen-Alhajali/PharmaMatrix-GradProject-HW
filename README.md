# PharmaMatrix – Smart Pharmacy Hardware System  
Graduation Project – Arduino Mega 2560  

PharmaMatrix is a smart hardware solution designed to automate pharmacy cabinet management.  
It provides real-time shelf monitoring, medicine detection, barcode scanning, robotic arm control, and a full inventory tracking system.

---

## 🚀 Project Overview

The goal of PharmaMatrix is to build an automated smart pharmacy cabinet capable of:

- Detecting available medicine inside each shelf  
- Identifying medicines using barcode scanning  
- Automatically inserting and retrieving medicine boxes using a robotic arm  
- Monitoring stock levels in real time  
- Sending status updates to an external system (dashboard/server)

The system relies on Arduino Mega 2560 for hardware control and modular code structuring for easier scaling.

---
## 🔧 Hardware Components

- **Arduino Mega 2560**  
- **Ultrasonic Sensors (x2 or more)** – shelf occupancy detection  
- **Stepper Motor + A4988 Driver** – robotic arm movements  
- **Barcode Scanner Module** – identifying medicine boxes  
- **LED Indicators** – showing states and errors  
- **Power Supply 12V / 5V**  
- **Wiring, connectors, cabinet structure**  

---

## 🧩 Features Implemented (Phase 1)

### ✓ Shelf Distance Detection  
Using ultrasonic sensors mounted on the top of each shelf to detect:

- Whether the shelf is empty or filled  
- Approximate number of medicine boxes (estimation based on height)  

### ✓ Modular Code Structure  
The project uses object-oriented C++ (classes) to keep the code:

- Clean  
- Scalable  
- Easy to expand  

---

## 📡 How the Ultrasonic System Works

Each shelf contains an ultrasonic sensor that measures the distance between:

- The sensor (mounted at the top)  
- The top of the highest medicine box  

**Logic:**

- If distance ≈ shelf height → **Shelf is empty**  
- If distance < shelf height → **Shelf contains medicine**  
- Decreasing distance = **more boxes**  

---
## 👥 Team  
PharmaMatrix Hardware Team – 2025  

- **Developers:**  
Haneen + Deema  
- **Advisor:**  
De.Abdallah
---

## 📜 License
This project is created for academic purposes as part of a graduation requirement.  
Not licensed for commercial distribution unless extended in future.

