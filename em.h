/*
 * Source copyright Jag Software Limited of England.  Modifications may
 * be made as long as this and other copyright notices (and or messages)
 * are retained in this file.
 *
 * (Why not do the sensible thing and email ME with the source code changes
 *  you would like to implement?)
 *
 *
 * $Source: /home/cvs/magick/Magick-I/em.h,v $
 *
 * $Log: em.h,v $
 * Revision 4.0.0.1  1998/10/17 13:04:53  root
 *
 *
 * Revision 1.4.2.1  1998/10/17 02:28:03  root
 *
 *
 * Revision 1.5  1997/06/29 22:32:15  dev
 * Added g_newer handling, Some minor bug fixes and
 * Mods to enable compilation on more Unix platforms.
 *
 * Revision 1.4  1997/04/03 03:12:57  dev
 * Added defs of invertedLogic(), emStart() and compareLocal()
 * enhancemnets.
 *
 * Revision 1.3  1997/02/11 20:51:41  dev
 * *** empty log message ***
 *
 * Revision 1.2  1996/11/14 02:16:56  dev
 * ftpcwd() returns int, getRegexp takes int
 * Added g_keepAll and g_bytes
 *
 * Revision 1.1  1996/07/10  23:38:13  root
 * Initial revision
 *
 *
*/
static char rcsId_em_h[] = "$Id: em.h,v 4.0.0.1 1998/10/17 13:04:53 root Exp $";



#define FILE_COL 55

#ifndef FALSE
#	define FALSE (0)
#endif

#ifndef TRUE
#	define TRUE (!FALSE)
#endif



#ifndef MAX_PATH
#	define MAX_PATH 255
#endif


#define EQ ==


/*
**	Remote file information.  This is then used to compare against local files
**	etc.
*/
typedef struct
{
	char *fname;	/* File/Dir name */
	char *lnk;		/* Link name (if link) */
	time_t date;	/* File last change date */
	char typ;		/* Type of file */
	long size;		/* Size of file in bytes */
} FileInfo;



#define TYP_DIR	'd'
#define TYP_LNK	'l'
#define TYP_REG	'-'



static char *ftpin( );
static char *ftppwd( );
static void ftpout( );
static void ftppipe( );
static void ftpopen( );
static int ftpcwd( );
static void ftpdup( );
static void ftpget( );


static int cacheRead();
static void cacheWrite();
static char *skip( );
static void ftptimeout( );
static void usage();
static void getRegexp( );
static int ignore();
static void dols_fromlist( );
static int invertedLogic();





#define CHECKERR	if ( errno )											\
					{														\
						fprintf(stderr,"ERROR:%d:%s\n",__LINE__,			\
														strerror(errno));	\
						abort();											\
					}


#define FTP_CONNECT		220
#define FTP_PASSWORD	331
#define FTP_LOGON		230
#define FTP_LOGONFAIL	530
#define FTP_PWD			257
#define FTP_PORTCOMMAND	200
#define FTP_OPEN		150
#define FTP_TRANSFERED	226
#define FTP_CWD			250
#define FTP_FILERROR	550
#define FTP_RECEIVED	432

#define FTP_BYE			221
#define FTP_CLOSE		421
#define FTP_RESET		426




static FileInfo *findFile( );
static void compareLocal();
static void removeDir( );
static time_t getTime( );
static void setTime( );
static int closeSession( );
static int checkforlist( );
static void emStart( );



/*
**	Exit codes
*/
#define EXIT_QUIT		1		/* Quit command actioned */
#define EXIT_ARGS		2		/* Argument error */
#define EXIT_GETDATE	3		/* Getdate() failed.  DATEMSK not set? */


#define EXIT_FTPDOWN	16		/* Ftp just went down */
#define EXIT_SIGNON		17		/* Unable to signon onto remote system */
#define EXIT_REMOTECD	18		/* CD command at remote host failed */

#define EXIT_LOCALCD	32		/* CD command at remote host failed */
#define EXIT_LINK		33		/* Symbolic link failed */
#define EXIT_OPENDIR	34		/* Failed to open a directory */
#define EXIT_STAT		35		/* Stat on a file failed */
#define EXIT_UNLINK		36		/* Failed to unlink */

#define EXIT_FDOPEN		128		/* fdopen() failure */
#define EXIT_PIPE		129		/* Pipe failure */
#define EXIT_FORKEXEC	130		/* fork() or exec() failed */



/*
**	Globals
*/
static int g_debug = FALSE;		/* Enable debug ? */

static int g_timeout = 180;		/* Command timeout. Default is 3 mins */

static unsigned int g_k = 120;	/* 1k should transfer in 2 minutes atleast */

static char g_cache[MAX_PATH+1];/* Cache file */

static int g_preserve = FALSE;	/* Preserve original times on files */

static int g_size = FALSE;		/* Do also compare size to local files, if
								   different then also retrieve file */

static int g_stamp = FALSE;		/* Force timestamps onto local files if
								   newer than remote file? */

static int g_bogus = FALSE;		/* When set to true, files are not
								   retrieved, empty files created instead */

static int g_keepAll = FALSE;	/* By default, non-mirrored files are removed
								   if present on the local system. This flag
  								   prevents local file deletion */

static char g_listfile[1024];	/* Full path spec to the listing file contain
									date/size info for all files on remote 
									system  (eg. ls-lR.gz) */

static char *g_remoteDir;		/* This is the remote directory being mirrored.
								   We need this information for when we use
								   a 'listingfile'.  We'll be searching in the
								   listing file for directory names excluding
								   g_remoteDir from the start.  ie. if we 
								   called em with the remote direcoty as
								   /pub/mirror/slackware, then that directory
								   would be seen as just '' and a subdirectory
								   like /pub/mirror/slackware/abc would be
								   seen as 'abc' */

static int g_openPort = 21;		/* FTP Port to open is defaulted to 21, this
								   can be specified at run-time if a different
								   port is required. */

static unsigned long g_bytes = 0;/*Number of completed bytes transferred */

static char *g_newer = NULL;	/* Only pull files newer than this supplied
								   date. */
