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
#ifndef _DEFINES_H_
#define _DEFINES_H_

#define EXTRACT 1
#define EXTRACT_DPB_ColMV_B 1

#define RD      "14.0"
#define VERSION "14.0"

/* ALF Optimization */
#define ALF_FAST	               1	// Discard Extended Border

#define RESERVED_PROFILE_ID      0x24
#define BASELINE_PICTURE_PROFILE 18
#define BASELINE_PROFILE         32  //0x20
#define BASELINE10_PROFILE       34  //0x22
#define TRACE                    0 //!< 0:Trace off 1:Trace on
#include <stdlib.h>

#ifndef WIN32
#include <sys/types.h>
#endif

/*
*************************************************************************************
* AVS2 macros start
*
*************************************************************************************
*/
#define ENC_FRAME_SKIP                     1

#define INTERLACE_CODING                   1
#if INTERLACE_CODING  //M3531: MV scaling compensation
//Luma component
#define HALF_PIXEL_COMPENSATION            1 //common functions definition
#define HALF_PIXEL_COMPENSATION_PMV        1 //spacial MV prediction
#define HALF_PIXEL_COMPENSATION_DIRECT     1 //B direct mode
#define HALF_PIXEL_COMPENSATION_M1         1 //MV derivation method 1, weighted P_skip mode
#define HALF_PIXEL_COMPENSATION_M1_FUCTION 1 //M1 related with mv-scaling function
#define HALF_PIXEL_COMPENSATION_MVD        1 //MV scaling from FW->BW
//Chroma components
#define HALF_PIXEL_CHROMA                  1 //chroma MV is scaled with luma MV for 4:2:0 format
#define HALF_PIXEL_PSKIP                   1 //half pixel compensation for p skip/direct

#define INTERLACE_CODING_FIX               1 //HLS fix
#define OUTPUT_INTERLACE_MERGED_PIC        1

#endif
/*
*************************************************************************************
AVS2 10bit/12bit profile
*************************************************************************************
*/
#define EXTEND_BD                1
#define DBFIX_10bit              1

/*
*************************************************************************************
AVS2 HIGH LEVEL SYNTAX
*************************************************************************************
*/
#define AVS2_HDR_HLS             1
#define DEMULATE_HLS             1

/*
*************************************************************************************
AVS2 S2
*************************************************************************************
*/
#define AVS2_S2_BGLONGTERM
#define AVS2_S2_S
#define AVS2_S2_SBD                     // M3357
#define AVS2_S2_FASTMODEDECISION 1
#define AVS2_SCENE_CD            1
#define AVS2_SCENE_GB_CD         1
#define AVS2_SCENE_MV			 1
#define AVS2_SCENE_STAT_DUP_GB	 1

//////////////////// 8x8 CU ////////////////////////////////////////
#define M3198_CU8                1

#if M3198_CU8
#define BBRY_CU8                 1
#endif

//////////////////// intra prediction /////////////////////////////
#define INTRA_ADI_FULL           0      // M3054
#define INTRA_2MPM_FIX           1      // M3304
#define M3460_DC_MUL_REMOVE      1
#define SIMP_INTRA               1      // M3459

//////////////////// prediction techniques /////////////////////////////
#define INTRA_2N                 1
#define LAM_2Level_TU            0.8

#define WEIGHTED_SKIP            1
#if WEIGHTED_SKIP
#define WPM_ACE_OPT              1     //M3383
#define WPM_ACE_FIX              1
#define ChromaWSM                1
#if ChromaWSM
#define ChromaWSMFix             1
#endif
#endif
#define DIRECTION                4
#define DS_FORWARD               4
#define DS_BACKWARD              2
#define DS_SYM                   3
#define DS_BID                   1

#define MH_PSKIP_NUM             4
#define NUM_OFFSET               0
#define BID_P_FST                1                                   
#define BID_P_SND                2
#define FW_P_FST                 3
#define FW_P_SND                 4
#define MH_PSKIP_NEW             1
#if WPM_ACE_FIX
#define WPM_NUM                  3
#else
#if WPM_ACE_OPT
#define WPM_NUM                  2
#endif
#endif
#define MAX_MVP_CAND_NUM         2     // M3330 changes it to 2, the original value is 3
#define MV_SCALE_14BIT           1     // M3388
#define IF_CHROMA_4TAP_FIX       1
#define DMH_MODE_NUM             5     // Number of DMH mode
#define DMH_ENC_COMP_REDUCT      1     // DMH encoder complexity reduction
#define RD_SYNTAX_BUF_MEM_FIX    1     // RD S/W Bug fix (Not directly related to DMH)

#define DMH_FAST_ENC             1 

#define DHP_ENC_OPT              1     // M3435 DHP encoder optimized
#define DHP_OPT                  1     // M3415 DHP optimized
#define TH_ME                    0     // Threshold of ME
//////////////////// reference picture management /////////////////////////////
#define REF_MAXBUFFER            8
#define REF_CODEC_DELETE         1
#define REF_BUG_FIX              1

//////////////////// deblocking   ///////////////////////////////////////
#define DBLK_16x16_BASED         1
#define DBLK_FRAME_BASED         1
#define CHROMA_DEBLOCK_BUG_FIX   1   // M3834, Fix bug of edge position for chroma block, sujing_jy@pku.edu.cn

///////////////////Adaptive Loop Filter//////////////////////////
#define NUM_ALF_COEFF_CTX        1
#define NUM_ALF_LCU_CTX          4

#define ALF_FCD                  1
#define ALF_BOUNDARY_FIX         1

#define BUFFER_CLEAN             1
#define ALF_USE_SAOLIKE_LAMBDA   1    //MTK, use SAO-like lambda settings
#if ALF_USE_SAOLIKE_LAMBDA
#define LAMBDA_SCALE_LUMA       (1.0)
#define LAMBDA_SCALE_CHROMA     (1.0)
#else
#define LAMBDA_SCALE_LUMA        (0.9)
#define LAMBDA_SCALE_CHROMA      (0.8)
#endif

#define ALF_ESTIMATE_AEC_RATE              1 //MTK, enable CABAC state update during RDO
#define ALF_ENC_IMPORVE_RATE_ESTIMATE      1 //MTK, add accurate rate estimate during redesigning filter in HE mode

//////////////////// entropy coding /////////////////////////////
#define CBP_MODIFICATION         1
#define CU_TYPE_BINARIZATION     0
#define NUN_VALUE_BOUND          254  // M3090: Make sure rs1 will not overflow for 8-bit unsign char
#define By_PASS_1024             1    //M3484
#if By_PASS_1024
#define Decoder_BYPASS_Final     1    // M3484
#define Encoder_BYPASS_Final     1    // M3484
#define Decoder_Bypass_Annex     0    // M3484 
#define Decoder_Final_Annex      0    // M3540
#define AEC_LCU_STUFFINGBIT_FIX  1
#endif

//////////////////// coefficient coding /////////////////////////////
#define CG_SIZE                  16    // M3035 size of an coefficient group, 4x4

#define SWAP(x,y)                {(y)=(y)^(x);(x)=(y)^(x);(y)=(x)^(y);}       

#define M3548_LASTCG_UNIFICATION 1
//////////////////// bug fix /////////////////////////////
#define HIE_BUGFIX               1
#define RAP_BUGFIX               1

#define MultiSliceFix                      1
//////////////////// encoder optimization //////////////////////////////////////////////
#define	TH                                 2
#define LAMBDA_CHANGE                      1  // M3410 change the value of lambda

#define M3624MDLOG

#define TDRDO                              1  // M3528
#define RATECONTROL                        1  // M3580 M3627 M3689
#define AQPO                               1  // M3623
#define AQPOM3694						               1
#define AQPOM3762						               1
#define MD5                                1  // M3685

#define DEMULATE_FIX                       1
//#define REPORT
//////////////////// Quantization   ///////////////////////////////////////
#define FREQUENCY_WEIGHTING_QUANTIZATION    1 // Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
#define CHROMA_DELTA_QP                     1
#define AWQ_WEIGHTING                       1
#define AWQ_LARGE_BLOCK_ENABLE              1
#define COUNT_BIT_OVERHEAD                  0
#define AWQ_LARGE_BLOCK_EXT_MAPPING         1
#endif

#define QuantClip                           1

#define WQ_MATRIX_FCD                       1
#if !WQ_MATRIX_FCD
#define WQ_FLATBASE_INBIT  7
#else
#define WQ_FLATBASE_INBIT  6
#endif

#if CHROMA_DELTA_QP
#define CHROMA_DELTA_QP_SYNTEX_OPT          1
#endif 
#define CHROMAQP_DB_FIX                     1


#define REFINED_QP                          1
#define REFINED_QP_FIX                      1
#define QUANT_FIX                           1
#define QUANT_RANGE_FIX_REF                 1    // 20151219, Fix invalid QP for P/F/B frame, falei.luo@vipl.ict.ac.cn

//////////////////// delta QP /////////////////////////////
#define MB_DQP                    1      // M3122: the minimum dQP unit is Macro block
#define LEFT_PREDICTION           1      // M3122: 1 represents left prediction and 0 represents previous prediction
////////////////////////SAO//////////////////////////////////////////////////////////////

#define NUM_BO_OFFSET             32  
#define MAX_NUM_SAO_CLASSES       32
#define NUM_SAO_BO_CLASSES_LOG2   5
#define NUM_SAO_BO_CLASSES_IN_BIT 5
#define MAX_DOUBLE                1.7e+308
#define NUM_SAO_EO_TYPES_LOG2 2
#define NUM_SAO_BO_CLASSES (1<<NUM_SAO_BO_CLASSES_LOG2)
#define SAO_RATE_THR            0.75
#define SAO_RATE_CHROMA_THR       1
#define SAO_SHIFT_PIX_NUM         4
#define SAO_ENC_FIX               1  //M3483

#define SAO_SLICE_BOUND_PRO       1  //M3552

#if SAO_SLICE_BOUND_PRO

#define SAO_CROSS_SLICE_FLAG      1

#if !SAO_CROSS_SLICE_FLAG
#define SAO_CROSS_SLICE           1
#endif

#define SAO_PARA_CROSS_SLICE      1
#define SAO_MULSLICE_FIX          1
#endif
///////////////////// Transform /////////////////////
#define SEC_TR_SIZE               4
#define SEC_TR_MIN_BITSIZE        3   // apply secT to greater than or equal to 8x8 block,

#if EXTEND_BD
#define BUGFIXED_COMBINED_ST_BD   1
#endif

///////////////////// Scalable /////////////////////
#define M3480_TEMPORAL_SCALABLE   1
#define TEMPORAL_MAXLEVEL         8 
#define TEMPORAL_MAXLEVEL_BIT     3




/*
*************************************************************************************
* AVS2 macros end
*
*************************************************************************************
*/

#define CHROMA                    1
#define LUMA_8x8                  2
#define NUM_BLOCK_TYPES           8

#define clamp(a,b,c) ( (a)<(b) ? (b) : ((a)>(c)?(c):(a)) )    //!< clamp a to the range of [b;c]

#define LOG2_MAX_FRAME_NUM_MINUS4    4           // POC200301 moved from defines.h
#if EXTEND_BD
#define MAX_CODED_FRAME_SIZE 15000000         //!< bytes for one frame
#else
#define MAX_CODED_FRAME_SIZE 1500000         //!< bytes for one frame
#endif

// ---------------------------------------------------------------------------------
// FLAGS and DEFINES for new chroma intra prediction, Dzung Hoang
// Threshold values to zero out quantized transform coefficients.
// Recommend that _CHROMA_COEFF_COST_ be low to improve chroma quality
#define _LUMA_COEFF_COST_         4 //!< threshold for luma coeffs

#define IMG_PAD_SIZE              64   //!< Number of pixels padded around the reference frame (>=4)



#define absm(A) ((A)<(0) ? (-(A)):(A)) //!< abs macro, faster than procedure
#define MAX_VALUE                999999   //!< used for start value for some variables

#define Clip1(a)            ((a)>255?255:((a)<0?0:(a)))
#define Clip3(min,max,val)  (((val)<(min))?(min):(((val)>(max))?(max):(val)))

// ---------------------------------------------------------------------------------

// block size of block transformed by AVS
#define PSKIPDIRECT               0
#define P2NX2N                    1
#define P2NXN                     2
#define PNX2N                     3
#define PHOR_UP                   4
#define PHOR_DOWN                 5
#define PVER_LEFT                 6
#define PVER_RIGHT                7
#define PNXN                      8
#define I8MB                      9
#define I16MB                     10
#define IBLOCK                    11
#define InNxNMB                   12
#define INxnNMB                   13
#define MAXMODE                   14   // add yuqh 20130824
#define  LAMBDA_ACCURACY_BITS     16
#define  LAMBDA_FACTOR(lambda)        ((int)((double)(1<<LAMBDA_ACCURACY_BITS)*lambda+0.5))
#define  WEIGHTED_COST(factor,bits)   (((factor)*(bits))>>LAMBDA_ACCURACY_BITS)
#define  MV_COST(f,s,cx,cy,px,py)     (WEIGHTED_COST(f,mvbits[((cx)<<(s))-px]+mvbits[((cy)<<(s))-py]))
#define  REF_COST(f,ref)              (WEIGHTED_COST(f,refbits[(ref)]))

#define  BWD_IDX(ref)                 (((ref)<2)? 1-(ref): (ref))
#define  REF_COST_FWD(f,ref)          (WEIGHTED_COST(f,((img->num_ref_pic_active_fwd_minus1==0)? 0:refbits[(ref)])))
#define  REF_COST_BWD(f,ref)          (WEIGHTED_COST(f,((img->num_ref_pic_active_bwd_minus1==0)? 0:BWD_IDX(refbits[ref]))))

#if INTRA_2N
#define IS_INTRA(MB)                  ((MB)->cuType==I8MB||(MB)->cuType==I16MB||(MB)->cuType==InNxNMB ||(MB)->cuType==INxnNMB)
#define IS_INTER(MB)                  ((MB)->cuType!=I8MB && (MB)->cuType!=I16MB&&(MB)->cuType!=InNxNMB &&(MB)->cuType!=INxnNMB)
#define IS_INTERMV(MB)                ((MB)->cuType!=I8MB && (MB)->cuType!=I16MB &&(MB)->cuType!=InNxNMB &&(MB)->cuType!=INxnNMB&& (MB)->cuType!=0)
#else
#define IS_INTRA(MB)                 ((MB)->cuType==I8MB)
#define IS_INTER(MB)                 ((MB)->cuType!=I8MB)
#define IS_INTERMV(MB)               ((MB)->cuType!=I8MB && (MB)->cuType!=0)
#endif

#define IS_DIRECT(MB)                ((MB)->cuType==PSKIPDIRECT     && (img ->   type==B_IMG))
#define IS_P_SKIP(MB)                ((MB)->cuType==PSKIPDIRECT     && (((img ->   type==F_IMG))||((img ->   type==P_IMG))))
#define IS_P8x8(MB)                  ((MB)->cuType==PNXN)

// Quantization parameter range
#define MIN_QP                       0
#if EXTEND_BD
#define MAX_QP                       63
#else
#define MAX_QP                       63
#endif
#define SHIFT_QP                     11

// Picture types
#define INTRA_IMG                    0   //!< I frame
#define INTER_IMG                    1   //!< P frame
#define B_IMG                        2   //!< B frame
#define I_IMG                        0   //!< I frame
#define P_IMG                        1   //!< P frame
#define F_IMG                        4  //!< F frame

#ifdef AVS2_S2_BGLONGTERM
#define BACKGROUND_IMG	             3
#endif

#ifdef AVS2_S2_S
#define BP_IMG                       5
#endif

// Direct Mode types
#if M3198_CU8
#define MIN_CU_SIZE                  8
#define MIN_BLOCK_SIZE               4
#define MIN_CU_SIZE_IN_BIT           3
#define MIN_BLOCK_SIZE_IN_BIT        2
#else
#define MIN_CU_SIZE                  16
#define MIN_BLOCK_SIZE               8
#define MIN_CU_SIZE_IN_BIT           4
#define MIN_BLOCK_SIZE_IN_BIT        3
#endif
#define BLOCK_MULTIPLE              (MIN_CU_SIZE/(MIN_BLOCK_SIZE))
#define MAX_CU_SIZE                  64
#define MAX_CU_SIZE_IN_BIT           6
#if M3198_CU8
#define B4X4_IN_BIT                  2
#endif
#define B8X8_IN_BIT                  3
#define B16X16_IN_BIT                4
#define B32X32_IN_BIT                5
#define B64X64_IN_BIT                6
#define NUM_INTRA_PMODE              33        //!< # luma intra prediction modes
#define NUM_MODE_FULL_RD             9         // number of luma modes for full RD search
#define NUM_INTRA_PMODE_CHROMA       5         //!< #chroma intra prediction modes

// luma intra prediction modes

#define DC_PRED                      0
#define PLANE_PRED                   1
#define BI_PRED                      2
#define VERT_PRED                    12
#define HOR_PRED                     24


// chroma intra prediction modes
#define DM_PRED_C                    0
#define DC_PRED_C                    1
#define HOR_PRED_C                   2
#define VERT_PRED_C                  3
#define BI_PRED_C                    4

#define EOS                          1         //!< End Of Sequence
#define SOP                          2                       //!< Start Of Picture

#define DECODING_OK                  0
#define SEARCH_SYNC                  1
#define DECODE_MB                    1
#ifndef WIN32
#define max(a, b)                   ((a) > (b) ? (a) : (b))  //!< Macro returning max value
#define min(a, b)                   ((a) < (b) ? (a) : (b))  //!< Macro returning min value
#endif

#define m3469                        1  //MVP and MV clips

#define M3472                        1  //3D Packing Mode

#define XY_MIN_PMV                   1
#if XY_MIN_PMV
#define MVPRED_xy_MIN                0
#else
#define MVPRED_MEDIAN                0
#endif
#define MVPRED_L                     1
#define MVPRED_U                     2
#define MVPRED_UR                    3

#define DUAL                         4
#define FORWARD                      0
#define BACKWARD                     1
#define SYM                          2
#define BID                          3
#define INTRA                        -1

#define M3175_SHIFT                  1
#define TRANS_16_BITS	             1
#define B_MODE_FIX                   1
#define CLIP_FIX                     1

#define RDOQ_ROUNDING_ERR            1 

#define BUF_CYCLE                    5

#define ROI_M3264			         1		// ROI Information Encoding

#define PicExtensionData             1

#define SHIFT_DQ_INV                 1  //M3317

#define NEW_AEC_CUTYPE               1  //M3336
#define REF_OUTPUT                   1  //M3337
#define REF_OUTPUT_BUG_F             1
#define LASTCB_BUG_FIX	             1  //M3325 

#define SIMP_MV                      1  //M3293 Simplified MV (w/o 4x4 MV; w/o 8x4 or 4x8 bi-dir MV)
                                        //M3293 should be enabled with CCD_P = 1 and NEW_AEC_CUTYPE = 1


#define UNIFY_BINARIZATION           1  //M3377 Unification of binarization
#define FIX_LEAK                     1


#define M3452_TC                     1
#define M3476_BUF_FIX                1
#define M3475_RANDOM_ACCESS          1
#define M3456_SIMP_F                 1
#define M3474_LEVEL                  1

#define MV_SCALING_UNIFY             1 // M3463 Unification of mv scaling
#if MV_SCALING_UNIFY
#define MULTI                        16384
#define HALF_MULTI                   8192
#define OFFSET                       14
#endif

#define   OFF_4X4PU                  1

#define   M3544_CONTEXT_OPT          1

// M3543 motion information storage compression
#define MV_COMPRESSION               1
#define MV_DECIMATION_FACTOR         4  // store the middle pixel's mv in a motion information unit

#define TEXT_RD92_ALIGN              1

#define M3595_REMOVE_REFERENCE_FLAG         1
#define M3578_CONTEXT_OPT                   1

#define D20141230_BUG_FIX                   1
#define HZC_COEFF_RUN_FIX                   1


#define HZC_INTRA_CHROMA_MODE               1

#define HZC_TR_FLAG                         1

#define CTP_Y_BUG_FIX                       1
#define CLIP                                1
#define SAO_MERGE_OPT                       1
#define SAO_INTERVAL_OPT                    1
#define ALF_REGION_OPT                      1
#define S_FRAME_OPT                         1


#define HZC_BUGFIX                          1

#define COEFF_BUGFIX                        1
#define PMV_BUGFIX                          1

#define BUGFIX20150607                      1

#define LAST_DQP_BUGFIX                     0

#define DEBLOCK_NXN                         0

#define BUGFIX_INTRA_REF_SAMPLES            1  // fix procedure of filling reference samples for intra prediction in multi slice cases, falei.luo@vipl.ict.ac.cn
#define BUGFIX_AVAILABILITY_INTRA           1  // availability for intra prediction, falei.luo@vipl.ict.ac.cn
#if BUGFIX_AVAILABILITY_INTRA
#define NEIGHBOR_INTRA_LEFT                 0
#define NEIGHBOR_INTRA_UP                   1
#define NEIGHBOR_INTRA_UP_RIGHT             2
#define NEIGHBOR_INTRA_UP_LEFT              3
#define NEIGHBOR_INTRA_LEFT_DOWN            4
#endif

#define RANDOM_BUGFIX                       1
#define MOTION_BUGFIX                       1

#endif // #if _DEFINES_H_