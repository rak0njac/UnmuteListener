# UnmuteListener
A background Win32 application which serves as a fix for a bug where a USB PnP Sound Device microphone is getting muted and/or having it's volume control changed immediately after being plugged in

# Basics
I was having an issue where multiple drivers would get caught doing scary stuff to my microphone endpoint after plugging in my USB headset. What would happen usually was that it would either get muted, or have it's volume control set to 0, or both. It's an issue related to communication between multiple drivers, in my case:
1. VEN_8086&DEV_AE50 - Intel® Smart Sound Technology for USB Audio
2. VEN_8086&DEV_AE20 - Intel® Smart Sound Technology for Digital Microphones
3. VEN_10EC&DEV_0236 - Realtek High Definition Audio

and the prey USB device:

4. VID_8086&PID_0808 - USB PnP Sound Device

# Solution
I didn't really care for having my computer remember the previouosly set microphone volume of all the microphone devices etc. The solution is really simple - this WINAPI program listens for a default device changed event in the capture device context using IMMNotificationClient. WNDPROC then responds to that event immediately by setting mute to false and volume to 100% using IAudioEndpointVolume. The solution is likely not ideal and not for everyone but it works perfectly in my case since it seems that the event response is being processed *after* the drivers have already finished fiddling with the microphone mute/volume settings. In any case, the code is really simple and should allow easy and quick modifications.

# Build instructions
## Windows
1. Open UnmuteListener.sln in Visual Studio 2019 or newer. 
2. Build as x86.
## Linux
Still using Linux on desktop in 2023?
1. Switch to Windows
2. Read Windows build instructions

# Can I build as x64?
Seriously?
No, really, I have no idea.
