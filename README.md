# Tea-for-Three
Tea for Three is an interactive IoT installation where three connected devices communicate through touch, creating a shared experience between participants.

## Team — Group 2

| Member | Contribution |
|---|---|
| **Chaoshuo Han** | Hardware structure, sensing setup |
| **Junrong Wang** | Enclosure modelling and visual design |
| **Tianrui Min** | Product concept, circuit design and overall system integration |
| **Zihang He** | Coding, communication logic |

---

## Links

- **Demo Video:** [Watch on YouTube](https://www.youtube.com/watch?v=3P0uTwmVIM8)
- **GitHub Repository:** [xms12138/Tea-for-Three](https://github.com/xms12138/Tea-for-Three)
- **Source Code:** [Final version](https://github.com/xms12138/Tea-for-Three/tree/main/Code/Final_version)

---

## Support
For questions or issues, please open an issue on this repository.

---

## 1. Introduction

"Tea for Three" is a set of three connected ambient tea devices designed to support a subtle sense of shared presence between close family members or friends living in different places. Developed in response to the theme: Connected Ambient Devices Across the Miles, the project explores how a familiar daily ritual can become a medium for remote connection. The following sections explain the design context, user problem, objectives and early design iterations that shaped the final prototype.

---

## 2. Literature Review

"Tea for Three" draws on calm technology and peripheral interaction research. Building on Weiser and Brown's foundational vision, Bakker, van den Hoven, and Eggen (2015) argued that interactive systems should support both perception and physical interaction in the periphery of attention, allowing technology to embed into everyday routines without demanding focus. Bakker and Niemantsverdriet (2016) extended this with an "interaction-attention continuum," showing that well-designed systems should be usable across varying levels of attention. In "Tea for Three," the LED feedback is intended to operate in this peripheral mode — glanceable and ambient — so users can sense a remote partner's presence without actively reading a screen.

The project also connects with slow technology. Odom, Stolterman, and Chen (2022) extended Hallnäs and Redström's original theory through artifact analysis, identifying design qualities such as implicit slowness and ongoingness that characterise artifacts designed for reflection rather than efficiency. "Tea for Three" follows this direction: it does not notify, measure, or demand a reply. Instead, it creates a quiet shared moment through a familiar tea ritual.

Within the Connected Environments research agenda, "Tea for Three" sits at the intersection of distributed IoT sensing and ambient interaction design. Rather than treating networked devices as tools for data exchange or remote control, the project explores how synchronised physical artifacts — communicating through MQTT and embedded force sensing — can mediate social presence across geographical distance. This positioning shapes several key technical decisions discussed later in the report: the choice of a publish/subscribe architecture rather than a centralised server; the use of a physical action (cup placement) as input rather than a screen-based interface; and the deliberate restriction of feedback to glanceable LED states. In doing so, the project extends the concerns of connected environments from monitoring and automation toward the emotional and ritual dimensions of everyday domestic life.

---

## 3. Problem Statement and Scenario

### 3.1 Motivation

Close relationships maintained across geographical distance often lose small daily moments of togetherness. Video calls and messages are effective for direct communication, but they can feel too deliberate or demanding for casual presence. Many people may want to express "I am here" or "I am joining you" without starting a full conversation. Based on this problem, the project asks: *How can a set of connected ambient tea devices support a subtle sense of shared presence between close family members or friends who are physically separated?*

### 3.2 User Scenario

"Tea for Three" is designed for small groups who share a close relationship but live apart. This includes international students and their families, friends separated by work or migration, and relatives who cannot meet regularly. These users do not necessarily need another notification-based communication platform. Instead, they may benefit from a quiet and unobtrusive way to feel connected during an ordinary part of the day.

A typical use scenario illustrates this. A student studying abroad sits down for a morning cup of tea before class; at the same hour, their parents are having an evening tea at home. Without sending a message or starting a video call, simply placing the cup on the device signals "I am here." When all three cups settle into place, the teapot indicator quietly glows, marking a small moment of shared presence. The interaction is asynchronous in feeling but synchronous in fact — it does not interrupt anyone's routine, yet weaves the three households briefly together through a familiar daily ritual.

---

## 4. Project Objectives

The project pursues three connected objectives:

- **Conceptual:** explore how connected devices can preserve the emotional bond between close family or friends sharing tea across a distance, conveying presence, care and companionship.
- **Technical:** develop three independent but synchronised devices that detect local interaction and communicate state to the others in real time over long distances.
- **Design & commercialisation:** create a form that conveys warmth and ritual whilst remaining intuitive, replicable through feasible manufacturing methods, and suitable for further development as a consumer product.

---

## 5. Form and Material Iterations

### 5.1 Initial Concept

The initial design featured an oval-shaped tray with two indicator lights, a space for a teapot and a space for a teacup. The surface of the tray is engraved with curved patterns, which provide a stable base for the teacup and teapot, and also allow water droplets to flow along the grooves towards the drain. However, in view of waterproofing and electrical safety concerns, the load-bearing capacity of the tray material, and the uncertainty regarding the size of the teapot, this proposal was rejected. Instead, the solution adopted was to place a decorative teapot-style indicator on the tray.

<div align="center">
  <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/5.1.png" alt="Initial Concept" width="50%">
  <p><em>(Figure 5.1: Initial concept sketch of the oval tea tray design)</em></p>
</div>

### 5.2 3D Printing Version

Early versions attempted to use 3D printing technology to manufacture the entire device, including the casing, tray and decorative elements. However, this design presented a number of problems: the production cycle was lengthy, it required a large amount of material, and the finished product had a cheap feel to it, with rough surface details, making it difficult to meet the requirements of a household tea set. Furthermore, it proved quite challenging to neatly integrate sensors, wires and LED lights within a single printed component.

<div align="center">
  <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/5.2.png" alt="3D Printing Version" width="50%">
  <p><em>(Figure 5.2: Early 3D-printed prototype showing integrated tray and teapot form)</em></p>
</div>

### 5.3 Laser-cut + 3D Print with Soft-wrapped Version

The final version replaces the original single-printed body with a laser-cut plywood base and a 3D-printed shell. Wood was chosen for the tray material due to its inherent warmth, which naturally evokes a sense of calm and tranquillity. Furthermore, wood is well-suited to modular assembly and replication. This version also features a soft material (faux leather) wrapping the outer shell, enhancing the product's aesthetic appeal and lending it a homely feel. The decorative elements remain 3D-printed, and the teapot-shaped light diffuser is retained as the core design feature. This hybrid manufacturing approach makes the device easier to assemble, repair and replicate.

<div align="center">
  <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/5.3.png" alt="Final Laser-cut Version" width="50%">
  <p><em>(Figure 5.3: Final iteration combining laser-cut plywood base with 3D-printed shell and faux leather wrapping)</em></p>
</div>

<div align="center">
  <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/top%20view.jpg" alt="Top View" width="50%">
  <p><em>(Figure 5.4: Top view of the finished prototype showing the cup placement and LED feedback zones)</em></p>
</div>

<div align="center">
  <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/side%20view.jpg" alt="Side View" width="50%">
  <p><em>(Figure 5.5: Side view of the final prototype with laser-cut plywood base and faux leather wrapping)</em></p>
</div>

---

## 6. Prototype Overview

### 6.1 Product Form

The "Tea for Three" product comprises three interconnected ambient devices. These devices can be used in different spaces and remain synchronised via network communication. Each device features a cup-holding area that serves as the user interaction zone. There are also two indicator lights shaped like teacups and one shaped like a teapot. Each device is powered via USB, catering to the vast majority of usage scenarios.

<div align="center">
  <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/6.1.1.png" height="280px">
  &nbsp;&nbsp;
  <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/6.1.2.png" height="280px">
  &nbsp;&nbsp;
  <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/6.1.3.png" height="280px">
  <p><em>(Figure 6.1: Final prototype — three devices forming a complete Tea for Three set)</em></p>
</div>

### 6.2 User Instruction

Once a user begins drinking tea, they place their teacup on the sensor area embedded with a pressure sensor. The device recognises the user's presence and transmits this status to the connected system. When other devices receive this message, the indicator light corresponding to that user on the device will light up. If the system detects that all three users are online, the teapot-shaped indicator light will illuminate with a special ambient lighting effect. This collective feedback represents the moment when all participants have joined the remote tea gathering.

### 6.3 System Components

| Component | Image |
|-----------|-------|
| Arduino MKR WiFi 1010 | <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/6.3Arduino%20MKR%20WiFi%201010.png" width="120px"> |
| Adafruit NeoPixel Stick | <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/6.3Adafruit%20NeoPixel%20Stick.png" width="120px"> |
| FSR402 Force Sensitive Resistor | <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/6.3FSR402%20Force%20Sensitive%20Resistor.png" width="120px"> |
| WS2812B RGB LED Strip | <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/6.3WS2812B%20RGB%20LED%20Strip.png" width="120px"> |
| PLA Plastic | <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/6.3PLA%20Plastic.png" width="120px"> |
| Maple Wood | <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/6.3Maple%20Wood.png" width="120px"> |
| Faux Leather | <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/6.3faux%20leather.png" width="120px"> |

---

## 7. Final Prototype & Implementation

This section details the hardware and software system implementation of the final Tea for Three prototype. The system consists of three identical physical devices, achieving low-latency remote state synchronisation through IoT technology.

### 7.1 Hardware & Circuit Design

The core microcontroller of the system is the **Arduino MKR WiFi 1010**, combined with an **FSR402 force sensitive resistor** to detect the cup placement status, providing visual feedback via **WS2812B RGB and Adafruit NeoPixel Stick LED strips**.

Regarding circuit wiring, to ensure the stable operation of multiple LEDs, the system utilises an external, independent 5V power supply. The specific pin assignments are as follows:

- **LED Data Pins**: LED1 (representing participant B) is connected to `D6`, and LED2 (representing participant C) is connected to `D7`.
- **Sensor Reading**: The FSR402 is connected to analog pin `A6` in series with a 10kΩ pull-down resistor connected to GND, ensuring the stability and accuracy of the 3.3V analogue signal reading.

### 7.2 Software Architecture

The software follows a **dual-mode operation** model. On boot, the device first checks whether valid WiFi credentials exist in flash memory. If not, it enters **Config Mode** and opens an AP hotspot with a local web server at `192.168.4.1`, allowing users to enter network details through a browser. Once credentials are stored, the device automatically switches to **Normal Mode**, which runs the main MQTT loop, polls the FSR sensor, and drives the LED feedback. An automatic reconnection mechanism handles temporary disconnection.

### 7.3 Sensor Input & MQTT Communication

To suppress noise from the analogue FSR readings, the input pipeline applies a moving average over 10 samples, a dual-threshold hysteresis (press ≥ 50, release ≤ 35) and a stable-count check requiring 3 consecutive matching cycles before a state change is confirmed. This combination eliminates flicker and false triggers in steady use.

For inter-device communication, the system uses the lightweight **MQTT protocol** in a publish/subscribe topology. Each device acts as both a publisher and a subscriber. When the FSR state of Device A changes, it publishes an `"on"` or `"off"` payload to the `student/MUJI/hzh/A` topic, whilst subscribing to topics B and C. When all three devices report `"on"` simultaneously, the system triggers the "Warm flowing tea-light" animation in the teapot area, providing the collective feedback that marks a complete shared moment.

<div align="center">
  <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/lighting.jpg" alt="Lighting Effect" width="50%">
  <p><em>(Figure 7.1: LED lighting feedback triggered by MQTT payloads indicating remote presence)</em></p>
</div>

---

## 8. Algorithm and Network Iterations

The core challenge of this project lies in how to make distributed physical devices operate reliably in unstable network environments. The iteration process therefore focused on **algorithm debouncing, network architecture optimisation, and the design of fault-tolerance mechanisms**.

### 8.1 Iteration of Sensor Reading Algorithm (Debouncing)

Force sensitive resistors (FSR) in physical environments generate a significant amount of analogue noise. Three algorithmic iterations were carried out at the input layer:

1. **Initial Raw Reading:** Initially, the analogue pin values were read directly. Even when the teacup was stationary, minor fluctuations in the values caused the lights to flicker frequently.
2. **Moving Average:** A smoothing algorithm was then introduced that averaged 10 samples. This mitigated high-frequency noise, but at the critical point when the cup was just placed, edge cases of the state rapidly bouncing back and forth still occurred.
3. **Final Hysteresis & Stable Count:** Finally, a **dual-threshold judgment with hysteresis** (press threshold set to 50, release threshold to 35) was designed and a counter was added: the state is only considered validly flipped when the condition is met for 3 consecutive cycles. This algorithm completely resolved the false trigger issue, ensuring absolute reliability of the input logic.

### 8.2 Iteration of Network Architecture & Robustness

The stability of multi-device collaborative work went through an iteration from "fragile" to "robust":

1. **Hardcoded Connections:** In the early prototypes, WiFi credentials and the MQTT Broker were hardcoded into the code. This meant that whenever the testing location changed, the program had to be reflashed, completely lacking portability.
2. **Captive Portal (Config Mode):** To solve this problem, a provisioning mode was developed. If there are no valid credentials in the flash memory, the device boots as an AP (Router IP: 192.168.4.1), allowing users to configure the network via a local web page and save it to the flash memory.
3. **Failure Recovery & Periodic Sync:** Field tests revealed that occasional network jitter would cause devices to drop offline or lose packets. Therefore, in the final iteration, an **automatic recovery mechanism (`NVIC_SystemReset`)** was added, with a **periodic state publishing logic** built on top of it. Even if a state-switch message is lost, the device periodically republishes its own state, ensuring that all devices — even those joining late or experiencing brief disconnections — ultimately achieve state synchronisation.

---

## 9. Evaluation and Reflection

### 9.1 Final Progress

The final prototype successfully demonstrates the core concept of "Tea for Three" as a connected ambient system. A complete set of three devices was implemented, allowing three physically separated users to participate in the same remote tea ritual. During internal testing, the three devices were placed in different rooms across separate networks, and consistently achieved state synchronisation within one to two seconds of cup placement, with the teapot animation reliably triggering when all three users joined. The most effective aspect of the prototype is its use of a simple physical gesture — cup placement — as the basis for remote communication. This makes the interaction feel more connected to the act of drinking tea than a button or app interface would, supporting the project's aim of creating a subtle sense of companionship across distance.

### 9.2 Limitations

Three limitations are worth noting. First, the interaction range remains relatively simple: the system can only indicate whether each participant is present, and cannot convey more nuanced emotional states such as waiting, inviting or missing someone. Second, the feedback medium is not sufficiently expressive: when all three users have joined, the teapot lights up to mark collective participation, but this acts more as a binary completion signal than as a rich emotional symbol of companionship. Third, the technical robustness of the prototype leaves room for improvement: real-time synchronisation across multiple devices is sensitive to unstable network conditions, and the physical construction would benefit from further optimisation in casing quality, cable management, water resistance and portability.

### 9.3 Future Work

Given more time, several directions could be pursued. The interaction vocabulary could be expanded beyond binary presence to convey richer emotional states such as "waiting", "inviting" or "missing", through gesture-based input, cup temperature sensing, or the duration of cup placement. The ambient feedback could be enhanced with gradient lighting transitions, subtle soundscapes, or slow colour shifts that better capture the atmosphere of a shared tea moment. The physical construction would benefit from improved waterproofing, cable management and a battery option to support genuine long-term domestic use. Finally, a longitudinal field study with families or friends separated across time zones would help evaluate whether the device meaningfully supports remote companionship over weeks of real use, rather than in short demonstration sessions.

---

## 10. Conclusion

Tea for Three explores how connected ambient devices can support emotional connection between family members or close friends who are physically separated. Instead of focusing on verbal communication, screens, or information exchange, the project uses a familiar tea-drinking ritual to create a subtle sense of shared presence across distance.

Overall, the project shows that remote communication does not always need to be direct or attention-demanding. Small physical gestures and glanceable ambient feedback can create a softer form of connection. Whilst the prototype still requires further refinement, it provides a clear foundation for a connected ambient product that transforms tea drinking into a shared remote experience.

---

## References

Bakker, S. and Niemantsverdriet, K. (2016) 'The interaction-attention continuum: Considering various levels of human attention in interaction design', *International Journal of Design*, 10(2), pp. 1–14.

Bakker, S., van den Hoven, E. and Eggen, B. (2015) 'Peripheral interaction: characteristics and considerations', *Personal and Ubiquitous Computing*, 19(1), pp. 239–254.

Hallnäs, L. and Redström, J. (2001) 'Slow technology: Designing for reflection', *Personal and Ubiquitous Computing*, 5(3), pp. 201–212.

Odom, W., Stolterman, E. and Chen, A.Y.S. (2022) 'Extending a theory of slow technology for design through artifact analysis', *Human–Computer Interaction*, 37(2), pp. 150–179.

Weiser, M. and Brown, J.S. (1997) 'The coming age of calm technology', in Denning, P.J. and Metcalfe, R.M. (eds.) *Beyond Calculation: The Next Fifty Years of Computing*. New York: Springer, pp. 75–85.

---

## License
MIT License — see [LICENSE](LICENSE) file for details.
