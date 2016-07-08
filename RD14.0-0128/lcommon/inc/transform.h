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
#include "commonVariables.h"
#include "defines.h"
#include "contributors.h"

#define shift_trans 10   // 8-pt trans: 9 + 17 - 16 = 10
#define shift_out   15   // 8-pt inv_trans: 13 + (28-17) - 9 = 15#
#if TRANS_16_BITS
#define PIXEL_BIT		8
#define RESID_BIT		(PIXEL_BIT + 1)
#define LIMIT_BIT		16
#define FACTO_BIT		5
#endif

/////////////////////////////////////////////////////////////////////////////
/// external function declaration
/////////////////////////////////////////////////////////////////////////////
void partialButterfly4( int **src, int **dst, int iNumRows );
void partialButterflyInverse4( int **src, int **dst, int iNumRows );

void partialButterfly8( int **src, int **dst, int iNumRows );

void partialButterflyInverse8( int **src, int **dst, int iNumRows );

void partialButterfly16( int **src, int **dst, int iNumRows );

void partialButterflyInverse16( int **src, int **dst, int iNumRows );

void partialButterfly32( int **src, int **dst, int iNumRows );

void partialButterflyInverse32( int **src, int **dst, int iNumRows );

void  wavelet64( int **curr_blk );
void  wavelet_NSQT( int **curr_blk,int is_Hor);
void  inv_wavelet_B64( int **curr_blk );
void  inv_wavelet_NSQT( int **curr_blk,int is_Hor );



#if TRANS_16_BITS

void array_shift_clip(int **src, int shift, int iSizeY, int iSizeX, int bit_depth);
#if EXTEND_BD
void transform_B8 ( int **curr_blk, unsigned int trans_BitSize, codingUnit *currMB, int isChroma, int secT_enabled, int sample_bit_depth);
#else
void transform_B8 ( int **curr_blk, unsigned int trans_BitSize, codingUnit *currMB, int isChroma, int secT_enabled);
#endif
#if EXTEND_BD
void transform_NSQT ( int **curr_blk, unsigned int trans_BitSize, codingUnit *currMB, int isChroma, int secT_enabled, int sample_bit_depth);
#else
void transform_NSQT ( int **curr_blk, unsigned int trans_BitSize, codingUnit *currMB, int isChroma, int secT_enabled);
#endif


#if EXTEND_BD
void quant_B8 ( int qp, int mode, int **curr_blk, int iSizeX, int iSizeY, int uiBitSize, int sample_bit_depth );
#else
void quant_B8 ( int qp, int mode, int **curr_blk, int iSizeX, int iSizeY, int uiBitSize );
#endif



#if EXTEND_BD
void inv_transform_B8 ( int **curr_blk, unsigned int trans_BitSize, codingUnit *currMB, int isChroma, int secT_enabled, int sample_bit_depth);
#else
void inv_transform_B8 ( int **curr_blk, unsigned int trans_BitSize, codingUnit *currMB, int isChroma, int secT_enabled);
#endif
#else
void transform_B8 ( int **curr_blk, unsigned int trans_BitSize ); // block to be transformed.
void quant_B8 ( int qp, int mode, int **curr_blk, unsigned int uiBitSize );
void inv_transform_B8 ( int **curr_blk, unsigned int trans_BitSize );
#endif


#if EXTEND_BD
void inv_transform_NSQT ( int **curr_blk, unsigned int trans_BitSize, codingUnit *currMB, int isChroma, int secT_enabled, int sample_bit_depth);
#else
void inv_transform_NSQT ( int **curr_blk, unsigned int trans_BitSize, codingUnit *currMB, int isChroma, int secT_enabled);
#endif


#if EXTEND_BD
extern short IQ_SHIFT[80];
extern unsigned short IQ_TAB[80];
extern unsigned short Q_TAB[80];
#else
extern short IQ_SHIFT[64];
extern unsigned short IQ_TAB[64];
extern unsigned short Q_TAB[64];
#endif
