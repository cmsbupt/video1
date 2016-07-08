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
* File name: b_frame.c
* Function: B picture decoding
*
*************************************************************************************
*/


#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "global.h"
#include "../../lcommon/inc/commonVariables.h"
#include "../../lcommon/inc/block_info.h"

#define max(a, b)                   ((a) > (b) ? (a) : (b))  //!< Macro returning max value 
#define min(a, b)                   ((a) < (b) ? (a) : (b))  //!< Macro returning min value 
/*
*************************************************************************
* Function:Copy decoded P frame to temporary image array
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void copy_Pframe ()
{
  int i, j;

  /*
  * the mmin, mmax macros are taken out
  * because it makes no sense due to limited range of data type
  */

  for ( i = 0; i < img->height; i++ )
  {
    for ( j = 0; j < img->width; j++ )
    {
      imgYPrev[i][j] = imgY[i][j];
    }
  }

  for ( i = 0; i < img->height_cr; i++ )
  {
    for ( j = 0; j < img->width_cr; j++ )
    {
      imgUVPrev[0][i][j] = imgUV[0][i][j];
    }
  }

  for ( i = 0; i < img->height_cr; i++ )
  {
    for ( j = 0; j < img->width_cr; j++ )
    {
      imgUVPrev[1][i][j] = imgUV[1][i][j];
    }
  }
}

/*
*************************************************************************
* Function:init codingUnit B frames
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/


void init_codingUnit_Bframe ( unsigned int uiPositionInPic )
{
  int i, j, k;
  int r, c, row, col, width, height;

  codingUnit *currMB = &img->mb_data[uiPositionInPic];//GB current_mb_nr];
  int N8_SizeScale = 1 << ( currMB->ui_MbBitSize - MIN_CU_SIZE_IN_BIT );
  int block8_y = ( uiPositionInPic / img->PicWidthInMbs ) << 1;
  int block8_x = ( uiPositionInPic % img->PicWidthInMbs ) << 1;
  int num_of_orgMB_in_row = N8_SizeScale;//4:1  5:2  6:4
  int num_of_orgMB_in_col = N8_SizeScale;
  int size = 1 << currMB->ui_MbBitSize;
  int pix_x = ( uiPositionInPic % img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;
  int pix_y = ( uiPositionInPic / img->PicWidthInMbs ) << MIN_CU_SIZE_IN_BIT;

  if ( pix_x + size >= img->width )
  {
    num_of_orgMB_in_col = min ( N8_SizeScale, ( img->width - pix_x ) >> MIN_CU_SIZE_IN_BIT );
  }

  if ( pix_y + size >= img->height )
  {
    num_of_orgMB_in_row = min ( N8_SizeScale, ( img->height - pix_y ) >> MIN_CU_SIZE_IN_BIT );
  }

  // reset vectors and pred. modes

  for ( i = 0; i < 2 * num_of_orgMB_in_col; i++ )
  {
    for ( j = 0; j < 2 * num_of_orgMB_in_row; j++ )
    {
      img->fw_mv[block8_y + j][block8_x + i][0] = img->fw_mv[block8_y + j][block8_x + i][1] = 0;
      img->bw_mv[block8_y + j][block8_x + i][0] = img->bw_mv[block8_y + j][block8_x + i][1] = 0;
    }
  }

  for ( i = 0; i < 2 * num_of_orgMB_in_col; i++ )
  {
    for ( j = 0; j < 2 * num_of_orgMB_in_row; j++ )
    {
      img->ipredmode[block8_y + j + 1][block8_x + i + 1] = -1; //cjw the default value should be -1
    }
  }

  // Set the reference frame information for motion vector prediction
  if ( IS_INTRA ( currMB ) || IS_DIRECT ( currMB ) )
  {
    for ( j = 0; j < 2 * num_of_orgMB_in_row; j++ )
    {
      for ( i = 0; i < 2 * num_of_orgMB_in_col; i++ )
      {
        img->fw_refFrArr[block8_y + j][block8_x + i] = -1;
        img->bw_refFrArr[block8_y + j][block8_x + i] = -1;
      }
    }
  }
  else
  {
    for ( j = 0; j < 2; j++ )
    {
      for ( i = 0; i < 2; i++ )
      {
        k = 2 * j + i;
        get_b8_offset( currMB->cuType, currMB->ui_MbBitSize, i , j , &col , &row, &width, &height );
        for ( r = 0; r < height; r++ )
        {
          for ( c = 0; c < width; c++ )
          {

            img->fw_refFrArr[block8_y + row + r][block8_x + col + c] = ( ( currMB->b8pdir[k] == FORWARD || currMB->b8pdir[k] == SYM || currMB->b8pdir[k] == BID ) && currMB->b8mode[k] != 0 ? 0 : -1 );
            img->bw_refFrArr[block8_y + row + r][block8_x + col + c] = ( ( currMB->b8pdir[k] == BACKWARD || currMB->b8pdir[k] == SYM || currMB->b8pdir[k] == BID ) && currMB->b8mode[k] != 0 ? 0 : -1 );

          }
        }
      }
    }
  }
}


