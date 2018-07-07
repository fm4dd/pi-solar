# Pi-Solar Design and Build Documentation

This folder contains documentation for the physical build  of a 40W mini solar power generation system. It includes CAD drawings, wiring schemas, bill of materials (BOM) and build pictures.

## PV Panels

Solar power generation comes from four 10W modules, Chinese-build panels from Autumn Technology, model number AT-MA10A.

<img src="..\images\pi-solar testpanel 10w.jpg" height="285px" width="380px">

|Electrical Data|Value |
|---------------|-|
|Maximum Power|10 W|
|Maximum Operating Voltage|17.5 V|
|Maximum Operating Current|0.57 A|
|Open Circuit Voltage|21.6 V|
|Short Circuit Current|0.62 A|
|Module Efficiency|19.5 %|

|Physical Data|Value|
|---------------|-|
|External Dimensions|235×335×18mm|
|Module Weight|0.9 Kg|


The panels are build from a string of 36 cell modules, using 2 different cell types: either Bosch, or JA-Solar. Depending on the module type, the panel characteristics are slightly different for operating voltage and current.

<img src="../cad/AT-MA10A 10W pv panel v10.png">


##  PV panel carrier frame parts

The PV modules are installed on a carrier frame, which also holds the electrical components including cable terminal, deep-cycle battery, charge controller, and voltage converter. The PV panel holder is mounted with tilt angle adjustment.

###  Side pillars A-B

<img src="../cad/pi-solar A-B side-pillars v1.0.png">

Side pillars A-B (left,right) fix a 300x200 panel 20mm above ground. The bottom panel holds the OBO T350 junction box containing electrical components.

###  Pivot Blocks C-D

<img src="../cad/pi-solar C-D pivot-blocks v1.0.png">

Pivot blocks attach the PV panel holder to the side pillars.

###  Main Bars E-F

<img src="../cad/pi-solar E-F main-bars v1.0.png">

The main bars provide the horizontal fixation over the PV panel group, connecting to the pivot blocks and fixate the PV panel vertical fixation bars. The main bar is assembled from two parts to reduce the overall length of a single part.

###  Vertical Bars G-K, Vertical End L-U

<img src="../cad/pi-solar G-U vertical-bars v1.0.png">

Five vertical bars, cut to the length of a single PV panel frame. Remaining material is used for the ten vertical bar end pieces on each side.

###  4-panel terminal block V

<img src="../cad/pi-solar V terminal-block v1.0.png">

The terminal block holds the cable terminal which combines the individual PV panels before routing a single cable back to the charge controller in the T350 junction box.

###  Junction box insert W

<img src="../cad/pi-solar W T350-insert v1.0.png">

The junction box insert W is the internal fixation for electrical components inside OBO T350.

### Stabilizer X-Y

<img src="../cad/pi-solar X-Y stabilizer v1.0.png">

For stabilizer parts extend the side pillars on four sides to establish a wider footing. They stabilize the PV pannel carrier frame against high winds.

