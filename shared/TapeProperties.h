#pragma once

#ifndef TAPE_PROPERTIES_H
#define  TAPE_PROPERTIES_H

#include <string>
#include <map>
#include <vector>


using namespace std;

typedef struct TapeProperties_struct
{
	int mBaudRate = 300;
	double mMinFBLeadTone = 5;
	double mMinOBLeadTone = 2;
	double mMinTrailerTone = 0.0;
	double mMinMicroLeadTone = 0.0;

} TapeProperties;



#endif