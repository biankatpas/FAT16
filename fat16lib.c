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


// Uncomment this to get more info than you really need through the serial PORT
//#define DEBUG_SERIE





#define INVALID 0xFFFFFFFF   // Invalid sector when searching for a FAT16 boot


//struct root_entry_data {unsigned int sec; unsigned int ptr;} root_entry;

// Structure with info about the FAT16 partition
struct partition_data {\
     unsigned long nsec;
    unsigned short n_fats; unsigned long fat1; unsigned long fat2;\
     unsigned long root; unsigned long data; unsigned long clusters;\
     unsigned short sectors_per_cluster; unsigned int sectors_per_fat;\
     unsigned int root_dir_entries; unsigned int sectors_in_root;\
     unsigned int bytes_per_sector;\
     unsigned short log_bytes_sector; unsigned short log_sectors_cluster;\
     unsigned short log_clusters_in_fat_sector;}\
card;

// Structure with info about an open file
struct file_data {unsigned int fat;unsigned short cluster;unsigned short next;\
                  unsigned int prev; unsigned long pointer;unsigned short sec;\
                  unsigned long size; unsigned int date; unsigned int time;\
                  unsigned int root_entry_sec; unsigned int root_entry_ptr;}\
                  fid;

//                unsigned short set_filesize; unsigned short fill_all_fats;}\

#define FILL_FATS (fs_status&0x01)
#define SET_SIZE  (fs_status&0x02)

// R/W bit (bit 2 in fs_status)
#define FS_RW     (fs_status&0x04)
#define SET_READ_MODE  fs_status&=0xFB         //Set R/W bit in fs_status to 0
#define SET_WRITE_MODE fs_status = (fs_status&0xFB)|0x04   // set R/W bit to 1

unsigned short fs_status=0x00;



char *user_data;
unsigned int buf_index;


unsigned int fat[256];


// Some routines to help with serial port debugging

char* Byte2hex(unsigned char a, char* dest)
{
    unsigned char t;
    unsigned char k=1;

    dest[0]=48; dest[1]=48; dest[2]=0;
    while (a)
    {
        t=(a&0x0F)+48; if(t>57) t+=7; dest[k--]=t;
        a>>=4;
    }
    return dest;
}

char* Int2hex(unsigned int a,char* dest)
{
    unsigned char t,k;

    for (k=0;k<4;k++) dest[k]=48;  dest[4]=0;

    k=3;
    while(a)
    {
        t=(a&0x0F)+48; if(t>57) t+=7; dest[k--]=t;
        a>>=4;
    }
    return dest;
}

char* Long2hex(unsigned long a,char* dest)
{
    unsigned char t,k;

    for (k=0;k<8;k++) dest[k]=48;  dest[8]=0;

    k=7;
    while(a)
    {
        t=(a&0x0F)+48; if(t>57) t+=7; dest[k--]=t;
        a>>=4;
    }
    return dest;
}


void dump_serie(char* txt, unsigned short end)
{
    int k=0;
    while(txt[k]) USART_Write(txt[k++]);
    if (end) { USART_write(0x0D); USART_Write(0x0A);}
}




void dump_fat(int n)
{
 int k;
 for(k=0;k<n;k++)
  {
   dump_serie(Int2Hex(fat[k],txt),0);
   dump_serie(" ",0);
   if ( (k%16)==15 ) dump_serie(" ",1);
  }
}

void dump_buffer(int n)
{
 int k;
 for(k=0;k<n;k++)
  {
   dump_serie(Byte2Hex(user_data[k],txt),0);
   dump_serie(" ",0);
   if ( (k%32)==31 ) dump_serie(" ",1);
  }
}
void show_error(char* name,unsigned short error)
{
  dump_serie(name,0);
  dump_serie(" Error ",0);
  dump_serie(Byte2Hex(error,txt),1);
}



unsigned long get_dword(unsigned short* pt)
{
    unsigned long* lptr= (unsigned long*) (pt);
    return *lptr;
}

unsigned int get_word(unsigned short* pt)
{
    unsigned int* lptr= (unsigned int*) (pt);
    return *lptr;
}


/// Checks if the sector loaded in user_data is a Fat16 boot sector
unsigned short is_FAT16()
{
    char* label="FAT16";
    unsigned short res;

    res=memcmp(label,user_data+0x36,5);  // Compares with label in FAT16 Boot
    if(res) return 0; else return 1;
}


/**************************************************************************/
/**************************************************************************/
// Searches for a FAT16 BOOT SECTOR. Reads sector 0, checks if it is a FAT
// boot sector or a MBR. If it is a MBR searchs for an ACTIVE FAT16 partition.
// Returns the offset to the FAT16 BOOT sector in the card.
// Returns INVALID (0xFFFFFFFF) if no valids FAT16 boot sectors were found.
// After a successful return, the BOOT sector is in user_data[].
unsigned long search_boot()
{
    unsigned short type,active;
    unsigned long ini_sec, n_sec;
    unsigned int label;
    unsigned int k,offset;
    unsigned int test;


    ini_sec=0; delay_ms(50);
    read_sector(ini_sec,user_data);  // Read sector 0

    // Read sector 0 again. Avoids issues with certain CF cards
    read_sector(ini_sec,user_data);

#ifdef DEBUG_SERIE
    //dump_buffer(512);
#endif

    if (is_FAT16()==1)  return ini_sec;  // FAT16 Boot sector found at sector 0


    // If it is not a FAT16, most likely, sector 0 is a MBR

#define pt0 0x1BE
#define type_ptr 0x04
#define ini_sec_ptr 0x08

    for(k=0,offset=pt0;k<4;k++,offset+=16)
    {
        active=(user_data[offset]==0x80)? 1:0;
        type=user_data[offset+type_ptr];
        ini_sec=get_dword(user_data+offset+ini_sec_ptr);
        dump_serie("Act : ",0); dump_serie(byte2hex(active,txt),1);
     dump_serie("Tipo: ",0); dump_serie(byte2hex(type,txt),1);

        // Check for an active FAT16 partition
        if ( (active==1) && ((type==0x06) || (type==0x04)) ) return ini_sec;
    }

    // If we are here we havent found any FAT16 partition
    return INVALID;
}

// Computes log2 of a power of two (2^p  -> p)
unsigned short log2(unsigned int n)
{
    unsigned short k;
    k=0; while(n>>=1) k++;
    return k;
}

/**************************************************************************/
/**************************************************************************/
/* Uses Boot sector to extract all kind of info about the partition */
// A nonzero returns indicates some error.
unsigned short get_partition_info(unsigned long ini_sec)
{
    unsigned short k;


    unsigned int reserved_sectors;
    unsigned long data_sectors;
    unsigned char l;

    read_sector(ini_sec,user_data);

    if (is_FAT16()==0) return 1;  // Double check that is indeed a FAT16

    card.n_fats=user_data[0x10];
    reserved_sectors=get_word(user_data+0x0E);
    card.sectors_per_fat=get_word(user_data+0x16);
    card.root_dir_entries=get_word(user_data+0x11);
    card.sectors_per_cluster=user_data[0x0D];
    card.bytes_per_sector=get_word(user_data+0x0B);

    card.log_bytes_sector=log2(card.bytes_per_sector);
    card.log_sectors_cluster=log2(card.sectors_per_cluster);


    card.fat1 = ini_sec + reserved_sectors;
    card.fat2 = card.fat1 + (card.sectors_per_fat);
    if(card.n_fats==1) card.fat2=card.fat1;

    card.root=card.fat2+ (card.sectors_per_fat);

    card.sectors_in_root = (card.root_dir_entries<<5)>>card.log_bytes_sector;
    card.data = card.root + card.sectors_in_root;


    card.nsec=get_dword(user_data+0x20);
    if(card.nsec==0) card.nsec=get_word(user_data+0x13);


    data_sectors=card.nsec;
    data_sectors-=reserved_sectors;
    for(k=0;k<card.n_fats;k++) data_sectors-=card.sectors_per_fat;
    data_sectors-=card.sectors_in_root;

    card.clusters=data_sectors>>card.log_sectors_cluster;
    card.log_clusters_in_fat_sector=card.log_bytes_sector-1;

    dump_serie("Reserved sec:",0); dump_serie(Int2hex(reserved_sectors,txt),1);
   dump_serie("Data  sector:",0);dump_serie(long2hex(data_sectors,txt),1);

   dump_serie("Bytes/sector:",0);
   dump_serie(Int2hex(card.bytes_per_sector,txt),0);
   dump_serie(" -> ",0);
   dump_serie(Byte2Hex(log2(card.bytes_per_sector),txt),1);

   dump_serie("Sect/cluster:",0);
   dump_serie(Byte2hex(card.sectors_per_cluster,txt),0);
   dump_serie(" -> ",0);
   dump_serie(Byte2Hex(log2(card.sectors_per_cluster),txt),1);

   dump_serie("Sectors/FAT :",0);
   dump_serie(Int2hex(card.sectors_per_fat,txt),1);
   dump_serie("Root entries:",0);
   dump_serie(Int2hex(card.root_dir_entries,txt),1);
   dump_serie("Sectors/root:",0);
   dump_serie(int2hex(card.sectors_in_root,txt),1);

   dump_serie("Inicio FAT 1:",0); dump_serie(Long2Hex(card.fat1,txt),1);
   dump_serie("Inicio FAT 2:",0); dump_serie(Long2Hex(card.fat2,txt),1);
   dump_serie("Ini Root dir:",0); dump_serie(Long2hex(card.root,txt),1);
   dump_serie("Data area   :",0); dump_serie(Long2Hex(card.data,txt),1);

   dump_serie("Total sector:",0);dump_serie(long2hex(card.nsec,txt),1);
   dump_serie("Max clusters:",0);dump_serie(long2hex(card.clusters,txt),1);

    return 0;
}


/*****************************************************************************/
/*****************************************************************************/
// Searches for the first free cluster AFTER cluster with index ini in the
// current FAT sector stored in memory.
// Returns the index to the next free cluster (1-255) of zero if not found
// (that means that ini is then the last free cluster in the current FAT sector.
unsigned short get_next_cluster(unsigned short ini)
{
    unsigned short next=0;

    next=ini+1;
    while (fat[next] && (next)) next++;

/*  {
 //  dump_serie(Byte2Hex(next,txt),0);  dump_serie(" -> ",0);   dump_serie(Int2Hex(fat[next],txt),1);
//  next++;
  }
*/
    return next;
}

/*****************************************************************************/
/*****************************************************************************/
// Searches the FAT for the first sector with a free cluster in it.
// Stores #sector and #cluster found in fid.fat and fid.cluster
// If not free clusters are found returns 1, otherwise 0.
unsigned short get_next_sector_in_fat(unsigned int ini)
{
    unsigned int k;
    unsigned int sector;
    unsigned res=0;
    unsigned int* sd_int = (unsigned int*)user_data;



    for (sector=ini;sector<card.sectors_per_fat;sector++)
    {
        read_sector(card.fat1+sector,user_data);  //dump_buffer(64);
        k=0; while( sd_int[k] && (k<256) ) k++;
//   dump_serie(Byte2Hex(k,txt),0);  dump_serie(" -> ",0);   dump_serie(Int2Hex(sd_int[k],txt),1);
        if (k<256) break;
    }

    fid.fat=sector; fid.cluster=k;
    if (sector==card.sectors_per_fat) res=1; // No hay nada libre

    return res;
}


/*****************************************************************************/
/*****************************************************************************/
// Dumps the contents of the data buffer (n bytes, with n<=512)
// to the corresponding sector of the flash card
void dump_sd_data(unsigned int n)
{
    unsigned long where;
    unsigned long cluster;

    cluster=(fid.fat<<card.log_clusters_in_fat_sector)+fid.cluster;
    where = card.data+ ((cluster-2)<<card.log_sectors_cluster) + fid.sec;
#ifdef DEBUG_SERIE
    // dump_serie(" ",0); LongToStr(where,txt); dump_serie(txt+2,0);
#endif
    write_sector(where,user_data);
    fid.sec++; fid.sec&=(card.sectors_per_cluster-1);
    fid.size+=n;
}

/*****************************************************************************/
/**************************************************************************/
/**************************************************************************/
// Searches for the next free entry in the root directory.
// If returns 0
// Returns: 0 if no free entries are found
//          1 a new entry has been found
//          2 an entry existed already for a file called name
// The root dir sector and position are stored in the fields
// fid.root_entry_sec & fid.root_entry_ptr
unsigned short get_free_entry(char* name)
{
    unsigned int k;
    unsigned int sector;
    unsigned short ch;
    unsigned short first=1;

    for (sector=0;sector<card.sectors_in_root;sector++)
    {
        read_sector(card.root+sector,user_data);
        for(k=0;k<card.bytes_per_sector;k+=32)
        {
            ch=user_data[k];

            //if( ch>0 && ch<0x80) {Usart_Write(ch); line_feed;} //dump_serie(

            if ( (ch==0xE5) && first)  // Unused entry. Save for later
            {
                first=0; fid.root_entry_sec=sector; fid.root_entry_ptr=k; continue;
            }

            if ( ch==name[0])
            {
                //dump_serie("COMP",1);
                if (memcmp(user_data+k,name,11)==0)
                {
                    fid.root_entry_sec=sector; fid.root_entry_ptr=k; return 2;
                }
                continue;
            }

            if ( ch==0x00)  // End of root dir entries
            {
                if (first) {fid.root_entry_sec=sector; fid.root_entry_ptr=k;}
                return 1;
            }

        }
    }

    return 0;
}


// Writes currently stored Fat to the specified FAT sector (sec) in the card.
// If the option fill_all_fats has been selected also populates the second FAT
void save_fat(unsigned int sec)
{
    write_sector(card.fat1+sec,(char*)fat);
// if (fid.fill_all_fats) write_sector(card.fat2+sec,(char*)fat);
    if (FILL_FATS) write_sector(card.fat2+sec,(char*)fat);
}


// Converts the hour/min/seg into a uint16 integer and places it in fid.time
// It does not (by itself) write the data to the root dir entry in the card.
void set_time(unsigned short hour,unsigned short min, unsigned short seg)
{
    unsigned int t;
    t=hour<<6; t+=min; t<<=5; t+=(seg>>1);
    fid.time=t;
}

// Converts the year/month,day into a uint16 integer and places it in fid.time
// It does not (by itself) write the data to the root dir entry in the card.
void set_date(unsigned int year,unsigned short mes,unsigned short day)
{
    unsigned int t;
    t=year; t-=1980; t<<=4; t+=mes; t<<=5; t+=day;
    fid.date=t;
}


// Writes the root_dir entry with the current filesize, date and time
// stored in fid.filesize, fid.date and fid.time
// It takes a sector reading and writing
// It is only done when closing the file or after a cluster is filled
// (if the corresponding option has been selected at FAT init)
void actualiza_filesize()
{
    unsigned long* filesize_ptr;
    unsigned short *entry;

    read_sector(card.root+fid.root_entry_sec,user_data);
    entry=user_data+fid.root_entry_ptr;
    filesize_ptr=(unsigned long*)(entry+28); *filesize_ptr=fid.size;
    memcpy(entry+22,&fid.time,2);
    memcpy(entry+24,&fid.date,2);
    write_sector(card.root+fid.root_entry_sec,user_data);
#ifdef DEBUG_SERIE
    // dump_serie("Current size ",0); dump_serie(Long2Hex(fid.size,txt),1);
#endif
}


// Follows the cluster chain starting at cluster (1st argument), marking all
// clusters as free (0x0000). It actually deletes the file whose staring cluster
// is the first argument
// The second parameter, size, is used to avoid freeing more clusters than the
// actual size of the file (this should not happen, unless the FAT is messy).
unsigned int libera_clusters(unsigned int cluster, unsigned long size)
{
    unsigned short sec,index,current;
    unsigned long L;
    unsigned int bytes_per_cluster;

    bytes_per_cluster=card.sectors_per_cluster<<card.log_bytes_sector;

    index=(cluster&0xFF);  sec=(cluster>>8);
    read_sector(card.fat1+sec,(char*)fat); current=sec; L=0;
    while(L<=size)
    {
        cluster = fat[index];  L+= bytes_per_cluster;
        fat[index]=0x0000;  // Mark as unused.
        if (cluster==0xFFFF) break;
        index=(cluster&0xFF);  sec=(cluster>>8);
        //  dump_serie(Int2Hex(cluster,txt), 0); dump_serie(" ",0);
        if (sec!=current)
        {
            save_fat(current);
            read_sector(card.fat1+sec,(char*)fat); current=sec;
        }
    }

    save_fat(current);

//  dump_serie("Cluster ",0); dump_serie(Int2Hex(cluster,txt), 1);
//  dump_serie("Size    ",0); dump_serie(Long2Hex(L,txt), 1);

    if (cluster==0xFFFF)  return 0;

    return 1;
}


/*****************************************************************************/
/*****************************************************************************/
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
unsigned short open_file(char* name)
{
    unsigned short res,i;
    unsigned int *ini_cluster_ptr;
    unsigned short *entry;
    unsigned int cluster;


    buf_index=0;

    if(get_next_sector_in_fat(fid.fat)) return 1;   // No free clusters, returns 1
    memcpy(fat,user_data,card.bytes_per_sector);

    // Searches next free entry in Root dir. If not found, return 2
    // After the call, the root dir sector is in the data area
    res=get_free_entry(name);
    if (res==0) return 2;

    if (res==2) // Found entry with the same name. Re-use initial cluster
    {
        entry=user_data+fid.root_entry_ptr;
        fid.size =    *(unsigned long*)(entry+28); cluster = *(unsigned int*) (entry+26);
        libera_clusters(cluster,fid.size);
        fid.fat=cluster>>8; fid.cluster=cluster&0xFF;
        read_sector(card.fat1+fid.fat,(char*)fat);
    }

    fid.prev=0xFFFF;
    fid.next=get_next_cluster(fid.cluster);
    fid.sec=0;
    fid.size=0;


    // Fill some fields in the corresponding root dir entry
    entry=user_data+fid.root_entry_ptr;  // points to the root dir entry

    memcpy(entry,name,11); // strcpy(entry,name);  // File name (0-10)
    entry[11]=0x20;      // File attributes (11)
    for(i=12;i<22;i++) entry[i]=0;   // Reserved (12-21)

    memcpy(entry+22,&fid.time,2);  // Time (22-23)
    memcpy(entry+24,&fid.date,2);  // Date (24-25)

    ini_cluster_ptr=(unsigned int*)(entry+26);
    *ini_cluster_ptr=(fid.fat<<8)+fid.cluster;  // Initial cluster (26-27)

    for(i=28;i<32;i++) entry[i]=0;  // Filesize (28-31)

#ifdef DEBUG_SERIE
    dump_serie("Abriendo: ",0); dump_serie(name,0);
 dump_serie(" Root sec ",0); dump_serie(Int2Hex(fid.root_entry_sec,txt),0);
 dump_serie(" Sec ptr ",0); dump_serie(Int2Hex(fid.root_entry_ptr,txt),1);
dump_serie("Current size ",0); dump_serie(Long2Hex(fid.size,txt),1);
#endif

    // write root dir sector with the new entry to the card
    write_sector(card.root+fid.root_entry_sec,user_data);


    fat[fid.cluster]=0xffff; // Marks start cluster as final (just in case)
    save_fat(fid.fat);

    SET_WRITE_MODE; //fid.mode=1;

    return 0;
}


// For debug purpose only
unsigned int check_fat(unsigned int cluster, unsigned long size)
{
    unsigned int nc;
    unsigned short sec,index,current;
    unsigned long L;
    unsigned int bytes_per_cluster;

#ifdef DEBUG_SERIE
    dump_serie("Check file. Init = ",0);
  dump_serie(Int2Hex(cluster,txt), 0); dump_serie(" ",0);
#endif

    bytes_per_cluster=card.sectors_per_cluster<<card.log_bytes_sector;

    index=(cluster&0xFF);  sec=(cluster>>8);
    read_sector(card.fat1+sec,(char*)fat); current=sec; L=0; nc=0;
    while(1)
    {
        cluster = fat[index];  L+= bytes_per_cluster; nc++;
        fat[index]=0x0000;  // Mark as unused.
        if (cluster==0xFFFF) break;
        index=(cluster&0xFF);  sec=(cluster>>8);
#ifdef DEBUG_SERIE
        dump_serie(Int2Hex(cluster,txt), 0); dump_serie(" ",0);
#endif
        if (sec!=current) {read_sector(card.fat1+sec,(char*)fat); current=sec; }
    }

#ifdef DEBUG_SERIE
    dump_serie(" ",1);
  dump_serie("LAST ",0); dump_serie(Int2Hex(cluster,txt), 1);
  dump_serie("Size in file ",0); dump_serie(Long2Hex(size,txt), 1);
  dump_serie("Size in card ",0); dump_serie(Long2Hex(L,txt), 0);
  if(size>L) dump_serie(" ???? ",0);
  dump_serie(". # clusters = ",0); dump_serie(Int2Hex(nc,txt), 1);
#endif


    if (cluster==0xFFFF)  return 0;

    return 1;
}



/*****************************************************************************/
/*****************************************************************************/
// Dumps any data in the user area to the currently opened file and closes it.
// Fixes any needed stuff with FATs table or RootDir entry
void close_file()
{
    unsigned short same_sector=0;
// unsigned int init;
// unsigned short *entry;
// unsigned long size;


    if (FS_RW==0) return; //fid.mode==0) return;  // File opened in read mode: nothing to do

    //////////////////////////////////////////////////////////////////////////////
    // Write or append mode code follows
    //////////////////////////////////////////////////////////////////////////////

    if (buf_index)  dump_sd_data(buf_index);  //  Dump remainng bytes to the card.

    if ((fid.sec==0) && (buf_index==0)) // Mark current as free, mark prev as final
    {
        same_sector = (fid.fat==(fid.prev>>8))? 1:0;
        fat[fid.cluster]=0x0000;  // current = free
        if (same_sector)  fat[fid.prev&0xFF]=0xFFFF;  // Same FAT sector
        save_fat(fid.fat);

        if (same_sector==0)
        {
            read_sector(card.fat1+(fid.prev>>8),(char*)fat);
            fat[fid.prev&0xFF]=0xFFFF;
            save_fat(fid.prev>>8);
        }

#ifdef DEBUG_SERIE
        dump_serie("Back search: ",0);
   dump_serie("Current ",0);  dump_serie(Int2Hex((fid.fat<<8)+fid.cluster,txt),1);
   dump_serie("Prev    ",0);  dump_serie(Int2Hex(fid.prev,txt),1);
#endif
    }


#ifdef DEBUG_SERIE
    // dump_fat(256);
#endif

    // Modifies root dir entry with current filesize, date & time
    actualiza_filesize();

// entry=user_data+fid.root_entry_ptr;
// size =    *(unsigned long*)(entry+28); init = *(unsigned int*) (entry+26);
// check_fat(init,size);


#ifdef DEBUG_SERIE
    //dump_serie(" ",1);
 dump_serie("Closed. Size ",0); dump_serie(Long2Hex(fid.size,txt),0);
 dump_serie(" Root sec ",0); dump_serie(Int2Hex(fid.root_entry_sec,txt),0);
 dump_serie(" Sec ptr ",0); dump_serie(Int2Hex(fid.root_entry_ptr,txt),1);
 dump_serie(" ",1);
#endif

    SET_READ_MODE; //fid.mode=0;
    buf_index=0;
}



/*****************************************************************************/
/*****************************************************************************/
// Writes the WHOLE user data area (512 bytes) to the card in the currently
// opened file. The contents of the user data area ARE NOT preserved after
// this function returns.
void write_sector_to_file()
{
    unsigned short c_cluster;
    unsigned int c_fat;
    unsigned int next_cluster;
    unsigned int k;

    dump_sd_data(512);


    // Modify file size (now 512 bytes larger)
    if ( (SET_SIZE) && (fid.sec==card.sectors_per_cluster-1)) actualiza_filesize();

// if ( (fid.set_filesize) && (fid.sec==card.sectors_per_cluster-1)) actualiza_filesize();


    if (fid.sec) return;  // Still in the same cluster. Nothing else to do.


    // Cluster has been filled. Plenty of stuff to do
    fid.prev=(fid.fat<<8)+fid.cluster;  // Keep the current cluster as previous

    if (fid.next)  // WE still have free clusters in the same FAT sector
    {
#ifdef DEBUG_SERIE
        //    dump_serie("Paso a cluster:",0); ByteToStr(fid.next,txt); dump_serie(txt,1);
#endif

        fat[fid.cluster]=(fid.fat<<card.log_clusters_in_fat_sector)+fid.next;
        fid.cluster=fid.next; fat[fid.cluster]=0xffff;
        save_fat(fid.fat);
    }
    else  // WE have to search for free clusters in a different FAT sector
    {
        c_fat=fid.fat; c_cluster=fid.cluster;  // Save current fat sector and cluster
        get_next_sector_in_fat(fid.fat+1);     // Find next sector & cluster

/*
   dump_serie("Current: Sec ",0); dump_serie(Int2Hex(c_fat,txt),0);
   dump_serie("Clus ",0); dump_serie(Byte2Hex(c_cluster,txt),1);
   dump_serie("Next   : Sec ",0); dump_serie(Int2Hex(fid.fat,txt),0);
   dump_serie("Clus ",0); dump_serie(Byte2Hex(fid.cluster,txt),1);
*/

        // Fill current FAT entry with the newly found cluster
        next_cluster = (fid.fat<<card.log_clusters_in_fat_sector)+fid.cluster;
        fat[c_cluster]= next_cluster;

/*
   dump_serie("Valor next cluster ",0);
   dump_serie(Int2Hex(next_cluster,txt),1);
   for(k=248;k<256;k++) { dump_serie(Int2Hex(fat[k],txt),0); dump_serie(" ",0);}
   line_feed;
*/

#ifdef DEBUG_SERIE
        // dump_serie("Paso a sector:",0); IntToStr(fid.fat,txt); dump_serie(txt,0);
  // dump_serie(" Cluster:",0); ByteToStr(fid.cluster,txt); dump_serie(txt,1);
#endif


        // Once modified, save the current FAT table to the card
        save_fat(c_fat);
        // Move the new FAT sector (now in user data area) to the FAT area
        memcpy(fat,user_data,card.bytes_per_sector);
        // Mark current cluster as final (just in case)
        fat[fid.cluster]=0xffff;
    }

    fid.next=get_next_cluster(fid.cluster);
}


/*****************************************************************************/
/*****************************************************************************/
// Init sequence before using the FAT16 filesystem
// This function is independent of the type of card (SD or CF) used.
// Input arguments:
// buffer : a pointer to a 512 bytes area reserved by the user program.
//          This area IS ALSO USED by the LIBRARY functions, so the user
//          SHOULD NOT TRUST the integrity of the data in this area
//          after a call to the FAT library functions
// set_filesize: 0 means that the file size is not actually written to the root
//               dir entry until the file is closed with close_file().
//               This makes the writing a bit faster but could lead to trouble
//               if the file is not properly closed (in case of a power down).
//               In this case a check disk in the PC fixes the problem
//               1 means that the current file size is regularly (once every
//               cluster) written to the root dir
// both_fats: 0 only the main FAT table is populated.
//            1 all FAT tables are populated
//            Again, 0 means a marginally faster writing to the card. Again, in
//            this case, a check disk in a PC should populate the second FAT
//
// For starters, init_fat(buffer,1,1) is recommended
//////////////////////////////////////////////////////////////////////////////
unsigned short init_fat(\
char* buffer, unsigned short set_filesize,unsigned short both_fats)
{
    unsigned long ini_sec;
    int k;


    user_data=buffer;
    buf_index=0;
//  sd_int=(unsigned int*)buffer;



    set_time(7,45,00);
    set_date(2007,12,12);

    ini_sec=search_boot();

    if (ini_sec==INVALID) return 3;  // Failed to find FAT16

#ifdef DEBUG_SERIE
    dump_serie("INI SECT ",0); dump_serie(Long2Hex(ini_sec,txt),1);
#endif

    if(get_partition_info(ini_sec)) return  2;

    if(get_next_sector_in_fat(0)) return 1;  //previo a cualquier otra cosa.

    memcpy(fat,user_data,card.bytes_per_sector);  // Cargamos FAT al nicio

#ifdef DEBUG_SERIE
    dump_serie("Free cluster:",0);
   dump_serie(Int2hex(fid.fat,txt),0); dump_serie(" ",0);
   ByteToStr(fid.cluster,txt); dump_serie(txt,1);
#endif

    fs_status = (fs_status & 0xFD) | (set_filesize<<1); //fid.set_filesize=set_filesize;
    fs_status = (fs_status & 0xFE) | both_fats;  //fid.fill_all_fats=both_fats;

    return 0;
}


unsigned short get_ready_to_append(unsigned int cluster, unsigned long size)
{
    unsigned short i,sec,index,current;
    unsigned long L,used_bytes;
    unsigned int bytes_per_cluster;
    unsigned short used_sec;
    unsigned long where;


    bytes_per_cluster=card.sectors_per_cluster<<card.log_bytes_sector;



    index=(cluster&0xFF);  sec=(cluster>>8);
    read_sector(card.fat1+sec,(char*)fat); current=sec;  L=0;
    while(1)  //L<=size)  Cambiar tras test
    {
        cluster = fat[index];  L+= bytes_per_cluster;
        if (cluster==0xFFFF) break;
#ifdef DEBUG_SERIE
        //  dump_serie("Size   = ",0); dump_serie(Long2Hex(L,txt), 1);
 // dump_serie("cluster= ",0); dump_serie(Int2Hex(cluster,txt), 1);
#endif
        index=(cluster&0xFF);  sec=(cluster>>8);
        if (sec!=current) {read_sector(card.fat1+sec,(char*)fat); current=sec;}
    }

#ifdef DEBUG_SERIE
    dump_serie("OUT L     = ",0); dump_serie(Long2Hex(L,txt), 1);
  dump_serie("OUT CLUS  = ",0); dump_serie(Int2Hex(cluster,txt), 1);
#endif

    // Didnt get to the end cluster (messy FAT?). Returns 1 (error)
    if (cluster!=0xFFFF) return 1;


    cluster = (sec<<8)+index;  // Last cluster (the one marked as 0xFFFF)
    L-=size;  // number of unused bytes in last cluster;
#ifdef DEBUG_SERIE
    dump_serie("Free bytes in last cluster ",0); dump_serie(Long2Hex(L,txt), 1);
  dump_serie("LAST ",0); dump_serie(Int2Hex(cluster,txt), 1);
#endif

//  dump_fat(96);

    // Initialitation of fields of fid struct
    fid.fat= sec;  fid.cluster=index;
    fid.next=get_next_cluster(index);

#ifdef DEBUG_SERIE
    dump_serie("Init fields",1);
  dump_serie("FAT SECTOR  ",0); dump_serie(Byte2Hex(fid.fat,txt),1);
  dump_serie("FAT Cluster ",0); dump_serie(Byte2Hex(fid.cluster,txt),1);
  dump_serie("NEXT in SEC ",0); dump_serie(Byte2Hex(fid.next,txt),1);
#endif

    if (L>0)  // Some room left in last cluster
    {
        L = bytes_per_cluster-L;
        used_sec = (L>> card.log_bytes_sector);
        used_bytes = L & (card.bytes_per_sector-1);

#ifdef DEBUG_SERIE
        dump_serie("Used bytes  in cluster ",0); dump_serie(Long2Hex(L,txt), 1);
    dump_serie("Used sector in cluster ",0); dump_serie(Byte2Hex(used_sec,txt), 1);
    dump_serie("Used bytes in last sec ",0); dump_serie(Long2Hex(used_bytes,txt), 1);
#endif

        fid.sec=used_sec;
        fid.size=size-used_bytes; // bytes in last sector

        // Load last sector in user_data area.
        where = card.data+ ((cluster-2)<<card.log_sectors_cluster) + fid.sec;
        read_sector(where,user_data);
    }
    else  // New data will start a new cluster.
    {
        used_bytes=0;
        fid.sec=card.sectors_per_cluster-1; fid.size=size-512;

        // Read last sector in cluster (and file)
        where = card.data+ ((cluster-2)<<card.log_sectors_cluster) + fid.sec;
        read_sector(where,user_data);
        // Rewrite last sector, and let write_sector_to_file() do the dirty job
        // of finding next clusters, saving fats around, etc.
        write_sector_to_file();
    }

    buf_index=used_bytes;
    return 0;
}


// Tries to open an existing file in order to APPEND more data.
// Arguments:  name of the file. This argument need not be a properly 0
// terminated char array, BUT IT MUST HAVE AT LEAST 11 chars.
// The first 8 chars make up the name and the last 3 ones the extension.
// name="FILETESTTXT" searches for a file named FILETEST.TXT in the filesystem.
// Return value = 0 : successfull
//              = 1 : no entry with that name was found in the rootdir
//              = 2 : problems with the FAT allocation for that file.
unsigned short append_file(char* name)
{
    unsigned short res;
    unsigned short *entry;
    unsigned long size;
    unsigned int cluster;

#ifdef DEBUG_SERIE
    dump_serie("Append: ",0); dump_serie(name,1);
#endif

    // Searches for name in the Root dir.
    // After the call, the root dir sector is in the data area
    res=get_free_entry(name);
    if (res<2) return 1;  // name not found in root dir

    // Load initial cluster and current size of the file to append
    entry=user_data+fid.root_entry_ptr;
    size =    *(unsigned long*)(entry+28); cluster = *(unsigned int*) (entry+26);

#ifdef DEBUG_SERIE
    dump_serie("Check before append",1); check_fat(cluster,size);
 dump_serie("SIZE  ",0);dump_serie(Long2Hex(size,txt),1);
 dump_serie("FIRST CLUSTER ",0);dump_serie(Int2Hex(cluster,txt),1);
#endif

    // Do the job of placing the right data in the fid structure

    if (get_ready_to_append(cluster,size)==1) return 2; // FAT troubles

    SET_WRITE_MODE; //fid.mode=2;

// if (n>=512) return 2; // Some trouble with the FAT of the file

    //buf_index=n;
    return 0;
}



// DEletes file with given name from the root dir.
// As usual, it does not delete the data of the file.
// It only marks both its rootdir entry and its clusters as free.
// If it doesnt find an entry with that name returns 1.
// Otherwise, returns 0;
unsigned short delete_file(char* name)
{
    unsigned short res;
    unsigned short *entry;
    unsigned long size;
    unsigned int cluster,n;


    // Searches for name in the root dir. If not found, return 1
    // After the call, the root dir sector is in the data area
    res=get_free_entry(name);
    if (res<2) return 1;  // name not found in root dir


    entry=user_data+fid.root_entry_ptr;
    size =    *(unsigned long*)(entry+28);
    cluster = *(unsigned int*) (entry+26);

#ifdef DEBUG_SERIE
    dump_serie("SIZE  ",0);dump_serie(Long2Hex(size,txt),1);
 dump_serie("FIRST CLUSTER ",0);dump_serie(Int2Hex(cluster,txt),1);
#endif

    libera_clusters(cluster,size);

    entry[0]=0xE5;
    write_sector(card.root+fid.root_entry_sec,user_data);

    return 0;
}


// Add n bytes starting at data to the currently opened file.
// Actually, it only writes the data to the buffer supplied by the user.
// This buffer will be saved to the card every time it is full.
// This is the recommended procedure for writing to the file.
void add_data(char* data, unsigned int n)
{
    unsigned int i;

    for(i=0;i<n;i++)
    {
        user_data[buf_index++]=data[i];
        if(buf_index==512)
        {
            write_sector_to_file();
            buf_index=0;
        }
    }
}

// Opens file for reading. It does not read anything
// Returns size of file or 0xFFFFFFFF if file does not exist
unsigned long read_file(char* name)
{
    unsigned short res;
    unsigned short *entry;
    unsigned long size;
    unsigned int cluster,n;


    // Searches for name in the root dir. If not found, return 1
    // After the call, the root dir sector is in the data area
    res=get_free_entry(name);
    if (res<2) return 0xFFFFFFFF;  // name not found in root dir


    entry=user_data+fid.root_entry_ptr;
    fid.size = *(unsigned long*)(entry+28);
    fid.prev = *(unsigned int*) (entry+26);  // Init cluster

#ifdef DEBUG_SERIE
    dump_serie("SIZE  ",0);dump_serie(Long2Hex(size,txt),1);
 dump_serie("FIRST CLUSTER ",0);dump_serie(Int2Hex(cluster,txt),1);
#endif

    fid.fat=fid.prev>>8; fid.cluster=fid.prev&0xFF; fid.sec=0; fid.pointer=0;

    read_sector(card.fat1+fid.fat,(char*)fat); // Load Fat sector of cluster #1

    SET_READ_MODE; //fid.mode=0;

    return (fid.size);
}

// Moves the file pointer to the nth sector of the file
// Next call to fread() will load the nth sector in the user area
unsigned short fseek(unsigned long n)
{
    unsigned int nc, n_cluster,cluster;
    unsigned short n_sec;
    unsigned short current, sec, index;

    fid.pointer=(n<<9);  // Move pointer to the start of nth sector

    if (fid.pointer >= fid.size) return 1;  // Sector n is past the end of file

    n_cluster = n >> card.log_sectors_cluster;
    n_sec = n - (n_cluster<<card.log_sectors_cluster);


    cluster = fid.prev;  // Initial cluster of file
    index=(cluster&0xFF);  sec=(cluster>>8);
    read_sector(card.fat1+sec,(char*)fat); current=sec; nc=0;

    while (nc<n_cluster)
    {
        cluster = fat[index];  nc++;
        index=(cluster&0xFF);  sec=(cluster>>8);
        if (sec!=current) {read_sector(card.fat1+sec,(char*)fat); current=sec; }
    }

    fid.fat=sec; fid.cluster=index; fid.sec=n_sec;

    return 0;
}


// Load the next sector of the file in the user data area
// Advances the file sector pointer.
unsigned short fread()
{
    unsigned long where,cluster;
    unsigned short same_sector;

    if (fid.pointer>=fid.size) return 1; // End of file reached

    cluster=(fid.fat<<card.log_clusters_in_fat_sector)+fid.cluster;
    where = card.data+ ((cluster-2)<<card.log_sectors_cluster) + fid.sec;
#ifdef DEBUG_SERIE
    dump_serie("Read ",0); LongToStr(where,txt); dump_serie(txt+2,0);
#endif
    read_sector(where,user_data);
    fid.sec++; fid.sec&=(card.sectors_per_cluster-1); fid.pointer+=512;


    if (fid.sec) return 0;   // We have not exhausted the current cluster

    // New cluster
    cluster = fat[fid.cluster];
    if (cluster==0xFFFF) return 2;  // End of File reached (should not get here)

    fid.cluster= cluster& 0xFF;

    if ((cluster>>8) != fid.fat)  // New  FAT sector
    {
        fid.fat=cluster>>8;
        read_sector(card.fat1+fid.fat,(char*)fat);  // Load new sector
    }

    // Now we are ready for the next fread()
    return 0;
}
