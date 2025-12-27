# Client Node: ATtiny85 & HC-12 as a transmitter

Low power client that sends short strings on an event: water detection, movement detection (PIR)...

## Test device

### Test button
![Test device](docs/attiny_hc12_test_button.jpg)
### PIR sensor
![Test device](docs/attiny_hc12_test_pir.jpg)
Resistor: 50kOhm, transistor: BC547

## Current consumption
Current consumption in waiting/sleep mode is good but it's interesting that it fluctuates between 0.01mA and 0.2mA on Li-Ion battery (~4V). I would estimate to be ~0.1mA. HC-12 could be put to sleep (by sending AT commands) or shut down using small MOSFET/transistor.
Device consumes ~25mA while sending data for a bit. In cool-down period and periods between repeated sends, device consumes 1.2mA.

**ATtiny85** is set to work on 1MHz.

For **HC-12**, AT command "AT+RX" returns: OK+B4800, OK+RC030, OK+RP:+08dBm, OK+FU2.

## TODO
- [ ] Try to find an optimal design for ATtiny85 & HC-12 client device: ATtiny, HC-12, battery, PIR, (battery indicator, battery charger)...
    - [ ] Make 2, 3 different designes for different purposes: smallest possible (not user friendly), user friendly (with batt. indicator and charger).
- [ ] Try to improve on current consumption (see ChatGPT "Sleep interrupt with HC-12" -> "Why the HC-12 causes current fluctuations"):
    - [ ] In code: AT commands for sleep/wake up HC-12. "Try sending AT+SLEEP from your working sketch, then measure current. (Look for OK+SLEEP reply.)"
    - [ ] Turn ON/OFF HC-12 using transistor "Power-gate the HC-12 (...small MOSFET/transistor)"

### Retrigger lockout / cool-down periods
2 switches can set the value (in minutes) for cool-down period like this:

|Switches   |2^x value  |3^x value  |
|--|--|--|
|00 |1	|1
|01	|2	|3
|10	|4	|9
|11	|8	|27

Second options seems to be better choice (wider range of options). Consider other options for setting cool-down period:
- fixed value in code (user can't change this value depending on given situation)
- clicking on a button (rembering the choice in EEPROM, confirming clicks via blinking LED)