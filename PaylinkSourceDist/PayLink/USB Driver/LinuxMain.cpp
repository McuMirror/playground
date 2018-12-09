/*--------------------------------------------------------------------------*\
 * Application Version Define.
\*--------------------------------------------------------------------------*/
char DriverVersion[] = "4.1.12.81" ;

/*--------------------------------------------------------------------------*\
 * System Includes.
\*--------------------------------------------------------------------------*/
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sched.h>
#include <errno.h>

/*--------------------------------------------------------------------------*\
 * Application Includes.
\*--------------------------------------------------------------------------*/
#include "DriverFuncs.h"

#ifdef LinuxPaylink
#include "USBDevices/LocalFuncs.h"
#else
bool            MonitoringComms = false ;
#endif
bool            RunHidden ;
bool            RunVisible ;
char*           Serial = 0 ;
char            LoggingFileName[256] ;
int             LogFileSize = 128 * 1024 ;
char            ConfigFileStr[256] = "Standard.cfg" ;
bool            ShowTraffic ;
int             LatencyParameter ;


#ifdef LinuxPaylink
    class TaskDefinition
        {
      public:
        TaskDefinition*         m_NextTask ;
        char *                  m_Description ;
        volatile unsigned short m_TaskTimer ;
        unsigned short          m_RepeatInterval ;
        unsigned short          m_Priority ;
        bool                    m_Debug ;
        unsigned char           m_TaskFlags ;

        virtual void            Execute(void) = 0 ;

        TaskDefinition() ;
        void PrintfId(const char *format, ...) ;             // This is implemented in Format.cpp
        } ;
    bool        USBUsed = true ;
#else
    bool        USBUsed = false ;
    char*       Stage = "" ;


    // In this build, these are null stubs to keep linker happy.
    bool SetupMergeProcess(void) {return true ;}
    bool MergeProcessInput(int Index, int Value) {return false ;}
    int  MergeProcessOutput(int Index, int Value) {return Value ;}
#endif

pthread_mutex_t  Mutex ;

/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*\
 * Public Global Data.
\*--------------------------------------------------------------------------*/
struct itimerval Timer;

/*--------------------------------------------------------------------------*\
 * Local Data.
\*--------------------------------------------------------------------------*/
static int     MMFile     = -1 ;


/*--------------------------------------------------------------------------*\
 * Externally declared functions.
\*--------------------------------------------------------------------------*/
extern bool CommunicateWithCard (void);
extern void DiagPutChar (char theChar);


static pid_t  ChildPid = -1;
extern char  *GetTime            (void);

/*--------------------------------------------------------------------------*\
 * ShowTrafficHanlder:
 * -------------------
 *
 *
 * Arguments:
 * ----------
 *
 * Return:
 * -------
 *
\*--------------------------------------------------------------------------*/
void
ShowTrafficHanlder                      (int             signum)
{
    if (ChildPid == getpid())
        ShowTraffic = !ShowTraffic;
}


/*--------------------------------------------------------------------------*\
 * ConfigErrorReport:
 * -------------------
 *
 *
 * Arguments:
 * ----------
 *
 * Return:
 * -------
 *
\*--------------------------------------------------------------------------*/
void ConfigErrorReport(char* Message)
{
    static bool PrintHead = true ;
    if (PrintHead)
    {
        PrintHead = false ;
        fprintf (stderr, "[AESCDriver] ERROR: problem with configuration file:\n") ;
    }
    fprintf (stderr, "        %s\n", Message) ;
}

/*--------------------------------------------------------------------------*\
 * DisplayHelpMessage:
 * -------------------
 *
 * Display a help message for the vi_test application.
 *
 * Arguments:
 * ----------
 * none.
 *
 * Returns:
 * --------
 * none.
 *
\*--------------------------------------------------------------------------*/
static int
DisplayHelpMessage                        (void)
{
    printf("AESCDriver [options] <Config File name>\n") ;
    printf("    - Aardvark Embedded Systems %s USB Driver Interface [v%s].\n", PRODUCT_NAME, DriverVersion);
    printf("    Options:\n");
    printf("      -?             Display this help message.     \n");
    printf("      -v             Show Diagnostic information.   \n");
    printf("      -t             Show Traffic to/from interface.\n");
    printf("      -p             Run at high priority.          \n");
    printf("      -s <Serial No> Communicate with given Paylink.\n");
    exit (EXIT_FAILURE);
}

char ShareName[32];
char SavedSerialNumber[12];

pthread_t Driver ;
pthread_t Local ;


//---------------------------------------------------------------------------
void* DriverExecute(void* t)
{
  while (true)
  {
    CurrentDriverState = NotConnected;

    if (!CommunicateWithCard ())
    {
      CurrentDriverState = NotConnected;
      DiagPrintf ("\n\n\nRetrying following failure....");
      Sleep (1000);
    }
  }
  return 0 ;
}


void StartDriverThread(void)
{
  pthread_create(&Driver, NULL, DriverExecute, NULL) ;
}



//-----------------------------------------------------------------------------
static void GetDiags(void)
{
     // Pick up any Staged diagnositcs
     int Logging = 0 ;
     int Result = 0 ;
     static bool FileError = true ;
     char Buffer[BUFFER_SIZE] ;
     unsigned int  BuffInd = 0 ;
     while ((long)(PCInternal->DiagWriteIndex & PCInternal->DiagIndexMask) !=
                         (long)(DiagReadIndex & PCInternal->DiagIndexMask))
        {
        char TheChar = PCInternal->DiagBuffer[DiagReadIndex++ & PCInternal->DiagIndexMask] ;
        if (TheChar == '\f')
            {
            BuffInd = 0 ;
            }

        if (TheChar != 0)
            {
            Buffer[BuffInd++] = TheChar ;
            }
        }


     // Dispose of any Staged diagnositcs
    if (BuffInd)
        {     // Dispose of any Staged diagnositcs
        if (LoggingFileName[0])
            {
            if (Logging == 0)
                {
                Logging = open(LoggingFileName,
                                O_RDWR | O_APPEND | O_CREAT,
                                S_IREAD | S_IWRITE) ;
                }
            if (Buffer[0] == '\f')
                {
                Result = write(Logging, "\n\n\n\n\n", 5) ;
                Result = write(Logging, Buffer + 1, BuffInd - 1) ;
                }
            else
                {
                Result = write(Logging, Buffer, BuffInd) ;
                }

           if ((Result < 0) && FileError)
                {
                FileError = false ;
                fprintf(stderr, "Error writing %s\n", LoggingFileName) ;
                }
            }


        if (RunVisible)
            {
            Buffer[BuffInd] = '\0' ;
            if (Buffer[0] == '\f')
                {
                printf("\n\n\n\n\n") ;
                fputs(Buffer + 1, stdout) ;
                }
            else
                {
                fputs(Buffer, stdout) ;
                }
            fflush(stdout) ;
            }
        }

    if (Logging)
        {
        // Now check the size of the file - we don't wnat to let this get too big as we want to be
        // able to e-mail it!
        struct stat Details;
        fstat(Logging , &Details);

       if (Details.st_size > LogFileSize)
            {
            Result = write(Logging, "\r\nMaximum file size reached\r\n", 29) ;
            close(Logging) ;

            // Delete the old file
            char BackupFile[1024] ;
            strcpy(BackupFile, LoggingFileName) ;
            strcat(BackupFile, ".old") ;
            remove(BackupFile) ;

            // rename the current one
            rename(LoggingFileName, BackupFile) ;

            // and restart it
            Logging = open(LoggingFileName,
                           O_RDWR | O_CREAT | O_TRUNC,
                           S_IREAD | S_IWRITE) ;
            }

        close(Logging) ;
        }
}






/*--------------------------------------------------------------------------*\
 * AbortHandler:
 * -------------
 *
 *
 * Arguments:
 * ----------
 *
 * Return:
 * -------
 *
\*--------------------------------------------------------------------------*/
void CloseOnExit() {
    shm_unlink(ShareName);
}

void
AbortHandler                            (int             signum)
{
    extern char* Stage ;

    fprintf(stderr, "Warning: Caught signal <%d> while ", signum) ;
    printf("Warning: Caught signal <%d> while ", signum) ;
#ifdef LinuxPaylink
    extern TaskDefinition* TheTask ;
    if (TheTask)
        {
        fprintf(stderr, "executing %s ", TheTask->m_Description) ;
        printf("executing %s ", TheTask->m_Description) ;
        }
#endif
    fprintf(stderr, "in %s\r\n", Stage) ;
    printf("in %s\r\n", Stage) ;

    //if (CloseOnExit)
        {
        printf("Calling Close function\n") ;
        CloseOnExit() ;
        }

    printf("Remaining diagnostics ....\r\n") ;

    GetDiags() ;

    printf("Exit\r\n") ;
    fflush(stdout) ;
    fflush(stderr) ;
    exit(0);
}




/*--------------------------------------------------------------------------*\
 * main:
 * -----
 *
 *
 * Arguments:
 * ----------
 *
 * Return:
 * -------
 *
\*--------------------------------------------------------------------------*/
int
main (int argc, char *argv[])
{
    char         tmp[256]   = "";
    int          option     = -1;
    int          num        = -1;
    int          highpri    =  0;
    int          pagesize   = getpagesize();
    /*struct  sched_param  sched;*/

    /*-- Catch signals so we exit cleanly -----------------------------*/
    signal(SIGHUP,    AbortHandler);    signal(SIGINT,  AbortHandler);
    signal(SIGQUIT,   AbortHandler);    signal(SIGILL,  AbortHandler);
    signal(SIGTRAP,   AbortHandler);    signal(SIGABRT, AbortHandler);
    signal(SIGBUS,    AbortHandler);    signal(SIGFPE,  AbortHandler);
    signal(SIGKILL,   AbortHandler);    signal(SIGUSR1, SIG_IGN);
    signal(SIGSEGV,   AbortHandler);    signal(SIGUSR2, AbortHandler);
    signal(SIGPIPE,   AbortHandler);    signal(SIGALRM, AbortHandler);
    signal(SIGTERM,   AbortHandler);    /* SIGCHLD caught with waitpid */
    signal(SIGCONT,   AbortHandler);    signal(SIGSTOP, AbortHandler);
    signal(SIGTSTP,   AbortHandler);    signal(SIGTTIN, AbortHandler);
    signal(SIGTTOU,   AbortHandler);    signal(SIGURG,  AbortHandler);
    signal(SIGXCPU,   AbortHandler);    signal(SIGXFSZ, AbortHandler);
    signal(SIGVTALRM, AbortHandler);    signal(SIGPROF, AbortHandler);
    signal(SIGWINCH,  SIG_IGN);         signal(SIGIO,   AbortHandler);
    /*signal(SIGPWR,    AbortHandler);*/ signal(SIGSYS,  AbortHandler);

    /*-- Ensure this is being executed by the super user --------------*/
    if (geteuid() != 0)
    {   fprintf (stderr, "ERROR: Application must be run as root [SuperUser] not %d\r\n", geteuid());
        fflush  (stderr);
        exit (EXIT_FAILURE);
    }

    /*-- Decode Command Line Arguments Supplied -----------------------*/
    while ((option=getopt(argc,argv,"?vtps:l:")) != -1)
    {   switch(option)
        { case 'v': RunVisible  = 1; break;
          case 't': ShowTraffic = 1; break;
          case 'p': highpri     = 1; break;
          case 's': Serial      = optarg ; break;
          case '?':
          default : DisplayHelpMessage();
        }
    }

    if (optind != argc)
    {
        strcpy(ConfigFileStr, argv[optind]) ;
    }

#ifdef LinuxPaylink
    printf("Paylink driver") ;
#else
    printf("AESCDriver") ;
#endif
    printf(", [v%s]", DriverVersion);
    printf(", %s %s", __DATE__, __TIME__);
    printf(", Configuration file: %s\n", ConfigFileStr) ;

    if (!ProcessConfig(ConfigFileStr))
    {                                           // We run a completely different "app", just to display the error
        fflush  (stderr);
        exit (EXIT_FAILURE);
    }


    /*-- Determine if a serial number has been given ------------------*/
    if (Serial != NULL)
    {
        strcpy(SavedSerialNumber, Serial) ;
        strcpy(ShareName, SHARED_NAME) ;
        if (SavedSerialNumber[0])
        {
            strcat(ShareName, SavedSerialNumber) ;
        }
    }
    else
    {
        strcpy(ShareName, SHARED_NAME) ;
    }


    /*-- Set up the shared Memory -------------------------------------*/
    MMFile = shm_open(ShareName, O_RDWR | O_CREAT,  0777 );
    if (MMFile == -1)
    {   fprintf (stderr, "[USBDriver] ERROR: Creating shared memory segment (shm_open) for file:%s, with err:%s.\r\n", ShareName, strerror(errno)) ;
        fflush  (stderr);
        exit(0) ;
    }

    /*-- Ensure permissions are set correctly on the shared memory ----*/
    sprintf (tmp, "/dev/shm/%s", ShareName);
    chmod (tmp, 0777);

    /*-- Calculate number of pages required ---------------------------*/
    num = abs(((EXTENDED_SHARED_SIZE) / pagesize));
    if ((num * pagesize) < (EXTENDED_SHARED_SIZE))
        num+=1;

    if ((ftruncate (MMFile, (num * pagesize))) == -1)
    {   fprintf (stderr, "[USBDriver] ERROR: Creating shared memory segment (ftruncate) with size:%d for file:%s, with err:%s.\r\n", num*pagesize, ShareName, strerror(errno)) ;
        fflush  (stderr);
        exit(0) ;
    }

    /*-- Set up the mirrors -------------------------------------------*/
    Base = (u_char *)mmap( 0, num*pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, MMFile, 0);
    if (Base == (void *) -1)
    {   fprintf (stderr, "[USBDriver] ERROR: Creating shared memory segment (mmap).\r\n") ;
        fflush  (stderr);
        exit(0) ;
    }
    SharedMemoryBase = (int *)Base ;

    /*-- Driver (re)start - so reset everything! ----------------------*/
    memset ((void *)SharedMemoryBase, 0, 8192);
    memset ((void *)Mirror,           0, 8192);

    /*-- Set up the diagnostics ---------------------------------------*/
    PCInternal                = PC_INTERNAL_BLOCK (Base);
    PCInternal->DiagIndexMask = BUFFER_SIZE - 1;
    DiagReadIndex             = PCInternal->DiagWriteIndex;

    /*-- Increase Priority of Driver Process --------------------------*/
    if (highpri)
    {   /*
	if (!sched_getparam(0, &sched))
        {   sched.sched_priority = sched_get_priority_max (SCHED_RR) - 10;
            sched_setscheduler (0, SCHED_RR, &sched);
        }*/
    }



    pthread_mutex_init(&Mutex, NULL) ;
    if (pthread_mutex_lock(&Mutex))
    {   fprintf (stderr, "[USBDriver] ERROR: Cant establish Mutex\r\n") ;
        fflush  (stderr);
        exit(0) ;
    }
    pthread_mutex_unlock(&Mutex) ;


                        // Everything OK - so run main application(s)

    /*-- Install signal handler to enable / disable ShowTraffic -------*/
    ChildPid = getpid();
    (void)signal(SIGUSR1, ShowTrafficHanlder);


    #ifdef LinuxPaylink
    if (!UsingDongle)
      if (!UsingDongle &&
          !UsingLite && !UsingPiHat)
    {                 // If we're not using a dongle, Lite or PiHat, we're talking to a Paylink
    #endif
      StartDriverThread() ;
    #ifdef LinuxPaylink
    }
    if ((USBConfigCount > 0)
     || (UsingLite || UsingPiHat))
    {                 // If we have any local devices, then we need to run them
      StartLocalThread() ;
    }
    #endif


    while (true)
    {
        Sleep(20) ;
        GetDiags() ;
    }
    return 0;
}
