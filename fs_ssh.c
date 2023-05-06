#include <stdint.h>
#include <stdio.h>
#include <libssh/libssh.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>          // for thread Items
#include "frbuff.h"
#include "ftty.h"
#include "fs_common.h"
#include "fs_ssh.h"

// pthread_t g_tid[3];             // posix thread array

// fs_ssh_data * gp_db;

//frbuff * g_p_kb_buff;

kb_data * gpkb;

#define STANDALONE_SSH




//#define WAI() printf("%d %s-%s\n",__LINE__,__FILE__,__FUNCTION__)
#define WAI()


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

int fs_ssh_start_interactive_shell_session( fs_ssh_data  * p)
{
  int rc;
         
  WAI();
  /* request a tty console on the far end */ 
  rc = ssh_channel_request_pty(p->channel);
  if (rc != SSH_OK) return rc;

  /* set the size of the window on the far side */
  rc = ssh_channel_change_pty_size(p->channel, 80, 24);
  if (rc != SSH_OK) return rc;

  /* run a shell ontop of the TTY console */
  rc = ssh_channel_request_shell(p->channel);
  if (rc != SSH_OK) return rc;
   p->interactive = 1;
  return 0;
}

int fs_ssh_end_interactive_shell_session( fs_ssh_data  * p)
{
  p->interactive = 0;
  return 0;
}


/* A very simple terminal emulator:
   - print data received from the remote computer
   - send keyboard input to the remote computer
*/
int interactive_shell_session( fs_ssh_data  * p)
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
  rc = ssh_channel_request_pty(p->channel);
  if (rc != SSH_OK) return rc;

  /* set the size of the window on the far side */
  rc = ssh_channel_change_pty_size(p->channel, 80, 24);
  if (rc != SSH_OK) return rc;

  /* run a shell ontop of the TTY console */
  rc = ssh_channel_request_shell(p->channel);
  if (rc != SSH_OK) return rc;


  printf("ctrl-C to exit \n\r");

  while (ssh_channel_is_open(p->channel) &&
         !ssh_channel_is_eof(p->channel) && (done== 0))
  {
    // read from SSH 
    nbytes = ssh_channel_read_nonblocking(p->channel, buffer, sizeof(buffer), 0);
    if (nbytes < 0) return SSH_ERROR;
    if (nbytes > 0)
    {
      // write to sdtout  ( 1 is stdout ,  0 is stdin )
      nwritten = write(1, buffer, nbytes);
      if (nwritten != nbytes) return SSH_ERROR;
    }
    // Fetch input keyboard
    r = ftty_kb_fetch( gpkb, &c );
    if ( r == 0 )
    {  
//      printf("in:  %c %x \n",*pc,*pc);
        // write out ssh
        nwritten = ssh_channel_write(p->channel, &c , 1);
       
        if (nwritten != 1) return SSH_ERROR;
        if ( *pc == 0x03) done =1;
    }
  }

  return rc;
}


// returns true (!=0) if the channel is open and EOF has not been received 
int fs_ssh_check_channel( fs_ssh_data * p)
{
    return (ssh_channel_is_open(p->channel) && !ssh_channel_is_eof(p->channel));
}

int fs_ssh_get_c(fs_ssh_data * p, char * c)
{
  char buffer[64];
  int nbytes;
    // I only want 1 byte  :-(
    nbytes = ssh_channel_read_nonblocking(p->channel, buffer, 1 /*sizeof(buffer)*/, 0);
     if (nbytes < 0) return SSH_ERROR;
     if (nbytes > 0)
     {
        *c = buffer[0];
         return 0;  // one character received
     }
     return 1;  // no data present 
}
int fs_ssh_put_c(fs_ssh_data * p, char c)
{
   int result;

   result = ssh_channel_write(p->channel, &c , 1);
   if( result != 1)  
        printf("error writing ssh channel %d\n",result);

   return result; 
}


int fs_ssh_open_channel(fs_ssh_data * p)
{
  int rc;
 
  WAI();
  p->channel = ssh_channel_new(p->session);
  if (p->channel == NULL)
    return -1;
 
  rc = ssh_channel_open_session(p->channel);
  if (rc != SSH_OK)
  {
    printf("[%d] Failed to open a channel on this session %d\n",__LINE__,rc);
    ssh_channel_free(p->channel);
    return -2;
  }
 
//  ssh_channel_send_eof(p->channel);
//  ssh_channel_close(p->channel);
//  ssh_channel_free(p->channel);
 
  return 0;
}


int  ssh_close_channel( fs_ssh_data * p )
{
  WAI();
  ssh_channel_send_eof(p->channel);
  ssh_channel_close( p->channel);
  ssh_channel_free( p->channel);
  p->channel = NULL;
  return 0;
}






// ssh_session is a struct ssh_session_struct *   aka it is a pointer !!

// create a terninal  /dev/ptmx


int show_remote_processes( fs_ssh_data * p )
{
  int rc;
  char buffer[256];
  int nbytes;

  char cmd[256];
  WAI();
 
  p->channel = ssh_channel_new(p->session);
  if (p->channel == NULL)
    return SSH_ERROR;
 
  rc = ssh_channel_open_session(p->channel);
  if (rc != SSH_OK)
  {
    ssh_channel_free(p->channel);
    return rc;
  }
 
//  strcpy(cmd,"ps aux");
//  strcpy(cmd,"ls -l /sys/class/net ");
  strcpy(cmd,"echo  1 > /tmp/BR549 ");
  printf (" %s %s %s\n",GREEN,cmd,NC);
  rc = ssh_channel_request_exec(p->channel, cmd);
  if (rc != SSH_OK)
  {
    ssh_channel_close(p->channel);
    ssh_channel_free(p->channel);
    return rc;
  }
 
  nbytes = ssh_channel_read(p->channel, buffer, sizeof(buffer), 0);
  while (nbytes > 0)
  {
    if (write(1, buffer, nbytes) != (unsigned int) nbytes)
    {
      ssh_channel_close(p->channel);
      ssh_channel_free(p->channel);
      return SSH_ERROR;
    }
    nbytes = ssh_channel_read(p->channel, buffer, sizeof(buffer), 0);
  }
 
  if (nbytes < 0)
  {
    ssh_channel_close(p->channel);
    ssh_channel_free(p->channel);
    p->channel = NULL;
    return SSH_ERROR;
  }
 
  ssh_channel_send_eof(p->channel);
  ssh_channel_close(p->channel);
  ssh_channel_free(p->channel);
  p->channel = NULL;
  return SSH_OK;
}

/*****
 *  create storage for an SSH SESSSION 
 *    need to expand to allow multiple connections on a single session.
 */
fs_ssh_data * fs_ssh_create(char * name )
{
    fs_ssh_data * p;
    p=(fs_ssh_data *) malloc(sizeof(fs_ssh_data));
    if ( p == NULL)
    {
        printf("%d:%s-%s  unable to malloc fs_ssh_data\n",__LINE__,__FILE__,__FUNCTION__);  
        return NULL;
    }

    memset( p,0,sizeof(fs_ssh_data));
    // could set up some default here, especially if unitialization causes 
    //             a seg fault;
    if ( name != NULL) 
    {
        strcpy(p->name,name); 
    }

    return p;
}

// for below, consider if session is started, that you write directly to libssh
void fs_ssh_set_user( fs_ssh_data * p, char * user )        {strcpy(p->user,user);}
void fs_ssh_set_password( fs_ssh_data * p, char * pass )    {strcpy(p->password,pass);}
void fs_ssh_set_dest_ip( fs_ssh_data * p, char* dst_ip)     {strcpy(p->dst_ip,dst_ip);}
void fs_ssh_set_verbosity( fs_ssh_data * p, int verbosity ) {p->verbosity = verbosity;}
void fs_ssh_set_port( fs_ssh_data * p, int port )           {p->port = port ;}




// I am going to do it all here.
//   consider breaking this up in the future.
int fs_ssh_connect( fs_ssh_data * p )
{
int err;
int result; 

    printf(" running interactive SSH TEST \n");

    RbuffInitialize ( &(p->tx), &result, sizeof(char),8192, 32);
    if( result != 0)
    {
        printf("Failed to allocate Tx Buffer  %d \n",result);
//        cleanup(pg_db);
        return -2;
    }

    RbuffInitialize ( &(p->rx), &result, sizeof(char),8192, 32); 
    if( result != 0)
    {
        printf("Failed to allocate Rx Buffer  %d \n",result);
//        cleanup(pg_db);
        return -3;
    }
    
    DEBUG( DEBUG_CFG_DATA_SSH ) 
    {
       fs_ssh_print_ssh_data(p);
    }
     
//  Launch Tx and Rx Threads

#if 0
     err = pthread_create(&(p->tid_rx), NULL, &ssh_rx, &gp_db);
     if (err != 0)
         printf("\ncan't create thread :[%s]", strerror(err));
     else
         printf("\n Thread created successfully\n");

     err = pthread_create(&(p->tid_tx), NULL, &ssh_tx, &gp_db);
     if (err != 0)
         printf("\ncan't create thread :[%s]", strerror(err));
     else
         printf("\n Thread created successfully\n");

    usleep(100000);  // for now sleep till threads run.  future, rework and      have thread signal it is running!
#endif
    err=fs_ssh_init_session(p);
    if( err < 0)
    {
       printf("unable to create a ssh session %d \n",err);
       return err;
    }
    // need 2 threads  
    {
        int result; 
        err=fs_ssh_open_channel(p);

        result=interactive_shell_session( p);

        if (result != 0 ) printf("result = %d\n",result); 
        ssh_close_channel( p );
    }

    // flag threads to close
    p->done = 1;
    sleep (2);

    fs_ssh_distroy_session(p );

    printf("\ndone\n");
   
    return 0;

}


void fs_ssh_close_channel( fs_ssh_data * p )
{
  if ( p == NULL)  return;

  p->interactive = 0;
  if( p->channel != NULL)
  {
      ssh_channel_send_eof(p->channel);
      ssh_channel_close(p->channel);
      ssh_channel_free(p->channel);
      p->channel = NULL ;
  }
   return ;
}

void fs_ssh_distroy_session(fs_ssh_data * p )
{
   if ( p != NULL)
   {
       fs_ssh_close_channel(p);

       if( p->session != NULL)
       {
           ssh_disconnect( p->session);
           ssh_free ( p->session);
           p->session = NULL ;
       }

       if( p->rx != NULL) { free (p->rx); p->rx=NULL;}
       if( p->tx != NULL) { free (p->tx); p->tx=NULL;}
    }
}



void fs_ssh_print_ssh_data(fs_ssh_data *p)
{
     printf(" name :       %s\n",p->name);
     printf(" dst_ip :     %s\n",p->dst_ip);
     printf(" port :       %d\n",p->port);
     printf(" user :       %s\n",p->user);
     printf(" password:    %s\n",p->password);
     printf(" verbosity:     %d\n",p->verbosity);
     printf(" session:     %p\n",p->session);
     printf(" channel:     %p\n",p->channel);
     printf(" interactive: %d\n",p->interactive);
     printf(" tx:          %p\n",p->tx);
     printf(" rx:          %p\n",p->rx);
     printf(" done:        %d\n",p->done);

}

 
int  fs_ssh_init_session(fs_ssh_data *  p)
{
    int rc;

    WAI();
    //d->verbosity =SSH_LOG_FUNCTIONS;
    //d->verbosity=SSH_LOG_WARNING;
    //d->verbosity=SSH_LOG_PROTOCOL;
    //d->verbosity=SSH_LOG_PACKET;
    p->verbosity=SSH_LOG_NOLOG;

    p->session = ssh_new();
    if (p->session == NULL)
    {
        return(-1);
    }

    ssh_options_set(p->session, SSH_OPTIONS_HOST, p->dst_ip );
    ssh_options_set(p->session, SSH_OPTIONS_PORT, &(p->port));
    ssh_options_set(p->session, SSH_OPTIONS_USER, p->user );
    ssh_options_set(p->session, SSH_OPTIONS_LOG_VERBOSITY, &(p->verbosity));

    rc = ssh_connect(p->session);
    if(rc != SSH_OK){
        fprintf(stderr, "Error connecting to localhost: %s\n", ssh_get_error( p->session) );
        return (-2);
    } 

// Verify the servers Identitiy
//  TBD

// Authenticate ourselves

    ssh_userauth_password(p->session, NULL, p->password);

    return 0;

}



# ifdef STANDALONE_SSH
char RED[]     = { 0x1b,'[','1',';','3','1','m',0x00 };
char GREEN[]   = { 0x1b,'[','1',';','3','2','m',0x00 };
char YELLOW[]  = { 0x1b,'[','1',';','3','3','m',0x00 };
char BLUE[]    = { 0x1b,'[','1',';','3','4','m',0x00 };
char MAGENTA[] = { 0x1b,'[','1',';','3','5','m',0x00 };
char CYAN[]    = { 0x1b,'[','1',';','3','6','m',0x00 };
char NC[]      = { 0x1b,'[','0','m',0x00 };


fs_ssh_data * gp_ssh;

uint64_t g_Debug;
uint64_t  g_error = 0;
// need to use the escape key to send special charactes
// this is because getchar does not return Ctrl Sequences
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void  cleanup(fs_ssh_data  *p)
{
    fs_ssh_distroy_session(p);

}


int main(int argc, char ** argv)
{
    int mode  ;
    int err;

    mode=3;
    if ( argc >= 2) mode=atoi(argv[1]);

    gpkb = ftty_kb_create();
    ftty_kb_start( gpkb );

    // Initialize Default Values & Data Structures
    gp_ssh = fs_ssh_create("10.75.47.44 " );

    fs_ssh_set_user( gp_ssh     , "fsmith" ); 
    fs_ssh_set_password( gp_ssh , "fsmith" );    
    fs_ssh_set_dest_ip( gp_ssh  , "10.75.47.44" );       
    fs_ssh_set_verbosity( gp_ssh, 0 ); 
    fs_ssh_set_port( gp_ssh     , 22 );             



    err=fs_ssh_init_session(gp_ssh);
    if ( err != 0)
    {
       printf(" Huston we have a problem\n");
    }
    if ( mode == 1) 
    {

//  test ftty.c   
// only need the KB thread
        int done=0;
        printf("ctrl-C to exit \n\r");
        while( done == 0)
        {
            char c;
            int r;
            r =  ftty_kb_fetch( gpkb, &c) ;
            if ( r == 0 )
            {  
                if( c < 0x20)
                    if ( c == 0x0d)  printf("\n\r");
                    else if ( c == 0x0a)  printf("\r");
                    else
                        printf("<%02x>",c);
                else
                    printf("%c",c);
                printf("\nin:  %c %x \n",c,c);
                if ( c == 0x03) done =1;
            }
         }
    }
    else if ( mode == 2)
    //    this opens a connection r750 and executes
    //       echo 1 >/tmp/BR549  
    // 
        show_remote_processes(gp_ssh);
    
    else if ( mode == 3 )
    {
       int done=0;
       char c;
       int  r;
        // interactive ssh session
        err=fs_ssh_open_channel(gp_ssh);
        if ( err  != 0 ) printf("err = %d\n",err); 


#if 1
        err=fs_ssh_start_interactive_shell_session( gp_ssh);
        while (done == 0)
        {
            if ( ! fs_ssh_check_channel(gp_ssh))  done = 1 ;
            r =  fs_ssh_get_c( gp_ssh, &c); 
            if ( r == 0)
            {
               printf("%c",c);    
            }    
            r =  ftty_kb_fetch( gpkb, &c) ;
            if ( r == 0)
            {
                fs_ssh_put_c( gp_ssh,  c );   
            }    
        }

#else
        err=interactive_shell_session( gp_ssh );
#endif
        if ( err  != 0 ) printf("err = %d\n",err); 
        fs_ssh_close_channel( gp_ssh);
    }

    else
       printf ("unrecognized mode\n");
    
    // flag threads to close
    gp_ssh->done = 1;
    sleep (2);
    // need to make shure the threads are closed before calling below

    ftty_kb_stop( gpkb );
    ftty_kb_distroy(  gpkb );

    fs_ssh_distroy_session ( gp_ssh );



    cleanup(gp_ssh);
    restore_tty();    
    printf("\ndone\n");
   
    return 0;

}

#endif
