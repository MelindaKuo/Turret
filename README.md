# Turret

ESP32 pan/tilt turret with ultrasonic ranging. A 28BYJ-48 stepper (via ULN2003 driver) handles pan, a hobby servo handles tilt, and an HC-SR04 measures distance. On boot it self-tests each part, then sweeps back and forth logging distance over serial.

## Hardware

| Component | Notes |
|---|---|
| ESP32 dev board | Main controller |
| 28BYJ-48 stepper motor | Pan axis, via ULN2003 driver |
| Hobby servo (SG90 or similar) | Tilt axis |
| HC-SR04 ultrasonic sensor | Distance sensing |

## Pinout

| Signal | ESP32 Pin |
|---|---|
| Stepper IN1 | 14 |
| Stepper IN2 | 27 |
| Stepper IN3 | 26 |
| Stepper IN4 | 25 |
| Servo signal | 18 |
| Ultrasonic TRIG | 17 |
| Ultrasonic ECHO | 19 |

## Behavior

- Pan: open-loop stepper, tracked in software as a step count, clamped to ±90°.
- Tilt: RC servo, clamped to 70°-130°, centered at 90°.
- Ranging: HC-SR04, median of 3 pings per reading, valid range 2-400 cm. Speed of sound assumes 21°C ambient (`AMBIENT_TEMP_C`).
- Startup runs `testStepper()`, `testServo()`, `testUltrasonic()`, then `loop()` sweeps pan in 5° steps, printing pan/tilt/distance to Serial at 115200 baud.

