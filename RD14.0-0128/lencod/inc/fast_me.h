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

* Function: Macro definitions and global variables for fast integer pel motion
        estimation and fractional pel motio estimation
*
*************************************************************************************
*/


#ifndef _FAST_ME_H_
#define _FAST_ME_H_
#include "global.h"
#include "../../lcommon/inc/commonVariables.h"
#define EARLY_TERMINATION  if(ref>0)  \
  {                                                                    \
  if ((min_mcost-pred_SAD_ref)<pred_SAD_ref*betaThird)             \
  goto third_step;                                             \
  else if((min_mcost-pred_SAD_ref)<pred_SAD_ref*betaSec)           \
  goto sec_step;                                               \
  }                                                                    \
  else if(blocktype>1)                                                 \
  {                                                                    \
  if ((min_mcost-pred_SAD_uplayer)<pred_SAD_uplayer*betaThird)     \
    {                                                                \
    goto third_step;                                             \
    }                                                                \
    else if((min_mcost-pred_SAD_uplayer)<pred_SAD_uplayer*betaSec)   \
    goto sec_step;                                               \
  }                                                                    \
  else                                                                 \
  {                                                                    \
  if ((min_mcost-pred_SAD_space)<pred_SAD_space*betaThird)         \
    {                                                                \
    goto third_step;                                             \
    }                                                                \
    else if((min_mcost-pred_SAD_space)<pred_SAD_space*betaSec)       \
    goto sec_step;	                                             \
  }


#define SEARCH_ONE_PIXEL  if(abs(cand_x - center_x) <=search_range && abs(cand_y - center_y)<= search_range) \
  { \
    if(!McostState[cand_y-center_y+search_range][cand_x-center_x+search_range]) \
    { \
      mcost = MV_COST (lambda_factor, mvshift, cand_x, cand_y, pred_x, pred_y); \
      mcost = PartCalMad(ref_pic, orig_pic, get_ref_line,blocksize_y,blocksize_x,blocksize_x4,mcost,min_mcost,cand_x,cand_y); \
      McostState[cand_y-center_y+search_range][cand_x-center_x+search_range] = mcost; \
      if (mcost < min_mcost) \
      { \
        best_x = cand_x; \
        best_y = cand_y; \
        min_mcost = mcost; \
      } \
    } \
  }
#define SEARCH_ONE_PIXEL1(value_iAbort) if(abs(cand_x - center_x) <=search_range && abs(cand_y - center_y)<= search_range) \
  { \
    if(!McostState[cand_y-center_y+search_range][cand_x-center_x+search_range]) \
    { \
      mcost = MV_COST (lambda_factor, mvshift, cand_x, cand_y, pred_x, pred_y); \
      mcost = PartCalMad(ref_pic, orig_pic, get_ref_line,blocksize_y,blocksize_x,blocksize_x4,mcost,min_mcost,cand_x,cand_y); \
      McostState[cand_y-center_y+search_range][cand_x-center_x+search_range] = mcost; \
      if (mcost < min_mcost) \
      { \
        best_x = cand_x; \
        best_y = cand_y; \
        min_mcost = mcost; \
        iAbort = value_iAbort; \
      } \
    } \
  }


int **McostState; //state for integer pel search

int pred_SAD_space,pred_SAD_ref,pred_SAD_uplayer;//SAD prediction
int pred_MV_time[2], pred_MV_ref[2], pred_MV_uplayer[2]; //pred motion vector by space or tempral correlation,Median is provided

//for half_stop
double Quantize_step;
double  Bsize[9];
int Thresh4x4;
double AlphaSec[9];
double AlphaThird[9];
int  flag_intra[1024];
int  flag_intra_SAD;
void DefineThreshold();
void DefineThresholdMB();
void decide_intrabk_SAD(int uiPositionInPic);
void skip_intrabk_SAD(int best_mode, int ref_max, int uiPositionInPic ,int uiBitSize);


byte **SearchState; //state for fractional pel search

int get_mem_mincost ( int****** mv );
int get_mem_bwmincost ( int****** mv );
int get_mem_FME();
void free_mem_mincost ( int***** mv );
void free_mem_bwmincost ( int***** mv );
void free_mem_FME();
void FME_SetMotionVectorPredictor ( unsigned int uiBitSize, unsigned int uiPositionInPic, int  pmv[2], int  **refFrArr,
                                   int  ***tmp_mv, int  ref_frame, int  mb_pix_x, int  mb_pix_y, int  blockshape_x, int  blockshape_y, int blocktype, int  ref, int  direct_mv );

extern void SetMotionVectorPredictor ( unsigned int uiBitSize, unsigned int uiPositionInPic, int  pmv[2], int  **refFrArr, int  ***tmp_mv, int  ref_frame, int  mb_pix_x,
                                       int  mb_pix_y, int  blockshape_x, int  blockshape_y, int  ref, int  direct_mv );


int                                     //  ==> minimum motion cost after search
FastIntegerPelBlockMotionSearch ( pel_t     orig_pic[MAX_CU_SIZE][MAX_CU_SIZE],     // <--  not used
                                  int       ref,          // <--  reference frame (0... or -1 (backward))
                                  int       pic_pix_x,    // <--  absolute x-coordinate of regarded AxB block
                                  int       pic_pix_y,    // <--  absolute y-coordinate of regarded AxB block
                                  int       blocktype,    // <--  block type (1-16x16 ... 7-4x4)
                                  int       pred_mv_x,    // <--  motion vector predictor (x) in sub-pel units
                                  int       pred_mv_y,    // <--  motion vector predictor (y) in sub-pel units
                                  int*      mv_x,         //  --> motion vector (x) - in pel units
                                  int*      mv_y,         //  --> motion vector (y) - in pel units
                                  int       search_range, // <--  1-d search range in pel units
                                  int       min_mcost,    // <--  minimum motion cost (cost for center or huge value)
                                  double    lambda,
                                  int       mb_nr,
                                  int       bit_size,
                                  int       block ) ;     // <--  lagrangian parameter for determining motion cost



int AddUpSADQuarter ( int pic_pix_x, int pic_pix_y, int blocksize_x, int blocksize_y,
                      int cand_mv_x, int cand_mv_y, pel_t **ref_pic, pel_t   orig_pic[MAX_CU_SIZE][MAX_CU_SIZE], int Mvmcost, int min_mcost, int useABT );

int                                                   //  ==> minimum motion cost after search
FastSubPelBlockMotionSearch ( pel_t     orig_pic[MAX_CU_SIZE][MAX_CU_SIZE],     // <--  original pixel values for the AxB block
                              int       ref,           // <--  reference frame (0... or -1 (backward))
                              int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                              int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                              int       blocktype,     // <--  block type (1-16x16 ... 7-4x4)
                              int       pred_mv_x,     // <--  motion vector predictor (x) in sub-pel units
                              int       pred_mv_y,     // <--  motion vector predictor (y) in sub-pel units
                              int*      mv_x,          // <--> in: search center (x) / out: motion vector (x) - in pel units
                              int*      mv_y,          // <--> in: search center (y) / out: motion vector (y) - in pel units
                              int       search_pos2,   // <--  search positions for    half-pel search  (default: 9)
                              int       search_pos4,   // <--  search positions for quarter-pel search  (default: 9)
                              int       min_mcost,     // <--  minimum motion cost (cost for center or huge value)
                              double    lambda,
                              int       useABT,
                              int       mb_nr,
                              int       bit_size,
                              int       block
                            );        // <--  lagrangian parameter for determining motion cost



int                                               //  ==> minimum motion cost after search
SubPelBlockMotionSearch ( pel_t     orig_pic[MAX_CU_SIZE][MAX_CU_SIZE],     // <--  original pixel values for the AxB block
                          int       ref,           // <--  reference frame (0... or -1 (backward))
                          int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                          int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                          int       blocktype,     // <--  block type (1-16x16 ... 7-4x4)
                          int       pred_mv_x,     // <--  motion vector predictor (x) in sub-pel units
                          int       pred_mv_y,     // <--  motion vector predictor (y) in sub-pel units
                          int*      mv_x,          // <--> in: search center (x) / out: motion vector (x) - in pel units
                          int*      mv_y,          // <--> in: search center (y) / out: motion vector (y) - in pel units
                          int       search_pos2,   // <--  search positions for    half-pel search  (default: 9)
                          int       search_pos4,   // <--  search positions for quarter-pel search  (default: 9)
                          int       min_mcost,     // <--  minimum motion cost (cost for center or huge value)
                          double    lambda ,        // <--  lagrangian parameter for determining motion cost
                          int       bit_size,
                          int       block
                        );


int                                               //  ==> minimum motion cost after search
SubPelBlockMotionSearch_sym ( pel_t     orig_pic[MAX_CU_SIZE][MAX_CU_SIZE],     // <--  original pixel values for the AxB block
                              int       ref,           // <--  reference frame (0... or -1 (backward))
                              int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                              int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                              int       blocktype,     // <--  block type (1-16x16 ... 7-4x4)
                              int       pred_mv_x,     // <--  motion vector predictor (x) in sub-pel units
                              int       pred_mv_y,     // <--  motion vector predictor (y) in sub-pel units
                              int*      mv_x,          // <--> in: search center (x) / out: motion vector (x) - in pel units
                              int*      mv_y,          // <--> in: search center (y) / out: motion vector (y) - in pel units
                              int       search_pos2,   // <--  search positions for    half-pel search  (default: 9)
                              int       search_pos4,   // <--  search positions for quarter-pel search  (default: 9)
                              int       min_mcost,     // <--  minimum motion cost (cost for center or huge value)
                              double    lambda,         // <--  lagrangian parameter for determining motion cost
                              int       bit_size,
                              int       block
                            );
int SubPelBlockMotionSearch_bid ( pel_t   orig_pic[MAX_CU_SIZE][MAX_CU_SIZE],     // <--  original pixel values for the AxB block
								 int       ref,           // <--  reference frame (0... or -1 (backward))
								 int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
								 int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
								 int       blocktype,     // <--  block type (1-16x16 ... 7-4x4)
								 int       pred_mv_x,     // <--  motion vector predictor (x) in sub-pel units
								 int       pred_mv_y,     // <--  motion vector predictor (y) in sub-pel units
								 int*      mv_x,          // <--> in: search center (x) / out: motion vector (x) - in pel units
								 int*      mv_y,          // <--> in: search center (y) / out: motion vector (y) - in pel units
								 int       pred_bmv_x_bid,     // <--  motion vector predictor (x) in sub-pel units
								 int       pred_bmv_y_bid,     // <--  motion vector predictor (y) in sub-pel units
								 int*      bmv_x_bid,          // <--> in: search center (x) / out: motion vector (x) - in pel units
								 int*      bmv_y_bid,          // <--> in: search center (y) / out: motion vector (y) - in pel units
								 int       search_pos2,   // <--  search positions for    half-pel search  (default: 9)
								 int       search_pos4,   // <--  search positions for quarter-pel search  (default: 9)
								 int       min_mcost,     // <--  minimum motion cost (cost for center or huge value)
								 double    lambda,         // <--  lagrangian parameter for determining motion cost
								 int       bit_size,
								 int       block
								 );


#endif


int                                         //  ==> minimum motion cost after search
FME_BlockMotionSearch ( int       ref,          // <--  reference frame (0... or -1 (backward))
                        int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                        int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                        int       blocktype,     // <--  block type (1-16x16 ... 7-4x4)
                        int       search_range,  // <--  1-d search range for integer-position search
                        double    lambda,
                        int       mb_nr,
                        int       bit_size,     // <--  lagrangian parameter for determining motion cost
                        int       block
                      );
int
FME_BlockMotionSearch_sym ( int       ref,          // <--  reference frame (0... or -1 (backward))
						   int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
						   int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
						   int       blocktype,     // <--  block type (1-16x16 ... 7-4x4)
						   int       search_range,  // <--  1-d search range for integer-position search
						   double    lambda,
						   int       mb_nr,
						   int       bit_size,      // <--  lagrangian parameter for determining motion cost
						   int       block,
						   int*      mcost,
						   int*      mcost_bid
						   );


int                                                   //  ==> minimum motion cost after search
FastSubPelBlockMotionSearch_sym ( pel_t   orig_pic[MAX_CU_SIZE][MAX_CU_SIZE],     // <--  original pixel values for the AxB block
                                  int       ref,           // <--  reference frame (0... or -1 (backward))
                                  int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                                  int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                                  int       blocktype,     // <--  block type (1-16x16 ... 7-4x4)
                                  int       pred_mv_x,     // <--  motion vector predictor (x) in sub-pel units
                                  int       pred_mv_y,     // <--  motion vector predictor (y) in sub-pel units
                                  int*      mv_x,          // <--> in: search center (x) / out: motion vector (x) - in pel units
                                  int*      mv_y,          // <--> in: search center (y) / out: motion vector (y) - in pel units
                                  int       search_pos2,   // <--  search positions for    half-pel search  (default: 9)
                                  int       search_pos4,   // <--  search positions for quarter-pel search  (default: 9)
                                  int       min_mcost,     // <--  minimum motion cost (cost for center or huge value)
                                  double    lambda,
                                  int       useABT,
                                  int       mb_nr,
                                  int       bit_size,
                                  int       block );       // <--  lagrangian parameter for determining motion cost

int                                                   //  ==> minimum motion cost after search
FastSubPelBlockMotionSearch_bid ( pel_t   orig_pic[MAX_CU_SIZE][MAX_CU_SIZE],     // <--  original pixel values for the AxB block
									 int       ref,           // <--  reference frame (0... or -1 (backward))
									 int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
									 int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
									 int       blocktype,     // <--  block type (1-16x16 ... 7-4x4)
									 int       pred_mv_x,     // <--  motion vector predictor (x) in sub-pel units
									 int       pred_mv_y,     // <--  motion vector predictor (y) in sub-pel units
									 int*      mv_x,          // <--> in: search center (x) / out: motion vector (x) - in pel units
									 int*      mv_y,          // <--> in: search center (y) / out: motion vector (y) - in pel units
									 int       pred_bmv_x_bid,
									 int       pred_bmv_y_bid,
									 int*      bmv_x_bid,
									 int*      bmv_y_bid,
									 int       search_pos2,   // <--  search positions for    half-pel search  (default: 9)
									 int       search_pos4,   // <--  search positions for quarter-pel search  (default: 9)
									 int       min_mcost,     // <--  minimum motion cost (cost for center or huge value)
									 double    lambda,
									 int  useABT,
									 int mb_nr,
									 int bit_size,
									 int block);



int
FME_BlockMotionSearch_dual ( int       ref,          // <--  reference frame (0... or -1 (backward))
						   int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
						   int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
						   int       blocktype,     // <--  block type (1-16x16 ... 7-4x4)
						   int       search_range,  // <--  1-d search range for integer-position search
						   double    lambda,
						   int       mb_nr,
						   int       bit_size,      // <--  lagrangian parameter for determining motion cost
						   int       block,
						   int*      mcost_dual);

int AddUpSADQuarter_sym ( int pic_pix_x, int pic_pix_y, int blocksize_x, int blocksize_y,
                          int cand_mv_x, int cand_mv_y, pel_t **ref_pic, pel_t **ref_pic_sym, pel_t   orig_pic[MAX_CU_SIZE][MAX_CU_SIZE],
                          int Mvmcost, int min_mcost, int useABT, int DistanceIndexFw, int DistanceIndexBw );


int AddUpSADQuarter_bid ( int pic_pix_x, int pic_pix_y, int blocksize_x, int blocksize_y,
							 int cand_mv_x, int cand_mv_y, pel_t **ref_pic, pel_t **ref_pic_sym, pel_t   orig_pic[MAX_CU_SIZE][MAX_CU_SIZE],
							 int Mvmcost, int min_mcost, int useABT, int cand_bmv_x_bid, int cand_bmv_y_bid );

