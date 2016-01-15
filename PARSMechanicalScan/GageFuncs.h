#ifndef GAGEFUNCS_H
#define GAGEFUNCS_H

#include "Source.h"
#include "CsAppSupport.h"
#include "CsTchar.h"
#include "CsSdkMisc.h"
#include <complex>
#include <cmath>





// Gage functions
int initializeGageAS();
int initializeGage(float stepSize);
int collectDataAS();
int collectData();
void checkScanCompleteAS();
int checkScanComplete();
int saveGageDataAS();
int saveStageData(int stageLocation[], int toX, int toY);
int saveGageData();
int checkBufferSize();


/*void startText(int stepTotalX, int stepX, int stepTotalY, int stepY, float stageStepSize);
int collectData();
int32 TransferTimeStamp(CSHANDLE hSystem, uInt32 u32SegmentStart, uInt32 u32SegmentCount, void* pTimeStamp);
int saveGageData(int saveCount, int stageLocation[], int bPeakDetection);
void SaveAllGageData();*/
#endif