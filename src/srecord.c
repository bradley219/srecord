#include "srecord.h"

static FILE *mytape;
static char *tape_buffer = NULL;
static int tape_buffer_size = 0;

int main( int argc, char *argv[] )
{
	int retval = 0;

	signal( SIGQUIT, sig_handler );
	signal( SIGUSR1, sig_handler );
	
	struct timeval main_start;
	gettimeofday(&main_start,NULL);

	parse_args( argc, argv );

	if( output_file != NULL )
	{
		mytape = fopen( output_file, "w" );

		while(record(mytape));
		
		fclose(mytape);
	}
	else if( input_file != NULL )
	{
		mytape = fopen( input_file, "r" );

		while(play_tape(mytape));
		
		fclose(mytape);
	}


	return retval;
}
void sig_handler( int signo )
{
	flush_tape_buffer(mytape);
	switch(signo)
	{
		case SIGUSR1:
			return;
		case SIGINT:
			return;
		case SIGQUIT:
			exit(-1);
			break;
	}
	return;
}

int play_tape( FILE *tape )
{
	int retval = 1;

	if(feof(tape))
		return 0;

	float delay;
	int length;
	char *string = malloc( sizeof(char) * ( BUFFER_SIZE + 1 ) );

	if( fread( (void*)&delay, sizeof(float), 1, tape ) != 1 )
		return 0;
	if( fread( (void*)&length, sizeof(int), 1, tape ) != 1 )
		return 0;
	if( fread( (void*)string, sizeof(char), length, tape ) != length )
		return 0;

	string[length] = '\0';

	struct timespec dt;

	dt.tv_sec = (long)delay;
	dt.tv_nsec = (delay - (float)dt.tv_sec) * 1000000000.0;

	debugp( 4, "%13.9f %5d  ", delay, length );
	fflush(stderr);

	nanosleep( &dt, NULL );

	debugp( 0, "%s", string );

	free(string);

	return retval;
}


int record( FILE *tape )
{
	int retval = 1;

	if( feof(tape) )
		return 0;

	fd_set readfds;
	FD_ZERO( &readfds );
	FD_SET( 0, &readfds );

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;

	int highfd = 0;
	highfd = 0 + 1;
	int ready = select( highfd, &readfds, NULL, NULL, &timeout );

	debugp( 5, "select() returned %d\n", ready );


	if( (ready==0) || (tape_buffer_size > 5000) )
	{
		flush_tape_buffer(tape);
	}

	if( ready && FD_ISSET( 0, &readfds ) )
	{
		size_t nbytes;
		char *buffer = malloc( sizeof(char) * ( BUFFER_SIZE + 1 ) );
		
		nbytes = read( 0, buffer, sizeof(char) * BUFFER_SIZE );
		buffer = realloc( buffer, sizeof(char) * (nbytes + 1) );
		buffer[nbytes] = '\0';


		float offset = get_time_offset();
		debugp( 4, "buffer=%8d read %4d bytes; offset=%20.6f\n", tape_buffer_size, nbytes, offset );
		
		printf( "%s", buffer );

		int length = strlen(buffer);

		add_to_tape_buffer( &offset, sizeof(float) );
		add_to_tape_buffer( &length, sizeof(int) );
		add_to_tape_buffer( buffer, sizeof(char) * length );

		free(buffer);
	}

	return retval;
}
int add_to_tape_buffer( void *in, int size )
{
	int start_size = tape_buffer_size;
	tape_buffer_size += size;
	tape_buffer = realloc( tape_buffer, sizeof(char) * tape_buffer_size );

	memcpy( tape_buffer + start_size, in, size );

	return 1;
}
int flush_tape_buffer( FILE *tape )
{
	int nbytes = 0;

	if( tape_buffer_size )
	{
		nbytes = fwrite( tape_buffer, sizeof(char), tape_buffer_size, tape );
		debugp( 4, "flushed %d bytes to file\n", nbytes );
		free( tape_buffer );
		tape_buffer = NULL;
		tape_buffer_size = 0;
		fflush(tape);
	}
	return nbytes;
}

float get_time_offset(void)
{
	static struct timeval start_time = { 0, 0 };
	float diff = 0;

	if( start_time.tv_sec == 0 && start_time.tv_usec == 0 )
	{
		gettimeofday( &start_time, NULL );
	}
	else
	{
		struct timeval now;
		gettimeofday( &now, NULL );

		double dstart = (double)start_time.tv_sec + (double)start_time.tv_usec/1000000.0;
		double dnow   = (double)now.tv_sec + (double)now.tv_usec/1000000.0;

		diff = dnow - dstart;
		
		gettimeofday( &start_time, NULL );
	}

	return diff;
}



void parse_args( int argc, char *argv[] )
{
	struct option long_options[] =
	{
		{ "syslog", optional_argument, NULL, 0 },
		{ "verbose", optional_argument, NULL, 'v' },
		{ 0, 0, 0, 0 }
	};
	int long_options_index;

	int c;
	while( ( c = getopt_long( argc, argv, "i:o:v", long_options, &long_options_index )) != -1 ) 
	{
		switch(c) {
			case 0: /* Long options with no short equivalent */
				if( strcmp( long_options[long_options_index].name, "syslog" ) == 0 ) {
					debugp( 0, "Changing debug facility to syslog... goodbye!\n" );
					setup_debugp_syslog( "srecord" );
					change_debug_facility( DEBUGP_SYSLOG );
					debugp( 4, "changed debug facility to syslog\n" );
				}
			case 'i':
				input_file = optarg;
				break;
			case 'o':
				output_file = optarg;
				break;
			case 'v':
				change_debug_level_by(1);
				debugp( 1, "Verbosity increased to %d\n", get_debug_level() );
				break;
			default:
				break;
		}
	}
	return;
}
