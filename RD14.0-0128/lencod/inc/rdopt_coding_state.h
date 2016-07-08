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
* File name: rdopt_coding_state.h
* Function:  Headerfile for storing/restoring coding state
       (for rd-optimized mode decision)
*
*************************************************************************************
*/


#ifndef _RD_OPT_CS_H_
#define _RD_OPT_CS_H_

#include "global.h"
#include "../../lcommon/inc/commonVariables.h"

int calcHAD8x2( int ** pi, int iWidth, int iHeight );//ZHAOHAIWU
int calcHAD2x8( int ** pi, int iWidth, int iHeight );//ZHAOHAIWU
int calcHAD16x4( int ** pi, int iWidth, int iHeight );//ZHAOHAIWU
int calcHAD4x16( int ** pi, int iWidth, int iHeight );//ZHAOHAIWU
int calcHAD4x4( int ** pi, int iWidth, int iHeight );//ZHAOHAIWU

void   delete_coding_state ( CSptr ); //!< delete structure
CSptr  create_coding_state ();        //!< create structure

void   store_coding_state ( CSptr );  //!< store parameters
void   reset_coding_state ( CSptr );  //!< restore parameters

//extern int Coeff_for_intra_luma[8][8];//qyu 0821
extern CSptr cs_mb, cs_b8, cs_cm;

extern CSptr cs_tmp;


#endif

