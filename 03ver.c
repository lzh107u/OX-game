/*
	log file:
	20201120-1214	get_content()		not finish, editing pos_inside_bar ...
	20201121-0015	login_handler()		finish the decision between 1118_login.html and 1118_game.html
	20201123-2322	main()			list_pf[ ... ] should be stored in main and copied to each child process
	20201126-0340	game part		myturn and playground

*/
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<time.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<fcntl.h>
#include<ctype.h>


#define BUFSIZE 8192
#define NUMUSER 10	//	NUMber of ProFile

typedef struct profile
{
	char* account;
	char* password;
	int online_status;
	int invite;
	int acknowledge;
	int gaming;
	int myturn;
	int opponent;
	int round;
	int winner;
	int map[ 9 ];
} profile;

profile list_pf[ NUMUSER ];
int recent_user;
char* user_status = NULL;
int fd_pipe[ 2 ];
int* fd_w;
int* fd_r;

int letter_insert( int count, char* buffer, char letter )
{
	buffer[ count ] = letter;
	count++;
	return count;
}

int letter_delete( int count, char* buffer )
{
	count--;
	buffer[ count ] = '\0';
	return count;
}

int JSONstring( int count, char* buffer, char* line )
{
	count = letter_insert( count, buffer, 34 );
	for( int i = 0; i < strlen( line ); i++ )
	{
		count = letter_insert( count, buffer, line[ i ] );
	}
	count = letter_insert( count, buffer, 34 );
	return count;
}

void ret_handler( int connfd, char* filename )
{
	char* buffer = ( char* )malloc( sizeof( char ) * ( BUFSIZE + 1 ) );
	char* line = ( char* )malloc( sizeof( char ) * 64 );
	memset( buffer, '\0', BUFSIZE + 1 );
	memset( line, '\0', 64 );
	
	int i, ret;
	char acnt[] = "acnt";
	char onsta[] = "onsta";
	char inv[] = "inv";
	char ack[] = "ack";
	char conn[] = "conn";
	char board[] = "board-";
	char change[] = "change";
	char* pos;
	int num_acnt = 0;
	int count = 0;
	int count_pipe = 0;
	/*	put { at the begining of JSON string	*/
	count = letter_insert( count, buffer, '{' );
	/*	online status of all players	*/
	if( strncmp( filename, "ret_online_status", 17 ) == 0 )
	{
		strcpy( line, onsta );
		count = JSONstring( count, buffer, line );
		memset( line, '\0', 64 );
		
		count = letter_insert( count, buffer, ':' );
		count = letter_insert( count, buffer, '[' );
		/*	(status),	*/
		for( i = 0; i < NUMUSER - 1; i++ )
		{
			count = letter_insert( count, buffer, list_pf[ i ].online_status + 48 );
			count = letter_insert( count, buffer, ',' );
		}
		/*	last status	*/
		count = letter_insert( count, buffer, list_pf[ i ].online_status + 48 );
		count = letter_insert( count, buffer, ']' );
	}
	else if( strncmp( filename, "ret_user", 8 ) == 0 )
	{
		strcpy( line, acnt );
		count = JSONstring( count, buffer, line );
		memset( line, '\0', 64 );
		count = letter_insert( count, buffer, ':' );
		strcpy( line, list_pf[ recent_user ].account );
		count = JSONstring( count, buffer, line );
	}
	else if( strncmp( filename, "ret_inv", 7 ) == 0 )
	{
		printf("inv handler !!\n");
		pos = strstr( filename, inv );
		count_pipe = 0;
		while( *pos != '\0' )
		{
			count_pipe = letter_insert( count_pipe, user_status, *pos );
			pos++;
		}
		ret = write( *fd_w, user_status, 1024 );
	}
	else if( strncmp( filename, "ret_ack", 7 ) == 0 )
	{
		printf("ack handler !!\n");
		pos = strstr( filename, ack );
		count_pipe = 0;
		while( *pos != '\0' )
		{
			count_pipe = letter_insert( count_pipe, user_status, *pos );
			pos++;
		}
		ret = write( *fd_w, user_status, 1024 );
	}
	else if( strncmp( filename, "ret_conn", 8 ) == 0 )
	{
		pos = strstr( filename, conn );
		pos = pos + strlen( conn ) + 1;
		num_acnt = num_acnt + ( ( *pos ) - 48 )*10;
		pos++;
		num_acnt = num_acnt + ( ( *pos ) - 48 ) - 1;

		strcpy( line, "gaming" );
		count = JSONstring( count, buffer, line );
		memset( line, '\0', 64 );
		count = letter_insert( count, buffer, ':' );
		count = letter_insert( count, buffer, ( list_pf[ num_acnt ].gaming + 48 ) );
		count = letter_insert( count, buffer, ',' );

		strcpy( line, "myturn" );
		count = JSONstring( count, buffer, line );
		memset( line, '\0', 64 );
		count = letter_insert( count, buffer, ':' );
		count = letter_insert( count, buffer, ( list_pf[ num_acnt ].myturn + 48 ) );
		count = letter_insert( count, buffer, ',' );

		strcpy( line, "opponent" );
		count = JSONstring( count, buffer, line );
		memset( line, '\0', 64 );
		count = letter_insert( count, buffer, ':' );

		if( list_pf[ num_acnt ].opponent == 9 )
		{
			count = letter_insert( count, buffer, '1' );
			count = letter_insert( count, buffer, '0' );
		}
		else
		{
			count = letter_insert( count, buffer, ( list_pf[ num_acnt ].opponent + 49 ) );
		}
	}
	else if( strncmp( filename, "ret_board", 9 ) == 0 )
	{
		pos = strstr( filename, board );
		pos = pos + strlen( board ) + 1;
		num_acnt = 0;
		num_acnt = num_acnt + ( ( *pos ) - 48 )*10;
		pos++;
		num_acnt = num_acnt + ( *pos ) - 49;
		
		strcpy( line, "status" );
		count = JSONstring( count, buffer, line );
		count = letter_insert( count, buffer, ':' );
		count = letter_insert( count, buffer, '[' );
		for( int i = 0; i < 8; i++ )
		{
			count = letter_insert( count, buffer, list_pf[ num_acnt ].map[ i ] + 48 );
			count = letter_insert( count, buffer, ',' );
		}
		count = letter_insert( count, buffer, list_pf[ num_acnt ].map[ 8 ] + 48 );
		count = letter_insert( count, buffer, ']' );
	}
	else if( strncmp( filename, "ret_change", 10 ) == 0 )
	{
		pos = strstr( filename, change );
		pos = pos + strlen( change ) + 1;
		num_acnt = 0;
		num_acnt = num_acnt + ( ( *pos ) - 48 )*10;
		pos++;
		num_acnt = num_acnt + ( *pos ) - 49;

		pos = pos + 2;

		if( list_pf[ num_acnt ].myturn == 1 && list_pf[ num_acnt ].map[ ( *pos )- 48 ] == 0 )
		{
			ret = sprintf( user_status, "board_change-%d-%d", num_acnt,( *pos ) -48 );
			ret = write( *fd_w, user_status, 1024 );
		}
	}
	count = letter_insert( count, buffer, '}' );
	write( connfd, buffer, strlen( buffer ) );

	free( line );
	free( buffer );
	return ;
}

void page_request( int connfd, char* filename )
{
	int local_fd = open( filename, O_RDONLY );
	if( local_fd < 0 )
	{
		printf("page_request, open(): ");
		printf("\n");
		return ;
	}

	int ret;
	char* buffer = ( char* )malloc( sizeof( char ) * ( BUFSIZE + 1 ) );
	memset( buffer, '\0', 	BUFSIZE + 1 );
	while( ( ret = read( local_fd, buffer, BUFSIZE ) ) > 0 )
	{
		write( connfd, buffer, BUFSIZE );
		memset( buffer, '\0', BUFSIZE + 1 );
	}

	free( buffer );
	close( local_fd );
	return ;
}

char* get_filename( char* buffer, char* method )
{
	char* filename = ( char* )malloc( sizeof( char ) * BUFSIZE + 1 );
	memset( filename, '\0', BUFSIZE );
	
	//	file name format:	(method) /(filename) /HTTP1.1

	char root[] = "GET / HTTP/1.1";
	char login_page[] = "1118_login.html";
	int len_root = strlen( root );
	int count = 0;
	char* pos_cur = buffer;
	char* pos_end = strstr( buffer, " HTTP/" );
	pos_cur = pos_cur + strlen( method );

	while( pos_cur != pos_end )
	{
		count = letter_insert( count, filename, *pos_cur );
		pos_cur++;
	}
	if( count == 0 )
	{
		strcpy( filename, login_page );
	}
	return filename;
}

/*	判定登錄結果	*/
void login_judgement( char* content, int connfd )
{
	char* pos_acnt = NULL;
	char* pos_pswd = NULL;
	char page_login[] = "1118_login.html";
	char page_lobby[] = "1123_lobby.html";
	int flag_login = 0;

	char* account = ( char* )malloc( sizeof( char ) * 8 );
	char* password = ( char* )malloc( sizeof( char ) * 8 );
	char* filename = ( char* )malloc( sizeof( char ) * 64 );
	memset( account, '\0', 8 );
	memset( password, '\0', 8 );
	memset( filename, '\0', 64 );

	pos_acnt = strstr( content, "account" );
	pos_pswd = strstr( content, "password" );

	/* --------------- parse account and password from content --------------- */
	/*	find position of account and password	*/
	if( pos_acnt == NULL )
	{
		perror("login_handler, strstr( account ), error: ");
		printf("\n");
		return ;
	}
	else if( pos_pswd == NULL )
	{
		perror("login_handler, strstr( password ), error: ");
		printf("\n");
		return ;
	}

	pos_acnt = pos_acnt + 9;
	pos_pswd = pos_pswd + 10;

	/*	ignores space before required parts	*/
	while( isspace( *pos_acnt ) != 0 && *pos_acnt != 0 )
		pos_acnt++;
	while( isspace( *pos_pswd ) != 0 && *pos_pswd != 0 )
		pos_pswd++;

	int count = 0;
	int ret;
	/*	reads required parts	*/
	while( isspace( *pos_acnt ) == 0 && *pos_acnt != 0 )
	{
		count = letter_insert( count, account, *pos_acnt );
		pos_acnt++;
	}
	count = 0;
	while( isspace( *pos_pswd ) == 0 && *pos_pswd != 0 )
	{
		count = letter_insert( count, password, *pos_pswd );
		pos_pswd++;
	}

	/*	shows required parts	*/
	printf("account: %s, password: %s\n", account, password );
	
	/*	compare account and password	*/
	for( int i = 0; i < NUMUSER; i++ )
	{
		if( strcmp( list_pf[ i ].account, account ) == 0 && strcmp( list_pf[ i ].password, password ) == 0 )
		{
			flag_login = 1;
			list_pf[ i ].online_status = 1;

			ret = sprintf( user_status, "login_user:%d", i );
			ret = write( *fd_w, user_status, 1024 );

			break;
		}
	}

	/*	load lobby page if pass; load login page again if block		*/
	if( flag_login == 1 )
	{
		strcpy( filename, page_lobby );
	}
	else if( flag_login == 0 )
	{
		strcpy( filename, page_login );
	}
	page_request( connfd, filename );
	return ;
}

char* post_content( char* buffer )
{
	char* content = ( char* )malloc( sizeof( char ) * BUFSIZE + 1 );
	char* barrier = ( char* )malloc( sizeof( char ) * 64 );
	char needle[] = "boundary=";
	char* pos_end;
	char* pos_cur;
	char* pos_start;
	char* pos_inside_bar;

	int len = strlen( needle );
	int count = 0;

	pos_start = strstr( buffer, needle );
	pos_start = pos_start + len;
	pos_end = strstr( pos_start, "\r" );
	pos_cur = pos_start;
	
	memset( barrier, '\0', 64 );
	count = letter_insert( count, barrier, '-' );
	count = letter_insert( count, barrier, '-' );
	while( pos_cur != pos_end )
	{
		count = letter_insert( count, barrier, *pos_cur );
		pos_cur++;
	}

	pos_start = strstr( buffer, barrier );  // barrier: ------barrierXXXXXXXXXXXXXXXXXXXXXX
	if( pos_start == NULL )
	{
		printf("post_content, pos_start failure ... \n");
		return NULL;
	}
	
	count = letter_insert( count, barrier, '-' );
	count = letter_insert( count, barrier, '-' );
	pos_end = strstr( buffer, barrier );	// barrier: ------barrierXXXXXXXXXXXXXXXXXXXXXX--
	if( pos_end == NULL )
	{
		printf("pos_content, pos_end failure ...\n");
		return NULL;
	}
	
	count = letter_delete( count, barrier );
	count = letter_delete( count, barrier );// barrier: ------barrierXXXXXXXXXXXXXXXXXXXXXX
	
	pos_cur = pos_start;
	count = 0;
	pos_inside_bar = strstr( pos_start, barrier );
	while( pos_cur != pos_end )
	{
		if( pos_inside_bar != NULL && pos_cur == pos_inside_bar )
		{
			pos_cur = pos_cur + strlen( barrier );

			while( isspace( *pos_cur ) == 1 )
			{
				pos_cur++;
			}

			pos_inside_bar = pos_inside_bar + strlen( barrier );
			pos_inside_bar = strstr( pos_inside_bar, barrier );
		}
		count = letter_insert( count, content, *pos_cur );
		pos_cur++;
	}
	free( barrier );
	return content;
}

void post_handler( int connfd, char* buffer )
{
	int ret;
	char* content;
	char method[] = "POST /";
	char passwd[] = "password";
	char logout[] = "Logout";
	char page_login[] = "1118_login.html";
	char name[] = "name=";
	char* filename = ( char* )malloc( sizeof( char ) * 64 );
	memset( filename, '\0', 64 );

	char* pos_start;
	int count = 0;
	int num_logout = 0;
	content = post_content( buffer );

	/* -------------------- cases handler -------------------- */
	/*	case: login page	*/
	if( strstr( content, passwd ) != NULL )
	{
		login_judgement( content, connfd );
	}
	/*	case: lobby page -> logout	*/
	else if( strstr( content, logout ) != NULL ) 
	{
		strcpy( filename, page_login );
//		printf("post_handler, logout request, buffer: \n");
//		printf("%s\n\n", buffer );
		page_request( connfd, filename );

		pos_start = strstr( content, name );
		pos_start = pos_start + strlen( name ) + 2;
		num_logout = num_logout + 10 * ( ( *pos_start ) - 48 );
		pos_start++;
		num_logout = num_logout + ( ( *pos_start ) - 48 );
		ret = sprintf( user_status, "logout_user:%d", num_logout - 1 );

		ret = write( *fd_w, user_status, 1024 );
	}

	if( content != NULL )
	{
		free( content );
	}
	return ;
}

void get_handler( int connfd, char* buffer )
{
	char* filename;
	char method[] = "GET /";
	filename = get_filename( buffer, method );

	if( strncmp( filename, "ret", 3 ) == 0 )
	{
//		printf("%s\n\n----------\n\n", buffer );
		ret_handler( connfd, filename );
	}
	else
	{
		page_request( connfd, filename );
	}
	free( filename );
	return ;
}

void packet_handler( int connfd )
{
	int ret;

	char* buffer = ( char* )malloc( sizeof( char )*( BUFSIZE + 1 ) );
	memset( buffer, '\0', BUFSIZE + 1 );

	ret = read( connfd, buffer, BUFSIZE );

	if( ret <= 0 )
	{
		perror("handler, read(), error:");
		printf("\n");
		exit( 3 );
	}

	if( strncmp( buffer, "GET ", 4 ) == 0 )
	{
		get_handler( connfd, buffer );
	}
	else if( strncmp( buffer, "POST ", 5 ) == 0 )
	{
		post_handler( connfd, buffer );
	}

	free( buffer );
	return ;
}

void reset_pf( int ID )
{
	list_pf[ ID ].invite = -1;
	list_pf[ ID ].acknowledge = -1;
	list_pf[ ID ].opponent = 0;
	list_pf[ ID ].gaming = 0;
	list_pf[ ID ].myturn = 0;
	list_pf[ ID ].winner = 0;
	list_pf[ ID ].round = 0;

	for( int i = 0; i < 9; i++ )
		list_pf[ ID ].map[ i ] = 0;
	return ;
}

void game_handler( int dec_maker, int opponent, int num )
{
	list_pf[ dec_maker ].map[ num ] = 1;
	list_pf[ opponent ].map[ num ] = 2;

	list_pf[ dec_maker ].round++;
	list_pf[ opponent ].round++;

	list_pf[ dec_maker ].myturn = 0;
	list_pf[ opponent ].myturn = 1;

	int winner = -1;
	if( list_pf[ dec_maker ].round == 9 )
	{
		/*	game over	*/
		for( int i = 0; i < 3; i++ )
		{
			if( list_pf[ dec_maker ].map[ i ] == list_pf[ dec_maker ].map[ i + 3 ] && list_pf[ dec_maker ].map[ i ] == list_pf[ dec_maker ].map[ i + 6 ] )
			{
				if( list_pf[ dec_maker ].map[ i ] == 1 )
					winner = dec_maker;
				else
					winner = opponent;
				break;
			}
			if(  list_pf[ dec_maker ].map[ 3*i ] == list_pf[ dec_maker ].map[ 3*i + 1 ] && list_pf[ dec_maker ].map[ 3*i ] == list_pf[ dec_maker ].map[ 3*i + 2 ] )
			{
				if( list_pf[ dec_maker ].map[ 3*i ] == 1 )
					winner = dec_maker;
				else
					winner = opponent;
				break;
			}
		}
		if( list_pf[ dec_maker ].map[ 0 ] == list_pf[ dec_maker ].map[ 4 ] && list_pf[ dec_maker ].map[ 0 ] == list_pf[ dec_maker ].map[ 8 ] )
		{
			if( list_pf[ dec_maker ].map[ 4 ] == 1 )
				winner = dec_maker;
			else
				winner = opponent;
		}
		else if( list_pf[ dec_maker ].map[ 2 ] == list_pf[ dec_maker ].map[ 4 ] && list_pf[ dec_maker ].map[ 4 ] == list_pf[ dec_maker ].map[ 6 ] )
		{
			if( list_pf[ dec_maker ].map[ 4 ] == 1 )
				winner = dec_maker;
			else
				winner = opponent;
		}
		list_pf[ dec_maker ].winner = winner;
		list_pf[ opponent ].winner = winner;

		reset_pf( dec_maker );
		reset_pf( opponent );

	}

	return ;
}

void game_start( int ID1, int ID2 )
{
	printf("ID1: %d, ID2: %d\n", ID1, ID2 );

	if( ID1 == ID2 )
	{
		return ;
	}
	list_pf[ ID1 ].round = 0;
	list_pf[ ID2 ].round = 0;

	list_pf[ ID1 ].gaming = 1;
	list_pf[ ID2 ].gaming = 1;

	list_pf[ ID1 ].opponent = ID2;
	list_pf[ ID2 ].opponent = ID1;
	
	list_pf[ ID1 ].myturn = 1;
	list_pf[ ID2 ].myturn = 0;

	printf("%d: myturn:%d\n", ID1, list_pf[ ID1 ].myturn );
	printf("%d: myturn:%d\n", ID2, list_pf[ ID2 ].myturn );
	return ;
}

int main( int argc, char** argv )
{
	int ret;

	int len_inet;
	struct sockaddr_in addr_serv, addr_clnt;
	pid_t pid;

	
	for( int i = 0; i < NUMUSER; i++ )
	{
		list_pf[ i ].account = ( char* )malloc( sizeof( char ) * 8 );
		list_pf[ i ].password = ( char* )malloc( sizeof( char ) * 8 );
		list_pf[ i ].online_status = 0;
		list_pf[ i ].gaming = 0;
		list_pf[ i ].invite = -1;
		list_pf[ i ].acknowledge = -1;
		list_pf[ i ].myturn = 0;
		list_pf[ i ].opponent = 0;

		for( int j = 0; j < 9; j++ )
			list_pf[ i ].map[ j ] = 0;
			
		ret = sprintf( list_pf[ i ].account, "%03d", i + 1 );
		ret = sprintf( list_pf[ i ].password, "a%02d", i + 1 );

		printf("account: %s, password: %s, status: %d\n", list_pf[ i ].account, list_pf[ i ].password, list_pf[ i ].online_status );
	}

	if( chdir("/mnt/d/internet/hw2") == -1 )
	{
		perror("main, chdir(), error: ");
		printf("\n");
		return 0;
	}

	int sockfd = socket( AF_INET, SOCK_STREAM, 0 );

	if( sockfd < 0 )
	{
		perror("main, socket(), error: ");
		printf("\n");
		return 0;
	}
	else
	{
		printf("socket fd: %d\n", sockfd );
	}


	addr_serv.sin_family = AF_INET;
	addr_serv.sin_addr.s_addr = inet_addr( "127.0.0.1" );
	addr_serv.sin_port = htons( 80 );

	ret = bind( sockfd, ( struct sockaddr* )&addr_serv, sizeof( addr_serv ) );
	if( ret < 0 )
	{
		perror("main, bind(), error:");
		printf("\n");
		return 0;
	}
	printf("binding finish...\n");


	ret = listen( sockfd, 10 );
	if( ret < 0 )
	{
		perror("main, listen(), error:");
		printf("\n");
		return 0;
	}

	int connfd;
	int status;
	int num_acnt;
	user_status = ( char* )malloc( sizeof( char ) * 1024 );
	
	char* pos_start;
	char* pos_cur;
	char* pos_end;
	char login_user[] = "login_user:";
	char logout_user[] = "logout_user:";
	char invite_master[] = "invite_master:";
	char invite_challenger[] = "invite_challenger:";
	char inv[] = "inv";
	char ack[] = "ack";
	char board_change[] = "board_change-";

	while( 1 )
	{
		len_inet = sizeof( addr_clnt );
		connfd = accept( sockfd, ( struct sockaddr* )&addr_clnt, &len_inet );
		ret = pipe( fd_pipe );
		memset( user_status, '\0', 1024 );
		if( ret < 0 )
		{
			perror("main, pipe(), error: ");
			printf("\n");
			return 0;
		}
		fd_w = &fd_pipe[ 1 ];
		fd_r = &fd_pipe[ 0 ];

		pid = fork();

		if( pid > 0 )
		{
			close( *fd_w );
			close( connfd );
			ret = read( *fd_r, user_status, 1024 );
			if( ret > 0 )
			{
				printf("pipe success, user_status: %s\n", user_status );
				
				if( ( pos_start = strstr( user_status, login_user ) ) != NULL )
				{
					pos_start = pos_start + strlen( login_user );
					printf("parse result: %d\n", ( *pos_start ) - 48 );
					list_pf[ ( *pos_start ) - 48 ].online_status = 1;
					recent_user = ( *pos_start ) - 48; 
				}
				else if( ( pos_start = strstr( user_status, logout_user ) ) != NULL )	/*	logout handler	*/
				{
					pos_start = pos_start + strlen( logout_user );
					list_pf[ ( *pos_start ) - 48 ].online_status = 0;
					reset_pf( ( *pos_start ) - 48 );
				}
				else if( ( pos_start = strstr( user_status, inv ) ) != NULL )
				{
					/*	int00X-00X	*/
					num_acnt = 0;
					pos_start = pos_start + strlen( inv );		/*	move through inv	*/
					pos_cur = pos_start + 3;			/*	move to the second ditit of the acnt	*/
					num_acnt = num_acnt + ( ( *pos_cur ) - 48 )*10; 
					pos_cur++;
					num_acnt = num_acnt + ( ( *pos_cur ) - 48 ) - 1; 
					/*	num_acnt: invite, ( *pos_start )-48: be invited	*/
					if( list_pf[ ( *pos_start ) - 48 ].acknowledge == -1 )
					{
						list_pf[ ( *pos_start ) - 48 ].acknowledge = num_acnt;	/* list_pf[ be invited ].acknowledge = invite */
						list_pf[ num_acnt ].invite = ( *pos_start ) - 48;	/* list_pf[ invite ].invite = be invited */
					}
				}
				else if( ( pos_start = strstr( user_status, ack ) ) != NULL )
				{
					/*	ack00X-00X	*/
					num_acnt = 0;
					pos_start = pos_start + strlen( ack );		/*	move through ack	*/
					pos_cur = pos_start + 3;			/*	move to the secound digit of the acnt	*/
					num_acnt = num_acnt + ( ( *pos_cur ) - 48 )*10;
					pos_cur++;
					num_acnt = num_acnt + ( ( *pos_cur ) - 48 ) - 1;
					/*	num_acnt: be invited, ( *pos_start )-48: invite	*/
					if( list_pf[ ( *pos_start ) - 48 ].acknowledge == -1 && list_pf[ ( *pos_start ) - 48 ].gaming == 0 )
					{
						list_pf[ ( *pos_start ) - 48 ].acknowledge = num_acnt;	/* list_pf[ invite ].acknowledge = be invited */
						list_pf[ num_acnt ].invite = ( *pos_start ) - 48;	/* list_pf[ be invited ].invite = invite */
						game_start( num_acnt, ( *pos_start ) - 48 );
						list_pf[ num_acnt ].invite = -1;
						list_pf[ num_acnt ].acknowledge = -1;
						list_pf[ ( *pos_start ) - 48 ].invite = -1;
						list_pf[ ( *pos_start ) - 48 ].acknowledge = -1;
					}
					else
					{
						/*	another player had entered a match, aborted connection	*/
						list_pf[ num_acnt ].invite = -1;
						list_pf[ num_acnt ].acknowledge = -1;
					}
				}
				else if( ( pos_start = strstr( user_status, board_change ) ) != NULL )
				{
					/*	board_change-(num_acnt)-(num)	*/
					num_acnt = 0;
					pos_start = pos_start + strlen( board_change );
					num_acnt = ( *pos_start ) - 48;
					pos_start = pos_start + 2;
					
					printf("main, dec_maker: %d, opponent: %d, place: %d\n", num_acnt, list_pf[ num_acnt ].opponent, ( *pos_start ) - 48 );

					game_handler( num_acnt, list_pf[ num_acnt ].opponent, ( *pos_start ) - 48 );
				}
				
			}
			close( *fd_r );
			ret = wait( &status );
			continue;
		}
		close( *fd_r );
		packet_handler( connfd );
	
		close( *fd_w );
		close( sockfd );
		exit( 0 );
	}

	free( user_status );

	return 0;
}
