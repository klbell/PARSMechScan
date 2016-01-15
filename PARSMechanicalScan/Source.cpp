#include "Source.h"
#define NUMSTAGELOCATIONS 4500000

int test;

int main()
{
	////////////////////////////////
	// Program settings 
	///////////////////////////////

	float		stageStepSize = 5.0/2; // in um
	int			stepTotalX = 1600;//2000/stageStepSize; 
	int			stepTotalY = 600;//2000/stageStepSize;
	int			stepX = 1;
	int			stepY = 1;
	int			stageLocation[2] = { 0, 0 };

	
	int16		*stageLocationSave[2]; // Ensure that this is made large enough for some spillover
	int			stageCount = 0;

	stageLocationSave[0] = new int16[NUMSTAGELOCATIONS];
	stageLocationSave[1] = new int16[NUMSTAGELOCATIONS];

	// Motors off
	XON(0);
	YON(0);


	
	////////////////////////////////
	// Initialize DAQ settings 
	///////////////////////////////

	TaskHandle	directionX = 0;
	TaskHandle	directionY = 0;

	DAQmxCreateTask("XDir", &directionX);
	DAQmxCreateTask("YDir", &directionY);

	DAQmxCreateAOVoltageChan(directionX, "Dev1/ao1", "XDir", 0.0, 5.0, DAQmx_Val_Volts, NULL); // X Direction
	DAQmxCreateAOVoltageChan(directionY, "Dev1/ao0", "YDir", 0.0, 5.0, DAQmx_Val_Volts, NULL); // Y Direction

	float64 fiveVoltOut[2] = { 5.0, 5.0 }; // 5.0 V
	float64 zeroVoltOut[2] = { 0.0, 0.0 }; // 0.0 V

	//Must be 2 bits
	uInt8 outClock[2] = { 0 , 1 };
	uInt8 onSig[2] = { 1, 1 };	
		

	////////////////////////////////
	// Initialize Gage card
	///////////////////////////////

	if (initializeGage(stageStepSize))
	{
		fprintf(stdout, ("Error initializing Gage card, please check settings\n\n"));
		getchar();
		return -1;
	}
	fprintf(stdout, ("Gage card sucessfully initialized.\n\n"));



	////////////////////////////////
	// Initial settings checks
	///////////////////////////////

	// Check program buffer sizes
	int currBufferSize = checkBufferSize();
	int totalNumStageMoves = (stepTotalX / stepX)*(stepTotalY / stepY);
	if (currBufferSize >= totalNumStageMoves && NUMSTAGELOCATIONS >= totalNumStageMoves)
	{
		fprintf(stdout, ("Buffer sizes appear correct.\n\n"));
	}
	else
	{
		fprintf(stdout, ("Buffer size is not set correctly. Stage locations (%d) and segment count (%d) must be greater than %d.\n\n"),
			(int)NUMSTAGELOCATIONS, currBufferSize, totalNumStageMoves);
		getchar();
		return -1;
	}
	

	////////////////////////////////
	// Print scan settings
	///////////////////////////////

	fprintf(stdout, ("\n\n Scan Settings\n---------------\n\n"));
	fprintf(stdout, ("X stage step size: %3.2f um\nY stage step size: %3.2f um\n"), (float)stepX*stageStepSize, (float)stepY*stageStepSize);
	fprintf(stdout, ("X range: %3.2f mm\nY range: %3.2f mm\n\n"), (float)stepTotalX * stageStepSize / 1000.0, (float)stepTotalY * stageStepSize / 1000.0);
	//fprintf(stdout, ("Total stage moves: %d\nStage clock speed: %d Hz\n\n---------------\n\n"), (stepTotalX / stepX)*(stepTotalY / stepY),getStageClockSpeed());
	fprintf(stdout, ("Total stage moves: %d\nStage clock speed: %d Hz\n"), (stepTotalX / stepX)*(stepTotalY / stepY), getStageClockSpeed());
	fprintf(stdout, ("Set trigger holdoff bettween %d us and %d us\n\n"), (int)(1.0 / (float)getStageClockSpeed()*1e6), (int)(2.0 / (float)getStageClockSpeed()*1e6));
	fprintf(stdout, ("---------------\n\n"));



	////////////////////////////////
	// Prep for scan
	///////////////////////////////

	// Ready to start scan
	fprintf(stdout, ("Ensure stage is centered.\nPress any key to start scan.\n\n"));
	getchar();

	fprintf(stdout, ("Moving stage to start location.\n\n"));

	// Motors on
	XON(1);
	YON(1);

	// Scan variables
	int steps;
	//int gageCollectStatus;
	int XDir = 1;
	int saveCount = 1;	
	
	// Move to start position
	steps = stepTotalX / 2;
	DAQmxWriteAnalogF64(directionX, 1, 1, 0.0, DAQmx_Val_GroupByChannel, zeroVoltOut, NULL, NULL); // dir
	moveXStage(steps, outClock);
	stageLocation[0] -= steps;

	steps = stepTotalY / 2;
	DAQmxWriteAnalogF64(directionY, 1, 1, 0.0, DAQmx_Val_GroupByChannel, zeroVoltOut, NULL, NULL); // dir 	
	moveYStage(steps, outClock); // Move
	stageLocation[1] -= steps;
	


	////////////////////////////////
	// Scan
	///////////////////////////////

	moveXStage(1, onSig); // hold on
	
	// Start data collection	
	if (collectData())
	{
		fprintf(stdout, ("Data Collection Failed"));
		getchar();
		return (-1);
	}


	// Scan along Y
	for (int countY = 0; countY < stepTotalY; countY += stepY)
	{
		if (countY != 0)
		{
			// Step Y
			DAQmxWriteAnalogF64(directionY, 1, 1, 0.0, DAQmx_Val_GroupByChannel, fiveVoltOut, NULL, NULL); // dir 			
			moveYStage(stepY, outClock); // Move--			
			stageLocation[1] += stepY;			
		}
		
		if (countY > 0)
		{
			_ftprintf(stdout, ("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"));
		}
		_ftprintf(stdout, _T("%04.0f um remaining in Y"), (float)(stepTotalY - countY) * stageStepSize);
		
		

		// Scan along X
		if (XDir == 1)
		{
			DAQmxWriteAnalogF64(directionX, 1, 1, 0.0, DAQmx_Val_GroupByChannel, fiveVoltOut, NULL, NULL); // dir 				
			stageLocation[0] += stepTotalX;
		}
		else {
			DAQmxWriteAnalogF64(directionX, 1, 1, 0.0, DAQmx_Val_GroupByChannel, zeroVoltOut, NULL, NULL); // dir 				
			stageLocation[0] -= stepTotalX;
		}
		moveXStage(stepTotalX, outClock); // Move			
		
		// Save stage data and change direction
		if (XDir == 1)
		{			
			XDir = 0;
		}
		else {
			XDir = 1;
		}		
	}

	_ftprintf(stdout, _T("\nScan complete. Returning to start position.\n\n"));

	// Move stage to center (0,0)
	if (stageLocation[0] > 0){
		steps = stageLocation[0];
		DAQmxWriteAnalogF64(directionX, 1, 1, 0.0, DAQmx_Val_GroupByChannel, zeroVoltOut, NULL, NULL); // dir 		
		moveXStage(steps, outClock); // Move
		stageLocation[0] -= steps;
	}
	else{
		steps = -stageLocation[0];
		DAQmxWriteAnalogF64(directionX, 1, 1, 0.0, DAQmx_Val_GroupByChannel, fiveVoltOut, NULL, NULL); // dir 
		moveXStage(steps, outClock); // Move
		stageLocation[0] += steps;
	}

	if (stageLocation[1] > 0){
		steps = stageLocation[1];
		DAQmxWriteAnalogF64(directionY, 1, 1, 0.0, DAQmx_Val_GroupByChannel, zeroVoltOut, NULL, NULL); // dir		
		moveYStage(steps, outClock); // Move
		stageLocation[1] -= steps;
	}
	else{
		steps = -stageLocation[1];
		DAQmxWriteAnalogF64(directionY, 1, 1, 0.0, DAQmx_Val_GroupByChannel, fiveVoltOut, NULL, NULL); // dir
		moveYStage(steps, outClock); // Move
		stageLocation[1] += steps;
	}


	moveXStage(2, onSig); // Move

	// Motors off
	XON(0);
	YON(0);

	//moveXStage(4, onSig); // hold on


	// Stage stage data
	int countX = 0;
	int countY = 0;
	XDir = 1;
	for (countY = 0; countY < stepTotalY; countY += stepY)
	{
		if (countY != 0)
		{
			stageLocationSave[0][stageCount] = countX;
			stageLocationSave[1][stageCount++] = countY;
		}
		if (XDir == 1)
		{
			for (countX = 0; countX < stepTotalX; countX += stepX)
			{
				stageLocationSave[0][stageCount] = countX;
				stageLocationSave[1][stageCount++] = countY;
			}
			XDir = 0;
		}
		else {
			for (countX = stepTotalX-1; countX > -1; countX -= stepX)
			{
				stageLocationSave[0][stageCount] = countX;
				stageLocationSave[1][stageCount++] = countY;
			}
			XDir = 1;
		}
	}

	// Check if aquisition is completed
	checkScanComplete();

	// Transfer and save data
	int nSaveError = saveGageData();
	if (nSaveError)
	{
		fprintf(stdout, ("Data Save Failed"));
		getchar();
		return (-1);
	}

	// Save stage data
	TCHAR StageFileName[MAX_PATH];
	FILE *fidStage;

	// Open Save files
	int nCount = 0;
	while (1)
	{
		nCount++;
		_stprintf(StageFileName, _T("E:\\GageData\\MechStageDataScan%d.txt"), nCount);
		fidStage = fopen(StageFileName, "r");
		if (fidStage)
		{
			fclose(fidStage);
		}
		else
		{
			fidStage = fopen(StageFileName, "w");
			break;
		}
	}

	//_stprintf(StageFileName, _T("E:\\GageData\\MechStageData.txt"));
	//fidStage = fopen(StageFileName, "w");

	
	for (int n = 0; n < stageCount; n++)
	{
		_ftprintf(fidStage, _T("%d %d\n"), stageLocationSave[0][n], stageLocationSave[1][n]); // print peakData
	}


	fclose(fidStage);
	fidStage = NULL;

	delete stageLocationSave[0];
	delete stageLocationSave[1];

	return 0;
}