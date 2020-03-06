This is the repo for ast3roiDS, a Nintendo 3DS version of the classic game using devkitpro.

# Compilation
To compile the game you need GNUMake (any unix-like 
system should have it, if using Windows it can be obtained via mingw) and [devkitpro](https://devkitpro.org/wiki/Getting_Started). Make sure DEVKITPRO and DEVKITARM are declared as enviroment variables.

To compile the game simply go to the repo's main folder and write:
`$ make`

The ast3roiDS.3dsx file is the game executable. It can be opened with a 3DS emulator like [citra](https://citra-emu.org/download/).

# Debugging
To compile a debug version of the game, pass the relevant enviroment variables to the make program:
`$ CFLAGS="-DDEBUG_LOGIC" make`

# Cleanup
To erase all build and output files, run:
`$ make clean`

Please do not commit binary files. Only the minimum amount of sources to build the game should be version-controlled.
