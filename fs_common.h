#ifndef _fs_common_h
#define  _fs_common_h

// THIS FILE IS DIFFERENT FOR EACH Project I DO;


// the struct below is passed to the two threads.
typedef struct
{
   int done;
   int fd;
   frbuff * p_kb_buff;      // ring buffer for keyboard
   frbuff * p_tx_io_buff;   // ring buffer for tx I_O - serial or ssh
   frbuff * p_rx_io_buff;   // ring buffre for rx I_O - serial or ssh
}datablock;





extern char RED[];    //  = { 0x1b,'[','1',';','3','1','m',0x00 };
extern char GREEN[];  //  = { 0x1b,'[','1',';','3','2','m',0x00 };
extern char YELLOW[]; //  = { 0x1b,'[','1',';','3','3','m',0x00 };
extern char BLUE[];   //  = { 0x1b,'[','1',';','3','4','m',0x00 };
extern char MAGENTA[]; // = { 0x1b,'[','1',';','3','5','m',0x00 };
extern char CYAN[];   //  = { 0x1b,'[','1',';','3','6','m',0x00 };
extern char NC[];     //  = { 0x1b,'[','0','m',0x00 };

#endif
