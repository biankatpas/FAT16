/********************************************************************
** @source FAT16 library for PIC microcontrollers
** @compiler: MikroC
**
** @author Copyright (C) 2008 Antonio Tabernero
** @version 1.00
** @version 1.01  // added support for reading files
** @modified Jan 10  2008      Antonio Tabernero. Version 1.0
** @modified Jan 14  2008      Antonio Tabernero. Version 1.1

** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Library General Public
** License as published by the Free Software Foundation; either
** version 2 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.
**
** You should have received a copy of the GNU Library General Public
** License along with this library; if not, write to the
** Free Software Foundation, Inc., 59 Temple Place - Suite 330,
** Boston, MA  02111-1307, USA.
********************************************************************/


// Opens file for reading. It does not read anything, but leaves
// things ready to read firtst sector of the file using fread();
// Returns size of file or 0xFFFFFFFF if file does not exist
unsigned long read_file(char* name);

// Moves the file pointer to the start of the nth sector of the file
// Next call to fread() will load the nth sector into the user area
unsigned short fseek(unsigned long n);

// Load the next sector of the file into the user data area
// Advances the file sector pointer.
unsigned short fread();


unsigned short init_fat(char* buffer, unsigned short set_size,unsigned short fats);
unsigned short open_file(char* name);
void close_file();
unsigned short append_file(char* name);
unsigned short delete_file(char* name);


void add_data(char* data, unsigned int n);

void set_time(unsigned short hour, unsigned short  min, unsigned short seg);
void set_date(unsigned int year,unsigned short mes,unsigned short day);

//unsigned short init_fat(char* buffer, unsigned short set_size,unsigned short fats);
/*****************************************************************************/
/*****************************************************************************/
// Init sequence before using the FAT16 filesystem
// This function is independent of the type of card (SD or CF) used, and
// should be called after you have prepare setup your card
// Input arguments:
// buffer : a pointer to a 512 bytes area reserved by the user program.
//          This area IS ALSO USED by the LIBRARY functions, so the user
//          SHOULD NOT TRUST the integrity of the data in this area
//          after a call to the FAT library functions
// set_filesize: 0 means that the file size is not actually written to the root
//               dir entry until the file is closed with close_file().
//               This makes the writing a bit faster but could lead to trouble
//               if the file is not properly closed (in case of a power down).
//               In this case a check disk in the PC fixes the problem.
//               1 means that the current file size is regularly (once every
//               cluster) written to the root dir
// both_fats: 0 only the main FAT table is populated.
//            1 all FAT tables are populated
//            Again, 0 means a marginally faster writing to the card. Again, in
//            this case, a check disk in a PC should populate the second FAT
//
// For starters, init_fat(buffer,1,1) is recommended
// The gains achieved with the (0,0) options are more relevant when using SD
// cards (I have measured up to a 15-20% improvement) than with CF cards
//////////////////////////////////////////////////////////////////////////////


/*****************************************************************************/
/*****************************************************************************/
// unsigned short open_file(char* name);
// Opens a file for writing whose name is the argument to the function
// The argument name need not be a properly 0 terminated char array,
// BUT IT MUST HAVE AT LEAST 11 chars.
// The first 8 chars make up the name and the last 3 ones the extension.
// name="FILETESTTXT" creates a file named FILETEST.TXT in the file system.
// Returns:  = 0 OK,
//           = 1 the cad is full (no more free clusters)
//           = 2 all entries in the root directory (usually 512) are used up.
// Note that if the file already exists, IT IS OVERWRITTEN.
// Use append_file() to add aditional data to an already existing file.



/*****************************************************************************/
/*****************************************************************************/
// void set_time(byte hour,byte min, byte seg);
// Converts the hour/min/seg into a uint16 integer and places it in fid.time
// It does not (by itself) write the data to the root dir entry in the card.

/*****************************************************************************/
/*****************************************************************************/
// void set_date(uint year,byte mes,byte day);
// Converts the year/month,day into a uint16 integer and places it in fid.time
// It does not (by itself) write the data to the root dir entry in the card.

/*****************************************************************************/
/*****************************************************************************/
// void add_data(char* data, unsigned int n);
// Add n bytes starting at data to the currently opened file.
// Actually, it only writes the data to the buffer supplied by the user.
// This buffer will be saved to the card every time it is full.
// This is the recommended procedure for writing to the file.


/*****************************************************************************/
/*****************************************************************************/
// unsigned short append_file(char* name);
// Tries to open an existing file in order to APPEND more data.
// Arguments:  name of the file. This argument need not be a properly 0
// terminated char array, BUT IT MUST HAVE AT LEAST 11 chars.
// The first 8 chars make up the name and the last 3 ones the extension.
// name="FILETESTTXT" searches for a file named FILETEST.TXT in the filesystem.
// Return value = 0 : successfull
//              = 1 : no entry with that name was found in the rootdir
//              = 2 : problems with the FAT allocation for that file.


/*****************************************************************************/
/*****************************************************************************/
// unsigned short delete_file(char* name);
// DEletes file with name given in the argument from the root dir.
// Again, this argument need not be a properly 0 terminated char array,
// BUT IT MUST HAVE AT LEAST 11 chars.
// The first 8 chars make up the name and the last 3 ones the extension.
// name="FILETESTTXT" deletes the file named FILETEST.TXT in the filesystem.
// As usual, it does not delete the data of the file.
// It only marks both its rootdir entry and its clusters as free.
// If it doesnt find an entry with that name returns 1.
// Otherwise, returns 0;


/*****************************************************************************/
/*****************************************************************************/
// void close_file();
// Dumps any data in the user area to the currently opened file and closes it.
// Fixes any needed stuff with FATs table or RootDir entry



