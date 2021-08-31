#include "../include/configuration.h"

#include <SmingCore/SmingCore.h>


String getFileName(fileType type)
{
	switch(type)
	{
	case nameFile:
	{
		return NameFile;
	}
	case tempFile:
	{
		return TempFile;
	}
	case secondsFile:
	{
		return SecondFile;
	}
	case vFile:
	{
		return VFile;
	}
	case freezeSecondsFile:
	{
		return  FreezeSecFile;
	}

	default:
		return "";
	}
	return "";
}

//check 0 becouse this end str
byte checkZero(byte &val)
{
	if(val == 0)
	 return 255;

	return val;
}


//if 255 ret 0
byte checkZeroChar(char val)
{
	if(val == 255)
	  return 0;

	return val;
}

String loadConfig(fileType type)
{
	String fileName = getFileName(type);
	if(fileExist(fileName))
	{
		String file = fileGetContent(fileName);

		return file;
	}
	return "";
}

void saveConfig(fileType type, String strVal)
{
	String fileName = getFileName(type);

	if(fileName.length())
		fileSetContent(fileName, strVal);
}






