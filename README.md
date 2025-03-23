# Kung Fu SID GD32 version

![Usage with MixSID on C64](pics/kfsid_units.jpg)

IMPORTANT! Firmware is adjusted for new Kung Fu SID hardware (and only)

A firmware for Kung Fu Flash and Kung Fu Flash GD32 which emulates SID via internal DAC of STM32F405RGT6
All hardware files are copied from "UltiSID" , projects are merged. 
Original UltiSID firmware is obsolete and current Kung Fu SID firmware just is partly based on it in IRQ and SID emulation. 

You can test Kung Fu SID by compiling firmware and flashing to your kung fu flash. 

To use it with original KungFuFlash cartridge output - check previous commits. 
Now only KungFuSID hardware is supported(and devices are selling)

Example usage with MixSIDs (you can use it directly, of course)
![Usage with MixSID on C64](pics/kfsid_mixsid.jpg)

Patched by avrem for GD32F405RGT6 microcontroller.
Patch includes:
* ARR timer fix for GD32
* Memory remap fix (since KFF is running from RAM)

You can use both GD32 or STM32. 