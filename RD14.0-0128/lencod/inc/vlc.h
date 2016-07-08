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
* Function: Prototypes for VLC coding funtions
*
*************************************************************************************
*/



#ifndef _VLC_H_
#define _VLC_H_


int   u_1 ( char *tracestring, int value, Bitstream *part );
int   se_v ( char *tracestring, int value, Bitstream *part );
int   ue_v ( char *tracestring, int value, Bitstream *part );
int   u_v ( int n, char *tracestring, int value, Bitstream *part );

int   writeSyntaxElement_UVLC ( SyntaxElement *se, Bitstream *this_dataPart );
int   writeSyntaxElement_fixed ( SyntaxElement *se, Bitstream *this_dataPart );

void  writeUVLC2buffer ( SyntaxElement *se, Bitstream *currStream );

int   symbol2uvlc ( SyntaxElement *se );
void  ue_linfo ( int n, int dummy, int *len, int *info );
void  se_linfo ( int mvd, int dummy, int *len, int *info );
void  cbp_linfo_intra ( int cbp, int dummy, int *len, int *info );
void  cbp_linfo_inter ( int cbp, int dummy, int *len, int *info );

#endif

