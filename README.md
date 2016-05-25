# XenobladeSave
Xenoblade Wii Save Parser 

This program will parse a Xenoblade save file for the Wii and rename the save
file accordingly. It can also convert the RGB565 TPL images stored in the save files
to an RGB565 format BMP file.

Xenoblade save files are 0x28000 bytes long, and start with the magic number
USRD. Empty save files start with DMMY. The save files are Big Endian.

####System Flags
If the save is a system save, (Where the game prompts you to save) a flag at
0x86 will be set.  
If the save is from a new game plus, a flag at 0x87 will be set.

####Save Location
The current map the player is in is stored as 4 bytes at 0x2e. for example,
0x00010001 is Colony 9, and 0x00040002 is the Ether Mines.



####Current Party
Party members and guests are stored, in order, as u32s from 0x34 to 0x63.  
The level of the lead party member is at 0x85.

####Play Time
Playtime is stored as a u32 at 0x11eb0. Every 0x1000 units is equal to one
hour. (0x1000 = 1:00:00 0x24000 = 36:00:00) But only the units from 0x00 to
about 0xefb are used within each hour.  
(0x23efb = 35:59:59 0x24000 = 36:00:00)

####Save Time
Year - 0x24 - 2 bytes  
Month - 0x27  
Day - 0x29  
Hour - 0x28  
Minute - 0x22  
Second - 0x23  

####Save Image
The save images are in standard tpl rgb565 format.
We convert the image by going through one block at a time, row
by row. The image is 164x116, and it's stored in 4x4 blocks, so there
are 29 rows, and 41 columns of 4x4 blocks. We just make the image so
that each pixel is stored sequentually, flip the byte order on each
pixel, and slap a bmp header on it.

    1  2  3  4  17 18 19 20     1   2   3   4   5   6   7   8   ...
    5  6  7  8  21 22 23 24 --> 165 166 167 168 169 170 171 172 ...
    9  10 11 12 25 26 27 28 --> 329 330 331 332 333 334 335 336 ...
    13 14 15 16 29 30 31 32     493 494 495 496 497 498 499 500 ...
