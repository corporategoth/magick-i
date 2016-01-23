/*
 * Source copyright Jag Software Limited of England.  Modifications may
 * be made as long as this and other copyright notices (and or messages)
 * are retained in this file.
 *
 * (Why not do the sensible thing and email ME with the source code changes
 *  you would like to implement?)
 *
 *
 * $Source: /home/cvs/magick/Magick-I/em.c,v $
 *
 * $Log: em.c,v $
 * Revision 4.0.0.1  1998/10/17 13:04:53  root
 *
 *
 * Revision 1.4.2.1  1998/10/17 02:28:03  root
 *
 *
 * Revision 1.11  1997/06/29 22:25:17  dev
 * Fixes to enable compilation on HP-UX (and others)
 * Various bug fixes (minor bugs)
 * jheiss@ugcs.caltech.edu fix for getting very big files.
 * Added -n <CCYYMMDDHHMMSS> to enable retrieval of files newer than the
 * specified date/time.
 *
 * Revision 1.9  1997/04/14 19:35:27  dev
 * a) Don't skip() past end of strings
 * b) Do return from checkit() on hitting end of string, This
 *    segment of code was previously placed Inside a 'debug' clause!
 *
 * Revision 1.8  1997/04/12 21:27:55  dev
 * We now use a forked session to run an em session.. We then monitor the
 * exit of this session to detect timeout etc.
 * Fix to code that retrieves the size of a program on the remote system.
 * If the userid/group is very big then the ls -l command can return info that
 * is incorrectly parsed.  Corrected this code.
 * Moved alot of code into emStart().  This function is now called when some
 * major work is required to be done.
 *
 * Revision 1.7  1997/03/10 22:48:58  root
 * When retrieving the 'list' file, Move the orignal to <file>.downloading.
 * This file is then moved back to <file> upon recovery..
 *
 * Revision 1.6  1997/02/22 13:41:42  dev
 * 1) Bug fix. When retrieving files from remote and the user/group/file size
 *    information is merged together (ie. long user/group names), EM was
 *    failing (totally)
 * 2) Don't create directories that we're not intrested in (ie. Don't litter
 *    your archive with empty directories)
 * 3) Bug fix to the awk/sed combination used in dols_fromlist()
 *
 * Revision 1.5  1997/02/11 20:41:03  dev
 * A Number of changes/enhancements..
 * 1) Bug fix to dates rolling over year (patch applied)
 * 2) -l option to specify listing file (ls-lR file)
 * 3) -o option to allow specification of port to open
 * 4) -g get files usage has changed considerably (Please check!)
 *
 * Revision 1.4  1996/11/14 02:26:41  dev
 * Actually added g_keepAll processing..
 *
 * Revision 1.3  1996/11/14  02:18:01  dev
 * Many changes..  -g switch to specify files to get.
 * -a switch to force local files not on remote to be kept
 * Messages cleaned up.  Reg-exps can be reversed using '!'
 *
 * Revision 1.2  1996/07/10  23:48:00  root
 * Corrected usage text
 *
 * Revision 1.1  1996/07/10  23:37:57  root
 * Initial revision
 *
 *
*/
static char rcsId_em_c[] = "$Id: em.c,v 4.0.0.1 1998/10/17 13:04:53 root Exp $";



#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>	/* waitpid & WEXITSTATUS etc. */
#include <sys/types.h>	/* utimbuf */
#include <utime.h>		/* utime() */
#include <string.h>		/* strstr() */
#include <unistd.h>		/* getopt(), alarm() */
#include <signal.h>		/* signal() */
#include <regex.h>		/* re_comp() & re_exec() */
#include <pwd.h>		/* getpwuid() */
#include <stdlib.h>		/* getenv() */
#include <limits.h>		/* UINT_MAX */

#include "em.h"


static char *dirL[3000];
static char *dirR[3000];
static int dirN = 0;

static char **g_argv;		/* Used for recovery - rerun */

main(argc,argv)
int argc;
char **argv;
{
	char opt;

	/*
	**	g_argv is used when we want to re-run this program
	*/
	g_argv = argv;


	/*
	**	No buffering wanted
	*/
	setbuf(stdout,NULL);
	setbuf(stdin,NULL);
	setbuf(stderr,NULL);

	printf("EasyMirror %s\n  (c) Jag Software Limited\n", rcsId_em_c);
	printf("Queries:   jag@jags.net, ftp://dist.jags.net/dist/em\n");
	printf("Mail list: send 'add em.list' in body to listserv@jags.net\n");
	printf("           Mailing list is em.list@jags.net\n\n");

	while ( (opt = getopt(argc,argv,"phdt:k:c:sfbigan:l:o")) != -1 )
	{
		switch ( opt )
		{
			case ':':
				fprintf(stderr,"ERROR: Missing arg\n");
				usage();

			case '?':
				fprintf(stderr,"ERROR: Unknown switch\n");
				usage();

			case 'h':
				usage();

			case 'd':
				g_debug = TRUE;
				break;

			case 't':
				g_timeout = atoi(optarg);
				break;

			case 'k':
				g_k = atoi(optarg);
				break;

			case 'c':
				strcpy(g_cache,optarg);
				break;

			case 'l':
				strcpy(g_listfile,optarg);
		
				/*
				**	Incase we had an earlier failed download,
				**	half-way through download the actual g_listfile, move
				**	the g_listfile.downloading to g_listfile
				*/
				{
				char command[1024];
				sprintf(command,"mv %s.downloading %s",
								g_listfile,
								g_listfile);
				system(command);
				}
				break;

			case 's':
				g_size = TRUE;
				break;

			case 'f':
				g_stamp = TRUE;
				break;

			case 'p':
				g_preserve = TRUE;
				break;

			case 'b':
				g_bogus = TRUE;
				break;

			case 'i':
				getRegexp(TRUE);
				break;

			case 'g':
				getRegexp(FALSE);
				break;

			case 'a':
				g_keepAll = TRUE;
				break;
	
			case 'n':
				g_newer = optarg;
				break;
	
			case 'o':
				g_openPort = atoi(optarg);
				break;
		}
	}

	/*
	**	Check args
	*/
	if ( argc - optind < 3 )
	{
		usage();
	}

	/*
	**	Default cache file if not already set
	*/
	if ( ! *g_cache )
	{
		sprintf(g_cache,"/tmp/%s",argv[optind]);
	}

	if ( g_debug )
	{
		printf("Cache is %s\n",g_cache);
		printf("Timeout is %d\n",g_timeout);
		printf("K is %d\n",g_k);
		printf("Preserve change time is %s\n",(g_preserve)?"ON":"OFF");
	}


	for(;;)
	{
		int pid = fork();
		int status;

		if ( pid EQ -1 )
		{
			fprintf(stderr,"Fork failed\n");
			exit(EXIT_FORKEXEC);	
		}

		/*
		**	Child
		*/
		if ( pid EQ 0 )
		{
			emStart(argc,argv,optind);
			exit(0);
		}

		waitpid(pid,&status,0);

		if ( WIFSIGNALED(status) && WTERMSIG(status) != SIGALRM )
		{
			fprintf(stderr,"Died on unknown signal: %d\n", WTERMSIG(status));
			exit(-1);
		}

		if ( !WIFSIGNALED(status) )
		{
			exit(WEXITSTATUS(status));
		}

		printf("Retry\n");
	}
}



/*******************************************************************************
*
*	ftpin( file handle )
*
*	Retrieve some text from ftp session	
*	
*******************************************************************************/


static char *
ftpin(	fin	)
FILE *fin;
{
	static char buff[1024];

	if ( fin )
	{
		if ( fgets(buff,sizeof(buff),fin) EQ NULL )
		{
			fprintf(stderr,"ERROR: FTP in stream error\n%s\n",
					strerror(errno));
			exit( EXIT_FTPDOWN );
		}

		if ( g_debug )
		{
			printf("<%s",buff);
		}
	}

	return(buff);
}


/*******************************************************************************
*
*	ftpout( fo,fi, command )
*
*******************************************************************************/


static void
ftpout( fo,fi, command )
FILE *fo;
FILE *fi;
char *command;
{
	/*
	**	Handle time-out's
	*/
	signal(SIGALRM,ftptimeout);
	alarm(g_timeout);

	/*
	**	Send command to ftp
	*/
	fprintf(fo,"%s\n",command);

	/*
	**	Get response
	*/
	while ( atoi(ftpin(fi)) != FTP_PORTCOMMAND )
	{
		/*
		**	Command may have been a 'pwd' command
		*/
		if ( atoi(ftpin(NULL)) == FTP_PWD )
		{
			fprintf(stderr,"Current dir: %s\n",ftpin(NULL) );
			break;
		}

		if ( closeSession(command) )
		{
			break;
		}
	}

	/*
	**	Disable alarm
	*/
	alarm(0);
}



static char *
ftppwd( fo,fi )
FILE *fo;
FILE *fi;
{
	static char pwd[MAX_PATH+1];
	char *q;
	char *p = pwd;

	fprintf(fo,"pwd\n");

	while ( atoi(ftpin(fi)) != FTP_PWD )
	{
		closeSession(NULL);	/* Checks for session close */
	}

	q = ftpin(NULL);
	while ( *q != '"' )
		q++;

	while ( *++q != '"' )
		*p++ = *q;
	*p = '\0';

	return(pwd);
}


static void
ftppipe( ftp,
		 func )
FILE *ftp;
void (*func)( );
{
	if ( atoi(ftpin(ftp)) != FTP_OPEN )
	{
		fprintf(stderr,"No output. Got instead: %s\n",ftpin(NULL));
		return;
	}

	while ( atoi(ftpin(ftp)) != FTP_TRANSFERED )
	{
		(*func)( ftpin(NULL) );
	}
}



FILE *f_fo;
FILE *f_fi;
static int f_pid = -1;

static void closeAll()
{
	kill (f_pid,SIGKILL);

	waitpid(f_pid,NULL,0);

	fclose(f_fo);
	fclose(f_fi);
}


static void
ftpopen( fi, fo,
		 host,
		 user,
		 pass )
FILE **fi;
FILE **fo;
char *host;
char *user;
char *pass;
{
	int childToParent[2];
	int parentToChild[2];

	/*
	**	If the child session (ftp) is already active. Then we abort it and
	**	then re-create it..
	*/
	if ( f_pid != -1 )
	{
		kill(f_pid,SIGKILL);

		fclose(*fi);
		fclose(*fo);

		close(childToParent[0]);
		close(childToParent[1]);
		close(parentToChild[0]);
		close(parentToChild[1]);
	}


	/*
	**	Open pipes for ftp
	*/
	if ( pipe(parentToChild) )
	{
		fprintf(stderr,"ERROR: Pipe failed\n%s\n",strerror(errno));
		exit( EXIT_PIPE );
	}

	if ( pipe(childToParent) )
	{
		fprintf(stderr,"ERROR: Pipe failed\n%s\n",strerror(errno));
		exit( EXIT_PIPE );
	}

	switch ( f_pid = fork() )
	{
		case -1:	fprintf(stderr,"ERROR: Fork failed\n%s\n",
							strerror(errno));
					exit(EXIT_FORKEXEC);

		case 0:		close(0);
					close(1);

					dup(childToParent[0]);
					close(childToParent[1]);

					dup(parentToChild[1]);
					close(parentToChild[0]);

					if ( execlp("ftp","ftp","-n","-v",NULL) )
					{
						fprintf(stderr,"ERROR: Unable to exec ftp session\n%s\n",
									strerror(errno));
						exit(EXIT_FORKEXEC);
					}
	}

	if ( (*fi = fdopen(parentToChild[0],"r")) EQ NULL )
	{
		fprintf(stderr,"ERROR: Fdopen failed\n%sn",strerror(errno));
		exit( EXIT_FDOPEN );
	}
	f_fi = *fi;
	setbuf(*fi,NULL);
	close(parentToChild[1]);

	if ( (*fo = fdopen(childToParent[1],"w")) EQ NULL )
	{
		fprintf(stderr,"ERROR: Fdopen failed\n%sn",strerror(errno));
		exit( EXIT_FDOPEN );
	}
	f_fo = *fo;
	setbuf(*fo,NULL);
	close(childToParent[0]);

	/*
	**	Host connection
	*/
	fprintf(*fo,"open %s %d\n",host,g_openPort);

	/*
	**	User login
	*/
	signal(SIGALRM,ftptimeout);
	alarm(g_timeout);

	while ( atoi(ftpin(*fi)) != FTP_CONNECT )
		;

	if ( g_debug )
	{
		printf("CONNECT OK\n");
	}

	alarm(0);

	fprintf(*fo,"user %s %s\n",user,pass);

	while ( atoi(ftpin(*fi)) != FTP_PASSWORD )
	{
		closeSession(NULL);
	}


	/*
	**	Wait for logon
	*/
	signal(SIGALRM,ftptimeout);
	alarm(g_timeout);

	while ( atoi(ftpin(*fi)) != FTP_LOGON )
	{
		closeSession(NULL);
	}

	alarm(0);

	if ( g_debug )
	{
		printf("LOGIN OK\n");
	}

	ftpout(*fo,*fi,"binary");
}



/*******************************************************************************
*
*	ftpcwd( fo,fi,  dir )
*
*	Set current working dir on remote system
*
*******************************************************************************/

static int
ftpcwd( fo,fi, cwd )
FILE *fo;
FILE *fi;
char *cwd;
{
	fprintf(fo,"cd %s\n",cwd);

	while ( atoi(ftpin(fi)) != FTP_CWD )
	{
		closeSession(NULL);

		if ( atoi(ftpin(NULL)) EQ FTP_FILERROR )
		{
			fprintf(stderr,"ERROR: Remote directory %s is invalid\n",
					cwd);
			return 1;
		}
	}

	return 0;
}


/*******************************************************************************
*
*	ftpget( fo,fi, file info )
*
*	Get specified file from remote
*
*******************************************************************************/

static void
ftpget( fo,
		fi,
		f )
FILE *fo;
FILE *fi;
FileInfo *f;
{
	char command[MAX_PATH+1];
	unsigned int timeout;
	char *speed;
	double spd;


	/*
	**	Issue the get command
	*/
	sprintf(command,"get %s",f->fname);
	ftpout(fo,fi,command);



	/*
	** Calculate the timeout for this transfer.  When transfering large
	** (>35 MB) files, this calculation can overflow an unsigned int.
	** As such, we check for that here.
	**
	** I've made g_k an unsigned int so that this arithmetic will be
	** done in that much space.  Alternatively, it could be changed
	** back to an int and we could do a cast in here.  Not only is
	** the current way prettier (IMHO), the resulting executable is
	** also about 5k smaller.
	** Jason Heiss (jheiss@ugcs.caltech.edu), 5/26/97
	*/
	if ( f->size > (UINT_MAX / g_k - 1024) )
	{
		timeout = UINT_MAX / 1024;	/* This should only happen if you've got
									g_k or the time ot transfer 1k set to a
									very high value */
	}
	else
	{
		timeout = (f->size * g_k + 1024) / 1024;
	}
 

	/*
	**	Set an alarm, based on the size of the file and the max ammount of
	**	time we expect 1k to take to transfer over in.
	*/
	if ( timeout < g_timeout )
	{
		timeout = g_timeout;
	}

	if ( g_debug )
	{
		printf("timeout %d, size = %ld\n",timeout,f->size);
	}

	signal(SIGALRM,ftptimeout);
	alarm(timeout);


	while ( atoi(ftpin(fi)) != FTP_TRANSFERED )
	{
		closeSession(NULL);

		/*
		**	Unable to get other file for some reason, move onto next
		**	file.
		*/
		if ( atoi(ftpin(NULL)) EQ FTP_FILERROR )
		{
			fprintf(stderr,"WARNING: Unable to get file %s\n%s\n",
							f->fname,
							ftpin(NULL) );
			return;
		}
	}

	alarm(0);


	/*
	**	If we want to preserve times on files, then do so now.
	*/
	if ( g_preserve )
	{
		setTime(f->fname,f->date);
	}

	/*
	**	Get the transfer speed (default to zero)
	**  THIS SECTION OF CODE COMMENTED OUT UNTIL DEBUGGED AND WORKING!
	**	If you uncomment this code, EM just hangs here,  Please feel
	**	free to debug!
	*/
#ifdef DONT_COMPILE
	signal(SIGALRM,ftptimeout); 
	alarm(g_timeout);
	speed = ftpin(fi);	/* Hangs here :-( */
	alarm(0);
	sscanf(speed,"%*d bytes received in %lf secs",&spd);
#endif
	spd = 0.0;

	g_bytes += f->size;

	printf("EM: GOT: %s %ld %lf\n",
			f->fname,f->size,spd);
}



static FileInfo getL[3000];
static int getN = 0;


/*******************************************************************************
*
*	checkit( ls output string )
*
*	Called for each line returned from ls from the remote system.
*
*******************************************************************************/

static void
checkit( txt )
char *txt;
{
	char *fspec = txt;
	int i;
	char *monthPosn=0;

	/*
	**	Initialisation call
	*/
	if ( txt == NULL )
	{
		getN = 0;
		return;
	}


	/*
	**	fspec starts at the filename position,  I work this out by skipping
	**	all data upto the month specifier and then skipping another 3
	**	fields.
	*/
	for ( fspec = skip(fspec,3) ; ; fspec = skip(fspec,1) )
	{
		if ((strncmp(fspec,"Jan",3) EQ 0 ||
			 strncmp(fspec,"Feb",3) EQ 0 ||
			 strncmp(fspec,"Mar",3) EQ 0 ||
			 strncmp(fspec,"Apr",3) EQ 0 ||
			 strncmp(fspec,"May",3) EQ 0 ||
			 strncmp(fspec,"Jun",3) EQ 0 ||
			 strncmp(fspec,"Jul",3) EQ 0 ||
			 strncmp(fspec,"Aug",3) EQ 0 ||
			 strncmp(fspec,"Sep",3) EQ 0 ||
			 strncmp(fspec,"Oct",3) EQ 0 ||
			 strncmp(fspec,"Nov",3) EQ 0 ||
			 strncmp(fspec,"Dec",3) EQ 0) &&
			 fspec[-1] EQ ' ' && isdigit(fspec[-2]) )
		{
			monthPosn=fspec;

			fspec = skip(fspec,3);
			break;	
		}

		if ( fspec[0] EQ '\0' )
		{
			if ( g_debug )
			{
				printf("+ign: %s\n",txt);
			}

			return;
		}
	}

	if ( g_debug )
	{
		printf("fspec = %s\n",fspec);
	}

	/*
	**	Remove pesky \n char  (If one is present)
	*/
	if ( *txt && iscntrl(txt[strlen(txt)-1]) )
	{
		txt[strlen(txt)-1] = '\0';
	}


	switch ( txt[0] )
	{
		/*
		**	Directory.  Save into getL[]
		*/
		case TYP_DIR:
			if ( strcmp(fspec,".") EQ 0 ||
				 strcmp(fspec,"..") EQ 0 )
			{
				if ( g_debug )
				{
					printf("+ign: %s\n",txt);
				}
			}
			else
			{
				getL[getN].typ = TYP_DIR;
				getL[getN].fname = (char *) realloc(getL[getN].fname,
													strlen(fspec)+1);
				strcpy(getL[getN].fname,fspec);

				if ( g_debug )
				{
					printf("+dir: '%s'\n",getL[getN].fname);
				}

				getN++;
			}
			break;

		/*
		**	Link, split into two parts and save into getL[]
		*/
		case TYP_LNK:
		{
			char *f1 = fspec;
			char *f2 = f1;

			while ( f2[0] != '-' || f2[1] != '>' )
			{
				f2++;
			}

			f2[-1] = '\0';
			f2 += 3;

			getL[getN].typ = TYP_LNK;
			getL[getN].fname = (char *) realloc(getL[getN].fname,strlen(f1)+1);
			strcpy(getL[getN].fname,f1);
			getL[getN].lnk = (char *) realloc(getL[getN].lnk,strlen(f2)+1);
			strcpy(getL[getN].lnk,f2);

			if ( g_debug)
			{
				printf("+lnk: %s -> %s\n",getL[getN].fname,getL[getN].lnk);
			}

			getN++;
			break;
		}

		/*
		**	File name.  Save into getL[]
		*/
		case TYP_REG:
			getL[getN].typ = TYP_REG;
			getL[getN].fname = (char *) realloc(getL[getN].fname,
												strlen(fspec)+1);
			strcpy(getL[getN].fname,fspec);

			fspec[-1] = '\0';
			getL[getN].date = getTime(fspec-13);

			/*
			**	Get size, it's the field before the month
			*/
			monthPosn--;
			do { monthPosn--; } while ( isdigit(*monthPosn) );
			monthPosn++;

			getL[getN].size = atol(monthPosn);
	
			if ( g_debug )
			{	
				printf("+reg: %s (%ld) dated %s",
						getL[getN].fname,
						getL[getN].size,
						ctime( &(getL[getN].date)) );
			}

			getN++;
			break;

		/*
		**	Others.  Ignored
		*/
		default:
			if ( g_debug )
			{
				printf("+ign: %s\n",txt);
			}
	}
}



/*******************************************************************************
*
*	ftpdup( fo,fi, remote dir, local dir )
*
*	Duplicate remote directory onto local dir
*
*******************************************************************************/

static void
ftpdup( fo, fi,
		remote,
		local )
FILE *fo;
FILE *fi;
char *remote, *local;
{
	int i;


	printf("ftpdup(%s,%s)\n",remote,local);


	/*
	**	Change dirs
	*/
	if ( ftpcwd(fo,fi,remote) )
	{
		return;
	}



	if ( strcmp(ftppwd(fo,fi),remote) )
	{
		fprintf(stderr,
				"ERROR: Ftp cd failed (wanted %s, got %s)\n",
				remote,
				ftppwd(fo,fi) );
		exit(EXIT_REMOTECD);
	}

	fprintf(fo,"lcd %s\n",local);
	if ( chdir(local) )
	{
		fprintf(stderr,
				"ERROR: Local CD failed (wanted %s)\n%s\n",
				local,
				strerror(errno));
		exit(EXIT_LOCALCD);
	}

	/*
	**	Pull list of files at remote.  Note that we may infact get this
	**	informaiton from a local list file.
	*/
	checkit(NULL);
	if ( g_listfile[0] )
	{
		if ( checkforlist(g_listfile) )
		{
			if ( g_debug )
			{
				printf("ls from list file; %s\n",g_listfile);
			}

			dols_fromlist(remote);
		}
		else
		{
			ftpout(fo,fi, "ls -l");
			ftppipe(fi,checkit);
		}
	}
	else
	{
		ftpout(fo,fi, "ls -l");
		ftppipe(fi,checkit);
	}


	if ( g_debug )
	{
		printf("DIR %d\n",getN);
	}


	/*
	**	Compare this directory to remote
	*/
	compareLocal(remote);

	/*
	**	Process list of files..
	*/
	for ( i = 0; i < getN; i++ )
	{
		char ndir[MAX_PATH+1];
		char localFname[MAX_PATH+1];
		char fileDateTime[15];

		if ( getL[i].fname EQ NULL )
		{
			continue;
		}


		/*
		**	Check if we've been supplied the -n switch to only get files
		**	newer than specified.
		*/
		if ( g_newer )
		{
			strftime(fileDateTime,sizeof(fileDateTime),
					 "%Y%m%d%H%M%S", localtime(&getL[i].date));
			if ( g_debug )
			{
				printf("newer:%s\n",fileDateTime);
			}

			if ( strcmp(fileDateTime,g_newer) < 0 )
			{
				continue;
			}
		}


		sprintf(localFname,"%s/%s",local,getL[i].fname);
		sprintf(ndir,"%s/%s", remote,getL[i].fname);

		switch ( getL[i].typ )
		{
			case TYP_REG:
				if ( g_debug )
				{
					printf("PROC reg: %s\n",ndir /*getL[i].fname*/ );
				}
			
				/*
				**	If we're building a bogus mirror, simply create a
				**	local file.
				*/
				if ( ignore(ndir) )
				{
					if ( g_debug )
					{
						printf("Ignored: %s\n",ndir);
					}
				}
				else if ( g_bogus )
				{
					char fname[MAX_PATH+1];
					FILE *f;

					sprintf(fname,"%s.bog",getL[i].fname);
					f = fopen(fname,"w");
					fprintf(f,"%s %s\n",g_argv[optind],remote);
					fclose(f);

					/*
					**	If we want to preserve times on files, then do so now.
					*/
					if ( g_preserve )
					{
						setTime(fname,getL[i].date);
					}
				}
				/*
				**	Not attempting to get the Listfile, just pull the file back
				*/
				else if ( strcmp(localFname,g_listfile) != 0 )
				{
					ftpget(fo,fi,&getL[i]);
				}
				/*
				**	Special care is needed here.. If we're getting back
				**	the 'list' file then we rename it before attempting
				**	to retrieve it.  EM renames this file before doing
				**	anything else, right at the start.  We name the file
				**	to .downloading.
				*/
				else
				{
					char command[1024];
					sprintf(command,"mv %s %s.downloading",
								g_listfile,	
								g_listfile);
					system(command);
					ftpget(fo,fi,&getL[i]);

					/*
					**	Remove temporary copy of list file once we've retrieved
					**	a new one successfully.
					*/
					sprintf(command,"rm -f %s.downloading",
								g_listfile);
					system(command);
				}
				break;

			case TYP_DIR:
				if ( g_debug )
				{
					printf("PROC dir: %s\n",getL[i].fname);
				}

				/*
				**	Make sure we don't bother to create directories that
				**	have been indicated to be ignored
				*/
				if ( invertedLogic() && ignore(ndir) )
				{
					if ( g_debug )
					{
						printf("Ignored dir: %s\n",ndir);
					}
				}
				else
				{
					mkdir(getL[i].fname,0755); errno = 0;

					sprintf(ndir,"%s/%s", local,getL[i].fname);
					dirL[dirN] = (char *) realloc(dirL[dirN],strlen(ndir)+1);
					strcpy(dirL[dirN],ndir);

					sprintf(ndir,"%s/%s", remote,getL[i].fname);
					dirR[dirN] = (char *) realloc(dirR[dirN],strlen(ndir)+1);
					strcpy(dirR[dirN],ndir);

					dirN++;
				}
				break;

			case TYP_LNK:
				if ( g_debug )
				{
					printf("PROC lnk: %s\n",getL[i].fname);
				}
				if ( symlink(getL[i].lnk,getL[i].fname) )
				{
					fprintf(stderr,"ERROR: Local symbolic link failed\n");
					fprintf(stderr,"       %s -> %s\n%s\n",
							getL[i].fname,
							getL[i].lnk,
							strerror(errno));
					exit( EXIT_LINK);
				}
				break;
		}
	}


	/*
	**	Write out the cache file.  (To contain a list of directories)
	*/
	cacheWrite();
}


/*******************************************************************************
*
*	compareLocal(remote dir)
*
*	Compares the remote list of files against the local list of files in the
*	current directory.
*
*	This includes :-
*
*		o	Removing local files not at the remote
*
*		o	Removing files from the list that are newer on the local host
*
*******************************************************************************/

static void
compareLocal( remote )
char *remote;
{
	DIR *dirp = opendir(".");
	struct dirent *d;

	if ( dirp EQ NULL )
	{
		fprintf(stderr,"ERROR: Opendir(.) failed\n%s\n",
				strerror(errno));

		exit( EXIT_OPENDIR );
	}

	while ( d = readdir(dirp) )
	{
		struct stat buf;
		FileInfo *remFile;
		char fname[1024];


		if ( strcmp(d->d_name,".") EQ 0 ||
			 strcmp(d->d_name,"..") EQ 0 )
		{
			continue;
		}
	
		if ( g_debug )
		{	
			printf("ENTRY:'%s'\n",d->d_name);
		}

		sprintf(fname,"%s/%s",remote,d->d_name);

		/*
		**	Find this file at remote host
		*/
		remFile = findFile(d->d_name);
		if ( g_debug )
		{
			if ( !remFile )
			{
				printf("file %s is redundant\n",d->d_name);
			}
			else
			{
				printf("file %s is %c\n",remFile->fname,remFile->typ);
			}
		}

		/*
		**	Get info on local file - Note, we use lstat().   We're not
		**	intrested in files pointed to by links, rahter the link
		**	itself.  We may even have created a link and not yet
		**	downloaded the file pointed to by it!
		*/
		if ( lstat(d->d_name,&buf) )
		{
			fprintf(stderr,"ERROR: Stat on %s failed\n%s\n",
					d->d_name,
					strerror(errno));
			exit( EXIT_STAT );
		}

		/*
		**	Local directory, If not present on remote, remove locally
		*/
		if ( S_ISDIR(buf.st_mode) )
		{
			if ( (remFile EQ NULL || remFile->typ != TYP_DIR) ||
				 (invertedLogic() && ignore(fname) && !g_keepAll) )
			{
				if ( g_debug )
				{
					printf("remove dir %s\n",d->d_name);
				}

				removeDir(d->d_name);
			}
		}
		/*
		**	Local regular file.  Remove from list if it's newer, remove
		**	from local directory if not on remote system.
		*/
		else if ( S_ISREG(buf.st_mode) )
		{
			/*
			**	File not on remote.  Remove locally
			*/
			if ( remFile EQ NULL || remFile->typ != TYP_REG ||
				 ignore(fname) )
			{
				/*
				**	But only remove if we aren't intrested in keeping ALL
				**	files, regardless of being present on remote
				*/
				if ( invertedLogic() && !g_keepAll )
				{
					if ( g_debug )
					{
						printf("remove %s\n",d->d_name);
					}


					if ( unlink(d->d_name) )
					{
						fprintf(stderr,"ERROR: Failed to remove %s\n%s\n",
								d->d_name,
								strerror(errno));
						exit(EXIT_UNLINK);
					}
				}
			}
			/*
			**	File older or same on remote.  Don't get (Remove from list)
			*/
			else
			{
				double diff = difftime(remFile->date,buf.st_mtime);
				char fileDateTime[15];

				/*
				**	Convert time/date of remote file suitable for a quick
				**	comparison against time/date supplied
				*/
				if ( g_newer )
				{
					strftime(fileDateTime,sizeof(fileDateTime),
							 "%Y%m%d%H%M%S", localtime(&(remFile->date)));
					if ( g_debug )
					{
						printf("newer:%s\n",fileDateTime);
					}
				}

				if ( g_debug )
				{
					printf("diff %s",ctime(&(remFile->date)));
					printf("  vs %s",ctime(&(buf.st_mtime)));
				}

				/*
				**	Don't get file if remote file is older than local file
				**	or we're only intrested in files newer than a particular
				**	date and this file is before the specified date.
				*/
				if ( diff <= 0 ||
						(g_newer && strcmp(fileDateTime,g_newer) < 0) )
				{
					/*
					**	Local file is newer..  If we also want to check sizes
					**	then do so.  We'll not get the file if we're not
					**	intrested in size or the sizes are equal.
					*/
					if ( diff <= 0 || g_size EQ FALSE ||
						 buf.st_size EQ remFile->size )
					{
						if ( g_debug )
						{
							printf("not taken: %s\n",remFile->fname);
						}

						free(remFile->fname);
						remFile->fname = NULL;

						/*
						**	If we're not getting the file and the local file
						**	is marked as being newer than the remote file,
						**	we copy the remote time stamp onto the local file.
						*/
						if ( g_stamp && diff  )
						{
							if ( g_debug )
							{
								printf("re-stamp\n");
							}

							setTime(d->d_name, remFile->date);
						}
					}
					else if ( g_debug )
					{
						printf("sizes diff\n");
					}
				}
			}
		}
		/*
		**	All link are remove (and re-created later on)
		*/
		else if ( S_ISLNK(buf.st_mode) )
		{
			if ( unlink( d->d_name ) )
			{
				fprintf(stderr,"ERROR: Failed to remove link %s\n%s\n",
						d->d_name,
						strerror(errno));
				exit(EXIT_UNLINK);
			}
		}
	}

	if ( closedir(dirp) )
	{
		fprintf(stderr,"WARNING: Failed to close directory\n%s\n",
				strerror(errno));
	}

}


static FileInfo *
findFile( fname )
char *fname;
{
	int i;

	for ( i = 0; i < getN; i++ )
	{
		if ( getL[i].fname && strcmp(getL[i].fname,fname) EQ 0 )
		{
			return &getL[i];
		}
	}

	return NULL;
}


/*******************************************************************************
*
*	removeDir( dir )
*
*******************************************************************************/

static void
removeDir( dir )
char *dir;
{
	char command[MAX_PATH+1];

	if ( g_debug )
	{
		printf("remove dir %s\n",dir);
	}

	sprintf(command,"rm -rf %s\n",dir);

	if ( system(command) )
	{
		fprintf(stderr,"WARNING: Unable to run %s\n%s\n",
				command,
				strerror(errno));
	}
}


/*******************************************************************************
*
*	getTime(text date)
*
*	Returns time_t for the date supplied (in text format)
*
*	'Mon Dd HH:MM'
*
*	'Mon Dd  YYYY'
*
*******************************************************************************/

static time_t
getTime( txt )
char *txt;
{
	char *MONTHS = "JanFebMarAprMayJunJulAugSepOctNovDec";

	static int first = TRUE;
	static struct tm tm_now;
	static time_t now;

	struct tm t;
	char srch[4];

	int mon,day,hh,mm,ccyy;

	static time_t madeTime;

	if ( g_debug )
	{
		printf("date in = '%s' = ",txt);
	}

	if ( first )
	{
		first = FALSE;

		now = time(NULL);
		memcpy(&tm_now,localtime(&now),sizeof(tm_now));
	}

	/*
	**	Slowly fill the tm structure.
	*/
	memset(&t,0,sizeof(t));

	/*
	**	Month
	*/
	strncpy(srch,txt,3);
	srch[3] = '\0';
	t.tm_mon = (strstr(MONTHS,srch) - MONTHS)/3;

	/*
	**	Day of the month
	*/
	t.tm_mday = atoi(txt+4);

	/*
	**	Hours & Minutes or Year
	*/
	t.tm_year = tm_now.tm_year;
	if ( txt[9] != ':' )
	{
		t.tm_year = atoi(txt+8) - 1900;
	}
	else
	{
		t.tm_hour = atoi(txt+7);
		t.tm_min = atoi(txt+10);
	}

	/*
	**	Check for and correct year rollover.
	*/
	madeTime = mktime(&t);
	if (madeTime > now && txt[9] == ':')
	{
		t.tm_year--;
		madeTime = mktime(&t);
	}

	if ( g_debug )
	{
		printf(asctime(&t));
	}


	return( madeTime );
}



/*******************************************************************************
*
*	usage()
*
*	Output usage string and exit.
*
*******************************************************************************/

static void
usage()
{

	fprintf(stderr,
		"Usage is :-\n\n" \
		"em [args] <host> <remote dir> <local dir> [signon password]\n\n" \
		"Additional args :-\n\n" \
		"-d            Enable debug messages\n\n" \
		"-t <seconds>  Timeout in seconds on commands.  Ftp session is closed\n" \
		"              and re-started on this timeout.\n\n" \
		"-k <seconds>  Maximum time to expect 1024 bytes to take in transfer.\n" \
		"              The ftp session is closed and a new-one re-started on\n" \
		"              exceeding this.\n\n" \
		"-c <file>     Alternative cache file.  The default cache file is\n" \
		"              /tmp/<host>.  This file is written to on a regular\n" \
		"              basis to trace current progress (and to enable\n" \
		"              recovery)\n\n" \
		"-f            Ensure that the local files are stamped with the same\n"\
		"              date/time as the remote files but never newer.\n\n" \
		"-s            When comparing files, compare dates and then also\n" \
		"              compare sizes.  If the local file is of a different\n" \
		"              size then assume it's out of date.\n\n" \
		"-p            Preserve times on files retrieved.\n\n" \
		"-b            Bogus archive.  Files are not retrieved, instead a\n" \
		"              .bog file is created in it's place.  The .bog file\n" \
		"              will contain the ftp address and location that the\n" \
		"              file can be obtained from.\n\n" \
		"-g            Input regular expressions on stdio to indicate files\n"\
		"              that we DO want to retrieve.  Prefix any regular exp\n"\
		"              with a '!' to indicate a file pattern that we don't\n" \
		"              want to retrieve.\n\n" \
		"-i            Input regular expressions on stdio to indicate files\n"\
		"              that we DON'T want to retrieve.  This switch is essentially\n" \
		"              the opposite of the -g switch.  A regular expression may\n" \
		"              be prefixed with a '!' to indicate files that we do want\n" \
		"              to retrieve.\n\n" \
		"-a            All files are kept, Rather than deleting any files that\n" \
		"              are not being mirrored from the remote site.\n\n" \
		"-l            Allows you to specify a 'listing file'.  This file must\n" \
		"              be in ls-lR format.  The file may be gzipped or compressed-\n" \
		"              EM will automatically uncompress the file if required.\n\n" \
		"-o            Allows an alternate port to be used for the ftp connection,\n" \
		"              rather than the default ftp port.\n\n" \
		"-n            Only retrieve files newer than the supplied date.\n" \
		"              Supplied date must be in CCYYMMDDHHMMSS format.  e.g.\n" \
		"              19970514020155\n\n"
					   );

	exit( EXIT_ARGS);
}



/*******************************************************************************
*
*	ftptimeout( signal )
*
*	Ftp timeout function.  This function will perform some cleanup and
*	then terminate via a SIGALRM signal.  The parent will know of this
*	'death on ALRM' and re-try the ftp get.
*
*******************************************************************************/

static void
ftptimeout( sig )
int sig;
{
	closeAll();


	printf("EM: TIMEOUT: %ld bytes received before timeout\n", g_bytes);


	signal(SIGALRM,SIG_DFL);
	kill(getpid(),SIGALRM);
}



/*******************************************************************************
*
*	skip( text, fields )
*
*	Function to skip specified number of fields in text and return address
*	within text of the next field.
*
*******************************************************************************/

static char *
skip( txt, n )
char *txt;
int n;
{
	int i;

	/*
	**	Skip the number of fields specified OR until we hit the end of the
	**	string.
	*/
	for ( i = 0; *txt && i < n; i++ )
	{
		while ( *txt && !isspace(*txt) )
			txt++;

		while ( *txt && isspace(*txt) )
			txt++;
	}

	return txt;
}


/*******************************************************************************
*
*	cacheWrite()
*
*	Function to write out the cache file.
*
*******************************************************************************/

static void
cacheWrite()
{
	char tmp[MAX_PATH+1];
	FILE *f;
	int i;

	sprintf(tmp,"%s.t",g_cache);
	f = fopen(tmp,"w");

	for ( i = 0; i < dirN; i++ )
	{
		fprintf(f,"%s %s\n",dirL[i], dirR[i] );
	}

	fclose(f);

	/*
	**	Move the temporary file to overwrite cache in one action.
	**	This protects us incase we die while writing the cache file
	*/
	if ( rename(tmp,g_cache) )
	{
		fprintf(stderr,"WARNING: Rename %s to %s failed\n%s\n",
						tmp,
						g_cache,
						strerror(errno));
	}
}


/*******************************************************************************
*
*	cacheRead()
*
*	Function to read the cache, used for recovery..
*
*	Returns 0 on successfull read of the cache, non-zero otherwise.
*	(0 indicates that we are therefore in a recovery situation)
*
*******************************************************************************/

static int
cacheRead()
{
	FILE *f = fopen(g_cache,"r");

	if ( f )
	{
		int s;

		char local[MAX_PATH+1];
		char remote[MAX_PATH+1];

		while ( (s = fscanf(f,"%s %s",local,remote)) EQ 2 )
		{
			dirL[dirN] = (char *) malloc(strlen(local)+1);
			strcpy(dirL[dirN],local);

			dirR[dirN] = (char *) malloc(strlen(remote)+1);
			strcpy(dirR[dirN],remote);

			if ( g_debug )
			{
				printf("Cache read: %s %s\n",
						dirL[dirN],
						dirR[dirN]);
			}

			dirN++;
		}
	

		fclose(f);

		if ( g_debug )	
		{
			printf("fscanf status was %d\n",s);
		}

		/*	
		**	Cache read ok
		*/
		return 0;
	}

	/*
	**	Failed to read cache
	*/
	return 1;
}




/*******************************************************************************
*
*	setTime( fname, time_t )
*
*	Sets the accesstime and modifytimes for a given file.
*
*******************************************************************************/


static void
setTime( fname, tim )
char *fname;
time_t tim;
{
	struct utimbuf t;

	t.actime = t.modtime = tim;

	if ( utime(fname,&t) )
	{
		fprintf(stderr,"WARNING: Failed to set time: %s\n",
				strerror(errno));
	}
}



static int
closeSession( command )
char *command;
{

	if ( atoi(ftpin(NULL)) EQ FTP_BYE )
	{
		if ( g_debug )
		{
			printf("Quit received\n");
		}

		/*
		**	If we wanted to quit out then that's ok, we'll just
		**	return TRUE.  Otherwise, we'll treat like a timeout
		*/
		if ( command EQ NULL ||
			strcmp(command,"quit") )
		{
			ftptimeout(-1);
		}

		return TRUE;
	}

	if ( atoi(ftpin(NULL)) EQ FTP_CLOSE )
	{
		if ( g_debug )
		{
			printf("Close received\n");
		}

		ftptimeout(-1);
	}

	if ( atoi(ftpin(NULL)) EQ FTP_RESET )
	{
		if ( g_debug )
		{
			printf("Reset reveived\n");
		}

		ftptimeout(-1);
	}

	if ( atoi(ftpin(NULL)) EQ FTP_LOGONFAIL )
	{
		if ( g_debug )
		{
			printf("Logon failed\n");
		}

		ftptimeout(-1);
	}

	return FALSE;
}



/*******************************************************************************
*
*	getRegexp( exclude )
*
*	Called to read in list of regular expressions into memory.
*
*	The list is read from stdio and is used to exclude certain files.
*
*	The supplied flag indicates whether the regular expression list indicates
*	files to get or to exclude.  ie.  -i switch is used to specify files to
*	exclude and -g to specify files to get.
*
*******************************************************************************/


#define MAX_REGLIST 100

static regex_t *reglist[100];
static int reglistExclude[100];
static int regs = 0;

/*
**	Effectively indicates if the -i or the -g flag is in use
*/
static int dontGetFile;

static void
getRegexp( exclude )
int exclude;
{
	char buff[132];		/* Read regexp into this */
	regex_t r;			/* Just for compilation?! */

	dontGetFile = !exclude;	

	while ( gets(buff) )
	{	
		char *b = buff;

		/*
		**	Allocate
		*/
		reglist[regs] = (regex_t *)
						calloc(1,sizeof(r));	
		reglistExclude[regs] = FALSE;

	
		if ( b[0] EQ '!' )
		{
			b++;
			reglistExclude[regs] = !exclude;
		}
		else
		{
			reglistExclude[regs] = exclude;
		}

		/*
		**	Compile
		*/
		if ( regcomp(reglist[regs], b, REG_NOSUB) != 0 )
		{
			printf("ERROR: Failed reg expression %s",buff);
			exit(EXIT_ARGS);
		}

		regs++;

		if ( g_debug )
		{
			printf("%2d] %s\n",regs,buff);
		}
	}
}

static int
invertedLogic()
{
	return !dontGetFile;
}

/*******************************************************************************
*
*	ignore( fname )
*
*	Returns TRUE if the supplied file is not to be pulled down from the
*	archive.
*
*******************************************************************************/

static int
ignore( fname )
char *fname;
{
	int i;

	for ( i = 0; i < regs; i++ )
	{
		if ( regexec( reglist[i],				/* Pattern */
						fname,strlen(fname),	/* String */
						NULL,					/* pmatch */
						0) != 0 )
		{	/* Not Matched */
			return reglistExclude[i];
		}
	}

	/*
	**	This flag is TRUE if -i is used.  If -g is used then this flag
	**	is FALSE (ie.  Get the file unless indicated otherwise above)
	*/
	return dontGetFile;
}


/*******************************************************************************
*
*	checkforlist(file)
*
*******************************************************************************/

static int
checkforlist( fname )
char *fname;
{
	FILE *f = fopen(fname,"r");

	if ( f )
	{
		fclose(f);
	}

	return (f != NULL);
}

/*******************************************************************************
*
*	dols_fromlist(remoteDir)
*
*	This function effectively runs a special awk command to obtain the 'ls'
*	information that we would normally have retrieved from the remote system.
*
*	This function then feeds the results to checkit()
*
*
*******************************************************************************/

static void
dols_fromlist( remoteDir )
char *remoteDir;
{
	char buff[256];		/* Ls/Pipe output into here */
	char awk[512];
	FILE *f;

	remoteDir += strlen(g_remoteDir);

	if ( remoteDir[0] EQ '/' )
	{	
		remoteDir++;
	}

	/*
	**	Top level directory in the mirror
	*/
	if ( remoteDir[0] EQ '\0' )	
	{
		/*
		**	.. This awk will simply get the first directory listed in the
		**		ls -lR listing file.
		*/
		sprintf(awk,
		"(zcat <%s || uncompress <%s || cat <%s) 2>/dev/null |"\
		"awk '{  if ( $1 == \"\" ) { exit } if ( NR > 2 ) { print } }'",
		g_listfile,g_listfile,g_listfile);
	}
	else
	{
		/*
		**	A directory has been supplied... This awk will get the directory
		**	information from the ls-lR listing file, for the dir supplied.
		*/
		sprintf(awk,
		"x=`echo %s | sed 's/[/]/\\[\\/\\]/g'`;" \
		"(zcat <%s || uncompress <%s || cat <%s) 2>/dev/null |" \
		"awk 'BEGIN { on = 0 } { if ( $1 ~ /^([.][/]'$x'|'$x'):/ ) { on=1 } if ( on && on++ > 2 ) { if ( $1 == \"\" ) { exit } print } }'",
		remoteDir,
		g_listfile,g_listfile,g_listfile);
	}


	if ( g_debug )
	{
		printf("awk: %s\n",awk);	
	}

	f = popen(awk,"r");
	if ( f EQ NULL )
	{
		fprintf(stderr,"FAILED on pipe: %s\n",awk);
		exit(EXIT_PIPE);
	}

	while( fgets(buff,sizeof(buff),f) )
	{
		checkit(buff);
	}

	pclose(f);
}


/******************************************************************************
*	emStart( argc,argv,optind )
*
******************************************************************************/

static void
emStart(	argc,
			argv,
			optind	)
int argc;
char **argv;
int optind;
{
	char *remote = NULL,
		 *local  = NULL;
	FILE *fi, *fo;

	/*
	**	Open an FTP session
	*/
	if ( argc - optind < 5 )
	{
		/*
		**	No username/password supplied, connect as the defaults..
		*/
		char pass[132];

		sprintf(pass,"%s@%s",
				getpwuid(getuid())->pw_name,
				(getenv("HOSTNAME") != NULL) ? getenv("HOSTNAME"):"localhost");

		printf("Connecting to %s as anonymous (password %s)\n",
				argv[optind],pass);

		ftpopen(&fi,&fo,
				argv[optind],"anonymous",pass);
	}
	else
	{
		printf("Connecting to %s as %s\n",
				argv[optind],argv[optind+3]);

		ftpopen(&fi,&fo,
				argv[optind],argv[optind+3],argv[optind+4]);
	}

	/*
	**	Dup, just a kick off.  This process the files in the 
	**	remote directory supplied. Note.  If the cache existed, then we
	**	read it and not run ftpdup() on the lowest level.  We'll just
	**	continue processing the other directories as specified in the
	**	cache.
	*/
	g_remoteDir = argv[optind+1];
	if ( cacheRead() )
	{
		ftpdup(fo,fi, argv[optind+1],argv[optind+2]);
	}


	/*
	**	Once we've called ftpdup() once, any directories in that top
	**	level remote directory will have been stuffed into dirR[]/dirL[] pair.
	**	We now process them in the loop below.  Note also that the loop
	**	below will keep going round and round until no more directories are
	**	to be processed.  ftpdup() MAY add additional directories to these
	**	arrays
	*/
	while ( dirN-- )
	{
		char *remote = (char *) malloc(strlen(dirR[dirN])+1);
		char *local = (char *) malloc(strlen(dirL[dirN])+1);

		strcpy(remote,dirR[dirN]);
		strcpy(local,dirL[dirN]);
		
		ftpdup(fo,fi, remote,local);

		free(remote);
		free(local);
	}

	/*
	**	Quit
	*/
	if ( g_debug )
	{
		printf("QUIT\n");
	}
	ftpout(fo,fi, "quit");


	/*
	**	YAHOO! Finished.  We can safely delete the cache file.
	*/
	if ( unlink(g_cache) )
	{
		fprintf(stderr,"WARNING: Failed to unlink %s\n%s\n",
				g_cache,
				strerror(errno));
	}

	fclose(fo);
	fclose(fi);


	if ( g_bytes )
	{
		printf("EM: FINISH: %ld bytes received\n", g_bytes);
	}
	else
	{
		printf("EM: FINISH: 0 bytes received. Archive upto-date.\n");
	}
}
