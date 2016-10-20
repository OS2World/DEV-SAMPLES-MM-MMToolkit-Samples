/*static char *SCCSID = "@(#)cdmccomn.c 13.2 92/04/10";*/
/****************************************************************************/
/*                                                                          */
/* SOURCE FILE NAME:  CDMCCOMN.C                                            */
/*                                                                          */
/* DESCRIPTIVE NAME:  CD MCD PARSER                                         */
/*                                                                          */
/* COPYRIGHT:  (c) IBM Corp. 1992                                           */
/*                                                                          */
/* FUNCTION:  This file contains the common functions between the CD MCD    */
/*            and the IBM CD-ROM Drive VSD.                                 */
/*                                                                          */
/*                                                                          */
/* FUNCTIONS:                                                               */
/*       parse_DevParm - Parses the device specific parameter.              */
/*       get_token     - gets next token and terminates it with a null.     */
/*                                                                          */
/*                                                                          */
/* MODIFICATION HISTORY:                                                    */
/* DATE      DEVELOPER         CHANGE DESCRIPTION                           */
/* 01/27/92  Garry Lewis       original creation                            */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

#include <os2.h>
#include <string.h>

/* prototypes */
static CHAR * get_token(CHAR *);

/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  parse_DevParm             DEVELOPER:  Garry Lewis      */
/*                                                                          */
/* DESCRIPTIVE NAME:  Parse Device Parameter                                */
/*                                                                          */
/* FUNCTION:  Parses the Device Specific Parameter that is supplied on      */
/*            an MCI_OPEN message.  A pointer to the string to be parsed    */
/*            is supplied, changed to upper case and sets NULLS after the   */
/*            drive and model tokens.  Pointers to these tokens are         */
/*            returned.  The pointer points to a NULL string if tokens are  */
/*            not found.                                                    */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      CHAR *pSource  -- Pointer to input string to be parsed.             */
/*      CHAR **ppDrive -- Pointer to Drive token.                           */
/*      CHAR **ppModel -- Pointer to Model token.                           */
/*                                                                          */
/* EXIT CODES:  None                                                        */
/*                                                                          */
/* NOTES:  WARNING!! The input string will be modified.                     */
/*         strtok() would have been cleaner to use, however, the global     */
/*         memory used created system errors.                               */
/*                                                                          */
/****************************************************************************/

VOID parse_DevParm(CHAR *pSource, CHAR **ppDrive, CHAR **ppModel)
{
   CHAR *pDrive, *pModel;
   int len;

   /* convert string to upper case */
   strupr(pSource);
   len = strlen(pSource);

   /* Find start of token labels */
   pDrive = strstr(pSource, "DRIVE");
   pModel = strstr(pSource, "MODEL");

   /* Find start of tokens */
   if (pDrive == NULL)
      *ppDrive = pSource + len;
   else
      *ppDrive = get_token(pDrive);

   if (pModel == NULL)
      *ppModel = pSource + len;
   else
      *ppModel = get_token(pModel);

}  /* of parse_DevParm() */


/****************************************************************************/
/*                                                                          */
/* SUBROUTINE NAME:  get_token                 DEVELOPER:  Garry Lewis      */
/*                                                                          */
/* DESCRIPTIVE NAME:  Get Token                                             */
/*                                                                          */
/* FUNCTION:  gets next token and terminates with a null.                   */
/*                                                                          */
/* PARAMETERS:                                                              */
/*      CHAR *pToken   -- Pointer to input string to be parsed.             */
/*                                                                          */
/* EXIT CODES:  None                                                        */
/*                                                                          */
/* NOTES:  WARNING!! The input string will be modified.                     */
/*                                                                          */
/****************************************************************************/

static CHAR * get_token(CHAR *pToken)
{
   CHAR *pReturn;

   /* find end of label */
   while (*pToken != ' '  && *pToken != '='  && *pToken != ','  &&
          *pToken != '\t' && *pToken != '\n' && *pToken != '\r' &&
          *pToken != '\0')
      pToken++;

   /* find next token */
   if (*pToken == '\0')
      pReturn = pToken;
   else
   {
      while (*pToken == ' '  || *pToken == '='  || *pToken == ','  ||
             *pToken == '\t' || *pToken == '\n' || *pToken == '\r')
         pToken++;

      pReturn = pToken;

      /* terminate token */
      if (*pToken != '\0')
      {
         while (*pToken != ' '  && *pToken != '='  && *pToken != ','  &&
                *pToken != '\t' && *pToken != '\n' && *pToken != '\r' &&
                *pToken != '\0')
            pToken++;

         *pToken = '\0';                 // terminate string

      }  /* of if token is found */
   }  /* of else white spaces exist */

   return(pReturn);

}  /* of get_token() */


