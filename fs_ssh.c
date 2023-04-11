#include <stdint.h>
#include <stdio.h>
#include <libssh/libssh.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>          // for thread Items
#include "frbuff.h"
#include "ftty.h"
#include "fs_common.h"

pthread_t g_tid[3];             // posix thread array
datablock g_db;   
datablock *pg_db = &g_db;   

//#define WAI() printf("%d %s-%s\n",__LINE__,__FILE__,__FUNCTION__)
#define WAI()

int  g_verbose       = {0};
int  g_port          = {0};
char g_dst_ip[32]   = {0};
char g_username[128] = {0};
char g_password[128] = {0};

#if 0
References:
https://api.libssh.org/stable/libssh_tutor_guided_tour.html
https://api.libssh.org/master/libssh_tutor_shell.html
https://stackoverflow.com/questions/54265917/how-to-properly-include-libssh-in-c

#endif
int READ_interactive_shell_session(ssh_channel channel)
{
  int rc;
  char buffer[256];
  int nbytes;

  WAI();

  rc = ssh_channel_request_pty(channel);
  if (rc != SSH_OK) return rc;

  rc = ssh_channel_change_pty_size(channel, 80, 24);
  if (rc != SSH_OK) return rc;

  rc = ssh_channel_request_shell(channel);
  if (rc != SSH_OK) return rc;

  while (ssh_channel_is_open(channel) &&
         !ssh_channel_is_eof(channel))
  {
    nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
    if (nbytes < 0)
      return SSH_ERROR;
    if (nbytes > 0)
      write(1, buffer, nbytes);
  }
  return rc;
}


/* A very simple terminal emulator:
   - print data received from the remote computer
   - send keyboard input to the remote computer
*/
int interactive_shell_session(ssh_channel channel)
{
  char buffer[256];
  int nbytes, nwritten;
  int rc;
  int done=0;
  char c;
  char *pc = &c;
  int r;
         
  WAI();
  /* request a tty console on the far end */ 
  rc = ssh_channel_request_pty(channel);
  if (rc != SSH_OK) return rc;

  /* set the size of the window on the far side */
  rc = ssh_channel_change_pty_size(channel, 80, 24);
  if (rc != SSH_OK) return rc;

  /* run a shell ontop of the TTY console */
  rc = ssh_channel_request_shell(channel);
  if (rc != SSH_OK) return rc;


  printf("ctrl-C to exit \n\r");

  while (ssh_channel_is_open(channel) &&
         !ssh_channel_is_eof(channel) && (done== 0))
  {
    nbytes = ssh_channel_read_nonblocking(channel, buffer, sizeof(buffer), 0);
    if (nbytes < 0) return SSH_ERROR;
    if (nbytes > 0)
    {
      // 1 is stdout
      nwritten = write(1, buffer, nbytes);
      if (nwritten != nbytes) return SSH_ERROR;
      // 0 is stdin ??
    }
    // Fetch input keyboard
    r = RBuffFetch( pg_db->p_kb_buff,(void **) &pc);
    if ( r == 0 )
    {  
      
//      printf("in:  %c %x \n",*pc,*pc);
        // write out ssh
        nwritten = ssh_channel_write(channel, pc , 1);
       
        if (nwritten != 1) return SSH_ERROR;
        if ( *pc == 0x03) done =1;
    }


  }

  return rc;
}


ssh_channel ssh_open_channel(ssh_session session)
{
  ssh_channel channel;
  int rc;
 
  WAI();
  channel = ssh_channel_new(session);
  if (channel == NULL)
    return NULL;
 
  rc = ssh_channel_open_session(channel);
  if (rc != SSH_OK)
  {
    printf("[%d] Failed to open a channel on this session %d\n",__LINE__,rc);
    ssh_channel_free(channel);
    return NULL;
  }
 
//  ssh_channel_send_eof(channel);
//  ssh_channel_close(channel);
//  ssh_channel_free(channel);
 
  return channel;
}


int  ssh_close_channel(ssh_channel channel )
{
  WAI();
  ssh_channel_send_eof(channel);
  ssh_channel_close(channel);
  ssh_channel_free(channel);
 
  return SSH_OK;
}






// ssh_session is a struct ssh_session_struct *   aka it is a pointer !!

// create a terninal  /dev/ptmx


int show_remote_processes(ssh_session session)
{
  ssh_channel channel;
  int rc;
  char buffer[256];
  int nbytes;

  char cmd[256];
  WAI();
 
  channel = ssh_channel_new(session);
  if (channel == NULL)
    return SSH_ERROR;
 
  rc = ssh_channel_open_session(channel);
  if (rc != SSH_OK)
  {
    ssh_channel_free(channel);
    return rc;
  }
 
//  strcpy(cmd,"ps aux");
//  strcpy(cmd,"ls -l /sys/class/net ");
  strcpy(cmd,"echo  1 > /home/fsmith/depot/fs_expect/BR549 ");
  printf (" %s %s %s\n",GREEN,cmd,NC);
  rc = ssh_channel_request_exec(channel, cmd);
  if (rc != SSH_OK)
  {
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    return rc;
  }
 
  nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
  while (nbytes > 0)
  {
    if (write(1, buffer, nbytes) != (unsigned int) nbytes)
    {
      ssh_channel_close(channel);
      ssh_channel_free(channel);
      return SSH_ERROR;
    }
    nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
  }
 
  if (nbytes < 0)
  {
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    return SSH_ERROR;
  }
 
  ssh_channel_send_eof(channel);
  ssh_channel_close(channel);
  ssh_channel_free(channel);
 
  return SSH_OK;
}




 
ssh_session fs_ssh_init(char * dst_ip, int port, char * user, char *pass,int verbosity) {
    int rc;
//    int port=22;
//    char pass[] = "fsmith";
//    char user[] = "fsmith";
//    char dst_ip[] = "10.75.47.44";
//    int verbosity ;

    WAI();
    verbosity=SSH_LOG_FUNCTIONS;
    verbosity=SSH_LOG_WARNING;
    verbosity=SSH_LOG_PROTOCOL;
    verbosity=SSH_LOG_PACKET;
    verbosity=SSH_LOG_NOLOG;

    ssh_session my_ssh_session = ssh_new();
    if (my_ssh_session == NULL)
    {
        exit(-1);
    }

    ssh_options_set(my_ssh_session, SSH_OPTIONS_HOST, dst_ip );
    ssh_options_set(my_ssh_session, SSH_OPTIONS_PORT, &port );
    ssh_options_set(my_ssh_session, SSH_OPTIONS_USER, user );
    ssh_options_set(my_ssh_session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity );

    rc = ssh_connect(my_ssh_session);
    if(rc != SSH_OK){
        fprintf(stderr, "Error connecting to localhost: %s\n", ssh_get_error(my_ssh_session) );
        exit(-1);
    } 

// Verify the servers Identitiy
//  TBD

// Authenticate ourselves

    ssh_userauth_password(my_ssh_session, NULL, pass);

    return my_ssh_session;

}

void fs_ssh_distroy(ssh_session session )
{
    ssh_disconnect( session);
    ssh_free( session);

}


# if 1
char RED[]     = { 0x1b,'[','1',';','3','1','m',0x00 };
char GREEN[]   = { 0x1b,'[','1',';','3','2','m',0x00 };
char YELLOW[]  = { 0x1b,'[','1',';','3','3','m',0x00 };
char BLUE[]    = { 0x1b,'[','1',';','3','4','m',0x00 };
char MAGENTA[] = { 0x1b,'[','1',';','3','5','m',0x00 };
char CYAN[]    = { 0x1b,'[','1',';','3','6','m',0x00 };
char NC[]      = { 0x1b,'[','0','m',0x00 };


uint64_t  g_error = 0;
// need to use the escape key to send special charactes
// this is because getchar does not return Ctrl Sequences
#define STATE_NORMAL  0
#define STATE_ESC     1
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////









int doTerminalServer( frbuff * rx,  frbuff * tx, frbuff * kb )
{
    char c;
    int r = 0;
    int d = 0;
    int state = STATE_NORMAL;

    WAI();

    RBuffFlush( rx );    // flush the Rx Buffer
//    g_RxThreadDisableRBuff = 1; // have Rx thread not stuff RX Buffer
//    g_RxThreadPrint = 1;        // have Rx thread print responses from the      target
    while( d == 0 )
    {
#ifdef FTTY
#else
       c = getchar();
#endif
//       printf("%c",c);   // local echo??

       switch (state) {
       case STATE_NORMAL:
           if( c == 0x1b)
           {
               state = STATE_ESC ;
           }
           else
           {
              r =  RBuffPut( tx, (void *) &c );    /* put value int      ring buffer */
              if( r != 0)
              {
                 g_error = r;
              }
           }
           break;

       case STATE_ESC:
           if( c == 0x1b)
           {
               d = 1;
           }
           else if ( c == 'q' ) // ctrl - Q  -> upper case Q
           {
              c = 0x0d;
              r =  RBuffPut( tx , (void *) &c );    /* put value int      ring buffer */
           }
           state = STATE_NORMAL;

           break;
       } // end state switch

   } //end while

    usleep(250000);           // wait a little
//    RBuffFlush(g_pRxSerial);  // flush the Rx Buffer
//    g_RxThreadPrint = 0;      // disable  Rx tread print responses from the      target
//    g_RxThreadDisableRBuff = 0; // have Rx thread not stuff RX Buffer
    return 0;
}


extern  int kb_thread(void* arg);

void *WorkerThread(void *arg)
{
pthread_t id = pthread_self();
   WAI();
   if(pthread_equal(id,g_tid[0]))
   {
       WAI();
       // tx
       kb_thread(arg);
   }
   else if (pthread_equal(id,g_tid[1]))
  {
      WAI();
      // rx
//      rx(arg);
  }
  else
  {
      WAI();
       // tx
  //     tx(arg);
  }
  return NULL;

}



extern int g_Debug;

void  cleanup(  datablock *p)
{
    WAI();
    if ( p->p_kb_buff != NULL)    RbuffClose( p->p_kb_buff );
    if ( p->p_tx_io_buff != NULL) RbuffClose( p->p_tx_io_buff );
    if ( p->p_rx_io_buff != NULL) RbuffClose( p->p_rx_io_buff );
}


int main(int argc, char ** argv)
{
    ssh_session session;
    int mode  ;
    int result = 0;
    int i;
    int err;
    int n_threads = 0;

    mode=3;
    if ( argc > 1 ) mode=atoi(argv[1]);

    strcpy (g_dst_ip, "10.75.1.87" );
    strcpy (g_username,"fsmith"); 
    strcpy (g_password,"fsmith");
    g_verbose=0;
    g_port=22;


    // Initialize Default Values & Data Structures
    memset((void*)&g_db,0,sizeof(datablock));
    pg_db = &g_db;

 //  Create Buffers for the kb and IO interfaced.  


     if ( mode == 1 )  
     {
         printf(" running ftty / keyboard behavior test \n");
         n_threads=1;
         if (g_Debug != 0 ) printf("pg_db->p_kb_buff %p \n",pg_db->p_kb_buff);
         RbuffInitialize ( &(pg_db->p_kb_buff), &result, sizeof(char),8192, 32); 
         if( result != 0)
         {
             printf("Failed to allocate kb_buff  %d \n",result);
             cleanup( pg_db);
             return -1;
         }
         if (g_Debug != 0 ) RBuffPrintMembers(pg_db->p_kb_buff, "KB_BUFF");
     }
     else if ( mode == 2 ) 
     {
         printf(" Launch SSH place a file on r750 \n");
     }
     else if (mode == 3) 
     {
         printf(" running interactive SSH TEST \n");
         n_threads=3;
        
         if (g_Debug != 0 )  
         {       
             printf("pg_db->p_kb_buff %p \n",pg_db->p_kb_buff);
             printf("pg_db->p_tx_io_buff   %p \n",pg_db->p_tx_io_buff);
             printf("pg_db->p_rx_io_buff   %p \n",pg_db->p_rx_io_buff);
         }

         RbuffInitialize ( &(pg_db->p_kb_buff), &result, sizeof(char),8192, 32); 
         if( result != 0)
         {
             printf("Failed to allocate kb_buff  %d \n",result);
             cleanup( pg_db);
             return -1;
         }

         RbuffInitialize ( &(pg_db->p_tx_io_buff), &result, sizeof(char),8192, 32);
         if( result != 0)
         {
             printf("Failed to allocate Tx Buffer  %d \n",result);
             cleanup(pg_db);
             return -2;
         }

         RbuffInitialize ( &(pg_db->p_rx_io_buff), &result, sizeof(char),8192, 32); 
         if( result != 0)
         {
             printf("Failed to allocate Rx Buffer  %d \n",result);
             cleanup(pg_db);
             return -3;
         }
      
         if (g_Debug != 0 ) 
         {
             RBuffPrintMembers(pg_db->p_kb_buff, "KB_BUFF");
             RBuffPrintMembers(pg_db->p_tx_io_buff  , "TX_BUFF");
             RBuffPrintMembers(pg_db->p_rx_io_buff, "RX_BUFF");
         }
     }
//  Launch Tx and Rx Threads


     i = 0;
     while(i < n_threads) 
      {
         err = pthread_create(&(g_tid[i]), NULL, &WorkerThread, &g_db);
          if (err != 0)
              printf("\ncan't create thread :[%s]", strerror(err));
          else
              printf("\n Thread created successfully\n");
          i++;
      }
 
     usleep(100000);  // for now sleep till threads run.  future, rework and      have thread signal it is running!

    session=fs_ssh_init(g_dst_ip,g_port,g_username,g_password,g_verbose);
    if ( mode == 1) 
    {

//  test ftty.c   
// only need the KB thread
        int done=0;
        printf("ctrl-C to exit \n\r");
        while( done == 0)
        {
            char c;
            char *pc = &c;
            int r;
            r = RBuffFetch( pg_db->p_kb_buff,(void **) &pc);
            if ( r == 0 )
            {  
                if( *pc < 0x20)
                    if ( *pc == 0x0d)  printf("\n\r");
                    else if ( *pc == 0x0a)  printf("\r");
                    else
                        printf("<%02x>",*pc);
                else
                    printf("%c",*pc);
//                printf("in:  %c %x \n",*pc,*pc);
                if ( *pc == 0x03) done =1;
            }
         }
    }
    else if ( mode == 2)
    // no threads needed.  
    // 
        show_remote_processes(session);
    
    else if ( mode == 3 )
    {
        // interactive ssh session
        // need all 3 threads  
        int result; 
        ssh_channel channel;
        channel=ssh_open_channel(session);

        result=interactive_shell_session( channel);

        if (result != 0 ) printf("result = %d\n",result); 
        ssh_close_channel( channel);
    }

    else
       printf ("unrecognized mode\n");
    
    // flag threads to close
    pg_db->done = 1;
    sleep (2);


    fs_ssh_distroy(session );

    cleanup(pg_db);
    restore_tty();    
    printf("\ndone\n");
   
    return 0;

}

#endif
