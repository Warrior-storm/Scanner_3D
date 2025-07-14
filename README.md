# Scanner_3D
In this proyect, we made a Scanner 3D with Arduino Mega.

Explanation about the Code
The Arduino sketch (sketch_jun9a.ino) is the main control program for this 3D scanner. It does the following:

Controls two stepper motors to rotate the platform (R axis) and move vertically (Z axis).
Reads distance measurements from an analog sensor to capture the shape of an object.
Uses an OLED display to show scanning progress, coordinates, and mode information.
Allows the user to interact using buttons: start scan, reset, change scan mode.
Supports two scanning modes: precise (slower, more detailed) and fast (quicker, less detailed).
Outputs scanned data (X, Y, Z coordinates) via the Serial port for later processing and 3D reconstruction.
The code is written entirely in C++ for the Arduino Mega platform, using libraries for the OLED display and standard Arduino functions for hardware control.
