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



#ifndef _BIARIENCOD_H_
#define _BIARIENCOD_H_



/************************************************************************
* D e f i n i t i o n s
***********************************************************************
*/


// some definitions to increase the readability of the source code

#define Elow            (eep->Elow)
#define E_s1            (eep->E_s1)
#define E_t1            (eep->E_t1)

#define Ebits_to_follow (eep->Ebits_to_follow)
#define Ebuffer         (eep->Ebuffer)
#define Ebits_to_go     (eep->Ebits_to_go)
#define Ecodestrm       (eep->Ecodestrm)
#define Ecodestrm_len   (eep->Ecodestrm_len)
#define Ecodestrm_laststartcode   (eep->Ecodestrm_laststartcode)


#define B_BITS  10

#define ONE        (1 << B_BITS)
#define HALF       (1 << (B_BITS-1))
#define QUARTER    (1 << (B_BITS-2))

#define LG_PMPS_SHIFTNO 2

#endif  // BIARIENCOD_H

