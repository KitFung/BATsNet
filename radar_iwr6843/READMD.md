# Radar

This folder keep all code that related to radar.

Currently only support model iwr6843.


`iwr6843_read.h` and `iwr6843_read.cc` is the driver logic of reading radar data from iwr6843 device and parsing it to stanard radar data format.


`radar_iwr6843.cc` is the binary for the radar driver.
`controller.cc` is the binary for the radar controller.
