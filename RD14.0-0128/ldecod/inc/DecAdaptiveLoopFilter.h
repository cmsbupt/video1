#ifndef DEC_ADAPTIVE_LOOP_FILTER
#define DEC_ADAPTIVE_LOOP_FILTER
#include "../../lcommon/inc/ComAdaptiveLoopFilter.h"
#include "../../lcommon/inc/commonStructures.h"
#include "../../ldecod/inc/global.h"

typedef struct
{
	int **m_filterCoeffSym;
	int m_varIndTab[NO_VAR_BINS];
	byte** m_varImg;
	Boolean** m_AlfLCUEnabled;
	ALFParam** m_alfPictureParam;
}DecALFVar;
extern DecALFVar *Dec_ALF;

void CreateAlfGlobalBuffer();
void ReleaseAlfGlobalBuffer();

#if EXTEND_BD
void ALFProcess_dec(ALFParam** alfParam, ImageParameters* img, byte* imgY_alf_Dec,byte** imgUV_alf_Dec, int sample_bit_depth);
void filterOneCTB(byte* pRest, byte* pDec, int stride, int compIdx, ALFParam* alfParam, int ctuYPos, int ctuHeight, int ctuXPos, int ctuWidth, 
  Boolean isAboveAvail, Boolean isBelowAvail, Boolean isLeftAvail, Boolean isRightAvail, Boolean isAboveLeftAvail, Boolean isAboveRightAvail, int sample_bit_depth);
#else
void ALFProcess_dec(ALFParam** alfParam, ImageParameters* img, byte* imgY_alf_Dec,byte** imgUV_alf_Dec);
void filterOneCTB(byte* pRest, byte* pDec, int stride, int compIdx, ALFParam* alfParam, int ctuYPos, int ctuHeight, int ctuXPos, int ctuWidth, Boolean isAboveAvail, Boolean isBelowAvail, Boolean isLeftAvail, Boolean isRightAvail);
#endif
void deriveLoopFilterBoundaryAvailibility(ImageParameters* img,int numLCUPicWidth, int numLCUPicHeight, int ctu,Boolean* isLeftAvail,Boolean* isRightAvail,Boolean* isAboveAvail,Boolean* isBelowAvail);

void deriveBoundaryAvail(int numLCUPicWidth, int numLCUPicHeight, int ctu, Boolean* isLeftAvail, Boolean* isRightAvail, Boolean* isAboveAvail, Boolean* isBelowAvail, Boolean* isAboveLeftAvail, Boolean* isAboveRightAvail);

void AllocateAlfPar(ALFParam** alf_par,int cID);
void freeAlfPar(ALFParam* alf_par,int cID);
void allocateAlfAPS(ALF_APS* pAPS);
void freeAlfAPS(ALF_APS* pAPS);
void Read_ALF_param ( char *buf, int startcodepos, int length );
#endif