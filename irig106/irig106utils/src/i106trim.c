/*==========================================================================

  FFTRIM.C - A program to trim Ver 3 data files based on start and/or
  stop times.

   Copyright (c) 2005
   Georgia Tech Research Corporation
   Atlanta, GA 30332-0415
  
   All rights reserved.
  
   Redistribution and use in source and binary forms, with or without 
   modification, are permitted provided that the following conditions are 
   met:
  
     * Redistributions of source code must retain the above copyright 
       notice, this list of conditions and the following disclaimer.
  
     * Redistributions in binary form must reproduce the above copyright 
       notice, this list of conditions and the following disclaimer in the 
       documentation and/or other materials provided with the distribution.
  
     * Neither the name of the Georgia Tech Applied Research Corporation 
       nor the names of its contributors may be used to endorse or promote 
       products derived from this software without specific prior written 
       permission.
  
   This software is provided by the copyright holders and contributors 
   "as is" and any express or implied warranties, including, but not 
   limited to, the implied warranties of merchantability and fitness for 
   a particular purpose are disclaimed. In no event shall the copyright 
   owner or contributors be liable for any direct, indirect, incidental, 
   special, exemplary, or consequential damages (including, but not 
   limited to, procurement of substitute goods or services; loss of use, 
   data, or profits; or business interruption) however caused and on any 
   theory of liability, whether in contract, strict liability, or tort 
   (including negligence or otherwise) arising in any way out of the use 
   of this software, even if advised of the possibility of such damage.

  Created 8 July 1998 by Bob Baggerman

  $RCSfile: i106trim.c,v $
  $Date: 2006-01-10 19:55:52 $
  $Author: irig106 $
  $Revision: 1.1 $

  ==========================================================================*/

#include <stdio.h>
#include <conio.h>
//#include <alloc.h>
//#include <mem.h>
#include <time.h>
#include <dos.h>

#include "blockio.h"
#include "ffdata.h"

/*
 * Macros and definitions
 * ----------------------
 */

#define MAJOR_VERSION  "03"
#define MINOR_VERSION  "02"

#if !defined(bTRUE)
#define bTRUE   (1==1)
#define bFALSE  (1==0)
#endif

#define SECS(hr,min,sec) (hr*60L*60L + min*60L + sec)

#define READ_BUFF_SIZE       100000

/*
 * Data structures
 * ---------------
 */


/*
 * Module data
 * -----------
 */

/*
 * Function prototypes
 * -------------------
 */

void vStats(char *szFileName);
void vUsage(void);



/* ======================================================================== */



int main (int argc, char *argv[])
  {
  SuFfdHandle      tFFD_In;    // Input data file handle
  SuFfdHandle      tFFD_Out;   // Output data file handle
  unsigned short   usMsgType;
  SuFfdrTime       tTime;
  unsigned short   usBuffSize;
  void *           pvBuff = NULL;

#if   defined(__BORLANDC__)
  struct date      tPCDate;
  struct time      tPCTime;
#else
  struct tm       *ptPCDateTime;
#endif

  int              iArgIdx;
  int              iInHour;
  int              iInMin;
  int              iInSec;

  long             lWriteMsgs = 0L;

  long             lStartTimeOffset = -1L;
  long             lStopTimeOffset  = -1L;
  long             lStartTime = -1L;
  long             lStopTime  = -1L;

  int              iStatus;


  // Make sure things stay on UTC
  putenv("TZ=GMT0");
  tzset();

/*
 * Process the command line arguments
 */

  if (argc < 2) {
    vUsage();
    return 1;
    }

  if (argc == 2) {
    vStats(argv[1]);
    return 0;
    }

  for (iArgIdx=3; iArgIdx<argc; iArgIdx++) {

    switch (argv[iArgIdx][0]) {

      case '+' :
        iStatus = sscanf(argv[iArgIdx],"+%d:%d:%d",&iInHour,&iInMin,&iInSec);
        if (iStatus != 3) {
          vUsage();
          return 1;
          }
        lStartTimeOffset = SECS(iInHour, iInMin, iInSec);
        break;

      case '-' :
        iStatus = sscanf(argv[iArgIdx],"-%d:%d:%d",&iInHour,&iInMin,&iInSec);
        if (iStatus != 3) {
          vUsage();
          return 1;
          }
        lStopTimeOffset = SECS(iInHour, iInMin, iInSec);
        break;

      } /* end command line arg switch */
    } /* end for all arguments */

/*
 * Open the input file
 */

  // Open the input data file
  usBuffSize = 0;
  iStatus = FfdOpen(&tFFD_In, argv[1], EnBioMode_READ, usBuffSize);
  if (iStatus != EnFfdStatus_OK) {
    printf ("\nERROR %d opening input file\n",iStatus);
    return 1;
    }

  // Make the input data buffer
  pvBuff = malloc(READ_BUFF_SIZE);

  // Read the first data packet to get the start time
  usBuffSize = (unsigned short)READ_BUFF_SIZE;
  iStatus = FfdReadNextMessage(&tFFD_In, &usMsgType, &tTime, &usBuffSize, pvBuff);

  // Set start and stop times
#if   defined(__BORLANDC__)
  unixtodos(tTime.lTimeMS, &tPCDate, &tPCTime);
  tPCTime.ti_hour = 0;
  tPCTime.ti_min  = 0;
  tPCTime.ti_sec  = 0;
  if (lStartTimeOffset != -1) {
    lStartTime = dostounix(&tPCDate, &tPCTime) + lStartTimeOffset;
    } // end set start time
  if (lStopTimeOffset != -1) {
    if (lStopTimeOffset < lStartTimeOffset)
      lStopTimeOffset += 60L*60L*24L;
    lStopTime = dostounix(&tPCDate, &tPCTime) + lStopTimeOffset;
    } // end set stop time
#else
// Watch out for time zone in mktime();
  ptPCDateTime = gmtime(&tTime.lTimeMS);
  ptPCDateTime->tm_hour = 0;
  ptPCDateTime->tm_min  = 0;
  ptPCDateTime->tm_sec  = 0;
  if (lStartTimeOffset != -1) {
    lStartTime = mktime(ptPCDateTime) + lStartTimeOffset;
    } // end set start time
  if (lStopTimeOffset != -1) {
    if (lStopTimeOffset < lStartTimeOffset)
      lStopTimeOffset += 60L*60L*24L;
    lStopTime = mktime(ptPCDateTime) + lStopTimeOffset;
    } // end set stop time

#endif

  // Reset the data file to the beginning
  FfdClose(&tFFD_In);
  FfdOpen(&tFFD_In, argv[1], EnBioMode_READ, usBuffSize);

/*
 * Open the output file
 */
  iStatus = FfdOpen(&tFFD_Out, argv[2], EnBioMode_OVERWRITE, tFFD_In.wMaxBuffSize);
  if (iStatus != EnFfdStatus_OK) {
    printf ("\nERROR %d opening output file\n",iStatus);
    return 1;
    }

/*
 * Print out some info and get started
 */

  printf("Input Data File  '%s'\n",argv[1]);
  printf("Output Data File '%s'\n",argv[2]);

/*
 * Read data packets until EOF
 */

  lWriteMsgs = 0;

  // Read and write header (i.e. MsgType < 0x1000) messages
  while (bTRUE) 
    {

    // Read data packet and look for trouble
    usBuffSize = (unsigned short)READ_BUFF_SIZE;
    iStatus = FfdReadNextMessage(&tFFD_In, &usMsgType, &tTime, &usBuffSize, pvBuff);

    if (iStatus == EnBioStatus_EOF) break;
    if (iStatus != EnBioStatus_OK)  continue;

    // If header message write it out
    if (usMsgType <= 0x0fff) 
      {
      iStatus = FfdWriteNextMessage(&tFFD_Out, usMsgType, &tTime, usBuffSize, pvBuff);
      lWriteMsgs++;
      } // end if write header message

    // Not header so backup and exit loop
    else
      {
      iStatus = FfdPrevMsg(&tFFD_In);
      break;
      }

    } // end while read/write headers

  // Try jumping to the start time
  if (lStartTimeOffset != -1) 
    {
    tTime.lTimeMS = lStartTime;
    tTime.sTimeLS = 0;
    iStatus = FfdSeekToTime (&tFFD_In, &tTime);
//    printf("FfdSeekToTime() = %d\n", iStatus);
    }

  // Now loop through packets
  while (bTRUE) {

    // Read data packet and look for trouble
    usBuffSize = (unsigned short)READ_BUFF_SIZE;
    iStatus = FfdReadNextMessage(&tFFD_In, &usMsgType, &tTime, &usBuffSize, pvBuff);

    if (iStatus == EnBioStatus_EOF) break;
    if (iStatus != EnBioStatus_OK)  continue;

    // Only check time if not a header message
    if (usMsgType > 0x0fff) {
      // Check the time and write it if time OK
      if ((lStartTime != -1) && (lStartTime >  tTime.lTimeMS)) continue;
      if ((lStopTime  != -1) && (lStopTime  <= tTime.lTimeMS)) break;
      } // end if not header message

    iStatus = FfdWriteNextMessage(&tFFD_Out, usMsgType, &tTime, usBuffSize, pvBuff);
    lWriteMsgs++;

    } // end while read/write

/*
 * Print some stats, close the files, and get outa here
 */

  printf("Packets Written %ld\n",lWriteMsgs);

  FfdClose(&tFFD_In);
  FfdClose(&tFFD_Out);


  return 0;
  }



/* ------------------------------------------------------------------------ */

void vStats(char *szFileName)
  {
  SuFfdHandle      tFFD;
  unsigned short   usMsgType;
  SuFfdrTime       tTime;
  unsigned short   usBuffSize;
  void *           pvBuff = NULL;
  int              iStatus;

  // Open the data file
  usBuffSize = 0;
  iStatus = FfdOpen(&tFFD, szFileName, EnBioMode_READ, usBuffSize);
  if (iStatus != EnFfdStatus_OK) {
    printf ("\nERROR %d opening input file\n",iStatus);
    return;
    }

  // Make the input data buffer
  usBuffSize = tFFD.wMaxBuffSize;
  pvBuff     = malloc (usBuffSize);

  // Read the first data packet to get the start time
  usBuffSize = tFFD.wMaxBuffSize;
  iStatus = FfdReadNextMessage(&tFFD, &usMsgType, &tTime, &usBuffSize, pvBuff);
  printf("Start  %s",  ctime(&tTime.lTimeMS));

  // Read the last data packet to get the end time
  BioLastBuff(&tFFD.suBioHandle);
  usBuffSize = tFFD.wMaxBuffSize;
  iStatus = FfdReadNextMessage(&tFFD, &usMsgType, &tTime, &usBuffSize, pvBuff);
  printf("End    %s",  ctime(&tTime.lTimeMS));

  free(pvBuff);

  return;
  }



/* ------------------------------------------------------------------------ */

void vUsage(void)
  {
  printf("FFTRIM "MAJOR_VERSION"."MINOR_VERSION" "__DATE__" "__TIME__"\n");
  printf("Usage: fftrim <infile> <outfile> [+hh:mm:ss] [-hh:mm:ss]\n");
  printf("  +hh:mm:ss - Start copy time\n");
  printf("  -hh:mm:ss - Stop copy time\n");
  printf("Or:    fftrim <infile> to get stats\n");
  return;
  }



