# OBS-VirtualCam
obs-virutalcam is a plugin for obs-studio , transforming the output video to a virtual directshow device.
# Features
* **virtual output** : the plugin sink Raw Video & Audio to Directshow Interface
* **virtual source** : Directshow Interface which can use in other software
# Downloads
The installer and compressed file can be found in [Release Page](https://github.com/CatxFish/obs-virtual-cam/releases). Using installer is recommended, but if you want to use compressed file to install manually , please follow these three instructions.

1. Unzip OBS-VirtualCam.zip and put it to your obs-studio install folder.
2. Run CMD as Administrator and register 32bit directshow source
> command: regsvr32 C:\Program Files (x86)\obs-studio\bin\32bit\obs-virtualsource.dll
3. Do it again to register 64bit directshow source
> command: regsvr32 C:\Program Files (x86)\obs-studio\bin\64bit\obs-virtualsource.dll

If you want to Remove the directshow filter , you can also use regsvr32 to do this
> command: regsvr32 /u C:\Program Files (x86)\obs-studio\bin\32bit\obs-virtualsource.dll
# Build
You will need cmake , visual studio2013 ,and build OBS project first. 
Set following Cmake variables:
- **QTDIR** (path): QT folder
- **DepsPath** (path): FFmpeg folder in OBS dependencies package
- **LIBOBS_INCLUDE_DIR** (path) : Libobs  include folder
- **LIBOBS_LIB** (filepath) : obs.lib path
- **OBS_FRONTEND_LIB** (filepath): obs-frontend-api.lib path
# Directshow output format
**Video** 
- YUY2
- 16:9 or 4:3 aspect ratio
- height can be divided by 2
- minimum size 320x240

**Audio**
- PCM signed 16-bit little-endian 44.1khz
