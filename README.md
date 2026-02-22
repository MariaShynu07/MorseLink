<p align="center">
  <img src="./img.png" alt="Project Banner" width="100%">
</p>

# MorseLink 🎯

## Basic Details

### Team Name: SparX

### Team Members
- Member 1: Maria Shynu - TocH Institute of Science and Technology
- Member 2: Jisna Raju -  TocH Institute of Science and Technology


### Project Description
This project presents an innovative communication solution designed to bridge the gap between individuals with different sensory abilities. By utilizing Morse Code as a universal input language, the system facilitates real-time messaging between the visually impaired, the deaf, the mute, and non-disabled users. The system leverages ESP-NOW technology for low-latency peer-to-peer wireless communication, providing feedback through visual (LCD), auditory (Buzzer), and haptic (Vibration) channels.

### The Problem statement
In this mordern era most of us take for granted that we can just "see" a text or "hear" a voice, but for a huge group of people, those channels are completely blocked. When a blind person and a deaf person need to talk to each other, they’re often stuck in "sensory silos" because a device that helps one usually ignores the other. Most of the tech out there that actually fixes this is either way too expensive or so complicated you need a manual just to say "hello." Our project steps in to fix that by making communication a physical, universal felt experience that doesn't care whether you can see, hear, or speak—it just works for everyone.

### The Solution
The solution is a Universal Communication System that strips communication down to its simplest form: timing and touch using a universal language- Morse Code. Instead of forcing users to adapt to complex screens or sounds, the device adapts to the user by turning a single button-press into a simultaneous "triple-threat" output.
By using the ESP32 to broadcast messages over ESP-NOW, we've created a system that doesn't need Wi-Fi to work—it's just one device talking directly to another. This means a blind user can "hear" the message through the buzzer, a deaf user can "read" it on the LCD, and a deaf-blind user can "feel" it through the vibration motor, all at the same time.

---

## Technical Details

### Technologies/Components Used

**For Software:**

Languages used: C++ (Arduino Framework) for microcontroller logic.
Protocols used: * ESP-NOW: For low-latency, peer-to-peer wireless communication.
I2C (Inter-Integrated Circuit): To minimize wiring for the LCD interface.
Libraries used:
  -esp_now.h & WiFi.h: For wireless data transmission.
  -LiquidCrystal_I2C.h: For display management.
  -Wire.h: For I2C communication.
Tools used: Arduino IDE (for coding and debugging), Serial Monitor (for real-time logic testing).

**For Hardware:**

- Main components: ESP32 DevKit V1,16x2 LCD with I2C Module,Tactile Push Button,Vibration Motor,Active/Passive Buzzer
- Specifications:
                Wireless Range: Approx. 100-200 meters (Open space via ESP-NOW).
                Operating Voltage: 3.3V (Logic) and 5V (LCD/Motor).
                Input Latency: < 50ms for real-time feedback.
                Morse Thresholds: Dot (\le 400ms), Dash (> 400ms).
- Tools required: Soldering iron and lead,Breadboard and Jumper wires, Multimeter

---

## Features

Universal Multi-Sensory Output: Simultaneously delivers messages via Text (LCD), Sound (Buzzer), and Vibration (Haptic Motor), ensuring the system is fully accessible to the visually impaired, deaf, mute, and deaf-blind.
Real-Time Tactile Verification: Provides instant haptic and auditory feedback during the input process, allowing users to "feel" and "hear" their Morse code as they type for improved accuracy.
Infrastructure-Free Communication: Uses the ESP-NOW protocol for direct, peer-to-peer wireless transmission, allowing units to communicate instantly without needing Wi-Fi, routers, or cellular networks.
Simplified Single-Button Input: Replaces complex keyboards with a one-button interface that uses a timing-based engine to decode dots and dashes into English characters automatically.
Incoming Message Alerts: Features a distinct "triple-pulse" vibration and sound pattern to notify users of a received message, ensuring they are alerted even without looking at the display.
---


### For Hardware:

#### Components Required
 Main components: 
 ESP32 DevKit V1
 16x2 LCD with I2C Module 
 Tactile Push Button 
 Vibration Motor 2mm thickness 100mA current flow
 Active/Passive Buzzer


#### Circuit Setup
There are 2 esp32's acting as two different devices which are not interconnected but induvidually connected to a 16x2 I2C LCD via SDA (Pin 21) and SCL (Pin 22) for visual output. A tactile button (Pin 18) uses an internal pull-up for Morse input. For accessibility,Pin 15 of one esp32 drives a vibration motor and other esp32 runs buzzer simultaneously, providing haptic and audio feedback..


---

## Project Documentation


### For Hardware:

#### Schematic & Circuit

![Circuit](Add your circuit diagram here)
*Add caption explaining connections*

![Schematic](Add your schematic diagram here)
*Add caption explaining the schematic*

#### Build Photos

![Team](Add photo of your team here)

![Components](Add photo of your components here)
*List out all components shown*

![Build](Add photos of build process here)
*Explain the build steps*

![Final](Add photo of final product here)
*Explain the final build*

---

## Additional Documentation

---

### For Hardware Projects:

#### Bill of Materials (BOM)

| **Component**                                                  | **Quantity** | **Specifications / Notes**           | **Approx Price (₹)** | **Link / Source**                             |
| -------------------------------------------------------------- | ------------ | ------------------------------------ | -------------------- | --------------------------------------------- |
| **[ESP32 DEVKITV1 30 Pin CP2102 NodeMCU Development Board]()** | 2            | ESP32 Dev Kit V1 (Wi‑Fi & Bluetooth) | ~₹369 each           | Buy on Indian Hobby Center                    |
| **[Yellow Green 16x2 I2C LCD Display Module]()**               | 1            | 16×2 LCD with I2C adapter            | ~₹77                 | Great budget choice                           |
| **[Original JHD 16x4 Character LCD Display]()**                | 1            | 16×4 Character LCD                   | ~₹499                | Standard LCD                                  |
| **[I2C LCD Interface Module]()**                               | 2            | I2C interface for LCDs               | ~₹45 each            | For 16×2 & can be used on 16×4 (if supported) |
| **[PBS-06-112-25-Push Button Switch-2 Pin]()**                 | 2            | Basic push button switches           | ~₹8 each             | Small tactile switches                        |
| **[Vibration Motor]()**                                        | 1            | Small vibration motor                | ~₹41                 | For alerts/haptics                            |
| **[Buzzer with Wire 6V - 12V DC]()**                           | 1            | Basic DC buzzer (sound)              | ~₹26                 | Standard buzzer                               |


**Total Estimated Cost:** ₹1,610

---

## Assembly Instructions

### Step 1: Prepare Components
1. Gather all components from the BOM.
2. Check the specifications to ensure compatibility (ESP32, LCDs, I2C modules, buttons, motor, buzzer).
3. Prepare your workspace.



---

### Step 2: Build the Power Supply
1. Connect the 3.3V or 5V output from ESP32 to the breadboard positive rail (depending on your LCD voltage requirements).
2. Connect ESP32 GND to breadboard negative rail.



---

### Step 3: Connect the LCDs via I2C
1. Connect SDA and SCL pins of each I2C module to ESP32 SDA and SCL pins (usually GPIO 21 & 22).
2. Connect VCC and GND of the I2C module to the breadboard power rails.
3. Attach the I2C module to the back of the LCD.



---

### Step 4: Add Push Buttons
1. Place push buttons on the breadboard.
2. Connect one side to GND and the other side to digital pins on ESP32 (e.g., GPIO 32 & 33) with pull-up resistor configuration.



---

### Step 5: Connect Vibration Motor and Buzzer
1. Connect the positive terminal of the vibration motor to a digital pin via a transistor if needed (ESP32 cannot drive directly).
2. Connect motor GND to breadboard negative rail.
3. Connect buzzer in a similar manner to another digital pin.



---

### Step 6: Final Connections
1. Check all connections: power, I2C, digital pins, buttons, motor, buzzer.
2. Ensure no short circuits.
3. Power up the ESP32 and upload the code.



---

## Notes
- Use **I2C modules** for simpler wiring and to save ESP32 GPIO pins.
- Use transistors if the vibration motor or buzzer requires more current than ESP32 pins can safely provide.
- Adjust **digital pins** in the code as per your wiring.

---

## Project Demo

### Video
[Add your demo video link here - YouTube, Google Drive, etc.]

*Explain what the video demonstrates - key features, user flow, technical highlights*

### Additional Demos
[Add any extra demo materials/links - Live site, APK download, online demo, etc.]

---

## AI Tools Used (Optional - For Transparency Bonus)

If you used AI tools during development, document them here for transparency:

**Tool Used:** ChatGPT, Claude,Gemini

**Purpose:** [What you used it for]
- Generating code
- debugging assistance
- finalsuggestions

**Percentage of AI-generated code:** 60%

**Human Contributions:**
- idea generation and execution
- circuit designing
- Integration and testing

## Team Contributions

- Maria Shynu: integration and coding
- Jisna Raju: Integration , documentation and testing 


---

Made with ❤️ at TinkerHub
