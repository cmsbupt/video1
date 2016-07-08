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
* File name: global.h
* Function:  global definitions for for AVS decoder.
*
*************************************************************************************
*/

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <stdio.h>                              //!< for FILE
#include "../../lcommon/inc/defines.h"
#include "../../lcommon/inc/commonStructures.h"
#include "../../lcommon/inc/commonVariables.h"

#define EXTRACT 1  
#if EXTRACT
#define EXTRACT_SliceByteAscii_08X 1 //按照2个字节后1个空格 %02x 2016-3-14
#define EXTRACT_08X 1
#define EXTRACT_D 0

	
#define EXTRACT_DPB_RCS 0
#define EXTRACT_DPB 0
#define EXTRACT_DPB_POC_B 1 // B 逆序打印
#define EXTRACT_DPB_ColMV_B 1 // B 逆序打印
#define EXTRACT_Coeff 0


#define cms_dpb 0
#define EXTRACT_lcu_info_BAC 0  //控制cu 信息是否打印
#define EXTRACT_lcu_info_BAC_inter 0
#define EXTRACT_F_MV_debug 0
#define EXTRACT_MV_debug 0


#define EXTRACT_lcu_info_debug 1  //控制lcu debug信息是否打印
#define EXTRACT_lcu_info_mvp_debug 0  //控制lcu debug信息是否打印
#define EXTRACT_lcu_info_cutype_debug 0  //控制lcu debug信息是否打印
#define EXTRACT_lcu_info_cutype_aec_debug 0  //控制lcu debug信息是否打印
#define EXTRACT_lcu_info_coeff_debug 1  //控制lcu debug信息是否打印
#define EXTRACT_lcu_info_cutype_aec_debug2 0  //控制lcu debug信息是否打印
#define EXTRACT_lcu_info_debug_IntraPred_UV 0  //控制lcu debug信息是否打印


#define EXTRACT_lcu_info_debug_intra 1  //控制lcu debug信息是否打印
#define EXTRACT_PIC_SAO 1  //2016-5-20 提取整帧的sao最后参数
#define EXTRACT_PIC_SAO_debug 0 

#define EXTRACT_PIC_YUV_PRED 1  //2016-5-23 提取整帧图像的经过预测后的数据
#define EXTRACT_PIC_YUV_TQ 1  //2016-5-23 提取整帧图像的经过预测+残差后的数据
#define EXTRACT_PIC_YUV_DF 0  //2016-5-23 提取整帧图像的经过去块后的数据
#define EXTRACT_PIC_YUV_SAO 0  //2016-5-23 提取整帧图像的经过sao处理后的数据
#define EXTRACT_PIC_YUV_ALF 0  //2016-5-23 提取整帧图像的经过sao处理后的数据


#define EXTRACT_FULL 1  //2016-5-31 提取完整的参数

#define EXTRACT_FULL_PPS  1 //2016-5-31 提取完整的参数
#define EXTRACT_FULL_PPS_debug  0 //2016-5-31 提取完整的参数
#define EXTRACT_FULL_PPS_ALF_final  0 //2016-5-31 提取完整的参数
#define EXTRACT_FULL_PPS_ALF_debug  0 //2016-5-31 提取完整的参数
#define EXTRACT_FULL_SLICE  1 //2016-5-31 提取完整的参数

#define EXTRACT_PIC_DF_BS 1  //2016-6-1 提取整帧的去块滤波Bs参数
#define EXTRACT_PIC_DF_BS_debug 0 
#define EXTRACT_PIC_DF_BS_Print_1 1//2016-6-1 提取整帧的去块滤波Bs参数

/* 
*************************************************************************************
提取数据
*************************************************************************************
*/
FILE *p_sps;
FILE *p_pps;
FILE *p_pic_sps;    //每个pic打印一次sps
FILE *p_mv_col;     //时域mv信息
FILE *p_slice_bit;  //提取slice的比特流
FILE *p_slice;      //提取slice的信息 QP
FILE *p_lcu;
FILE *p_lcu_coeff;
FILE *p_lcu_mv;
FILE *p_aec;        

#if EXTRACT_lcu_info_debug   //控制lcu debug信息是否打印
FILE *p_lcu_debug;
#endif

#if EXTRACT_PIC_SAO   //2016-5-20 提取整帧的sao最后参数
FILE *p_pic_sao;
#endif

#if EXTRACT_PIC_YUV_PRED   //2016-5-23 提取整帧图像的经过预测后的数据
int *pPicYPred;
int *pPicUVPred[2];
#endif
#if  EXTRACT_PIC_DF_BS   //2016-6-1 提取整帧的去块滤波Bs参数
FILE *p_pic_df_bs;
//建立2个数组，分别存放垂直方向、水平方向的Bs值
//垂直方向 picVerEdgeBsY[Height][Width / 8]; 每隔8个点有一个垂直边界
//水平方向 picHorEdgeBsY[Height / 8][Width]; 每隔8个点有一个水平边界
byte ** picVerEdgeBsY;
byte ** picHorEdgeBsY;

byte ** picVerEdgeBsCb;
byte ** picHorEdgeBsCb;

byte ** picVerEdgeBsCr;
byte ** picHorEdgeBsCr;

#endif

#if  EXTRACT_PIC_YUV_PRED  //2016-5-23 提取整帧图像的经过预测后的数据
FILE *p_pic_yuv_pred;
#endif
#if EXTRACT_PIC_YUV_TQ     //2016-5-23 提取整帧图像的经过预测+残差后的数据
FILE *p_pic_yuv_tq;
#endif
#if EXTRACT_PIC_YUV_DF     //2016-5-23 提取整帧图像的经过去块后的数据
FILE *p_pic_yuv_df;
#endif
#if EXTRACT_PIC_YUV_SAO    //2016-5-23 提取整帧图像的经过sao处理后的数据
FILE *p_pic_yuv_sao;
#endif
#if EXTRACT_PIC_YUV_ALF   //2016-5-23 提取整帧图像的经过sao处理后的数据
FILE *p_pic_yuv_alf;
#endif

#endif
#if EXTRACT
/*
*************************************************************************
提取数据所需变量
*************************************************************************
*/
char newdir[100];//新建目录
char filename[100];
char infile[100];
int  spsNum;
int  spsNewStart;
int  sliceNum;
int  etr_slice_vertical_position;
int  etr_slice_vertical_position_ext;
int  etr_slice_horizontal_position;
int  etr_slice_horizontal_position_ext;
int  etr_fixed_slice_qp;
int  etr_slice_qp;
int  RefPicNum;
int  lcuNum;
int  lcuPosPixX,lcuPosPixY;//LCU 在图像中的像素坐标
int  cuPosInLCUPixX, cuPosInLCUPixY;//CU 在LCU中的像素坐标
//int  puNumInCU;

#if  EXTRACT_FULL  //2016-5-31 提取完整的参数
int sps_progressive_sequence;
int sps_field_coded_sequence;
int sps_sample_precision;
int sps_encoding_precision;
int sps_aspect_ratio;
int sps_frame_rate_code;
int sps_bit_rate;
int sps_low_delay;
int sps_temporal_id_enable;
int sps_bbv_buffer_size;
int sps_weight_quant_enable;
int sps_load_seq_weight_quant_data_flag;
int sps_scene_picture_disable;
int sps_secondary_transform_enable_flag;
int sps_adaptive_loop_filter_enable;
int sps_num_of_rcs;
int sps_output_reorder_delay;
int sps_cross_slice_loopfilter_enable;

#if  EXTRACT_FULL_PPS  //2016-5-31 提取完整的参数
int pps_loop_filter_disable;
int pps_AlphaCOffset;
int pps_BetaOffset;
int pps_chroma_quant_param_delta_cb;
int pps_chroma_quant_param_delta_cr;
int pps_WeightQuantMatrix4x4[4][4];
int pps_WeightQuantMatrix8x8[8][8];
int pps_alf_filter_num_minus1;
int pps_alf_region_distance[16];//16个
int pps_AlfCoeffLuma[16][9];//16个region 每个有9个系数
int pps_AlfCoeffChroma0[9];//U 有9个系数
int pps_AlfCoeffChroma1[9];//V 9个系数
int pps_AlfCoeffLuma_Final[16][9];//16个region 每个有9个系数 处理过后的
int pps_AlfCoeffChroma0_Final[9];//U 有9个系数 每个有9个系数 处理过后的
int pps_AlfCoeffChroma1_Final[9];//V 9个系数 每个有9个系数 处理过后的

int pps_bbv_delay;
int pps_time_code_flag;
int pps_time_code;
int pps_decode_order_index;
int pps_temporal_id;
int pps_picture_output_delay;
int pps_random_access_decodable_flag;
int pps_bbv_check_times;
int pps_progressive_frame;
int pps_top_field_first;
int pps_repeat_first_field;
int pps_use_rcs_flag;
int pps_rcs_index;
#endif


#if  EXTRACT_FULL_SLICE  //2016-5-31 提取完整的参数
int slice_slice_sao_enable[3];
#endif
#endif


int  lcuSAOInfo[3][12];
    //[0-2]是3 个分量
    //lcuSAOInfo[i][0] 是0=off   1=BO 2=Edge
    //lcuSAOInfo[i][1-4] 是4个offset 有符号
    //lcuSAOInfo[i][5] 是区间模式时起始
    //lcuSAOInfo[i][6] 是区间模式时起始偏移 minus2
    //lcuSAOInfo[i][7] 是边缘模式时的类型0=0度，1=90度，2=135度，3=45度
    //lcuSAOInfo[i][8] 是merge FLAG,0 =noMerge,1=merge-left,2=merge-up
     
int  cuInfoIntra[8];
     //0-3是亮度预测模式，
     //4是色度预测模式，
     //5是帧内transform split flag
     //6是帧内intra_pu_type_index  
int  cuInfoInter[8];//
int  puInfoInter[4][16];//4//4个pu,[pu index][ direction]
	 //cuInfoInter[0]:是帧间预测方向：0,1,2,3,4 ->Sym,bipred,double_fwd,bck,single_fwd
	 //cuInfoInter[1]:是帧间预测子类型	skip\direct下才有
                    //P:
                    //B:10种 Skip\Direct 下各有5种
                    //F:12种 Skip\Direct 下各有6种参见表85/表91，0~9(或0~11)分别与”编码单元子类型”按由上至下的顺序一一对应

int  EcuInfoInter[8];//
   //EcuInfoInter[0]= act_sym;//cu_type_index 文档对应
   //EcuInfoInter[1]= -1;//shape_of_partion_index 文档对应
int  EcuInfoInterSyntax[64];//在每个CU 开始时进行初始化
	//EcuInfoInterSyntax[0] 存放句法cu_type_index			默认0, 对于I图像，不会传输，默认为0，对于P/B/F 需要传输，0=skip，1=direct ，2=2Nx2n,3=Hor,4=Ver,5=NxN,6=Intra
	//EcuInfoInterSyntax[1] 存放句法shape_of_partion_index	默认0,对于帧间PBF图像，且CU大于8x8,且允许AMP划分，且CU 是2划分，则需要进一步解析 PU的划分方式，
	//EcuInfoInterSyntax[2] 存放句法b_pu_type_index			默认0
	//EcuInfoInterSyntax[3] 存放句法f_pu_type_index Table74	默认0	
	//EcuInfoInterSyntax[4] 存放句法wighted_skip_mode		默认0		
	//EcuInfoInterSyntax[5] 存放句法F cu_subtype_index		默认0		
	//EcuInfoInterSyntax[6] 存放句法B cu_subtype_index		默认0		
	//EcuInfoInterSyntax[ 7-10] 存放句法B b_pu_type_index2 	NxN 划分时4个PU的预测方向
	//EcuInfoInterSyntax[11-14] 存放句法F f_pu_type_index2 	NxN 划分时4个PU的预测方向
	//EcuInfoInterSyntax[15-18] 存放句法前向PF pu_reference_index	4个PU的参考帧索引
	//EcuInfoInterSyntax[19]    存放句法F dir_multi_hypothesis		默认0		

	//EcuInfoInterSyntax[20] 存放句法PU 0:前向mvd_x		默认0		
	//EcuInfoInterSyntax[21] 存放句法PU 0:前向mvd_y		默认0		
	//EcuInfoInterSyntax[22] 存放句法PU 1:前向mvd_x		默认0		
	//EcuInfoInterSyntax[23] 存放句法PU 1:前向mvd_y		默认0		
	//EcuInfoInterSyntax[24] 存放句法PU 2:前向mvd_x		默认0		
	//EcuInfoInterSyntax[25] 存放句法PU 2:前向mvd_y		默认0		
	//EcuInfoInterSyntax[26] 存放句法PU 3:前向mvd_x		默认0		
	//EcuInfoInterSyntax[27] 存放句法PU 3:前向mvd_y		默认0		

	//EcuInfoInterSyntax[28] 存放句法PU 0:后向mvd_x		默认0		
	//EcuInfoInterSyntax[29] 存放句法PU 0:后向mvd_y		默认0		
	//EcuInfoInterSyntax[30] 存放句法PU 1:后向mvd_x		默认0		
	//EcuInfoInterSyntax[31] 存放句法PU 1:后向mvd_y		默认0		
	//EcuInfoInterSyntax[32] 存放句法PU 2:后向mvd_x		默认0		
	//EcuInfoInterSyntax[33] 存放句法PU 2:后向mvd_y		默认0		
	//EcuInfoInterSyntax[34] 存放句法PU 3:后向mvd_x		默认0		
	//EcuInfoInterSyntax[35] 存放句法PU 3:后向mvd_y		默认0		

	//EcuInfoInterSyntax[36] 存放句法CU ctp 	默认0		
	//EcuInfoInterSyntax[37] 存放句法CU transform_split_flag 	默认0		
	//EcuInfoInterSyntax[38] 存放句法CU cu_qp _delta
	//EcuInfoInterSyntax[39] 存放句法CU qp 	
	
	//EcuInfoInterSyntax[40-43] 存放句法后向PF pu_reference_index	4个PU的参考帧索引
	//EcuInfoInterSyntax[44]    存放句法CU ctp_zero_flag 	默认0		

  //cuInfoInter[0]:是帧间预测方向：0,1,2,3,4 ->Sym,bipred,double_fwd,bck,single_fwd
  //cuInfoInter[1]:是帧间预测子类型  skip\direct下才有
                  //P:
                  //B:10种 Skip\Direct 下各有5种
                  //F:12种 Skip\Direct 下各有6种参见表85/表91，0~9(或0~11)分别与”编码单元子类型”按由上至下的顺序一一对应


	//1.6 MV解码数据
	int EcuInfoInterMv[64];//用于存放解码后的MV 信息
		//EcuInfoInterMv[0];//last_cu  当前LCU在图像中的位置，
							          //{LCUaddr_y>>4, LCUaddr_x>>4}，各占16bits。
							          //LCUaddr_y，LCUaddr_x表示像素点坐标。
		//EcuInfoInterMv[1];//last_cu 
		//EcuInfoInterMv[2];//CU position  y  CU position  x	,{CUaddr_y>>3, CUaddr_x>>3},
						            //CUaddr_y，CUaddr_x  表示LCU内的像素点坐标。
		//EcuInfoInterMv[3];//log2CbSize  CU 的对数大小 4==> 16
		//EcuInfoInterMv[4];//CU pred_mode_flag  0-inter, 1-intra
		//EcuInfoInterMv[5];//CU part_mode CU的类型主要是划分
		//EcuInfoInterMv[6];//PU0: predFlagL0   01-list0; 10-list1; 11-Bi; 
		//EcuInfoInterMv[7];//PU0: predFlagL1   01-list0; 10-list1; 11-Bi; 
		//EcuInfoInterMv[8];//PU1: predFlagL0   01-list0; 10-list1; 11-Bi; 
		//EcuInfoInterMv[9];//PU1: predFlagL1   01-list0; 10-list1; 11-Bi;
		//EcuInfoInterMv[10];//PU2: predFlagL0   01-list0; 10-list1; 11-Bi; 
		//EcuInfoInterMv[11];//PU2: predFlagL1   01-list0; 10-list1; 11-Bi;		
		//EcuInfoInterMv[12];//PU3: predFlagL0   01-list0; 10-list1; 11-Bi; 
		//EcuInfoInterMv[13];//PU3: predFlagL1   01-list0; 10-list1; 11-Bi;

		//EcuInfoInterMv[14];//PU0: refIdxL0
		//EcuInfoInterMv[15];//PU0: refIdxL1
		//EcuInfoInterMv[16];//PU1: refIdxL0
		//EcuInfoInterMv[17];//PU1: refIdxL1
		//EcuInfoInterMv[18];//PU2: refIdxL0
		//EcuInfoInterMv[19];//PU2: refIdxL1
		//EcuInfoInterMv[20];//PU3: refIdxL0
		//EcuInfoInterMv[21];//PU3: refIdxL1
		
		//EcuInfoInterMv[22];//PU0: mvL0_x
		//EcuInfoInterMv[23];//PU0: mvL0_y
		//EcuInfoInterMv[24];//PU0: mvL1_x
		//EcuInfoInterMv[25];//PU0: mvL1_x

		//EcuInfoInterMv[26];//PU1: mvL0_x
		//EcuInfoInterMv[27];//PU1: mvL0_y
		//EcuInfoInterMv[28];//PU1: mvL1_x
		//EcuInfoInterMv[29];//PU1: mvL1_x

		//EcuInfoInterMv[30];//PU2: mvL0_x
		//EcuInfoInterMv[31];//PU2: mvL0_y
		//EcuInfoInterMv[32];//PU2: mvL1_x
		//EcuInfoInterMv[33];//PU2: mvL1_x

		//EcuInfoInterMv[34];//PU3: mvL0_x
		//EcuInfoInterMv[35];//PU3: mvL0_y
		//EcuInfoInterMv[36];//PU3: mvL1_x
		//EcuInfoInterMv[37];//PU3: mvL1_x

		//EcuInfoInterMv[38];//PU0: intra_luma_pred_mode
		//EcuInfoInterMv[39];//PU1: intra_luma_pred_mode
		//EcuInfoInterMv[40];//PU2: intra_luma_pred_mode
		//EcuInfoInterMv[41];//PU3: intra_luma_pred_mode
		//EcuInfoInterMv[42];//CU: intra_chroma_pred_mode
		
		//EcuInfoInterMv[43];//CU:  pu 个数		
#endif

#ifdef AVS2_S2_BGLONGTERM
#if EXTEND_BD
byte **background_frame[3];
#else
unsigned char **background_frame[3];
#endif
int background_reference_enable;

int background_picture_flag;
int background_picture_output_flag;
int background_picture_enable;

int background_number;
#ifdef WIN32
void write_GB_frame(int p_dec);
#else
void write_GB_frame(FILE *p_dec);
#endif
#endif

#ifdef AVS2_S2_S
int background_pred_flag;
#endif

//Reference Manage
int displaydelay;
int picture_reorder_delay;
#define MAXREF    4
#define MAXGOP    32
#if M3480_TEMPORAL_SCALABLE
int temporal_id_exist_flag;
#endif
typedef struct reference_management{
	int poc;
	int qp_offset;
	int num_of_ref;
	int referd_by_others;
	int ref_pic[MAXREF];
	int predict;
	int deltaRPS;
	int num_to_remove;
	int remove_pic[MAXREF];
}ref_man;
int gop_size;
ref_man decod_RPS[MAXGOP];
ref_man curr_RPS;
int last_output;
int trtmp;
#if M3480_TEMPORAL_SCALABLE
int cur_layer;
#endif
int ref_poc[REF_MAXBUFFER][4];

//! Bitstream
typedef struct
{
  // AEC Decoding
  int           read_len;           //!< actual position in the codebuffer, AEC only
  int           code_len;           //!< overall codebuffer length, AEC only
  // UVLC Decoding
  int           frame_bitoffset;    //!< actual position in the codebuffer, bit-oriented, UVLC only
  int           bitstream_length;   //!< over codebuffer lnegth, byte oriented, UVLC only
  // ErrorConcealment
  unsigned char          *streamBuffer;      //!< actual codebuffer for read bytes
} Bitstream;
extern Bitstream *currStream;

typedef struct
{
	unsigned int    Dbuffer;
	int             Dbits_to_go;
	unsigned char   *Dcodestrm;
	int             *Dcodestrm_len;
} DecodingEnvironment;
typedef DecodingEnvironment *DecodingEnvironmentPtr;

// global picture format dependend buffers, mem allocation in ldecod.c ******************


byte **imgYRef;                                //!< reference frame find snr
byte ***imgUVRef;







//added at rm52k version


struct inp_par;

struct syntaxelement
{
  int           type;                  //!< type of syntax element for data part.
  int           value1;                //!< numerical value of syntax element
  int           value2;                //!< for blocked symbols, e.g. run/level
  int           len;                   //!< length of code
  int           inf;                   //!< info part of UVLC code
  int           context;               //!< AEC context

  int           golomb_grad;       //!< Needed if type is a golomb element
  int           golomb_maxlevels;    //!< If this is zero, do not use the golomb coding
#if TRACE
#define       TRACESTRING_SIZE 100           //!< size of trace string
  char          tracestring[TRACESTRING_SIZE]; //!< trace string
#endif

  //! for mapping of UVLC to syntaxElement
  void ( *mapping ) ( int len, int info, int *value1, int *value2 );
  void ( *reading ) ( struct syntaxelement *, DecodingEnvironmentPtr, codingUnit *MB, int uiPosition );
};


#if TRACE
void  trace2out ( SyntaxElement *se );
#endif


//! struct to characterize the state of the arithmetic coding engine
typedef struct datapartition
{

  Bitstream           *bitstream;
  DecodingEnvironment de_AEC;
  int ( *readSyntaxElement ) ( SyntaxElement *, struct datapartition *, codingUnit *currMB, int uiPosition );

} DataPartition;

//! Slice
struct slice
{
  int                 picture_id;
  int                 qp;
  int                 picture_type; //!< picture type
  int                 start_mb_nr;
  int                 max_part_nr;  //!< number of different partitions

  //added by lzhang
  DataPartition       *partArr;     //!< array of partitions
  SyntaxInfoContexts  *syn_ctx;     //!< pointer to struct of context models for use in AEC
};

struct alfdatapart
{
	Bitstream           *bitstream;
	DecodingEnvironment de_AEC;
	SyntaxInfoContexts  *syn_ctx;
};
static int alfParAllcoated=0;





// input parameters from configuration file
struct inp_par
{
  char  infile[1000];              //<! Telenor AVS input
  char  outfile[1000];             //<! Decoded YUV 4:2:0 output
#if ROI_M3264
  char  out_datafile[1000];        //<! Location data output...
  int ROI_Coding;
#endif
  char  reffile[1000];             //<! Optional YUV 4:2:0 reference file for SNR measurement
  int   FileFormat;                //<! File format of the Input file, PAR_OF_ANNEXB or PAR_OF_RTP
  int   buf_cycle;                 //<! Frame buffer size
  int   LFParametersFlag;          //<! Specifies that loop filter parameters are included in bitstream
  int   yuv_structure;             //<! Specifies reference frame's yuv structure
  int   check_BBV_flag;            //<! Check BBV buffer (0: disable, 1: enable)
  int   ref_pic_order;             //<! ref order
  int   output_dec_pic;            //<! output_dec_pic
  int   slice_set_enable;          //<! slice_set_enable
  int   profile_id;
  int   level_id;
  int   chroma_format;
  int   g_uiMaxSizeInBit;
  int   alpha_c_offset;
  int   beta_offset;
  int   useNSQT;
#if MB_DQP
  int   useDQP;
#endif
  int   useSDIP;
  int sao_enable;
#if M3480_TEMPORAL_SCALABLE
  int temporal_id_exist_flag;
#endif
  int alf_enable;

  int crossSliceLoopFilter;

#if EXTEND_BD
  int   sample_bit_depth;  // sample bit depth
  int   output_bit_depth;  // decoded file bit depth (assuming output_bit_depth is less or equal to sample_bit_depth)
#endif

#ifdef MD5
  int MD5Enable;   
#endif
#if OUTPUT_INTERLACE_MERGED_PIC
  int output_interlace_merged_picture;
#endif

};

extern struct inp_par *input;

typedef struct StatBits
{
  int   curr_frame_bits;
  int   prev_frame_bits;
  int   emulate_bits;
  int   prev_emulate_bits;
  int   last_unit_bits;
  int   bitrate;
  int   total_bitrate[1000];
  int   coded_pic_num;
  int   time_s;
} StatBits;

typedef struct
{
	STDOUT_DATA stdoutdata[8];
	int buffer_num;
}outdata;
outdata outprint;
int curr_IDRcoi;
int curr_IDRtr;
int next_IDRtr;
int next_IDRcoi;
int end_SeqTr;

// files
#ifdef WIN32
int   p_out;
int   p_ref;
#ifdef AVS2_S2_BGLONGTERM
int   p_ref_background;
int   p_out_background;
#endif
#else
FILE *p_out;                  //<! pointer to output YUV file
FILE *p_ref;                  //<! pointer to input original reference YUV file file
#ifdef AVS2_S2_BGLONGTERM
FILE *p_ref_background;
FILE *p_out_background;
#endif
#endif

FILE *p_sreport;                //rm52k
void read_ipred_block_modes ( int b8, unsigned int uiPositionInPic );
// prototypes
void init_conf ( int numpar, char **config_str ); // 20071224, command line
void report ( SNRParameters *snr );
#ifdef WIN32
void find_snr ( SNRParameters *snr, int p_ref );
#ifdef AVS2_S2_BGLONGTERM
void find_snr_background ( SNRParameters *snr, int p_ref );
#endif
#else
void find_snr ( SNRParameters *snr, FILE *p_ref );
#endif
void init ();

int  decode_one_frame ( SNRParameters *snr );
void init_frame ();

// Fix by Sunil for RD5.0 test in Linux (2013.11.06)
#ifdef WIN32
void write_frame(int p_out, int pos);
#else
void write_frame(FILE *p_out, int pos);
#endif
void copy_Pframe (); // B pictures


void picture_data ();
void start_codingUnit ( unsigned uiBitSize );
void init_codingUnit_Bframe ( unsigned int uiPositionInPic ); // B pictures
void Init_SubMB ( unsigned int uiBitSize, unsigned int uiPositionInPic );
int  read_SubMB ( unsigned int uiBitSize, unsigned int uiPositionInPic );
int decode_SMB ( unsigned int uiBitSize, unsigned int uiPositionInPic );
int  decode_SubMB ();

int  sign ( int a , int b );

int Header();

void DeblockFrame ( byte **imgY, byte ***imgUV );

// Direct interpolation
#if EXTEND_BD
void get_block ( int ref_frame, int x_pos, int y_pos, int step_h, int step_v, int block[MAX_CU_SIZE][MAX_CU_SIZE], byte **ref_pic 
#else
void get_block ( int ref_frame, int x_pos, int y_pos, int step_h, int step_v, int block[MAX_CU_SIZE][MAX_CU_SIZE], unsigned char **ref_pic 
#endif
#ifdef AVS2_S2_SBD
				, int ioff, int joff
#endif				
				);


void error ( char *text, int code );

// dynamic memory allocation
int  init_global_buffers ();
void free_global_buffers ();

void Update_Picture_Buffers();
void frame_postprocessing ();

void report_frame (outdata data, int pos);

#define PAYLOAD_TYPE_IDERP 8

Bitstream *AllocBitstream();
void      FreeBitstream();
#if TRACE
void      tracebits2 ( const char *trace_str, int len, int info );
#endif

//int   direct_mv[45][80][4][4][3]; // only to verify result

int demulate_enable;
int currentbitoffset;

int aspect_ratio_information;
int frame_rate_code;
int bit_rate_lower;
int bit_rate_upper;
int  marker_bit;
int bbv_buffer_size;
int video_format;
int color_description;
int color_primaries;
int transfer_characteristics;
int matrix_coefficients;

int progressive_sequence;
#if INTERLACE_CODING
int is_field_sequence;
#endif
int low_delay;
int horizontal_size;
int vertical_size;
int sample_precision;
int video_range;

int display_horizontal_size;
int display_vertical_size;
#if M3472
int TD_mode;
int view_packing_mode;
int view_reverse;
#endif

int b_pmvr_enabled;

int dhp_enabled;

int b_dmh_enabled;

#if MH_PSKIP_NEW
int b_mhpskip_enabled;
#endif

int wsm_enabled;

int b_secT_enabled;

int tmp_time;
int FrameNum;
int eos;
int pre_img_type;
int pre_img_types;
//int pre_str_vec;
int pre_img_tr;
int pre_img_qp;
int pre_tmp_time;
int RefPicExist;   // 20071224
int BgRefPicExist;
int dec_ref_num;                //ref order

/* video edit code */ // M1956 by Grandview 2006.12.12
int vec_flag;

/* Copyright_extension() header */
int copyright_flag;
int copyright_identifier;
int original_or_copy;
int copyright_number_1;
int copyright_number_2;
int copyright_number_3;
/* Camera_parameters_extension */
int camera_id;
int height_of_image_device;
int focal_length;
int f_number;
int vertical_angle_of_view;
int camera_position_x_upper;
int camera_position_x_lower;
int camera_position_y_upper;
int camera_position_y_lower;
int camera_position_z_upper;
int camera_position_z_lower;
int camera_direction_x;
int camera_direction_y;
int camera_direction_z;
int image_plane_vertical_x;
int image_plane_vertical_y;
int image_plane_vertical_z;

#if AVS2_HDR_HLS
/* mastering_display_and_content_metadata_extension() header */
int display_primaries_x0;
int display_primaries_y0;
int display_primaries_x1;
int display_primaries_y1;
int display_primaries_x2;
int display_primaries_y2;
int white_point_x;
int white_point_y;
int max_display_mastering_luminance;
int min_display_mastering_luminance;
int maximum_content_light_level;
int maximum_frame_average_light_level;
#endif

/* I_pictures_header() */
int top_field_first;
int repeat_first_field;
int progressive_frame;
#if INTERLACE_CODING
int is_top_field;
#endif
// int fixed_picture_qp;   //qyu 0927
int bbv_delay;
int picture_qp;
int fixed_picture_qp;
int time_code_flag;
int time_code;
int loop_filter_disable;
int loop_filter_parameter_flag;
//int alpha_offset;
//int beta_offset;

/* Pb_picture_header() */
int picture_coding_type;
#if !M3595_REMOVE_REFERENCE_FLAG
int picture_reference_flag;
#endif 


/*picture_display_extension()*/
int frame_centre_horizontal_offset[4];
int frame_centre_vertical_offset[4];

/* slice_header() */
int img_width;
int slice_vertical_position;
int slice_vertical_position_extension;
int fixed_slice_qp;
int slice_qp;
//int slice_set_enable;       //added by mz, 2008.04
int slice_horizontal_positon;       //added by mz, 2008.04
int slice_horizontal_positon_extension;



int StartCodePosition;


#define I_PICTURE_START_CODE    0xB3
#define PB_PICTURE_START_CODE   0xB6
#define SLICE_START_CODE_MIN    0x00
#define SLICE_START_CODE_MAX    0x8F
#define USER_DATA_START_CODE    0xB2
#define SEQUENCE_HEADER_CODE    0xB0
#define EXTENSION_START_CODE    0xB5
#define SEQUENCE_END_CODE       0xB1
#define VIDEO_EDIT_CODE         0xB7
 

#define SEQUENCE_DISPLAY_EXTENSION_ID            2
#define COPYRIGHT_EXTENSION_ID                   4
#define CAMERAPARAMETERS_EXTENSION_ID            11
#define PICTURE_DISPLAY_EXTENSION_ID             7
#if M3480_TEMPORAL_SCALABLE
#define TEMPORAL_SCALABLE_EXTENSION_ID           3
#endif

#if ROI_M3264
#define LOCATION_DATA_EXTENSION_ID               15
#endif

#if AVS2_HDR_HLS
#define MASTERING_DISPLAY_AND_CONTENT_METADATA_EXTENSION     10
#endif


// Adaptive frequency weighting quantization
#if FREQUENCY_WEIGHTING_QUANTIZATION
int weight_quant_enable_flag;
int load_seq_weight_quant_data_flag;

int pic_weight_quant_enable_flag;
int pic_weight_quant_data_index;
int weighting_quant_param;
int weighting_quant_model;
short quant_param_undetail[6];		//M2148 2007-09
short quant_param_detail[6];		//M2148 2007-09
int WeightQuantEnable;				//M2148 2007-09
int mb_adapt_wq_disable;            //M2331 2008-04
int mb_wq_mode;                     //M2331 2008-04
#if CHROMA_DELTA_QP
int chroma_quant_param_disable;
int chroma_quant_param_delta_u;
int chroma_quant_param_delta_v;
#endif

int b_pre_dec_intra_img;
int pre_dec_img_type;
int CurrentSceneModel;
#endif


byte **integerRefY[REF_MAXBUFFER];
byte **integerRefUV[REF_MAXBUFFER][2];

byte **integerRefY_fref[2]; //integerRefY_fref[ref_index][height(height/2)][width] ref_index=0 for B frame, ref_index=0,1 for B field
byte **integerRefY_bref[2]; //integerRefY_bref[ref_index][height(height/2)][width] ref_index=0 for B frame, ref_index=0,1 for B field
byte **integerRefUV_fref[2][2];//integerRefUV_fref[ref_index][uv][height/2][width] ref_index=0 for B frame, ref_index=0,1 for B field
byte **integerRefUV_bref[2][2];//integerRefUV_bref[ref_index][uv][height/2][width] ref_index=0 for B frame, ref_index=0,1 for B field
#if EXTEND_BD
byte ***b_ref[2], ***f_ref[2];
#else
unsigned char ***b_ref[2], ***f_ref[2];
#endif



void malloc_slice ();
void free_slice ();

DataPartition *AllocPartition();
void FreePartition ( DataPartition *dp, int n );

void read_ipred_modes ();




int  AEC_startcode_follows ( int eos_bit );

#if MB_DQP
int lastQP;
// FILE * testQP;
#endif

#if LAST_DQP_BUGFIX
int last_dquant;
#endif 

extern unsigned int max_value_s;

#endif

