# Tea-for-Three
Tea for Three is an interactive IoT installation where three connected devices communicate through touch, creating a shared experience between participants.


## 1. Introduction

Project "Tea for Three" is a set of three connected ambient tea devices designed to support a subtle sense of shared presence between close family members or friends living in different places. Developed in response to the theme: Connected Ambient Devices Across the Miles, the project explores how a familiar daily ritual can become a medium for remote connection. The following sections explain the design context, user problem, objectives and early design iterations that shaped the final prototype.

*(可复现说明待补充)*

---

## 2. Literature Review

"Tea for Three" draws on calm technology and ambient display research. Weiser and Brown (1997) argued that well-designed technology should move between the centre and periphery of human attention, informing users without constantly demanding focus. The idea of "Tea for Three" is closely related to ambient displays, which communicate information through subtle changes in light, sound or movement. Wisneski et al. (1998) showed how digital information can be embedded into physical space rather than displayed only on screens, while Mankoff et al. (2003) identified low cognitive load and intuitive state representation as important qualities for ambient displays. In this project, the LED feedback is intended to be glanceable and peripheral, so users can notice presence without actively reading a screen.

The project also relates to minimal communication and remote presence. Conventional remote communication tools, such as messages, phone calls and video calls, are useful, but they require active initiation, attention and often language-based exchange. Kaye (2006) showed that even a minimal one-bit communication device could become emotionally meaningful when users shared an understanding of the gesture. "Tea for Three" applies this idea by reducing remote participation to one physical action: placing a cup.

Finally, the project connects with slow technology. Hallnäs and Redström (2001) argued that technology can support reflection and presence rather than only efficiency. "Tea for Three" follows this direction: it does not notify, measure or demand a reply. Instead, it creates a quiet shared moment through a slow and familiar tea ritual.

---

## 3. Problem Statement and Scenario

### 3.1 Motivation

Close relationships maintained across geographical distance often lose small daily moments of togetherness. Video calls and messages are effective for direct communication, but they can feel too deliberate or demanding for casual presence. Many people may want to express "I am here" or "I am joining you" without starting a full conversation. Based on this problem, the project asks: *How can a set of connected ambient tea devices support a subtle sense of shared presence between close family members or friends who are physically separated?*

### 3.2 User Scenario

"Tea for Three" is designed for small groups who share a close relationship but live apart. This includes international students and their families, friends separated by work or migration, and relatives who cannot meet regularly. These users do not necessarily need another notification-based communication platform. Instead, they may benefit from a quiet and unobtrusive way to feel connected during an ordinary part of the day.

---

## 4. Project Objectives

### 4.1 Conceptual and Emotional Objective

"Tea for Three" aims to explore how connected devices can preserve the emotional bond between close family members or friends when sharing a cup of tea across a distance, building a bridge that conveys a sense of presence, care and companionship.

### 4.2 Technical Objective

The technical objective is to develop an interconnected system comprising three independent yet synchronously operating devices. Each device should detect interactions by local users and provide feedback to the other devices. The system prototype should be capable of gathering environmental information and communicating in real time over long distances.

### 4.3 Design, Form and Commercialisation Objective

The product prototype must convey a sense of warmth, ritual and family togetherness, whilst ensuring practicality in manufacturing and demonstrating commercial potential. Its design should be intuitive and visually appealing, capable of being mass-produced using feasible manufacturing methods, and suitable for further optimisation as a consumer-facing connected product.

---

## 5. Design Iterations

### 5.1 Initial Concept

The initial design featured an oval-shaped tray with two indicator lights, a space for a teapot and a space for a teacup. The surface of the tray is engraved with curved patterns, which provide a stable base for the teacup and teapot, and also allow water droplets to flow along the grooves towards the drain. However, in view of waterproofing and electrical safety concerns, the load-bearing capacity of the tray material, and the uncertainty regarding the size of the teapot, this proposal was rejected. Instead, the solution adopted was to place a decorative teapot-style indicator on the tray.

### 5.2 3D Printing Version

Early versions attempted to use 3D printing technology to manufacture the entire device, including the casing, tray and decorative elements. However, this design presented a number of problems: the production cycle was lengthy, it required a large amount of material, and the finished product had a cheap feel to it, with rough surface details, making it difficult to meet the requirements of a household tea set. Furthermore, it proved quite challenging to neatly integrate sensors, wires and LED lights within a single printed component.

### 5.3 Laser-cut + 3D Print with Soft-wrapped Version

The final version replaces the original single-printed body with a laser-cut plywood base and a 3D-printed shell. Wood was chosen for the tray material due to its inherent warmth, which naturally evokes a sense of calm and tranquillity. Furthermore, wood is well-suited to modular assembly and replication. This version also features a soft material (faux leather) wrapping the outer shell, enhancing the product's aesthetic appeal and lending it a homely feel. The decorative elements remain 3D-printed, and the teapot-shaped light diffuser is retained as the core design feature. This hybrid manufacturing approach makes the device easier to assemble, repair and replicate.

---

## 6. Prototype Overview

### 6.1 Product Form

*(外观照片待插入)*

The 'Tea Gathering for Three' product comprises three interconnected ambient devices. These devices can be used in different spaces and remain synchronised via network communication. Each device features a cup-holding area that serves as the user interaction zone. There are also two indicator lights shaped like teacups and one shaped like a teapot. Each device is powered via USB, catering to the vast majority of usage scenarios.

### 6.2 User Instruction

Once a user begins drinking tea, they place their teacup on the sensor area embedded with a pressure sensor. The device recognises the user's presence and transmits this status to the connected system. When other devices receive this message, the indicator light corresponding to that user on the device will light up. If the system detects that all three users are online, the teapot-shaped indicator light will illuminate with a special ambient lighting effect. This collective feedback represents the moment when all participants have joined the remote tea gathering.

### 6.3 System Components

*(系统架构图待插入)*

- Arduino MKR WiFi 1010
- Adafruit NeoPixel Stick
- FSR402 Force Sensitive Resistor
- WS2812B RGB LED Strip
- PLA Plastic
- Maple Wood
- Faux leather

---

## 7. Final Prototype & Implementation

This section details the hardware and software system implementation of the final Tea for Three prototype. The system consists of three identical physical devices, achieving low-latency remote state synchronization through IoT technology.

### 7.1 Hardware & Circuit Design

The core microcontroller of the system is the **Arduino MKR WiFi 1010**, combined with an **FSR402 force sensitive resistor** to detect the cup placement status, providing visual feedback via **WS2812B RGB and Adafruit NeoPixel Stick LED strips**.

Regarding circuit wiring, to ensure the stable operation of multiple LEDs, the system utilizes an external, independent 5V power supply. The specific pin assignments are as follows:

- **LED Data Pins**: LED1 (representing participant B) is connected to `D6`, and LED2 (representing participant C) is connected to `D7`.
- **Sensor Reading**: The FSR402 is connected to analog pin `A6` in series with a 10kΩ pull-down resistor connected to GND, ensuring the stability and accuracy of the 3.3V analog signal reading.

### 7.2 Software Architecture & WiFi Provisioning

To enhance the system's robustness across different real-world network environments, the software architecture adopts a **Dual-Mode Operation**:

- **Config Mode**: When the device boots for the first time or the current network is unavailable, it automatically opens an AP hotspot and establishes a local web server. Users can connect to the hotspot via their mobile phones and dynamically input WiFi credentials on the web page, avoiding hardcoding the network configuration into the program.
- **Normal Mode**: Automatically switches after a successful network configuration. It is responsible for executing the core MQTT loop, sensor polling, and LED control, and it includes an automatic reconnection mechanism upon disconnection.

### 7.3 Sensor Debouncing & MQTT Communication

Due to the inherent data noise of physical sensors, the system implements a strict **debouncing algorithm** at the input end: it takes the average of 10 samples and sets a dual threshold (press >= 50, release <= 35). Only when a stable state is read for 3 consecutive cycles does the system confirm a state change, thereby greatly reducing false triggers.

For inter-board communication, the system uses the lightweight **MQTT protocol** with a Pub/Sub topology. Each device acts as both a publisher and a subscriber. When the FSR state of Device A changes, it publishes an `"on"` or `"off"` payload to the `student/MUJI/hzh/A` topic; simultaneously, it subscribes to topics B and C. When all three devices receive an `"on"` signal from each other, the system triggers the "Warm flowing tea-light" animation in the teapot area, completing the final feedback of the collective shared moment.

<div align="center">
  <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/lighting.jpg" alt="Lighting Effect" width="50%">
  <p><em>(Figure 7.1: LED lighting feedback triggered by MQTT payloads indicating remote presence)</em></p>
</div>

---

## 8. Design Iterations (Technical)

The core challenge of this project lies in how to make distributed physical devices operate reliably in unstable network environments. Therefore, our iteration process primarily focused on **algorithm debouncing, network architecture optimization, and the design of fault-tolerance mechanisms**, while also including the evolution of the physical form to match the interaction logic.

### 8.1 Iteration of Sensor Reading Algorithm (Debouncing)

Force sensitive resistors (FSR) in physical environments generate a significant amount of analog noise. We conducted three algorithmic iterations at the input layer:

1. **Initial Raw Reading:** Initially, we read the analog pin values directly. We found that even when the teacup was stationary, minor fluctuations in the values caused the lights to flicker frequently.
2. **Moving Average:** We then introduced a smoothing algorithm that averaged 10 samples. This mitigated high-frequency noise, but at the critical point when the cup was just placed, there were still edge cases of the state rapidly bouncing back and forth.
3. **Final Hysteresis & Stable Count:** Finally, we designed a **dual-threshold judgment with hysteresis** (press threshold set to 50, release threshold to 35) and added a counter: the state is only considered validly flipped when the condition is met for 3 consecutive cycles. This algorithm completely resolved the false trigger issue, ensuring absolute reliability of the input logic.

### 8.2 Iteration of Network Architecture & Robustness

The stability of multi-device collaborative work went through an iteration from "fragile" to "robust":

1. **Hardcoded Connections:** In the early prototypes, WiFi credentials and the MQTT Broker were hardcoded into the code. This meant that whenever the testing location changed, the program had to be reflashed, completely lacking portability.
2. **Captive Portal (Config Mode):** To solve this problem, we developed a provisioning mode. If there are no valid credentials in the Flash memory, the device boots as an AP (Router IP: 192.168.4.1), allowing users to configure the network via a local web page and save it to the Flash memory.
3. **Failure Recovery & Periodic Sync:** Field tests revealed that occasional network jitter would cause devices to drop offline or lose packets. Therefore, in the final iteration, we added an **automatic recovery mechanism (`NVIC_SystemReset`)** and built a **periodic state publishing logic** on top of it. Even if a state-switch message is lost, the device periodically republishes its own state, ensuring that all devices — even those joining late or experiencing brief disconnections — ultimately achieve state synchronization.

### 8.3 Iteration of Physical Form and Materials

As the hardware algorithms matured, the exterior design underwent corresponding adjustments. Early on, we experimented with a unified, single-piece tea tray and 3D-printed PLA enclosures, but found them too "industrial" in both visual and tactile terms.

Ultimately, we shifted to a **combination of laser-cut wood and cork**, introducing the design of an "Embedded Sensor Coaster." This iteration of materials and form not only concealed the complex internal wiring and sensors but also endowed the technological product with the warmth of "home," making the physical touch more aligned with the emotional tone of a "remote tea ritual."

<div align="center">
  <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/top%20view.jpg" alt="Top View" width="50%">
  <p><em>(Figure 8.1: Top view of the finished prototype showing the cup placement and LED feedback zones)</em></p>
</div>

<div align="center">
  <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/side%20view.jpg" alt="Side View" width="50%">
  <p><em>(Figure 8.2: Final iteration using laser-cut wood and embedded sensors to enhance domestic aesthetics)</em></p>
</div>

---

## 9. Evaluation and Reflection

### 9.1 Final Progress

The final prototype successfully demonstrates the core concept of "Tea for Three" as a connected ambient system. A complete set of three devices has been implemented, allowing three physically separated users to participate in the same remote tea ritual. Each device can detect local user interaction through the pressure-sensitive tea cup area, communicate this state to the network, and update the light feedback on the other devices. The most effective aspect of the prototype is its use of a simple physical gesture as the basis for remote communication. By using cup placement as the main input, the interaction feels more connected to the act of drinking tea than a button or app interface would. This supports the project's aim of creating a subtle sense of companionship across distance.

### 9.2 Limitations

The current prototype also has some limitations. Firstly, the range of interactions remains relatively simple. At this stage, the system can only indicate whether each participant is present; it cannot convey more nuanced emotional states such as waiting, inviting or missing someone. Consequently, whilst emotional communication exists, it is rather limited.

Secondly, the medium for feedback is not sufficiently clear. In the current version, when all three users have joined, the teapot shape lights up to symbolise collective participation. However, this feedback primarily serves as a signal of completion rather than conveying richer emotional meaning. If the product is intended to embody companionship, a shared atmosphere, and the climax of a remote tea gathering, it requires a more meaningful medium for conveying information.

Thirdly, the overall technical standard of the prototype leaves room for improvement. As the experience relies on real-time synchronisation across multiple devices, unstable network connections can affect the smoothness of interactions. The physical construction also requires further optimisation, particularly regarding the quality of the casing, cable management, water resistance, portability and improvements for long-term use.

---

## 10. Conclusion

Tea for Three explores how connected ambient devices can support emotional connection between family members or close friends who are physically separated. Instead of focusing on verbal communication, screens, or information exchange, the project uses a familiar tea-drinking ritual to create a subtle sense of shared presence across distance.

Overall, the project shows that remote communication does not always need to be direct or attention-demanding. Small physical gestures and glanceable ambient feedback can create a softer form of connection. While the prototype still requires further refinement, it provides a clear foundation for a connected ambient product that transforms tea drinking into a shared remote experience.

---

## References

Hallnäs, L. and Redström, J. (2001) 'Slow technology: Designing for reflection', *Personal and Ubiquitous Computing*, 5(3), pp. 201–212.

Ishii, H. and Ullmer, B. (1997) 'Tangible bits: Towards seamless interfaces between people, bits and atoms', in *Proceedings of the SIGCHI Conference on Human Factors in Computing Systems*. New York: ACM, pp. 234–241.

Kaye, J. (2006) 'I just clicked to say I love you: Rich evaluations of minimal communication', in *CHI '06 Extended Abstracts on Human Factors in Computing Systems*. New York: ACM, pp. 363–368.

Mankoff, J., Dey, A.K., Hsieh, G., Kientz, J., Lederer, S. and Ames, M. (2003) 'Heuristic evaluation of ambient displays', in *Proceedings of the SIGCHI Conference on Human Factors in Computing Systems*. New York: ACM, pp. 169–176.

Weiser, M. and Brown, J.S. (1997) 'The coming age of calm technology', in Denning, P.J. and Metcalfe, R.M. (eds.) *Beyond Calculation: The Next Fifty Years of Computing*. New York: Springer, pp. 75–85.

Wisneski, C., Ishii, H., Dahley, A., Gorbet, M., Brave, S., Ullmer, B. and Yarin, P. (1998) 'Ambient displays: Turning architectural space into an interface between people and digital information', in Streitz, N.A., Konomi, S. and Burkhardt, H.-J. (eds.) *Cooperative Buildings: Integrating Information, Organization, and Architecture*. Berlin: Springer, pp. 22–32.
