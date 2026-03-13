# Sensor Read
This binary is made to interface with the sensor and process its output into a window(for now)

## Build 

For windows using msys ucrt64:
Install dependencies : 
```
$ pacman -S mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-glfw mingw-w64-ucrt-x86_64-libusb
```

Build : 
```
$ gcc main.c uvc.c esp770u.c ar0134.c -I/c/msys64/ucrt64/include/libusb-1.0 -I/c/msys64/ucrt64/include/SDL2 -L/c/msys64/ucrt64/lib -lmingw32 -lSDL2main -lSDL2 -lusb-1.0 -lgl -o goggles
```

## Run
```.\goggles.exe``` (sensor id 0 : first, 1 : second plugged in) *to be done exposure and gain settings* 
You can change gain and exposure inside the window by pressing G or E and use modifier Shift to increase/decrease these values.
E.G. : press e to decrease exposure and press shift and e to increase.