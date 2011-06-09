EESchema Schematic File Version 2  date 9/06/2011 7:57:07 p.m.
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:special
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
EELAYER 24  0
EELAYER END
$Descr A4 11700 8267
Sheet 1 1
Title "noname.sch"
Date "9 jun 2011"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Wire Wire Line
	5800 1850 5800 1600
Wire Wire Line
	2850 4400 5250 4400
Wire Wire Line
	5250 4400 5250 4600
Wire Wire Line
	5800 3050 6700 3050
Wire Wire Line
	4450 2650 4450 3400
Wire Wire Line
	4450 3400 4800 3400
Wire Wire Line
	7300 3300 7450 3300
Wire Wire Line
	7450 3300 7450 3950
Wire Wire Line
	7450 3950 7300 3950
Wire Wire Line
	5400 3400 5400 3800
Wire Wire Line
	5400 3800 5300 3800
Connection ~ 4450 2650
Wire Wire Line
	5750 4800 5550 4800
Wire Wire Line
	7800 2650 2850 2650
Wire Wire Line
	5800 3900 5800 3600
Wire Wire Line
	5250 5000 5250 5500
Wire Wire Line
	5650 5350 5650 5400
Wire Wire Line
	5650 5400 5250 5400
Connection ~ 5250 5400
Wire Wire Line
	5650 4850 5650 4800
Connection ~ 5650 4800
Wire Wire Line
	2850 3800 4900 3800
Wire Wire Line
	5500 3400 5300 3400
Connection ~ 5400 3400
Wire Wire Line
	7750 3600 7450 3600
Connection ~ 7450 3600
Wire Wire Line
	6250 4800 6700 4800
Wire Wire Line
	6700 4800 6700 3950
Wire Wire Line
	6700 3950 6900 3950
Wire Wire Line
	6700 3050 6700 3300
Wire Wire Line
	6700 3300 6900 3300
Wire Wire Line
	5800 2350 5800 3200
Connection ~ 5800 3050
$Comp
L +5V #PWR?
U 1 1 4DF07C86
P 5800 1600
F 0 "#PWR?" H 5800 1690 20  0001 C CNN
F 1 "+5V" H 5800 1690 30  0000 C CNN
	1    5800 1600
	1    0    0    -1  
$EndComp
$Comp
L R R?
U 1 1 4DF07C74
P 5800 2100
F 0 "R?" H 5900 2000 50  0000 C CNN
F 1 "5K1" V 5800 2100 50  0000 C CNN
	1    5800 2100
	1    0    0    -1  
$EndComp
$Comp
L CAPAPOL C?
U 1 1 4DF07C38
P 7100 3300
F 0 "C?" V 7150 3150 50  0000 L CNN
F 1 "33u 5V" V 6900 3300 50  0000 L CNN
	1    7100 3300
	0    -1   -1   0   
$EndComp
$Comp
L CAPAPOL C?
U 1 1 4DF07BDF
P 7100 3950
F 0 "C?" V 7150 4050 50  0000 L CNN
F 1 "33u 5V" V 7300 3950 50  0000 L CNN
	1    7100 3950
	0    1    1    0   
$EndComp
Text Label 2850 4400 2    60   ~ 0
MCU_PPM_IN
Text Label 7750 3600 0    60   ~ 0
PPM_JACK
$Comp
L DIODE D?
U 1 1 4DF0798F
P 5100 3800
F 0 "D?" H 5100 3900 40  0000 C CNN
F 1 "DIODE" H 5100 3700 40  0000 C CNN
	1    5100 3800
	-1   0    0    1   
$EndComp
$Comp
L R R?
U 1 1 4DF07954
P 5650 5100
F 0 "R?" H 5550 5200 50  0000 C CNN
F 1 "150K" V 5650 5100 50  0000 C CNN
	1    5650 5100
	-1   0    0    1   
$EndComp
$Comp
L R R?
U 1 1 4DF0794C
P 6000 4800
F 0 "R?" V 6080 4800 50  0000 C CNN
F 1 "5K1" V 6000 4800 50  0000 C CNN
	1    6000 4800
	0    1    1    0   
$EndComp
$Comp
L R R?
U 1 1 4DF07942
P 5050 3400
F 0 "R?" V 5130 3400 50  0000 C CNN
F 1 "5K1" V 5050 3400 50  0000 C CNN
	1    5050 3400
	0    1    1    0   
$EndComp
Text Label 2850 3800 2    60   ~ 0
MCU_SIM_CTRL_OUT
Text Label 2850 2650 2    60   ~ 0
MCU_PPM_OUT
Text Label 7800 2650 0    60   ~ 0
TX_MODULE_PPM_IN
$Comp
L GND #PWR?
U 1 1 4DF0789A
P 5800 3900
F 0 "#PWR?" H 5800 3900 30  0001 C CNN
F 1 "GND" H 5800 3830 30  0001 C CNN
	1    5800 3900
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR?
U 1 1 4DF07895
P 5250 5500
F 0 "#PWR?" H 5250 5500 30  0001 C CNN
F 1 "GND" H 5250 5430 30  0001 C CNN
	1    5250 5500
	1    0    0    -1  
$EndComp
Text Notes 4650 5950 0    118  ~ 24
Incomplete / Untested\nConcept Only Circuit
$Comp
L NPN Q?
U 1 1 4DF07828
P 5350 4800
F 0 "Q?" H 5350 4650 50  0000 R CNN
F 1 "NPN" H 5350 4950 50  0000 R CNN
	1    5350 4800
	-1   0    0    -1  
$EndComp
$Comp
L NPN Q?
U 1 1 4DF07819
P 5700 3400
F 0 "Q?" H 5700 3250 50  0000 R CNN
F 1 "NPN" H 5700 3550 50  0000 R CNN
	1    5700 3400
	1    0    0    -1  
$EndComp
$EndSCHEMATC
