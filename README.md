# Simple mod player
A mod player that downloads a random music from https://api.modarchive.org/,
save the music in a directory and play it (using xmp) back 
showing information of the music file like name, length and who compose it.

## Dependencies
- [xmp-cli](https://github.com/libxmp/xmp-cli)

## Features
- Automatically download a random MOD file from [modarchive.org](https://modarchive.org/)
- Download only if not previously downloaded
- Accepts 2 types of MOD files:
  - M.K. (Amiga)
  - IMPM
- Accepts music name to play a mod file (command line argument)

## Quick start
- clone project
- execute
```console
$ ./build.sh
$ ./modplayer
```
