#ifndef _SRECORD_H_
#define _SRECORD_H_
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <mysql.h>
#include <pthread.h>
#include <debugp.h>
#include <math.h>


#define BUFFER_SIZE 4096

char *input_file  = NULL;
char *output_file = NULL;

struct string_event {
	float delay;
	unsigned length;
	char *string;
};

float get_time_offset(void);
int play_tape( FILE *tape );
int record( FILE *tape );
void parse_args( int argc, char *argv[] );
void sig_handler( int signo );
int add_to_tape_buffer( void *in, int size );
int flush_tape_buffer( FILE *tape );



#endif
