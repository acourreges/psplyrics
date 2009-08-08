# PSPLyrics

### [Website](http://www.adriancourreges.com/projects/psplyrics/)&nbsp;&nbsp;&nbsp;&nbsp;[Download](http://www.adriancourreges.com/projects/psplyrics/#dl-section)&nbsp;&nbsp;&nbsp;&nbsp;

PSPLyrics is a homebrew for [Sony's PSP](http://en.wikipedia.org/wiki/PlayStation_Portable) which displays lyrics synchronized with the music as a song is being played.  
It includes its own MP3 player (MP3 ME Player) and is able to read LRC files which are widely available on the Internet.

PSPLyrics can handle most of the sets of characters, including non-latin ones like Japanese or Chinese symbols. 

![screenshot-1](http://www.adriancourreges.com/projects/psplyrics/psplyrics-screen2.png)

The program will look for MP3 files located in the `/MUSIC` folder.
Lyrics files (.lrc) must be put inside the `/LRC` folder. The LRC file must have the same name as the corresponding MP3 file (except for the extension).
If you experience some problem with the display of the lyrics, make sure your LRC files are UTF-8 encoded. 

## Requirements

Your PSP needs to be running a custom firmware otherwise you won't be able to launch the homebrew.  
PSPLyrics has been successfully tested against a 5.0 M33-4. 

## License

PSPLyrics is published under the GNU GPLv2 license. 

## Credits

- the [ps2dev community](http://ps2dev.org/) for their great work on the PSP SDK.
- caliarbor, sakya, joek2100 for their sample codes to play mp3 files through the PSP’s Media Engine.
- BenHur for the excellent intraFont library.

