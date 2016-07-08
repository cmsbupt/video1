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
* File name:
* Function:
*
*************************************************************************************
*/

#ifndef _BIT_STREAM_H

#define _BIT_STREAM_H

#include <stdio.h>

#define SVA_START_CODE_EMULATION

#ifdef SVA_START_CODE_EMULATION

#define SVA_STREAM_BUF_SIZE 1024 //must large than 3
typedef struct
{
  FILE *f;
  unsigned char buf[SVA_STREAM_BUF_SIZE];  
  unsigned int  uPreBytes;
  int iBytePosition;    
  int iBitOffset;     
  int iNumOfStuffBits;  
  int iBitsCount;    
} OutputStream;
int write_start_code ( OutputStream *p, unsigned char code );
extern OutputStream *pORABS;
#endif

void CloseBitStreamFile();
void OpenBitStreamFile ( char *Filename );
int  WriteSequenceHeader();
int  WriteSequenceDisplayExtension();
#if AVS2_HDR_HLS
int  WriteMasteringDisplayContentMetadataExtension();
#endif
int  WriteUserData ( char *userdata );
int  WriteSequenceEnd();
int  WriteVideoEditCode();
void WriteBitstreamtoFile();
void WriteSlicetoFile();
int  WriteCopyrightExtension();
int  WriteCameraParametersExtension();
int  WriteScalableExtension();

#if PicExtensionData
int picture_display_extension(Bitstream *bitstream);
int picture_copyright_extension(Bitstream *bitstream);
int picture_cameraparameters_extension();
#endif
#endif
