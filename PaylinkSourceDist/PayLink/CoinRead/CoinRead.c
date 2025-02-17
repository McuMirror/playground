/*--------------------------------------------------------------------------*\
 * System Includes.
\*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>

/*--------------------------------------------------------------------------*\
 * AES Includes.
\*--------------------------------------------------------------------------*/
#include "Aesimhei.h"

/*--------------------------------------------------------------------------*\
 * AbortHandler:
 * -------------
 *
 * This ensures the AESAccess Shared library destructor function is called.
 * If the application exits normaly the destructor is always called, however
 * if the application receives a signal (SIGINT...) the destructor is not
 * called, that is why we need this signal handler (force exit() call).
 *
 * Arguments:
 * ----------
 *
 * Return:
 * -------
 *
\*--------------------------------------------------------------------------*/
void
AbortHandler(int signum){ exit(0); }

char* SerialNo = NULL ;

/*--------------------------------------------------------------------------*\
 * SetCoinLevel:
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

void printAcceptorCoin(AcceptorCoin* acceptorCoin) {
    printf("    -----\n");
    printf("    Value: %d\n", acceptorCoin->Value);
    printf("    Inhibit: %d\n", acceptorCoin->Inhibit);
    printf("    Count: %d\n", acceptorCoin->Count);
    printf("    Path: %d\n", acceptorCoin->Path);
    printf("    PathCount: %d\n", acceptorCoin->PathCount);
    printf("    PathSwitchLevel: %d\n", acceptorCoin->PathSwitchLevel);
    printf("    DefaultPath: %c\n", acceptorCoin->DefaultPath);
    printf("    FutureExpansion: %c\n", acceptorCoin->FutureExpansion);
    printf("    HeldInEscrow: %c\n", acceptorCoin->HeldInEscrow);
    printf("    FutureExpansion2: %c\n", acceptorCoin->FutureExpansion2);
    printf("    CoinName: %s\n", acceptorCoin->CoinName);
    printf("    -----\n");
}

void printAcceptorBlock(AcceptorBlock* acceptorBlock) {
    printf("---AcceptorBlock---Begin---\n");
    printf("Unit: %d\n", acceptorBlock->Unit);
    printf("Status: %d\n", acceptorBlock->Status);
    printf("NoOfCoins: %d\n", acceptorBlock->NoOfCoins);
    printf("InterfaceNumber: %d\n", acceptorBlock->InterfaceNumber);
    printf("UnitAddress: %d\n", acceptorBlock->UnitAddress);
    printf("DefaultPath: %d\n", acceptorBlock->DefaultPath);
    printf("BarcodesStacked: %d\n", acceptorBlock->BarcodesStacked);

    printf("Currency: %s\n", acceptorBlock->Currency);
    for(unsigned int i = 0; i<acceptorBlock->NoOfCoins; ++i) {
        printAcceptorCoin(&acceptorBlock->Coin[i]);
    }
    printf("SerialNumber: %d\n", acceptorBlock->SerialNumber);
    printf("Description: %s\n", acceptorBlock->Description);
    printf("EscrowBarcodeHere: %d\n", acceptorBlock->EscrowBarcodeHere);
    printf("---AcceptorBlock---End-----\n");
}


void
SetCoinLevel                            (int             Level)
{
    AcceptorBlock AcceptorDetails ;
    int AcceptorNo ;
    int CoinNo ;

    for (AcceptorNo = 0 ;
         ReadAcceptorDetails(AcceptorNo, &AcceptorDetails) ;
         ++AcceptorNo)
    {
        printAcceptorBlock(&AcceptorDetails);

        for (CoinNo = 0 ; CoinNo < AcceptorDetails.NoOfCoins ; ++CoinNo)
        {
           if (AcceptorDetails.Coin[CoinNo].Value == 100)
           {
               AcceptorDetails.Coin[CoinNo].Path = 1 ;
               AcceptorDetails.Coin[CoinNo].PathSwitchLevel = Level +
                                          AcceptorDetails.Coin[CoinNo].PathCount;

               printf("Acceptor %d, Coin %d, Current count on path %d\n",
                       AcceptorNo, CoinNo, AcceptorDetails.Coin[CoinNo].PathCount) ;

               WriteAcceptorDetails(AcceptorNo, &AcceptorDetails) ;
           }
        }
    }
}

/*--------------------------------------------------------------------------*\
 * AcceptCurrencyExample:
 * ---------------------
 *
 *
 * Arguments:
 * ----------
 *
 * Return:
 * -------
 *
\*--------------------------------------------------------------------------*/
void AcceptCurrencyExample (int NoOfChanges)
{
    long LastCurrencyValue ;
    long NewCurrencyValue  ;
    long OpenStatus ;

    if (SerialNo)
    {
        printf("Using specific Paylink \"%s\", ", SerialNo) ;
        OpenStatus = OpenSpecificMHE(SerialNo) ;
    }
    else
    {
        OpenStatus = OpenMHE() ;
    }


    if (OpenStatus != 0)
    {
        printf("IMHEI open failed - %ld\n", OpenStatus) ;
        return ;
    }

    /*-- Then the open call was successful ----------------------------*/
    /*-- Currency acceptance is currently disabled --------------------*/
    LastCurrencyValue = CurrentValue() ;

    printf("Initial currency accepted = %ld\n", LastCurrencyValue) ;

    SetCoinLevel(NoOfChanges / 2) ;

    EnableInterface() ;

    printf("Processing %d change events\n", NoOfChanges) ;

    while (NoOfChanges > 0)
    {   Sleep(100) ;
        NewCurrencyValue = CurrentValue() ;
        if (NewCurrencyValue != LastCurrencyValue)
        {   /*-- More money has arrived (we do not care where from) ---*/
             printf("The user has just inserted %ld\n",
                         NewCurrencyValue - LastCurrencyValue) ;
             LastCurrencyValue = NewCurrencyValue ;
             --NoOfChanges ;
        }
    }
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
int main (int ac, char **av)
{
    int Count = 2 ;
    int option ;
    /*-- Catch signals so we exit cleanly -----------------------------*/
    (void)signal(SIGHUP,    AbortHandler); (void)signal(SIGINT,  AbortHandler);
    (void)signal(SIGQUIT,   AbortHandler); (void)signal(SIGILL,  AbortHandler);
    (void)signal(SIGTRAP,   AbortHandler); (void)signal(SIGABRT, AbortHandler);
    (void)signal(SIGBUS,    AbortHandler); (void)signal(SIGFPE,  AbortHandler);
    (void)signal(SIGKILL,   AbortHandler); (void)signal(SIGUSR1, AbortHandler);
    (void)signal(SIGSEGV,   AbortHandler); (void)signal(SIGUSR2, AbortHandler);
    (void)signal(SIGPIPE,   AbortHandler); (void)signal(SIGALRM, AbortHandler);
    (void)signal(SIGTERM,   AbortHandler); (void)signal(SIGCHLD, AbortHandler);
    (void)signal(SIGCONT,   AbortHandler); (void)signal(SIGSTOP, AbortHandler);
    (void)signal(SIGTSTP,   AbortHandler); (void)signal(SIGTTIN, AbortHandler);
    (void)signal(SIGTTOU,   AbortHandler); (void)signal(SIGURG,  AbortHandler);
    (void)signal(SIGXCPU,   AbortHandler); (void)signal(SIGXFSZ, AbortHandler);
    (void)signal(SIGVTALRM, AbortHandler); (void)signal(SIGPROF, AbortHandler);
    (void)signal(SIGWINCH,  AbortHandler); (void)signal(SIGIO,   AbortHandler);
    /*(void)signal(SIGPWR,    AbortHandler); (void)signal(SIGSYS,  AbortHandler);*/


    /*-- Decode Command Line Arguments Supplied -----------------------*/
    while ((option=getopt(ac,av,"?c:s:")) != -1)
    {
        switch(option)
        {
            case 'c': Count     = atoi(optarg) ; break;
            case 's': SerialNo  = optarg       ; break;
        }
    }

    AcceptCurrencyExample(Count) ;
    return 1 ;
}
