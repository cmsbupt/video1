#ifndef __ADAPTIVEQP__
#define __ADAPTIVEQP__

#include "global.h"

#define max(a, b)      (((a) > (b)) ? (a) : (b))  //!< Macro returning max value
#define min(a, b)      (((a) < (b)) ? (a) : (b))

void adaptiveQPInit( int iWidth, int iHeight, unsigned int uiMaxAQDepth);
void xPreanalyze(int iWidth, int iHeight);


#endif // __adaptiveQP__