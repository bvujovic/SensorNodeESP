# SensorNodeESP

Data from sensor (PIR, CO2, smoke/water detectors...) is logged and sometimes sent to user (WhatsApp message, E-mail).

## TODO

- [x] ATtiny sleep, wake up on button, PIR...
- [x] ATtiny sends signal via STX882, ESP8266 receives that signal
- [ ] Hub - central component: ESP (8266/32) receives and logs messages (source/sensor, type:info/warning/error/alarm..., message/data ...) and then sends WA message or e-mail.
    - [ ] Add web server component to a Hub.
    - [ ] Is code in ESP_IR_TV project compatibile with this Hub component - concerned with delay() in loop()
- [ ] ESP8266 sending message via ESP Now or HTTP web request to another ESP (8266/32).
