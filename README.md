# DIYGameConsole

I started this project because i always wanted a game console and they were expensive, so i thought this would be cheaper (it isn't). And also i like to do my own things so it is kind of fun!

## Feature overview

[![Watch the video](https://github.com/user-attachments/assets/9050162f-ae0c-4aab-9f75-4fed0fd997a0)](https://drive.google.com/file/d/1AkpKpJxUvkrZwO-a-s5R-MTxC0ftZYuF/preview)

## 1. Computer

I went with the Raspberry Pi 4 4GB, witch needs 5V/3A unlike the Pi 5 witch needs 5V/5A. I could not find a hat that would output 5V/5A.
Thanks to [High Seas](https://highseas.hackclub.com/), i ordered a Raspberry Pi CM5 and i will try it out if it works, also, i wan't to design a custom pcb for the cm5 later.

## 2. Screen

I picked the [elecrow rc050s](https://www.elecrow.com/rc050s-hdmi-5-inch-800x480-capacitive-touch-monitor-built-in-speaker-with-backlight-control.html) because i wanted at least a 5 inch screen, not so expensive and with speakers so i could output sound through HDMI. And i especially like the HDMI and USB connectors since i don't have to deal with some cables for this.

![20250125_110155](https://github.com/user-attachments/assets/48a1e757-2eee-46a7-94ed-2fe63a07b154)

## 3. Battery

Here, i think the best choise is any PiSugar, i specifically went with the PiSugar 3 Plus since i wanted to display the battery percentage on the screen.

## 4. Gamepad

I wanted to make a game console like the nintendo switch or something like that so i needed a built in controller. I ordered some [buttons](https://amzn.eu/d/3mklatI), some joysticks and an 5V arduino pro micro since it has the atmega32u4 chip (apparently you can make keyboards/gamepads easy with this chip).
Firstly i made it on a breadboard but when i saw that it worked, i ordered some [perfboards](https://amzn.eu/d/5Spa3rH) and soldered the components to it
It turned out pretty well, though in the future i will design a custom pcb for the gamepad. ([source code](https://github.com/anton2026gamca/DIYGameConsole/tree/main/Gamepad))

![20250125_104146](https://github.com/user-attachments/assets/ffa881f9-7f4e-4f9e-96d8-b4a8121690f5)

## 5. Software

Firstly i installed [retropie](https://retropie.org.uk/) on it, coded some overlays for the battery, volume and power buttons using the dispmanx library ([source code](https://github.com/anton2026gamca/DIYGameConsole/tree/main/ConsoleUI/dispmanx)), but i wanted to try minecraft on it and minecraft works only on a 64 bit systems and the dispmanx library is only for 32 bit systems so i needed something else. I decided to run the emulationstation from a X11 desktop and create a transparent window on top of it, it worked! ([source code](https://github.com/anton2026gamca/DIYGameConsole/tree/main/ConsoleUI/x11))

Yeah, and i installed minecraft using [Pi-Apps](https://pi-apps.io/).

![20250125_104614](https://github.com/user-attachments/assets/23d79d3e-446d-4c80-bcc1-24bdb06ae3e9)

## 6. Case

The case is here! It is not the best but it works!

https://github.com/user-attachments/assets/293d6e5e-0a7c-4e68-9f84-87ab1f90e6ef

https://github.com/user-attachments/assets/acda32a5-d430-4d29-b4fa-1ae6db6a9b4d

## 7. Cusom PCB

I've designed a custom pcb for the console. It's nothing much, but what do you expect? It's my first pcb!

![Screenshot 2025-01-31 192927](https://github.com/user-attachments/assets/158f0544-ea43-41f7-b3ba-4160be894086)

Also i've designed a small board to connect the PiSugar to the Raspberry Pi instead of the wires like in the video

![Screenshot 2025-01-31 204851](https://github.com/user-attachments/assets/e558f460-9612-4ce6-89d4-82547fbbf516)
