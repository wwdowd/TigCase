# TigCase
Arduino-based environmental control system for copepods (or other small marine critters). 

Reads in series of pH and dissolved oxygen (% of air-equilibrated values) setpoints for every minute of the day and adjusts flow rates of O2, N2, and CO2 gases into a chamber to generate the desired patterns. Automatically compensates O2 solubility values for water temperature (measured with K-type thermocouple), salinity (entered manually by user via serial input), and calculated seawater density. 

Tig_ versions use Honeywell Durafet III pH probe via ADC and Anderaa O2 probe via RS-232 communication. 
Tig2_ versions use ADC to read inputs from a Honeywell UDA1282 analyzer, which puts out a 4-20mA signal for both DO (ppm probe) and pH (Durafet III probe). 

Tig_ versions first written by Felicia King, with subsequent modification by Brian Hizon, Justin Joseph, and Ezequiel Partida. 
Tig2_ by WWD. 
