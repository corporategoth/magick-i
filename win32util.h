#ifndef _WIN32UTIL_H
#define _WIN32UTIL_H
#ifdef WIN32

#include <sys/stat.h>

#define S_ISDIR(m)  ((m) & _S_IFDIR)

#define SIGALRM 13
#define SIGUSR1 18
extern void alarm(int seconds);
#define sleep(x) Sleep(x*1000);

#endif /*MSC_VER*/
#endif /*_WIN32UTIL_H*/
