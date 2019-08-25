[![Showcase Video](20190825_145248.jpg)](https://www.youtube.com/watch?v=mPuATmjmrOs)
# DosiMate
In this repository you can find all the code that is used to monitor a persons activity, movements and wellbeing. Dosimate is a device that's supposed to be like a smartwatch that's specifically suited for Parkinson Patients. All that's needed is:
- ESP32 Lite
- 0,96" IPS Display
- KY-040 Rotary Encoder
- Passive Buzzer
- Two Tilt sensors

## Functionality
It monitors 4 kinds of movements; large, medium, small and shakings. That's done through two tilt sensors and a circulair buffer.
The DosiMate also offers a walking-aid since some Parkinson Patients who find it difficult to walk are able to walk agian with beeps at a specific interval.
A questionaire through which the severity of currrent parkinson symptoms can be monitored is also included. This list is made by the parksinson foundation.
A server-side program saves these statistics in a MySQL database. Then a hospital or doctor can connect through the API to collect these statistics.
All functionalities from the "Smartwatch" are shown in [this](https://www.youtube.com/watch?v=mPuATmjmrOs) video.
