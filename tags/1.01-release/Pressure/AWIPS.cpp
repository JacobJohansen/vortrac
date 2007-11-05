/*
 *  AWIPS.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/19/06.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "AWIPS.h"
#include<QStringList>

AWIPS::AWIPS()
: PressureData()
{
}

AWIPS::AWIPS(const QString& ob)
: PressureData()
{
	
  readObs(ob);
	
}

void AWIPS::readObs(const QString& ob)
{
  uint unixTime = (int)ob.left(15).toFloat();
  time.setTimeSpec(Qt::UTC);
  time.setTime_t(unixTime);
  latitude = ob.mid(17,5).toFloat();
  longitude = ob.mid(24,7).toFloat();
  pressure = ob.mid(33,6).toFloat();
  stationName = ob.mid(41).replace(QRegExp("\\s+"),"_");
  
  windSpeed = -999;
  windDirection = -999;
  altitude = 10;

}	
