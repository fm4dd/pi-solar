# Pi-Solar

Pi-Solar is a solar power generation project for the Raspberry Pi weather station <a href="https://github.com/fm4dd/pi-weather">Pi-Weather</a>, with the goal to achieve independent and longterm "off-the-grid" operation. This project documents the system design and the solar power generation monitoring software that watches over the electrical energy balance.

<img align="left" src="images/pi-solar operation1.png">

The solar power generator after construction, during the initial burn-in trials.

## Design

The solar panel array has four 10W Autumn Technology AT-MA10A pv panels, connected via Victron BlueSolar MPPT 75/10 charge controller to a Yuasa NPH12-12 deep-cycle battery.  The solar panel array angle can be adjusted up to approx 48 degrees.

<img align="left" src="cad/pi-solar panel frame front v11.png">

Battery and charge controller are placed in a OBO T350 junction box enclosure below the panel array. There is space for a second battery to enhance the capacity and increase the bad weather safety margin. 

<img align="left" src="cad/pi-solar panel frame back v11.png">

## Charge Controller monitoring software

The charge controller selection zeroed in on Victron's BlueSolar and SmartSolar series. The selection criteria besides panel power match-up are compact size, ruggedness, and a data interface with a open specification. 

<img src="images/pi-solar testpanel 10w.jpg" height="120px" width="160px"> <img src="images/pi-solar firmware upgrade.jpg" height="120px" width="160px"> <img src="images/pi-solar serial connect1.jpg" height="120px" width="160px"> <img src="images/pi-solar test setup1.jpg" height="120px" width="160px"> <img src="images/pi-solar raspi-interface2.png" height="120px" width="160px">

The serial line interfacing with a Raspberry Pi worked fine, using a small prototyping circuit board.  Initial software development was mostly done on a NanoPi Neo2 Raspberry clone. The monitoring software was written while powering the controller from a 12V wall plug adapter.  

<img src="images/nighttime-shading example.png">

Charging graph example, showing the panel voltage over 24 hours. The necessary software can be cloned from the <a href="code">code</a> directory.
 
## Solar Panel Array Frame

The <a href="cad">cad</a> directory has the hardware BOM and CAD drawings for panel frame manufacture, and lists the required components. Design goals for the frame are ruggedness, panel tilt adjustment, wood material, assembly/dissassembly options, and portable parts with a size not exceeding 60 cm length.

<img src="images/pi-solar assembly2.png" height="120px" width="160px"> <img src="images/pi-solar assembly3.png" height="120px" width="160px"> <img src="images/pi-solar assembly4.png" height="120px" width="160px"> <img src="images/pi-solar assembly6.png" height="120px" width="160px"> <img src="images/pi-solar assembly7.png" height="120px" width="160px">

Panel fitting and wire routing:

<img align="left" src="images/pi-solar assembly8.png">

The solar panel wiring has been upgraded to 2mm copper wire of equal length, and a 10A10 rectifier bypass diode has been installed.

<img align="left" src="images/pi-solar assembly9.png">

<img align="left" src="images/pi-solar assembly10.png">

The completed frame after assembly. The stand turned out to be insufficiently stable due to the panel weight with a high center of gravity. Four stabilizers (not pictured here) were added afterwards to sufficiently counter the weight and prevent tipping in high wind situations.

## Power observations

Initial power trials with three panels (30W) showed the need for greater power "buffer" even with a MPPT charger. Poor conditions such as bad weather and obstructive shading reduce the power generation to near zero, and need to be compensated with charging as fast as possible when conditions are right. The initial solar array frame design, envisioning three panels, was reworked to incorporate one more panel for a total of four.

<img src="images/pi-solar web presentation.jpg">

3-Panel trials data collection. With panels arranged in series, achieving a higher voltage did not result in better perfomance.
