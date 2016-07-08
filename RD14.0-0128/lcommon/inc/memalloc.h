/*
*****************************************************************************
* COPYRIGHT AND WARRANTY INFORMATION
*
* Copyright 2002-2012, Advanced Audio Video Coding Standard, Part II
*
* DISCLAIMER OF WARRANTY
*
* The contents of this file are subject to the Mozilla Public License
* Version 1.1 (the "License"); you may not use this file except in
* compliance with the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS"
* basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
* License for the specific language governing rights and limitations under
* the License.
*
* THIS IS NOT A GRANT OF PATENT RIGHTS - SEE THE AVS PATENT POLICY.
* The AVS Working Group doesn't represent or warrant that the programs
* furnished here under are free of infringement of any third-party patents.
* Commercial implementations of AVS, including shareware, may be
* subject to royalty fees to patent holders. Information regarding
* the AVS patent policy for standardization procedure is available at
* AVS Web site http://www.avs.org.cn. Patent Licensing is outside
* of AVS Working Group.
*
* The Initial Developer of the Original Code is Video subgroup of AVS
* Workinggroup.
* Contributors: Qin Yu,         Zhichu He,  Weiran Li,    Yong Ling,
*               Zhenjiang Shao, Jie Chen,   Junjun Si,    Xiaozhen Zheng, 
*               Jian Lou,       Qiang Wang, Jianwen Chen, Haiwu Zhao,
*               Guoping Li,     Siwei Ma,   Junhao Zheng, Zhiming Wang
*               Li Zhang,
******************************************************************************
*/



/*
*************************************************************************************
* File name: memalloc.h
* Function: Memory allocation and free helper funtions
*
*************************************************************************************
*/


#ifndef _MEMALLOC_H_
#define _MEMALLOC_H_

#include "global.h"
#include "commonVariables.h"
int get_mem1Dint( int **array1D, int num );
void free_mem1Dint( int *array1D);
int get_mem1D( byte **array1D, int num );
void free_mem1D( byte *array1D);
int  get_mem2D ( byte ***array2D, int rows, int columns );
int  get_mem2Dint ( int ***array2D, int rows, int columns );
int  get_mem3D ( byte ****array2D, int frames, int rows, int columns );
int  get_mem3Dint ( int ****array3D, int frames, int rows, int columns );
int  get_mem4Dint ( int *****array4D, int idx, int frames, int rows, int columns );

void free_mem2D ( byte **array2D );
void free_mem2Dint ( int **array2D );
void free_mem3D ( byte ***array2D, int frames );
void free_mem3Dint ( int ***array3D, int frames );
void free_mem4Dint ( int ****array4D, int idx, int frames );

void no_mem_exit ( char *where );

int get_mem3DSAOstatdate(  SAOStatData**** array3D, int num_SMB, int num_comp,int num_class);
int get_mem2DSAOParameter(SAOBlkParam*** array2D, int num_SMB, int num_comp);
void free_mem3DSAOstatdate(  SAOStatData*** array3D, int num_SMB, int num_comp);
void free_mem2DSAOParameter(SAOBlkParam** array2D, int num_SMB);
#endif
