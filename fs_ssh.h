#ifndef  _fs_ssh_h
#define  _fs_ssh_h


// you will need to have

//   #include <libssh/libssh.h>
// somewhere before you include this file


typedef struct connect_info_struct {
      char        name[128];
      char        dst_ip[32];
      int         port;
      char        password[128];
      char        user[128];
      int         verbosity;
      int         interactive;  // if interactive code has been called
      ssh_session session;      // these are pointers to data structures from libssh.so
      ssh_channel channel;      // these are pointers to data structures from libssh.so
      frbuff *    tx; 
      frbuff *    rx;
      pthread_t   tid_rx;
      pthread_t   tid_tx;
      int         done ;      // signal thread to end   
} fs_ssh_data ;




// if you error, you can figure out which connection base on "name" 
fs_ssh_data * fs_ssh_create(char * name ) ;

void fs_ssh_set_user( fs_ssh_data * p, char * user );        
void fs_ssh_set_password( fs_ssh_data * p, char * pass );    
void fs_ssh_set_dest_ip( fs_ssh_data * p, char* dst_ip );    
void fs_ssh_set_verbosity( fs_ssh_data * p, int verbosity ); 
void fs_ssh_set_port( fs_ssh_data * p, int port );           

int  fs_ssh_init_session( fs_ssh_data *  p);

int  fs_ssh_strt_session( fs_ssh_data * p );

int fs_ssh_open_channel(fs_ssh_data * p);

//int  interactive_shell_session( fs_ssh_data  * p);

int fs_ssh_start_interactive_shell_session( fs_ssh_data  * p);

int fs_ssh_check_channel( fs_ssh_data * p);
int fs_ssh_get_c(fs_ssh_data * p, char * c);
int fs_ssh_put_c(fs_ssh_data * p, char c);

int fs_ssh_end_interactive_shell_session( fs_ssh_data  * p);

///int  fs_ssh_open_channel_and_connect( fs_ssh_data * p );

void fs_ssh_close_channel( fs_ssh_data * p );

void fs_ssh_distroy_session( fs_ssh_data * p );

// otehr functions

void  fs_ssh_print_ssh_data( fs_ssh_data * p ); 
frbuff * fs_ssh_get_tx_ring( fs_ssh_data * p );
frbuff * fs_ssh_get_rx_ring( fs_ssh_data * p );


#endif
