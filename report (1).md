<div align="center">

**CASA0021 — Connected Environments**

**Group Project Report**

<br><br>

# Tea for Three

## A Set of Connected Ambient Devices for Remote Tea Rituals

<br><br>

**Group 2**

Chaoshuo Han  
Junrong Wang  
Tianrui Min  
Zihang He

<br><br>

UCL Centre for Advanced Spatial Analysis

26 April 2026

<br>

Word count: 1992 words

<br><br>

---

**Project Resources**

Demo Video: [https://www.youtube.com/watch?v=3P0uTwmVIM8](https://www.youtube.com/watch?v=3P0uTwmVIM8)

GitHub Repository: [https://github.com/xms12138/Tea-for-Three](https://github.com/xms12138/Tea-for-Three)

Source Code: [Final version](https://github.com/xms12138/Tea-for-Three/tree/main/Code/Final_version)

</div>

<div style="page-break-after: always;"></div>

## 1. Introduction

"Tea for Three" is a set of three connected ambient tea devices designed to support a subtle sense of shared presence between close family members or friends living in different places. Developed in response to the theme _Connected Ambient Devices Across the Miles_, the project explores how a familiar daily ritual can become a medium for remote connection. The following sections explain the design context, user problem, design and technical iterations, and reflections that shaped the final prototype.

## 2. Literature Review

"Tea for Three" draws on calm technology and peripheral interaction research. Building on Weiser and Brown's foundational vision, Bakker, van den Hoven, and Eggen (2015) argued that interactive systems should support both perception and physical interaction in the periphery of attention, allowing technology to embed into everyday routines without demanding focus. Bakker and Niemantsverdriet (2016) extended this with an "interaction-attention continuum," showing that well-designed systems should be usable across varying levels of attention. In "Tea for Three," the LED feedback is intended to operate in this peripheral mode — glanceable and ambient — so users can sense a remote partner's presence without actively reading a screen.

The project also connects with slow technology. Odom, Stolterman, and Chen (2022) extended Hallnäs and Redström's original theory through artifact analysis, identifying design qualities such as implicit slowness and ongoingness that characterise artifacts designed for reflection rather than efficiency. "Tea for Three" follows this direction: it does not notify, measure, or demand a reply. Instead, it creates a quiet shared moment through a familiar tea ritual.

Within the Connected Environments research agenda, "Tea for Three" sits at the intersection of distributed IoT sensing and ambient interaction design. Rather than treating networked devices as tools for data exchange or remote control, the project explores how synchronised physical artifacts — communicating through MQTT and embedded force sensing — can mediate social presence across geographical distance. This positioning shapes several key technical decisions discussed later in the report: the choice of a publish/subscribe architecture rather than a centralised server; the use of a physical action (cup placement) as input rather than a screen-based interface; and the deliberate restriction of feedback to glanceable LED states. In doing so, the project extends the concerns of connected environments from monitoring and automation toward the emotional and ritual dimensions of everyday domestic life.

## 3. Problem Statement and Scenario

Close relationships maintained across geographical distance often lose small daily moments of togetherness. Video calls and messages are effective for direct communication, but they can feel too deliberate or demanding for casual presence. Many people may want to express "I am here" or "I am joining you" without starting a full conversation. The project therefore asks: _How can a set of connected ambient tea devices support a subtle sense of shared presence between close family members or friends who are physically separated?_

The intended users are small groups who share a close relationship but live apart — international students and their families, friends separated by work or migration, and relatives who cannot meet regularly. These users do not necessarily need another notification-based platform; they may benefit more from a quiet, unobtrusive way to feel connected during an ordinary part of the day.

A typical use scenario illustrates this. A student studying abroad sits down for a morning cup of tea before class; at the same hour, their parents are having an evening tea at home. Without sending a message or starting a video call, simply placing the cup on the device signals "I am here." When all three cups settle into place, the teapot indicator quietly glows, marking a small moment of shared presence. The interaction is asynchronous in feeling but synchronous in fact — it does not interrupt anyone's routine, yet weaves the three households briefly together through a familiar daily ritual.

## 4. Project Objectives

The project pursues three connected objectives. The **conceptual objective** is to explore how connected devices can preserve the emotional bond between close family or friends sharing tea across a distance, conveying presence, care and companionship. The **technical objective** is to develop three independent but synchronised devices that detect local interaction and communicate state to the others in real time over long distances. The **design and commercialisation objective** is to create a form that conveys warmth and ritual whilst remaining intuitive, replicable through feasible manufacturing methods, and suitable for further development as a consumer product.

## 5. Form and Material Iterations

The physical design went through three iterations. The initial concept featured an oval-shaped tray with engraved curves that doubled as drainage grooves, with dedicated spaces for a teapot and a teacup. This was rejected due to waterproofing and electrical safety concerns, the load-bearing limitations of the tray material, and uncertainty over teapot dimensions; the teapot was instead replaced with a decorative teapot-shaped indicator on the tray.

A second iteration attempted to fabricate the entire device — casing, tray and decorative elements — by 3D printing. This proved unsatisfactory: the production cycle was lengthy, material consumption was high, and the surface finish felt cheap and rough, falling short of the warmth expected from a household tea set. Integrating sensors, wiring and LEDs neatly within a single printed component also proved difficult.

The final iteration replaces the single printed body with a hybrid construction: a laser-cut plywood base, a 3D-printed shell, and a faux leather wrapping around the outer shell. Wood was chosen for its inherent warmth and its suitability for modular assembly and replication, whilst the soft wrapping enhances aesthetic appeal and lends a homely feel. Decorative elements remain 3D-printed, and the teapot-shaped light diffuser is retained as the core design feature. This hybrid approach makes the device significantly easier to assemble, repair and replicate.

<div align="center">
  <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/side%20view.jpg" alt="Final Prototype" width="60%">
  <p><em>Figure 1: Final prototype with laser-cut plywood base and faux leather wrapping</em></p>
</div>

## 6. Prototype Overview

The "Tea for Three" product comprises three interconnected ambient devices that can be used in different spaces and remain synchronised over the network. Each device features a cup-holding area as the user interaction zone, two indicator lights shaped like teacups, and one teapot-shaped indicator. Each device is powered via USB.

Once a user begins drinking tea, they place their teacup on the sensor area. The device detects the user's presence and transmits this status to the connected system. When other devices receive this message, the indicator light corresponding to that user illuminates. When the system detects that all three users are online, the teapot-shaped indicator activates with a special ambient lighting effect, marking the moment when all participants have joined the remote tea gathering.

## 7. Implementation

The system is built around the **Arduino MKR WiFi 1010** microcontroller, combined with an **FSR402 force sensitive resistor** for cup detection and **WS2812B and Adafruit NeoPixel Stick LED strips** for visual feedback. An external 5V supply ensures stable LED operation.

The software follows a **dual-mode operation** model. On boot, the device checks whether valid WiFi credentials exist in flash memory. If not, it enters Config Mode and opens a local web server through an AP hotspot, allowing users to enter network details through a browser. Once credentials are stored, the device switches to Normal Mode, which runs the main MQTT loop, polls the FSR sensor and drives the LED feedback.

For inter-device communication, the system uses the lightweight **MQTT protocol** in a publish/subscribe topology. Each device acts as both publisher and subscriber: when its own FSR state changes, it publishes an `"on"` or `"off"` payload, whilst subscribing to the topics of the other two devices. When all three devices report `"on"` simultaneously, the system triggers a "warm flowing tea-light" animation in the teapot area, providing the collective feedback that marks a complete shared moment.

<div align="center">
  <img src="https://raw.githubusercontent.com/xms12138/Tea-for-Three/main/Picture/lighting.jpg" alt="Lighting Effect" width="55%">
  <p><em>Figure 2: LED feedback triggered by MQTT payloads indicating remote presence</em></p>
</div>

## 8. Algorithm and Network Iterations

The core technical challenge was making distributed physical devices operate reliably in unstable network environments. The iteration process focused on sensor debouncing, network architecture and fault tolerance.

**Sensor reading.** Force sensitive resistors generate considerable analogue noise. Three iterations were carried out at the input layer. The initial implementation read the analogue pin values directly, causing the lights to flicker even when the cup was stationary. A moving-average smoothing filter mitigated high-frequency noise but still allowed the state to bounce at the moment a cup was placed. The final design combined dual-threshold hysteresis with a stable-count check: a state change is only confirmed when the condition is met for several consecutive cycles, eliminating false triggers entirely.

**Network architecture.** The stability of multi-device collaboration evolved from "fragile" to "robust." Early prototypes hardcoded WiFi and MQTT credentials, requiring the program to be reflashed whenever the testing location changed. To address this, a Captive Portal was developed: when no valid credentials are present in flash, the device boots as an Access Point with a local configuration web page. Field tests then revealed that occasional network jitter caused devices to drop offline or lose packets. The final iteration added an automatic recovery mechanism alongside a periodic state-publishing logic, so that even if a state-switch message is lost, the device republishes its own state at regular intervals. This ensures that all devices — including those joining late or experiencing brief disconnections — eventually reach a synchronised state.

<div style="page-break-before: always;"></div>

## 9. Evaluation and Reflection

### 9.1 Final Progress

The final prototype successfully demonstrates the core concept of "Tea for Three" as a connected ambient system. A complete set of three devices was implemented, allowing three physically separated users to participate in the same remote tea ritual. During internal testing, the three devices were placed in different rooms across separate networks, and consistently achieved state synchronisation within one to two seconds of cup placement, with the teapot animation reliably triggering when all three users joined. The most effective aspect of the prototype is its use of a simple physical gesture — cup placement — as the basis for remote communication. This makes the interaction feel more connected to the act of drinking tea than a button or app interface would, supporting the project's aim of creating a subtle sense of companionship across distance.

### 9.2 Limitations

Three limitations are worth noting. First, the interaction range remains relatively simple: the system can only indicate whether each participant is present, and cannot convey more nuanced emotional states such as waiting, inviting or missing someone. Second, the feedback medium is not sufficiently expressive: when all three users have joined, the teapot lights up to mark collective participation, but this acts more as a binary completion signal than as a rich emotional symbol of companionship. Third, the technical robustness of the prototype leaves room for improvement: real-time synchronisation across multiple devices is sensitive to unstable network conditions, and the physical construction would benefit from further optimisation in casing quality, cable management, water resistance and portability.

### 9.3 Future Work

Given more time, several directions could be pursued. The interaction vocabulary could be expanded beyond binary presence to convey richer emotional states such as "waiting", "inviting" or "missing", through gesture-based input, cup temperature sensing, or the duration of cup placement. The ambient feedback could be enhanced with gradient lighting transitions, subtle soundscapes, or slow colour shifts that better capture the atmosphere of a shared tea moment. The physical construction would benefit from improved waterproofing, cable management and a battery option to support genuine long-term domestic use. Finally, a longitudinal field study with families or friends separated across time zones would help evaluate whether the device meaningfully supports remote companionship over weeks of real use, rather than in short demonstration sessions.

## 10. Conclusion

"Tea for Three" explores how connected ambient devices can support emotional connection between family members or close friends who are physically separated. Rather than focusing on verbal communication, screens or information exchange, the project uses a familiar tea-drinking ritual to create a subtle sense of shared presence across distance. The work demonstrates that remote communication does not always need to be direct or attention-demanding: small physical gestures and glanceable ambient feedback can create a softer form of connection. Whilst the prototype still requires further refinement, it provides a clear foundation for a connected ambient product that transforms tea drinking into a shared remote experience.

<div style="page-break-before: always;"></div>

## References

Bakker, S. and Niemantsverdriet, K. (2016) 'The interaction-attention continuum: Considering various levels of human attention in interaction design', _International Journal of Design_, 10(2), pp. 1–14.

Bakker, S., van den Hoven, E. and Eggen, B. (2015) 'Peripheral interaction: characteristics and considerations', _Personal and Ubiquitous Computing_, 19(1), pp. 239–254.

Hallnäs, L. and Redström, J. (2001) 'Slow technology: Designing for reflection', _Personal and Ubiquitous Computing_, 5(3), pp. 201–212.

Odom, W., Stolterman, E. and Chen, A.Y.S. (2022) 'Extending a theory of slow technology for design through artifact analysis', _Human–Computer Interaction_, 37(2), pp. 150–179.

Weiser, M. and Brown, J.S. (1997) 'The coming age of calm technology', in Denning, P.J. and Metcalfe, R.M. (eds.) _Beyond Calculation: The Next Fifty Years of Computing_. New York: Springer, pp. 75–85.
