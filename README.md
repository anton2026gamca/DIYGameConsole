# DIYGameConsole

## 1. Computer

I went with the Raspberry Pi 4 4GB, witch needs 5V/3A unlike the Pi 5 witch needs 5V/5A. I could not find a hat that would output 5V/5A.

## 2. Screen

I picked the [elecrow rc050s](https://www.elecrow.com/rc050s-hdmi-5-inch-800x480-capacitive-touch-monitor-built-in-speaker-with-backlight-control.html) because i wanted at least a 5 inch screen, not so expensive and with speakers so i could output sound through HDMI.

## 3. Battery

Here, i think the best choise any the PiSugar, i specifically went with the PiSugar 3 Plus since i wanted to display the battery percentage on the screen.

## 4. Gamepad

I wanted to make a game console like the nintendo switch or something like that so i needed a built in controller. I ordered some [buttons](https://amzn.eu/d/3mklatI), some joysticks and an 5V arduino pro micro since it has the atmega32u4 chip (apparently you can make keyboards/gamepads easy with this chip).
Firstly i made it on a breadboard but when i saw that it worked, i ordered some [perfboards](https://amzn.eu/d/5Spa3rH) and soldered the components to it
It turned out pretty well, though in the future i will design a custom pcb for the gamepad. ([source code](https://github.com/anton2026gamca/DIYGameConsole/tree/main/Gamepad))

![20250125_104146](https://github.com/user-attachments/assets/ffa881f9-7f4e-4f9e-96d8-b4a8121690f5)

## 5. Software

Firstly i installed [retropie](https://retropie.org.uk/) on it, coded some overlays for the battery, volume and power buttons using the dispmanx library ([source code](https://github.com/anton2026gamca/DIYGameConsole/tree/main/ConsoleUI/dispmanx)), but i wanted to try minecraft on it and minecraft works only on a 64 bit systems and the dispmanx library is only for 32 bit systems so i needed something else. I decided to run the emulationstation from a X11 desktop and create a transparent window on top of it, it worked! ([source code](https://github.com/anton2026gamca/DIYGameConsole/tree/main/ConsoleUI/x11))

Yeah, and i installed minecraft using [Pi-Apps](https://pi-apps.io/).

![20250125_104614](https://github.com/user-attachments/assets/23d79d3e-446d-4c80-bcc1-24bdb06ae3e9)
