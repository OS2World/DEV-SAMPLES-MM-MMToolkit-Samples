
/**************************START OF SPECIFICATIONS **************************/
/*                                                                          */
/* SOURCE FILE NAME:  CDEVHLP.H                                             */
/*                                                                          */
/* DISCRIPTIVE NAME: C functional prototype interface for OS/2 DevHlp       */
/*                   services.                                              */
/*                                                                          */
/* NOTE:                                                                    */
/*   DEPENDENCIES:                                                          */
/*   RESTRICTIONS:                                                          */
/*                                                                          */
/************************** END OF SPECIFICATIONS ***************************/

USHORT  DevHlp_AllocGDTSelector(ULONG ulNumGDTSelectors, PVOID paGDTSel);

USHORT  DevHlp_AllocPhys(ULONG BlockSize,
                         USHORT Location,
                         PVOID  pRetAddr);


USHORT  DevHlp_PhysToGDTSelector(PVOID  PhysAddress,
                                 USHORT ByteLength,
                                 USHORT Selector);


USHORT  DevHlp_PhysToVirt(PVOID  PhysAddress,
                          ULONG  Size,
                          PVOID  pRetAddress);

USHORT  DevHlp_VirtToPhys(PVOID VirtAddress,
                          PVOID pRetPhysAddress);
