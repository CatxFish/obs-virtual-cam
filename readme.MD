# OBS-VirtualCam
obs-virutalcam is a plugin for obs-studio , transforming the output video to a virtual directshow device.

**Supported Platforms** : Windows 7 , Windows 8 and Windows 10

**Supported OBS Studio version** : 21.0.0+

# Features
* **virtual output** : A output plugin sink raw video & audio to directshow interface.
* **virtual filter output** : A filter plugin sink obs source video to directshow interface.
* **virtual source** : Four directshow Interfaces which can use in 3rd party software.

# Install
The installer and compressed file can be found in [Release Page](https://github.com/CatxFish/obs-virtual-cam/releases). Using installer is recommended, but if you want to use compressed file to install manually , please follow these instructions.

1. Unzip OBS-VirtualCam.zip and put it to your obs-studio install folder.
2. Run CMD as Administrator and register 32bit directshow source
```
regsvr32 "C:\Program Files\obs-studio\bin\32bit\obs-virtualsource.dll"
```
3. Do it again to register 64bit directshow source
```
regsvr32 "C:\Program Files\obs-studio\bin\64bit\obs-virtualsource.dll"
```
- If you want to Remove the directshow filter , you can also use regsvr32 to do this
```
regsvr32 /u "C:\Program Files\obs-studio\bin\32bit\obs-virtualsource.dll"
```

# Register specific number of virtual cameras
Unregister then register 2 directshow camera (up to 4)
```
regsvr32 /u "C:\Program Files\obs-studio\bin\64bit\obs-virtualsource.dll" 
regsvr32 /n /i:"2" "C:\Program Files\obs-studio\bin\64bit\obs-virtualsource.dll"
```


# Build
You need to install cmake , visual studio 2017 ,and build OBS project first. 
Set following Cmake variables:
- **QTDIR** (path): QT folder
- **DepsPath** (path): FFmpeg folder in OBS dependencies package
- **LIBOBS_INCLUDE_DIR** (path) : Libobs  include folder
- **LIBOBS_LIB** (filepath) : obs.lib path
- **OBS_FRONTEND_LIB** (filepath): obs-frontend-api.lib path
- **PTHREAD_LIB** (filepath): w32-pthread.lib path

# Donate
If you like my work on this plugin , you can donate via [Paypal.me](https://www.paypal.me/obsvirtualcam)
