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
* File name: header.h
* Function: Prototypes for header.c
*
*************************************************************************************
*/


#ifndef _HEADER_H_
#define _HEADER_H_
#include "global.h"
#include "../../lcommon/inc/commonVariables.h"

void calc_picture_distance ();
void SequenceHeader ( char *buf, int starcodepos, int length );
void extension_data ( char *buf, int starcodepos, int length );
void user_data();
void copyright_extension();
void cameraparameters_extension();
#if M3480_TEMPORAL_SCALABLE
void scalable_extension();
#endif
void sequence_display_extension();
#if AVS2_HDR_HLS
void mastering_display_and_content_metadata_extension();
#endif
void picture_display_extension();
void video_edit_code_data();
void I_Picture_Header();
void PB_Picture_Header();
void SliceHeader();
#endif

