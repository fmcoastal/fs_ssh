#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <termios.h>

#include "ftty.h" 
#include "frbuff.h" 

#define TEST_TTY

//#define WAI() printf("%d:%s %s\n",__LINE__,__FILE__,__FUNCTION__)
#define WAI() 

#ifdef  TEST_TTY

int g_Debug=0;

// the struct below is passed to the two threads.
typedef struct datablock_struct
{
   int done;
   int fd;
   frbuff * kb_buff;      // ring buffer for keyboard
   frbuff * tx_io_buff;   // ring buffer for tx I_O - serial or ssh
   frbuff * rx_io_buff;   // ring buffre for rx I_O - serial or ssh

} datablock ;

#else

// extern datablock 

#endif



/* the OS holds control till a character is hit,
   the OS returns a character when hit, It does
   not wait for <cr> to be input
*/

/* need create a mode where a thread that sits 
   on getch and puts the results  in a fifo which
   I can pull from when I want to look and not 
   stall my other processing.  
*/ 

int            g_valid_tty_opts_backup=0;
struct termios g_tty_opts_backup={0};


void  reconfig_tty(void)
{
     struct termios  tty_opts_raw;

     if (!isatty(STDIN_FILENO)) {
       printf("Error: stdin is not a TTY\n");
       exit(1);
     }
     printf("stdin is %s\n", ttyname(STDIN_FILENO));

     // Back up current TTY settings
     tcgetattr(STDIN_FILENO, &g_tty_opts_backup);
     // set the flag     
     g_valid_tty_opts_backup=1;

     if( g_Debug != 0 )
     {
         printf(" c_oflag::OPOST   = %d \n", ( !! ( g_tty_opts_backup.c_oflag & OPOST ) ));
         printf(" c_oflag::ONLCR   = %d \n", ( !! ( g_tty_opts_backup.c_oflag & ONLCR ) ));
         printf(" c_oflag::XTABS   = %d \n", ( !! ( g_tty_opts_backup.c_oflag & XTABS ) ));
         printf(" c_oflag::ENOENT  = %d \n", ( !! ( g_tty_opts_backup.c_oflag & ENOENT ) ));
     }
   
     // Change TTY settings to raw mode
     // https://www.qnx.com/developers/docs/6.5.0SP1.update/com.qnx.doc.neutrino_lib_ref/c/cfmakeraw.html
     // The cfmakeraw() function sets the terminal attributes as follows:

     //     termios_p->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
     //     termios_p->c_oflag &= ~OPOST;
     //     termios_p->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
     //     termios_p->c_cflag &= ~(CSIZE|PARENB);
     //     termios_p->c_cflag |= CS8; 

     // https://www.gnu.org/software/libc/manual/html_node/Mode-Data-Types.html
     cfmakeraw(&tty_opts_raw);

     if( g_Debug != 0 )
     {
         printf(" c_oflag::OPOST   = %d \n", ( !! ( tty_opts_raw.c_oflag & OPOST ) ));
         printf(" c_oflag::ONLCR   = %d \n", ( !! ( tty_opts_raw.c_oflag & ONLCR ) ));
         printf(" c_oflag::XTABS   = %d \n", ( !! ( tty_opts_raw.c_oflag & XTABS ) ));
         printf(" c_oflag::ENOENT  = %d \n", ( !! ( tty_opts_raw.c_oflag & ENOENT ) ));
     }
     // leave the output (stdout) behavoir the same :-)
     tty_opts_raw.c_oflag =  g_tty_opts_backup.c_oflag ;

     tcsetattr(STDIN_FILENO, TCSANOW, &tty_opts_raw);
}


void   restore_tty(void)
{
   if ( g_valid_tty_opts_backup != 0 )
   {
       // Restore previous TTY settings
       tcsetattr(STDIN_FILENO, TCSANOW, &g_tty_opts_backup);
       // clear the flag
       g_valid_tty_opts_backup = 0;
   }
}


void test_tty(void)
{
    printf("hit 'ctrl-C' to exit \r\n");
    // Read and print characters from stdin
     int c, i = 1;
     for (c = getchar(); c != 3; c = getchar()) {
         printf("%d. 0x%02x (0%02o)\r\n", i++, c, c);
    }
    printf("You typed 0x03 (003). Exiting.\r\n");
 
}


 int kb_thread(void* arg)
 {
     datablock * pdb =(datablock *)arg;
     char c;
     int r;
     int done;   // there is a problem I need to figure out.  if some other thread kill the
                 // program, we can be sitting inside the getchar() function.  
                 // not clear if teh restor_tty() function get called which leave the console
                 // in a funky state. 
                 // also need to make sure the "restore_tty()" is safe if called multiple times

     WAI();
     printf("Starting kb  Thread\n");
     reconfig_tty();          // configure getchar() to return every keystroke
     done =0;
     while( ( pdb->done == 0 ) && (done == 0))
     {
        c = getchar();
        r = RBuffPut( pdb->kb_buff,(void *) &c); //push value into ring buffer */
        if ( r == 0)
        {
            if(g_Debug != 0)
            {
                 printf("\n[%s] %c  0x%x\n",__FUNCTION__,c,c);
            }
            if( c == 0x03) done=1;
        }
     }
     restore_tty();
     printf("Ending kb Thread\n");
     return 1;
 }



//  #ifdef  TEST_TTY
//  
//  
//  
//  
//  void main(int arc, char ** argv)
//  {
//  
//    reconfig_tty();
//    test_tty();
//    restore_tty();
//  
//  }
//  
//  #endif
