# handycam.exe
This is a video/image player interface based on the Sony 8mm Handycam interface written in C++ with Allegro library. It is supposed to look as close to the original camera interface as possible and provide a simple playback system to "retroify" them. Font is traced from image capture (scaled 50%) of a real Handycam.

Drag image in to application (on windows) to display it. Type to set custom message. Zoom with pad plus/minus.

## build
- build and install allegro 5
- modify source (commits welcome...)
- modify font (bmpsonyfont.ase = Aseprite)
- convert images to header files:
	- `xxd -i handycam_font.png handycam_font.h`
	- `xxd -i icon.png icon.h`
- modify Makefile for include/library dirs etc. then build and enjoy!
