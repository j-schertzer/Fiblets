# Fiblets for Real-Time Rendering of Massive Brain Tractograms

# Installation instructions

This code was tested under Linux using Ubuntu 18.04, 19.04, and 20.04, as well as on Windows 10.

This code requires Qt5 to work as well as cmake, version 3.9 minimum.

## Ubuntu

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
cd ../bin
```

Usage:
```
./FiberVisualier -i NAME_OF_INPUT_FILE [OPTIONS]
Or for manual use: ./FiberVisualizer
Formats supported :
Input: tck, fft
Output: fft
By default, compress the tck input file, except with '-p' and '-c' options
-------Options--------------------
-h: display this help
-o output: specify the name of the output file for compression, default: Input with new extension
-c: capture a full hd frame and exit
-p: compute performances using the input file
-----------------------------------
```

## Windows
A stand alone version of the code, compiled for x64 and tested on windows 10 can be directly executed in the subfolder 
.../standalone/bin/FiberVisualizer.exe

Tested with Visual Studio 2015 on windows10.

With CmakeGui, fill the path of the root CMakeLists.txt (PATHPROJECT) of the project<br>
Fill the build path with : PATHPROJECT/build <br>
Configure for your compilator<br>
If QT 5 is not properly installed cmake will ask for the path to cmake.
(C:\Qt\5.15.2\msvc2015_64\lib\cmake\Qt5) for example

TIP : -it may happen that the development environment executes the compiled code in a wrong directory, 
in the file scene.h there is a commented //#define USE_EXPLICIT_FILENAME_SYSTEM that forces a specific path instead of
a relative path to look for the ressources.

To execute the code, the following QT5 dll (release) must be copied to /bin subfolder
- Qt5Core.dll
- Qt5Gui.dll
- Qt5OpengGL.dll
- Qt5Widget.dll
- platforms/qwindows.dll

# Quick guide

CONTROLS
Camera movement : 
- rotation : leftclick + move mouse
- panning : shift + leftclick + move mouse
- translate along camera direction : shift + rightclick + move mouse
- FOV change : mouse wheel
- Saving camera location : Key S
- Loading camera location : Key C
- Top view : Key Z
- Side view : Key X
- Rear view : Key Y

Individual selection: 
- slider size defines the width of the neighboring selection
- slider radius defines the radius of the tessellated individual fibers
- slider alpha changes the blending factor of individual fibers within the tractogram
- Erase selection: interface button or ctrl + rightclick or ctrl + leftclick on a background area
- Select fibers: ctrl + left click on fibers.

Misc :
- rotation animation: Key A
- Fullscreen: Key F [works only once on windows]
- Benchmark: Key T, proceed to render a rotation with all pipelines and generates a framerate report: "FrameRates.txt"


Meshcut : 
- Use: enable mesh cut culling
- Draw: displays to user the position of the occluder mesh
- Cull inside/outside: selects if the occluder mesh volume is culling or if it is its complementary
- Precise: culls at the fiber level if not activated and at the fragment level if activated (may harm perfs) 

Diffuse/Illuminated lines: shading mode<br>

Culling criterion: select the fibers to display depending on its criterion value (between 0 and 255), see MENU/Criterion

Optimizations: Enables or Disables coverage culling<br>
Aggressive: Deletes a % of fiber with distance to brain

Pipeline: Selects the rendering pipeline, ```clicking on mesh shader with an architecture < Turing (series 20XX) will crash the program```


MENU
File/Open: opens an already compressed tractograms in our .fft format or opens .tck
```Console will notify if anything wrong happens```<br>
File/Open uncompressed: opens a .tck and also fills a basic vbo with the data so that ("Visualization/Draw uncompressed fibers" displays without compression/uncompression)<br>
File/Save: save a file in the fft format<br>
File/Exit: exit<br>

Mode/Normal: allows camera movements and normal interaction<br>
Mode/Spectator: Locks the camera used for the rendering, when culling optimisation are enables, it allows exploring the culling from "behind the scenes"<br>
Mode/Occluder: locks the camera and overrides the camera controls to move the occluder (Tips : tick use and draw occluder  for better control, camera can still be moved using X Y Z keys)<br>

Visualization/White background: change background color<br>
Visualization/Draw uncompressed fibers: see File/Open uncompressed<br>
Visualization/Screenshot: prints screen to a png [key P] also<br>

Shading/Random: Random color per fiber
Shading/Black White: Solid grey color for all fibers
Shading/Orientation: RGB = abs(direction.xyz)
Shading/Colormaps: Color depending on criterion

Criterion/None: no color criterion selected, colormaps shading is not relevant
Criterion/Length: criterion is proportional to the length of fibers
Criterion/Zone of interest: Pressing ```alt + left click``` defines the center from where the distance to the fiber is evaluated, thus computing a distance criterion. Lowest distance is mapped to 255.
Criterion/Lateralization: The criterion is computed with the y coordinate of the center of mass of fibers
# Datase

An example file in fft is available in the data folder.
This file contains 500,000 fibers, 430,000,000 segments and it weighs 5.21GB in the tck format.



# License
Source code provided FOR REVIEW ONLY, as part of the submission entitled
"Fiblets for Real-Time Rendering of Massive Brain Tractograms".

A proper version of this code will be released if the paper is accepted
with the proper licence, documentation and bug fix.
Currently, this material has to be considered confidential and shall not
be used outside the review process.

All right reserved. The Authors
