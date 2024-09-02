# MSP430 Sensor Interface 🚀

This embedded C project is developed using an MSP430FR2355 development board. The software monitors temperature readings provided by an NTC thermistor connected in a resistive bridge with a 1kΩ resistor, powered by a 3.3V supply. The temperature is read through the ADC module (Pin P1.1 - A1, ADC Channel 1) with a resolution of 10 bits.

## Key Features ✨

- **🌡️ Temperature Monitoring:** The temperature is calculated using the ADC values and converted to a readable format using a formula.
- **📊 Threshold Comparison:** The measured temperature is compared against a threshold value, which can be modified using buttons connected to pins P2.3 and P4.1.
- **⏲️ Periodic ADC Readings and Data Transfer through UART:** ADC readings and UART data transfer are triggered periodically using Timer B0 as a clock signal.
- **🖥️ I2C Interface:** The temperature and threshold values are displayed on an LCD screen via an I2C connection.
- **🚨 Visual and Audible Alerts:**
  - A **🔔 Buzzer** is activated when the temperature exceeds the threshold using Timer B1.
  - **💡 LED Indicators:** A green LED indicates the temperature is below the threshold, and a red LED indicates the temperature exceeds the threshold.
- **🔄 UART Communication:**
  - The temperature data can be transmitted via UART.
  - The threshold value can be increased or decreased using UART TX commands ("<" and ">", ASCII 60 and 62).

## Project Overview 🖼️

<img src="https://github.com/user-attachments/assets/55e61db3-7699-4fa4-a508-e7967d0ff5d5" alt="MSP430 Sensor Interface" width="500"/>
*Figure 1: Overview of the MSP430 Sensor Interface project showing connections and components.*

<img src="https://github.com/user-attachments/assets/2dc5a040-4a60-4561-98cb-20fd11c03869" alt="msp_pROJ_DIAGRAM" width="500"/>
*Figure 2: Block diagram illustrating the system architecture and data flow.*

---

Feel free to explore the code and contribute to this project!
