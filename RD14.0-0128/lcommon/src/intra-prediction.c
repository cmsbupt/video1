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
#include <errno.h>
#include "intra-prediction.h"


#define Clip(x)            ( min(255, max( 0, (x)) ) )															///< clip with bit-depth range




#define max(a, b)                   ((a) > (b) ? (a) : (b))  //!< Macro returning max value 
#define min(a, b)                   ((a) < (b) ? (a) : (b))  //!< Macro returning min value 

/////////////////////////////////////////////////////////////////////////////
/// variables definition
/////////////////////////////////////////////////////////////////////////////
const unsigned char g_aucXYflg[NUM_INTRA_PMODE]=
{
  0,0,0,0,0,
  0,0,0,0,0,
  0,0,0,0,0,
  0,0,0,0,0,
  0,0,0,0,0,
  1,1,1,1,1,
  1,1,1
};

const char g_aucDirDx[NUM_INTRA_PMODE] =
{
  0,0,0,11,2,
  11,1,8,1,4,
  1,1,0,1,1,
  4,1,8,1,11,
  2,11,4,8,0,
  8,4,11,2,11,
  1,8,1
};

const char g_aucDirDy[NUM_INTRA_PMODE] =
{
  0,0,0,-4,-1,
  -8,-1,-11,-2,-11,
  -4,-8,0,8,4,
  11,2,11,1,8,
  1,4,1,1,0,
  -1,-1,-4,-1,-8,
  -1,-11,-2
};

const char g_aucSign[NUM_INTRA_PMODE]=
{
  0, 0, 0,-1,-1,
  -1,-1,-1,-1,-1,
  -1,-1, 0, 1, 1,
  1, 1, 1, 1, 1,
  1, 1, 1, 1, 0,
  -1,-1,-1,-1,-1,
  -1,-1,-1
};
const char g_aucDirDxDy[2][NUM_INTRA_PMODE][2] =
{
  { // dx/dy
    {0, 0}, {0,0}, {0,0}, {11,2}, {2,0},
    {11,3}, {1,0}, {93,7}, {1,1}, {93,8}, 
    {1,2}, {1,3}, {0,0}, {1,3}, {1,2},
    {93,8}, {1,1}, {93,7}, {1,0}, {11,3}, 
    {2,0}, {11,2}, {4,0}, {8,0}, {0,0}, 
    {8,0}, {4,0}, {11,2}, {2,0},{11,3},
    {1,0}, {93,7},{1,1},
  },
  { // dy/dx
    {0, 0}, {0,0}, {0,0}, {93,8}, {1,1},
    {93,7}, {1,0}, {11,3}, {2,0}, {11,2},
    {4,0}, {8,0}, {0,0}, {8,0}, {4,0},
    {11,2}, {2,0},{11,3}, {1,0}, {93,7},
    {1,1}, {93,8}, {1,2}, {1,3}, {0,0}, 
    {1,3}, {1,2}, {93,8}, {1,1}, {93,7}, 
    {1,0}, {11,3}, {2,0}
    }
};

/////////////////////////////////////////////////////////////////////////////
/// local function declaration
/////////////////////////////////////////////////////////////////////////////
static void xPredIntraVertAdi( short* pSrc, int ** pDst, int iWidth, int iHeight );
static void xPredIntraHorAdi( short* pSrc, int ** pDst, int iWidth, int iHeight );
#if EXTEND_BD
static void xPredIntraDCAdi( short* pSrc, int ** pDst, int iWidth, int iHeight, int bAboveAvail, int bLeftAvail, int sample_bit_depth );
static void xPredIntraPlaneAdi( short* pSrc, int ** pDst, int iWidth, int iHeight, int sample_bit_depth );
static void xPredIntraBiAdi( short* pSrc, int ** pDst, int iWidth, int iHeight, int sample_bit_depth );
#else
static void xPredIntraDCAdi( short* pSrc, int ** pDst, int iWidth, int iHeight, int bAboveAvail, int bLeftAvail );
static void xPredIntraPlaneAdi( short* pSrc, int ** pDst, int iWidth, int iHeight );
static void xPredIntraBiAdi( short* pSrc, int ** pDst, int iWidth, int iHeight );
#endif
static void xPredIntraAngAdi( short* pSrc, int ** pDst, int uiDirMode, int iWidth, int iHeight );
static int  getContextPixel( int uiDirMode, int uiXYflag, int iTempD, int* offset );

/////////////////////////////////////////////////////////////////////////////
/// function definition
/////////////////////////////////////////////////////////////////////////////
void xPredIntraVertAdi( short* pSrc, int ** pDst, int iWidth, int iHeight )
{  
	int x, y;
	short* rpSrc = pSrc + 1;

	for ( y = 0; y < iHeight; y++ )
	{
		for (x = 0; x < iWidth; x++)
		{
			pDst[y][x]=rpSrc[x];
		}
	}
}

void xPredIntraHorAdi( short* pSrc, int ** pDst, int iWidth, int iHeight )
{
	int x, y;
	short* rpSrc = pSrc - 1;

	for ( y = 0; y < iHeight; y++ )
	{
		for ( x = 0; x < iWidth; x++ )
		{
			pDst[y][x]=rpSrc[-y];
		}
	}
}
#if EXTEND_BD
void xPredIntraDCAdi( short* pSrc, int ** pDst, int iWidth, int iHeight, int bAboveAvail, int bLeftAvail, int sample_bit_depth )
#else
void xPredIntraDCAdi( short* pSrc, int ** pDst, int iWidth, int iHeight, int bAboveAvail, int bLeftAvail )
#endif
{
	int   x, y;
	int   iDCValue = 0;
	short*  rpSrc = pSrc - 1;

	if (bLeftAvail )
	{
		for ( y = 0; y < iHeight; y++ )
		{
			iDCValue += rpSrc[-y];
		}

		rpSrc = pSrc + 1;
		if (bAboveAvail )
		{
			for ( x = 0; x < iWidth; x++ )
			{
				iDCValue += rpSrc[x];
			}

#if M3460_DC_MUL_REMOVE
			iDCValue += ((iWidth+iHeight)>>1);
			iDCValue=(iDCValue*(512/(iWidth + iHeight)))>>9;

#else
			iDCValue += iWidth;
			iDCValue /= iWidth + iHeight;
#endif
		}
		else
		{
			iDCValue += iHeight / 2;
			iDCValue /= iHeight;
		}
	}
	else
	{
		rpSrc = pSrc + 1;
		if (bAboveAvail)
		{
			for ( x = 0; x < iWidth; x++ )
			{
				iDCValue += rpSrc[x];
			}

			iDCValue += iWidth / 2;
			iDCValue /= iWidth;
		}
		else
		{
#if EXTEND_BD
			iDCValue = 1 << (sample_bit_depth - 1);
#else
			iDCValue = 128;
#endif
		}
	}

	for ( y = 0; y < iHeight; y++ )
	{
		for ( x = 0; x < iWidth; x++ )
		{
			pDst[y][x]=iDCValue;
		}
	}
}

#if EXTEND_BD
void xPredIntraPlaneAdi(short* pSrc, int ** pDst, int iWidth, int iHeight, int sample_bit_depth )
#else
void xPredIntraPlaneAdi(short* pSrc, int ** pDst, int iWidth, int iHeight )
#endif
{
	int iH = 0;
	int iV = 0;
	int iA, iB, iC;
	int x, y;
	int iW2 = iWidth>>1;
	int iH2 = iHeight>>1;
  int ib_mult[5]={13, 17, 5, 11, 23};
  int ib_shift[5]={7, 10, 11, 15,19};

  int im_h=ib_mult[g_log2size[iWidth]-2];
  int is_h=ib_shift[g_log2size[iWidth]-2];
  int im_v=ib_mult[g_log2size[iHeight]-2];
  int is_v=ib_shift[g_log2size[iHeight]-2];
	int iTmp, iTmp2;

	short*  rpSrc = pSrc;

	rpSrc = pSrc + 1;
	rpSrc += (iW2 - 1);
	for ( x = 1; x < iW2+1; x++ )
	{
		iH += x * (rpSrc[x] - rpSrc[-x]);
	}

	rpSrc = pSrc - 1;
	rpSrc -= (iH2 - 1);

  for( y = 1; y < iH2+1; y++ )
  {
    iV += y * (rpSrc[-y] - rpSrc[y]);
  }

	rpSrc = pSrc;
  iA = ( rpSrc[-1 - (iHeight-1)] + rpSrc[1 + iWidth-1] ) << 4;
  iB = ( (iH<<5)*im_h + (1<<(is_h-1)) ) >> is_h;
  iC = ((iV<<5)*im_v + (1<<(is_v-1)) ) >> is_v;

  iTmp = iA - (iH2-1)*iC - (iW2-1)*iB + 16;
	for ( y = 0; y < iHeight; y++ )
	{
		iTmp2 = iTmp;
		for ( x = 0; x < iWidth; x++ )
		{
			//img->mprr[PLANE_PRED][y][x] = Clip( iTmp2 >> 5 );
#if EXTEND_BD
			pDst[y][x]=Clip3(0, (1 << sample_bit_depth) - 1, iTmp2 >> 5 );
#else
			pDst[y][x]=Clip( iTmp2 >> 5 );
#endif
			iTmp2 += iB;
		}
		iTmp += iC;
	}
}

#if EXTEND_BD
void xPredIntraBiAdi( short* pSrc, int ** pDst, int iWidth, int iHeight, int sample_bit_depth )
#else
void xPredIntraBiAdi( short* pSrc, int ** pDst, int iWidth, int iHeight )
#endif
{
  int x, y;
  int ishift_x=g_log2size[iWidth];
  int ishift_y=g_log2size[iHeight];
  int ishift=min(ishift_x,ishift_y);
  int ishift_xy=ishift_x+ishift_y+1;
  int offset=1<<(ishift_x+ishift_y);
  int a,b,c,w,wxy,tmp;
  int predx;
  int pTop[MAX_CU_SIZE], pLeft[MAX_CU_SIZE], pT[MAX_CU_SIZE], pL[MAX_CU_SIZE],wy[MAX_CU_SIZE];

#if EXTRACT_lcu_info_debug_intra
  if (1)
  {
    fprintf(p_lcu_debug, "xPredIntraBiAdi\n ");
    fprintf(p_lcu_debug, "ishift_x=%2d ishift_y=%2d ishift=%2d ishift_xy=%2d \n ", ishift_x, ishift_y, ishift, ishift_xy);
  }
#endif
  for( x=0; x<iWidth; x++)
  {
    pTop[x] = pSrc[1+x];
  }
  for( y=0; y<iHeight; y++)
  {
    pLeft[y] = pSrc[-1-y];
  }

  a = pTop[iWidth-1];
  b = pLeft[iHeight-1];
  c=(iWidth==iHeight) ? (a+b+1)>>1 : (((a<<ishift_x) + (b<<ishift_y))*13 + (1<<(ishift+5)) ) >>(ishift+6);
  w=(c<<1)-a-b;
#if EXTRACT_lcu_info_debug_intra
  if (1)
  {
    fprintf(p_lcu_debug, "xPredIntraBiAdi\n ");
    fprintf(p_lcu_debug, "a=%2d b=%2d c=%2d w=%2d \n ", a, b, c, w);
  }
#endif


  for ( x=0; x<iWidth; x++)
  {
    pT[x]=b-pTop[x];
    pTop[x] <<=ishift_y;
  }
  tmp=0;
  for ( y=0; y<iHeight; y++)
  {
    pL[y]=a-pLeft[y];
    pLeft[y] <<=ishift_x;
    wy[y]=tmp;
    tmp+=w;
  }


  for( y=0; y<iHeight; y++ )
  {
    predx=pLeft[y];
    wxy=0;
    for( x=0; x<iWidth; x++ )
    {
      predx+=pL[y];
      
      pTop[x]+=pT[x];
#if !CLIP
      pDst[y][x] = ( (predx<<ishift_y) + (pTop[x]<<ishift_x) + wxy + offset )>> ishift_xy ;
#else 
	  pDst[y][x] = Clip3(0, (1 << sample_bit_depth) - 1, (( (predx<<ishift_y) + (pTop[x]<<ishift_x) + wxy + offset )>> ishift_xy));
#endif 
	  wxy+=wy[y];
    }
  }

}
int getContextPixel(int uiDirMode, int uiXYflag, int iTempD, int* offset)
{
  int imult= g_aucDirDxDy[uiXYflag][uiDirMode][0];
  int ishift= g_aucDirDxDy[uiXYflag][uiDirMode][1];

  int iTempDn = iTempD * imult >> ishift;
  *offset = ((iTempD * imult * 32) >> ishift) - iTempDn*32;
	return iTempDn;
}
void xPredIntraAngAdi( short* pSrc, int ** pDst, int uiDirMode, int iWidth, int iHeight )
{
	int  iDx, iDy, i, j, iTempDx, iTempDy, iXx, iXy, iYx, iYy;
	int  uixyflag = 0; // 0 for x axis 1 for y axis
	Boolean bSetFlag1, bSetFlag2;
  int offset, offsetx, offsety;
  int iX, iY, iXn, iYn, iXnN1, iYnN1, iXnP2, iYnP2;
  int iDxy;
	int iDxP, iDxN, iDyP, iDyN;
	int iWidth2 = iWidth << 1;
	int iHeight2 = iHeight << 1;
  short*  rpSrc = pSrc;
  iDx = g_aucDirDx[ uiDirMode ];
  iDy = g_aucDirDy[ uiDirMode ];
  uixyflag = g_aucXYflg[ uiDirMode ];
  iDxy = g_aucSign[uiDirMode];

  for ( j = 0 ; j < iHeight; j++)
  {
    for ( i = 0; i < iWidth; i++ )
    {
      {
        if ( iDxy < 0 )   // select context pixel based on uixyflag
        {
          if ( uixyflag == 0 )
          {
            // case xline
            iTempDy = j - ( -1 );
            iTempDx = getContextPixel( uiDirMode, 0, iTempDy, &offset );
            iX = i + iTempDx;
            iY = -1;
          }
          else
          {
            // case yline
            iTempDx = i - ( -1 );
            iTempDy = getContextPixel( uiDirMode, 1, iTempDx, &offset );
            iX = -1;
            iY = j + iTempDy;

#if EXTRACT_lcu_info_debug_intra
            if (1)
            {
              fprintf(p_lcu_debug, "i,j=%2d %2d : iX=%2d iY=%2d iTempDx=%2d iTempDy=%2d offset=%2d \n ",i,j, iX, iY, iTempDx, iTempDy, offset);
            }
#endif
          }
        }
        else  // select context pixel based on the distance
        {
          iTempDy = j - ( -1 );
          iTempDx = getContextPixel( uiDirMode, 0, iTempDy, &offsetx );
          iTempDx = -iTempDx;
          iXx = i + iTempDx;
          iYx = -1;

          iTempDx = i - ( -1 );
          iTempDy = getContextPixel( uiDirMode, 1, iTempDx, &offsety );
          iTempDy = -iTempDy;
          iXy = -1;
          iYy = j + iTempDy;

          if ( iYy <= -1 )
          {
            iX = iXx;
            iY = iYx;
            offset = offsetx;
          }
          else
          {
            iX = iXy;
            iY = iYy;
            offset = offsety;
          }
        }

        if ( iY == -1 )
        {
          rpSrc = pSrc + 1;

          if ( iDxy < 0 )
          {
            iXn = iX + 1;
          }
          else
          {
            iXn = iX - 1;
          }


          if ( iDxy < 0 )
          {
            iXnP2 = iX + 2;
            iXnN1 = iX - 1;
          }
          else
          {
            iXnP2 = iX - 2;
            iXnN1 = iX + 1;
          }
          iX    = min(iWidth2-1, iX);
          iXn   = min(iWidth2-1, iXn);
          iXnN1 = min(iWidth2-1, iXnN1);
          iXnP2 = min(iWidth2-1, iXnP2);

          pDst[j][i] = ( rpSrc[iXnN1] * ( 32 - offset ) + rpSrc[iX] * ( 64 - offset ) + rpSrc[iXn] * ( 32 + offset ) + rpSrc[iXnP2] * offset + 64 ) >> 7;
        }
        else if ( iX == -1 )
        {
          rpSrc = pSrc - 1;

          if ( iDxy < 0 )
          {
            iYn = iY + 1;
          }
          else
          {
            iYn = iY - 1;
          }

          if ( iDxy < 0 )
          {
            iYnP2 = iY + 2;
            iYnN1 = iY - 1;
          }
          else
          {
            iYnP2 = iY - 2;
            iYnN1 = iY + 1;
          }
          iY    = min(iHeight2-1, iY);
          iYn   = min(iHeight2-1, iYn);
          iYnN1 = min(iHeight2-1, iYnN1);
          iYnP2 = min(iHeight2-1, iYnP2);
#if EXTRACT_lcu_info_debug_intra
          if (1)
          {
            fprintf(p_lcu_debug, "iY=%2d iYn=%2d iYnN1=%2d iYnP2=%2d \n ", iY, iYn, iYnN1, iYnP2);
            fprintf(p_lcu_debug, "rpSrc[-iYnN1]=%4d rpSrc[-iY] =%2d rpSrc[-iYn]=%2d rpSrc[-iYnP2]=%2d \n ", rpSrc[-iYnN1], rpSrc[-iY], rpSrc[-iYn], rpSrc[-iYnP2]);
          }
#endif
          pDst[j][i] = ( rpSrc[-iYnN1] * ( 32 - offset ) + rpSrc[-iY] * ( 64 - offset ) + rpSrc[-iYn] * ( 32 + offset ) + rpSrc[-iYnP2] * offset + 64 ) >> 7;
        }
      }
    }
  }
}

#if EXTEND_BD
void predIntraLumaAdi(short* pSrc, int** piPredBuf,int uiDirMode, unsigned int uiBitSize, int bAbove, int bLeft,int bs_y,int bs_x, int sample_bit_depth )
#else
void predIntraLumaAdi(short* pSrc, int** piPredBuf,int uiDirMode, unsigned int uiBitSize, int bAbove, int bLeft,int bs_y,int bs_x )
#endif
{


	switch( uiDirMode )
	{
	case VERT_PRED:  // Vertical
		{
			xPredIntraVertAdi( pSrc, piPredBuf, bs_x, bs_y );   
			break;
		}
	case HOR_PRED:  // Horizontal
		{
			xPredIntraHorAdi( pSrc, piPredBuf, bs_x, bs_y  );
			break;      
		}
	case DC_PRED:  // DC
		{
#if EXTEND_BD
			xPredIntraDCAdi( pSrc, piPredBuf, bs_x, bs_y , bAbove, bLeft, sample_bit_depth );
#else
			xPredIntraDCAdi( pSrc, piPredBuf, bs_x, bs_y , bAbove, bLeft );
#endif
			break;
		}
	case PLANE_PRED:  // Plane
		{
#if EXTEND_BD
			xPredIntraPlaneAdi( pSrc, piPredBuf, bs_x, bs_y, sample_bit_depth  );
#else
			xPredIntraPlaneAdi( pSrc, piPredBuf, bs_x, bs_y  );
#endif
			break;
		}
	case BI_PRED:  // bi
		{
#if EXTEND_BD
			xPredIntraBiAdi ( pSrc, piPredBuf, bs_x, bs_y, sample_bit_depth  );
#else
			xPredIntraBiAdi ( pSrc, piPredBuf, bs_x, bs_y  );
#endif
			break;
		}
	default:
		{
			xPredIntraAngAdi(pSrc, piPredBuf, uiDirMode,bs_x, bs_y );
			break;
		}
	}
}

#if EXTEND_BD
void predIntraChromaAdi( short * pSrc, int** piPredBuf, int uiDirMode, unsigned int uiBitSize, int bAbove, int bLeft, int LumaMode, int sample_bit_depth )
#else
void predIntraChromaAdi( short * pSrc, int** piPredBuf, int uiDirMode, unsigned int uiBitSize, int bAbove, int bLeft, int LumaMode )
#endif
{
	int uiWidth = 1 << uiBitSize;
	int uiHeight = 1 << uiBitSize;

  if( uiDirMode == DM_PRED_C && ( LumaMode == VERT_PRED || LumaMode == HOR_PRED || LumaMode == DC_PRED || LumaMode == BI_PRED ))
	{
		uiDirMode = LumaMode==VERT_PRED ? VERT_PRED_C : ( LumaMode==HOR_PRED ? HOR_PRED_C : ( LumaMode==DC_PRED ?  DC_PRED_C : BI_PRED_C ) );
	}

	switch( uiDirMode )
	{
  case DM_PRED_C: // DM
	  {
		  switch( LumaMode )
		  {
		  case PLANE_PRED:  // Plane
			  {
#if EXTEND_BD
				  xPredIntraPlaneAdi( pSrc, piPredBuf, uiWidth, uiHeight, sample_bit_depth );
#else
					xPredIntraPlaneAdi( pSrc, piPredBuf, uiWidth, uiHeight );
#endif
				  break;
			  }
		  default:
			  {
				  xPredIntraAngAdi( pSrc, piPredBuf, LumaMode, uiWidth, uiHeight );
				  break;
			  }
		  }
		  break;
	  }
  case DC_PRED_C:  // DC
	  {
#if EXTEND_BD
		  xPredIntraDCAdi( pSrc, piPredBuf, uiWidth, uiHeight, bAbove,  bLeft, sample_bit_depth );
#else
			xPredIntraDCAdi( pSrc, piPredBuf, uiWidth, uiHeight, bAbove,  bLeft );
#endif
		  break;
	  }
  case HOR_PRED_C:  // Horizontal
	  {
		  xPredIntraHorAdi( pSrc, piPredBuf, uiWidth, uiHeight );
		  break;      
	  }
  case VERT_PRED_C:  // Vertical
	  {
		  xPredIntraVertAdi( pSrc, piPredBuf, uiWidth, uiHeight );   
		  break;
	  }
  case BI_PRED_C: // Bilinear
	  {
#if EXTEND_BD
		  xPredIntraBiAdi ( pSrc, piPredBuf, uiWidth, uiHeight, sample_bit_depth );
#else
			xPredIntraBiAdi ( pSrc, piPredBuf, uiWidth, uiHeight );
#endif
		  break;
	  }
  default:
	  {
//		error ( "illegal chroma intra prediction mode", 600 );
		  printf ( "\n illegal chroma intra prediction mode\n" );//ZHAOHAIWU
		  break;
	  }
	}
}

#if BUGFIX_AVAILABILITY_INTRA
/* -----------------------------------------------------------------------
* Function: return 1 when a neighboring block is in the same slice
*/
static int isBlockAvailable(int x_4x4, int y_4x4, int dx_4x4, int dy_4x4, int cur_slice_idx)
{
    int x2_4x4 = x_4x4 + dx_4x4;
    int y2_4x4 = y_4x4 + dy_4x4;
    int width_in_4x4 = img->width >> 2;
    int height_in_4x4 = img->height >> 2;
    int width_in_mincu = img->width >> MIN_CU_SIZE_IN_BIT;

    if (x2_4x4 < 0 || y2_4x4 < 0 || x2_4x4 >= width_in_4x4 || y2_4x4 >= height_in_4x4) {
        return 0;  /* return 0 for blocks outside the picture */
    } else {
        return cur_slice_idx == img->mb_data[(y2_4x4 >> 1) * width_in_mincu + (x2_4x4 >> 1)].slice_nr;
    }
}

/* -----------------------------------------------------------------------
* Function: get availabilities of neighboring blocks 
*/
void getIntraNeighborAvailabilities(codingUnit *currMB, int maxCuSizeInBit, int img_x, int img_y, int bsx, int bsy, int *p_avail)
{
    int cur_slice_idx = currMB->slice_nr;
    int b8_x = img_x >> 2;
    int b8_y = img_y >> 2;
    int leftdown, upright;

    /* 1. 检查相邻块是否属于同一个Slice */
    p_avail[NEIGHBOR_INTRA_LEFT]      = isBlockAvailable(b8_x, b8_y,             -1,              0, cur_slice_idx);
    p_avail[NEIGHBOR_INTRA_UP]        = isBlockAvailable(b8_x, b8_y,              0,             -1, cur_slice_idx);
    p_avail[NEIGHBOR_INTRA_UP_LEFT]   = isBlockAvailable(b8_x, b8_y,             -1,             -1, cur_slice_idx);
    p_avail[NEIGHBOR_INTRA_LEFT_DOWN] = isBlockAvailable(b8_x, b8_y,             -1, (bsy >> 1) - 1, cur_slice_idx);  // ((bsy * 2 / 4) - 1)
    p_avail[NEIGHBOR_INTRA_UP_RIGHT]  = isBlockAvailable(b8_x, b8_y, (bsx >> 1) - 1,             -1, cur_slice_idx);  // ((bsx * 2 / 4) - 1)

    /* 2. 检查相邻块是否在当前块之前重构 */
    if (maxCuSizeInBit == B64X64_IN_BIT) {
        leftdown = g_Left_Down_Avail_Matrix64[(b8_y + (bsy >> 2) - 1) & 15][(b8_x) & 15];
        upright  =  g_Up_Right_Avail_Matrix64[(b8_y) & 15][(b8_x + (bsx >> 2) - 1) & 15];
    } else if (maxCuSizeInBit == B32X32_IN_BIT) {
        leftdown = g_Left_Down_Avail_Matrix32[(b8_y + (bsy >> 2) - 1) & 7][(b8_x) & 7];
        upright  =  g_Up_Right_Avail_Matrix32[(b8_y) & 7][(b8_x + (bsx >> 2) - 1) & 7];
    } else if (maxCuSizeInBit == B16X16_IN_BIT) {
        leftdown = g_Left_Down_Avail_Matrix16[(b8_y + (bsy >> 2) - 1) & 3][(b8_x) & 3];
        upright  =  g_Up_Right_Avail_Matrix16[(b8_y) & 3][(b8_x + (bsx >> 2) - 1) & 3];
    } else if (maxCuSizeInBit == B8X8_IN_BIT) {
        leftdown = g_Left_Down_Avail_Matrix8[(b8_y + (bsy >> 2) - 1) & 1][(b8_x) & 1];
        upright  =  g_Up_Right_Avail_Matrix8[(b8_y) & 1][(b8_x + (bsx >> 2) - 1) & 1];
    }

    p_avail[NEIGHBOR_INTRA_LEFT_DOWN] = (leftdown && p_avail[NEIGHBOR_INTRA_LEFT_DOWN]);
    p_avail[NEIGHBOR_INTRA_UP_RIGHT]  = (upright  && p_avail[NEIGHBOR_INTRA_UP_RIGHT]);
}
#endif
