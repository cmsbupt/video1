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
* File name: vlc.h
* Function: header for (CA)VLC coding functions
*
*************************************************************************************
*/


#ifndef _VLC_H_
#define _VLC_H_

int   se_v ( char *tracestring );
int   ue_v ( char *tracestring );

int   u_v ( int LenInBits, char *tracestring );


// UVLC mapping
void  linfo_ue ( int len, int info, int *value1, int *dummy );
void  linfo_se ( int len, int info, int *value1, int *dummy );

void  linfo_cbp_intra ( int len, int info, int *cbp, int *dummy );
void  linfo_cbp_inter ( int len, int info, int *cbp, int *dummy );

int   readSyntaxElement_VLC ( SyntaxElement *sym );
int   GetVLCSymbol ( unsigned char buffer[], int totbitoffset, int *info, int bytecount );

int   readSyntaxElement_FLC ( SyntaxElement *sym );
int   GetBits ( unsigned char buffer[], int totbitoffset, int *info, int bytecount, int numbits );

int   get_uv ( int LenInBits, char*tracestring ); // Yulj 2004.07.15  for decision of slice end.
int   GetSyntaxElement_FLC ( SyntaxElement *sym ); // Yulj 2004.07.15  for decision of slice end.

#endif

