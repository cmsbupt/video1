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
//#include <math.h>
//#include <stdlib.h>
#include <assert.h>
//#include <string.h>

#include "../../ldecod/inc/global.h"
#include "inter-prediction.h"

/////////////////////////////////////////////////////////////////////////////
/// local function declaration 
/////////////////////////////////////////////////////////////////////////////

#if !(HALF_PIXEL_COMPENSATION || HALF_PIXEL_CHROMA)
static int calculate_distance ( int blkref, int fw_bw );
#endif





/////////////////////////////////////////////////////////////////////////////
///function definition
/////////////////////////////////////////////////////////////////////////////
/*
******************************************************************************
*  Function: calculated field or frame distance between current field(frame)
*            and the reference field(frame).
*     Input:
*    Output:
*    Return:
* Attention:
*    Author: Yulj 2004.07.14
******************************************************************************
*/
int calculate_distance ( int blkref, int fw_bw ) //fw_bw>=: forward prediction.
{
	int distance = 1;

	//if ( img->picture_structure == 1 ) // frame
	{
    if ( (img->type == F_IMG)||(img->type == INTER_IMG) )		
		{
			if ( blkref == 0 )
			{
				distance = picture_distance * 2 - img->imgtr_fwRefDistance[0] * 2 ;  // Tsinghua 200701
			}
			else if ( blkref == 1 )
			{
				distance = picture_distance * 2 - img->imgtr_fwRefDistance[1] * 2;  // Tsinghua 200701
			}
			else if ( blkref == 2 )
			{
				distance = picture_distance * 2 - img->imgtr_fwRefDistance[2] * 2;
			}
			else if ( blkref == 3 )
			{
				distance = picture_distance * 2 - img->imgtr_fwRefDistance[3] * 2 ;
			}
			else
			{
				assert ( 0 ); //only two reference pictures for P frame
			}
		}
		else //B_IMG
		{
			if ( fw_bw >= 0 ) //forward
			{
				distance = picture_distance * 2 - img->imgtr_fwRefDistance[0] * 2;  // Tsinghua 200701
			}
			else
			{
				distance = img->imgtr_next_P * 2  - picture_distance * 2;  // Tsinghua 200701
			}
		}
	}

	distance = ( distance + 512 ) % 512; // Added by Xiaozhen ZHENG, 20070413, HiSilicon
	return distance;
}
/*Lou 1016 End*/
/*Lou 1016 Start*/
//The unit of time distance is calculated by field time
/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
int scale_motion_vector ( int motion_vector, int currblkref, int neighbourblkref, int ref ) //qyu 0820 modified , int currsmbtype, int neighboursmbtype, int block_y_pos, int curr_block_y, int ref, int direct_mv)
{
	int sign = ( motion_vector > 0 ? 1 : -1 );
	int mult_distance;
	int devide_distance;

	motion_vector = abs ( motion_vector );

	if ( motion_vector == 0 )
	{
		return 0;
	}

	mult_distance = calculate_distance ( currblkref, ref );
	devide_distance = calculate_distance ( neighbourblkref, ref );

#if MV_SCALING_UNIFY
	motion_vector = sign * ( ( motion_vector * mult_distance * ( MULTI / devide_distance ) + HALF_MULTI ) >> OFFSET );
#else
	motion_vector = sign * ( ( motion_vector * mult_distance * ( 512 / devide_distance ) + 256 ) >> 9 );
#endif
#if	m3469
	motion_vector = Clip3(-32768, 32767, motion_vector);
#endif
	return motion_vector;
}
/*Lou 1016 End*/

void scalingMV(int *cur_mv_x,int* cur_mv_y, int curT, int ref_mv_x, int ref_mv_y, int refT, int factor_sign)
{
#if MV_SCALE_14BIT==1
#if MV_SCALING_UNIFY
	*cur_mv_x = (curT * ref_mv_x * (MULTI / refT) + HALF_MULTI) >> OFFSET;
	*cur_mv_y = (curT * ref_mv_y * (MULTI / refT) + HALF_MULTI) >> OFFSET;
#else
    *cur_mv_x = (curT * ref_mv_x * (16384 / refT) + 8192) >> 14;
	*cur_mv_y = (curT * ref_mv_y * (16384 / refT) + 8192) >> 14;
#endif
#if m3469
	*cur_mv_x = Clip3(-32768, 32767, ( *cur_mv_x));
	*cur_mv_y = Clip3(-32768, 32767, ( *cur_mv_y));
#endif
#else
	int scaleFator;
	scaleFator = ( curT * ( 0x4000 + refT / 2 ) / refT + 32 ) >> 6;
	*cur_mv_x = factor_sign * ((scaleFator * ref_mv_x + 127 + ( scaleFator * ref_mv_x < 0)) >> 8);
	*cur_mv_y = factor_sign * ((scaleFator * ref_mv_y + 127 + ( scaleFator * ref_mv_y < 0)) >> 8);
#endif
}

