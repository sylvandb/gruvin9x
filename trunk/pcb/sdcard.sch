EESchema Schematic File Version 2  date 25/01/2011 1:13:37 p.m.
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
LIBS:sdc_mmc_socket
LIBS:sdcard-cache
EELAYER 24  0
EELAYER END
$Descr A4 11700 8267
Sheet 1 1
Title "sdcard.sch"
Date "25 jan 2011"
Rev "1.0"
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Wire Wire Line
	5450 3050 5450 2550
Wire Wire Line
	5450 2550 5300 2550
Connection ~ 5000 4850
Wire Wire Line
	5100 4750 5100 4850
Wire Wire Line
	5100 4850 4900 4850
Wire Wire Line
	4700 4050 7250 4050
Connection ~ 5200 3000
Wire Wire Line
	5200 3100 5200 3000
Wire Wire Line
	4350 2550 4250 2550
Wire Wire Line
	4250 2550 4250 2650
Wire Wire Line
	4000 3100 4450 3100
Wire Wire Line
	4000 3200 4150 3200
Wire Wire Line
	6200 3000 6150 3000
Wire Wire Line
	4900 4850 4900 4750
Wire Wire Line
	6450 4150 7350 4150
Wire Wire Line
	7350 4150 7350 3850
Connection ~ 4350 3700
Wire Wire Line
	4000 3700 4350 3700
Wire Wire Line
	4000 3600 5000 3600
Wire Wire Line
	5000 3600 5000 3000
Wire Wire Line
	4000 3500 4900 3500
Wire Wire Line
	4900 3500 4900 3850
Wire Wire Line
	4900 3850 5600 3850
Wire Wire Line
	7900 3550 7150 3550
Wire Wire Line
	7150 3550 7150 3950
Wire Wire Line
	7150 3950 6450 3950
Wire Wire Line
	4000 3300 4700 3300
Wire Wire Line
	4700 3300 4700 4050
Wire Wire Line
	7250 4050 7250 3650
Wire Wire Line
	7250 3650 7900 3650
Connection ~ 6750 3000
Wire Wire Line
	6750 3150 6750 3000
Wire Wire Line
	7900 3350 7350 3350
Wire Wire Line
	7800 4200 7800 3950
Wire Wire Line
	7800 3950 7900 3950
Wire Wire Line
	7350 3850 7900 3850
Wire Wire Line
	6400 3300 6400 3750
Wire Wire Line
	6400 3750 7900 3750
Wire Wire Line
	6600 3000 7350 3000
Wire Wire Line
	7350 3000 7350 3350
Wire Wire Line
	6750 3650 6750 3750
Connection ~ 6750 3750
Wire Wire Line
	5950 3950 4800 3950
Wire Wire Line
	4800 3950 4800 3800
Wire Wire Line
	4800 3800 4000 3800
Wire Wire Line
	7900 3450 7050 3450
Wire Wire Line
	7050 3450 7050 3850
Wire Wire Line
	7050 3850 6100 3850
Wire Wire Line
	5100 4250 5100 3850
Connection ~ 5100 3850
Wire Wire Line
	4350 4050 4350 3400
Wire Wire Line
	4350 3400 4000 3400
Wire Wire Line
	5950 4150 4600 4150
Wire Wire Line
	4600 4150 4600 3900
Wire Wire Line
	4600 3900 4000 3900
Wire Wire Line
	4900 4250 4900 4150
Connection ~ 4900 4150
Wire Wire Line
	5000 3000 5550 3000
Wire Wire Line
	5450 3500 5450 3450
Wire Wire Line
	5000 3200 4650 3200
Connection ~ 5000 3200
Wire Wire Line
	4950 3100 5000 3100
Connection ~ 5000 3100
Wire Wire Line
	4750 2550 4800 2550
Wire Wire Line
	5000 4950 5000 4850
Wire Wire Line
	5200 4150 5200 3600
Connection ~ 5200 4150
Connection ~ 5450 3000
$Comp
L R R7
U 1 1 4D3E13E1
P 5200 3350
F 0 "R7" V 5280 3350 50  0000 C CNN
F 1 "10K" V 5200 3350 50  0000 C CNN
	1    5200 3350
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR1
U 1 1 4D1F959F
P 4250 2650
F 0 "#PWR1" H 4250 2650 30  0001 C CNN
F 1 "GND" H 4250 2580 30  0001 C CNN
	1    4250 2650
	1    0    0    -1  
$EndComp
$Comp
L R R4
U 1 1 4D1F9592
P 5050 2550
F 0 "R4" V 5130 2550 50  0000 C CNN
F 1 "100" V 5050 2550 50  0000 C CNN
	1    5050 2550
	0    1    1    0   
$EndComp
$Comp
L LED D1
U 1 1 4D1F9570
P 4550 2550
F 0 "D1" H 4550 2650 50  0000 C CNN
F 1 "LED" H 4550 2450 50  0000 C CNN
	1    4550 2550
	-1   0    0    1   
$EndComp
NoConn ~ 7900 3250
NoConn ~ 4000 2900
NoConn ~ 4000 3000
$Comp
L R R2
U 1 1 4D1F947B
P 4700 3100
F 0 "R2" V 4780 3100 50  0000 C CNN
F 1 "10K" V 4700 3100 50  0000 C CNN
	1    4700 3100
	0    1    1    0   
$EndComp
$Comp
L R R1
U 1 1 4D1F946A
P 4400 3200
F 0 "R1" V 4480 3200 50  0000 C CNN
F 1 "10K" V 4400 3200 50  0000 C CNN
	1    4400 3200
	0    1    1    0   
$EndComp
$Comp
L GND #PWR4
U 1 1 4D1F9457
P 5450 3500
F 0 "#PWR4" H 5450 3500 30  0001 C CNN
F 1 "GND" H 5450 3430 30  0001 C CNN
	1    5450 3500
	1    0    0    -1  
$EndComp
$Comp
L C C1
U 1 1 4D1F944C
P 5450 3250
F 0 "C1" H 5500 3350 50  0000 L CNN
F 1 "C" H 5500 3150 50  0000 L CNN
	1    5450 3250
	1    0    0    -1  
$EndComp
$Comp
L INDUCTOR L1
U 1 1 4D1F9438
P 5850 3000
F 0 "L1" V 5800 3000 40  0000 C CNN
F 1 "10uH" V 5950 3000 40  0000 C CNN
	1    5850 3000
	0    -1   -1   0   
$EndComp
$Comp
L R R3
U 1 1 4D1F93EA
P 4900 4500
F 0 "R3" V 4980 4500 50  0000 C CNN
F 1 "10K" V 4900 4500 50  0000 C CNN
	1    4900 4500
	1    0    0    -1  
$EndComp
$Comp
L R R11
U 1 1 4D1F93CC
P 6200 4150
F 0 "R11" V 6280 4150 50  0000 C CNN
F 1 "1K" V 6200 4150 50  0000 C CNN
	1    6200 4150
	0    1    1    0   
$EndComp
$Comp
L GND #PWR3
U 1 1 4D1F93AF
P 5000 4950
F 0 "#PWR3" H 5000 4950 30  0001 C CNN
F 1 "GND" H 5000 4880 30  0001 C CNN
	1    5000 4950
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR2
U 1 1 4D1F938F
P 4350 4050
F 0 "#PWR2" H 4350 4050 30  0001 C CNN
F 1 "GND" H 4350 3980 30  0001 C CNN
	1    4350 4050
	1    0    0    -1  
$EndComp
$Comp
L R R5
U 1 1 4D1F9326
P 5100 4500
F 0 "R5" V 5180 4500 50  0000 C CNN
F 1 "10K" V 5100 4500 50  0000 C CNN
	1    5100 4500
	1    0    0    -1  
$EndComp
$Comp
L R R8
U 1 1 4D1F9309
P 5850 3850
F 0 "R8" V 5930 3850 50  0000 C CNN
F 1 "5K1" V 5850 3850 50  0000 C CNN
	1    5850 3850
	0    1    1    0   
$EndComp
$Comp
L SDC_MMC_SOCKET P1
U 1 1 4D1F92B8
P 3700 3400
F 0 "P1" H 3350 2750 60  0000 C CNN
F 1 "SDC_MMC_SOCKET" V 3150 3450 60  0000 C CNN
	1    3700 3400
	1    0    0    -1  
$EndComp
$Comp
L R R10
U 1 1 4D1F9178
P 6200 3950
F 0 "R10" V 6280 3950 50  0000 C CNN
F 1 "5K1" V 6200 3950 50  0000 C CNN
	1    6200 3950
	0    1    1    0   
$EndComp
$Comp
L CONN_8 P2
U 1 1 4D1F86A0
P 8250 3600
F 0 "P2" V 8220 3600 60  0000 C CNN
F 1 "CONN_8" V 8320 3600 60  0000 C CNN
	1    8250 3600
	1    0    0    -1  
$EndComp
$Comp
L R R12
U 1 1 4D1F904E
P 6750 3400
F 0 "R12" V 6830 3400 50  0000 C CNN
F 1 "10K" V 6750 3400 50  0000 C CNN
	1    6750 3400
	1    0    0    -1  
$EndComp
$Comp
L MOSFET_P Q1
U 1 1 4D1F8F07
P 6400 3100
F 0 "Q1" H 6400 3290 60  0000 R CNN
F 1 "MOSFET_P" V 6200 3400 60  0000 R CNN
	1    6400 3100
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR5
U 1 1 4D1F869F
P 7800 4200
F 0 "#PWR5" H 7800 4200 30  0001 C CNN
F 1 "GND" H 7800 4130 30  0001 C CNN
	1    7800 4200
	1    0    0    -1  
$EndComp
Text Label 7400 3750 0    60   ~ 0
SD_Pwr
Text Label 7400 3850 0    60   ~ 0
SD_CS
Text Label 7400 3550 0    60   ~ 0
SD_MOSI
Text Label 7400 3650 0    60   ~ 0
SD_MISO
Text Label 7400 3450 0    60   ~ 0
SD_SCK
Text Notes 7900 4150 0    60   ~ 0
MM/SD CARD (5V!)
Text Label 7400 3350 0    60   ~ 0
Vcc_3V3
$EndSCHEMATC
