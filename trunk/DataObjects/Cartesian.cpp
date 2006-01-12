/*
 *  Cartesian.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/29/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "Cartesian.h"
#include <math.h>

CartesianData::CartesianData() : GriddedData()
{

  coordSystem = cartesian;
  xDim = yDim = zDim = 0;
  xGridsp = yGridsp = zGridsp = 0.0;

  
}

CartesianData::~CartesianData()
{
}


void CartesianData::gridData(RadarData *radarData, QDomElement cappiConfig,
			     float *vortexLat, float *vortexLon)
{

  // Set the output file
  QString cappiPath = cappiConfig.firstChildElement("dir").text();
  QString cappiFile = radarData->getDateTimeString();
  cappiFile += ".asi";
  cappiPath += cappiFile;
  
  // Get the dimensions from the configuration
  xDim = cappiConfig.firstChildElement("xdim").text().toFloat();
  yDim = cappiConfig.firstChildElement("ydim").text().toFloat();
  zDim = cappiConfig.firstChildElement("zdim").text().toFloat();
  xGridsp = cappiConfig.firstChildElement("xgridsp").text().toFloat();
  yGridsp = cappiConfig.firstChildElement("ygridsp").text().toFloat();
  zGridsp = cappiConfig.firstChildElement("zgridsp").text().toFloat();

  // Get the relative center and expand the grid around it
  relDist = new float[2];
  relDist = relLocation(radarData->getRadarLat(), radarData->getRadarLon(),
			vortexLat, vortexLon);
  xmin = relDist[0] - (xDim/2)*xGridsp;
  xmax = relDist[0] + (xDim/2)*xGridsp;
  ymin = relDist[1] - (yDim/2)*xGridsp;
  ymax = relDist[1] + (yDim/2)*yGridsp;
  float distance = sqrt(relDist[0] * relDist[0] + relDist[1] * relDist[1]);
  float beamHeight = radarData->radarBeamHeight(distance, radarData->getSweep(0)->getElevation());
  zmin = (float(int(beamHeight/zGridsp)))*zGridsp;
  zmax = zmin + zDim*zGridsp;

  // Interpolate the data depending on method chosen
  QString interpolation = cappiConfig.firstChildElement("interpolation").text();
  if (interpolation == "barnes") {
    BarnesInterpolation(radarData);
  } /* else if (interpolation == "bilinear") {
    doBilinear(&radarData);
    } */

   
   // Do something with the cappi arrays
   int stop = 0;
   

}

void CartesianData::BarnesInterpolation(RadarData *radarData)
{

  // Barnes Interpolation (see Koch et al, 1983 for details)

  float falloff_x = 5.052*pow((4* xGridsp / Pi),2);
  float falloff_y = 5.052*pow((4* yGridsp / Pi),2);
  float falloff_z = 5.052*pow((4* zGridsp / Pi),2);
  float smoother = 0.3;

  // Find good values
  class goodRef {
   public:
    float refValue;
    float x;
    float y;
    float z;
  };
  class goodVel {
   public:
    float velValue;
    float swValue;
    float x;
    float y;
    float z;
  };

  goodRef refValues[20000];
  goodVel velValues[60000];
  int r = 0;
  int v = 0;
  
  for (int n = 0; n <= radarData->getNumRays(); n++) {
    Ray* currentRay = radarData->getRay(n);
    float theta = deg2rad * (currentRay->getAzimuth());
    float phi = deg2rad * (90. - (currentRay->getElevation()));
    
    if (currentRay->getRef_numgates() > 0) {

      float* refData = currentRay->getRefData();
      for (int g = 0; g <= (currentRay->getRef_numgates()-1); g++) {
	if (refData[g] == -999.) { continue; }
	float range = (currentRay->getFirst_ref_gate() +
	  (g * currentRay->getRef_gatesp()))/1000.;
	float x = range*sin(phi)*cos(theta);
	if ((x < (xmin - xGridsp)) or x > (xmax + xGridsp)) { continue; }
	float y = range*sin(phi)*sin(theta);
	if ((y < (ymin - yGridsp)) or y > (ymax + yGridsp)) { continue; }
	float z = radarData->radarBeamHeight(range,
					     currentRay->getElevation() );
	if ((z < (zmin - zGridsp)) or z > (zmax + zGridsp)) { continue; }

	// Looks like a good point
	refValues[r].refValue = refData[g];
	refValues[r].x = x;
	refValues[r].y = y;
	refValues[r].z = z;
	r++;
      }

    }
    if (currentRay->getVel_numgates() > 0) {

      float* velData = currentRay->getVelData();
      float* swData = currentRay->getSwData();
      for (int g = 0; g <= (currentRay->getVel_numgates()-1); g++) {
	if (velData[g] == -999.) { continue; }
	float range = (currentRay->getFirst_vel_gate() +
	  (g * currentRay->getVel_gatesp()))/1000.;
	float x = range*sin(phi)*cos(theta);
	if ((x < (xmin - xGridsp)) or x > (xmax + xGridsp)) { continue; }
	float y = range*sin(phi)*sin(theta);
	if ((y < (ymin - yGridsp)) or y > (ymax + yGridsp)) { continue; }
	float z = radarData->radarBeamHeight(range,
					     currentRay->getElevation() );
	if ((z < (zmin - zGridsp)) or z > (zmax + zGridsp)) { continue; }

	// Looks like a good point
	velValues[v].velValue = velData[g];
	velValues[v].swValue = swData[g];
	velValues[v].x = x;
	velValues[v].y = y;
	velValues[v].z = z;
	v++;
      }
    }


  }

  // Subtract off one from the count
  r--;
  v--;

  for (int k = 0; k <= int(zDim-1); k++) { 
    for (int j = 0; j <= int(yDim-1); j++) {
      for (int i = 0; i <= int(xDim-1); i++) {

	cartGrid[0][i][j][k] = -999.;
	cartGrid[1][i][j][k] = -999.;
	cartGrid[2][i][j][k] = -999.;

	float sumRef = 0;
	float sumVel = 0;
	float sumSw = 0;
	float refWeight = 0;
	float velWeight = 0;

	for (int n = 0; n <= r; n++) {
	  float dx = refValues[n].x - (xmin + i*xGridsp);
	  if (fabs(dx) > sqrt(20 * falloff_x)) { continue; }

	  float dy = refValues[n].y - (ymin + j*xGridsp);
	  if (fabs(dy) > sqrt(20 * falloff_y)) { continue; }

	  float dz = refValues[n].z - (zmin + k*xGridsp);
	  if (fabs(dz) > sqrt(20 * falloff_z)) { continue; }
	  
	  float weight = exp(-(dx*dx)/falloff_x 
			     -(dy*dy)/falloff_y
			     -(dz*dz)/falloff_z);

	  refWeight += weight;
	  sumRef += weight*refValues[n].refValue;

	}

	for (int n = 0; n <= v; n++) {
	  float dx = velValues[n].x - (xmin + i*xGridsp);
	  if (fabs(dx) > sqrt(20 * falloff_x)) { continue; }

	  float dy = velValues[n].y - (ymin + j*xGridsp);
	  if (fabs(dy) > sqrt(20 * falloff_y)) { continue; }

	  float dz = velValues[n].z - (zmin + k*xGridsp);
	  if (fabs(dz) > sqrt(20 * falloff_z)) { continue; }
	  
	  float weight = exp(-(dx*dx)/falloff_x 
			     -(dy*dy)/falloff_y
			     -(dz*dz)/falloff_z);

	  velWeight += weight;
	  sumVel += weight*velValues[n].velValue;
	  sumSw += weight*velValues[n].swValue;

	}

	if (refWeight > 0) {
	  cartGrid[0][i][j][k] = sumRef/refWeight;
	}
	if (velWeight > 0) {
	  cartGrid[1][i][j][k] = sumVel/velWeight;
	  cartGrid[2][i][j][k] = sumSw/velWeight;
	}	  
      }
    }
  }

  for (int k = 0; k <= int(zDim-1); k++) {
    for (int j = 0; j <= int(yDim-1); j++) {
      for (int i = 0; i <= int(xDim-1); i++) {

	float sumRef = 0;
	float sumVel = 0;
	float sumSw = 0;
	float refWeight = 0;
	float velWeight = 0;

	for (int n = 0; n <= r; n++) {

	  float dx = refValues[n].x - (xmin + i*xGridsp);
	  if (fabs(dx) > sqrt(20 * falloff_x)) { continue; }

	  float dy = refValues[n].y - (ymin + j*xGridsp);
	  if (fabs(dy) > sqrt(20 * falloff_y)) { continue; }

	  float dz = refValues[n].z - (zmin + k*xGridsp);
	  if (fabs(dz) > sqrt(20 * falloff_z)) { continue; }
	  
	  float weight = exp(-(dx*dx)/(falloff_x*smoother)
			     -(dy*dy)/(falloff_y*smoother)
			     -(dz*dz)/(falloff_z*smoother));

	  
	  refWeight += weight;
	  float interpRef = bilinear(dx,dy,dz,0);
	  sumRef += weight*(refValues[n].refValue - interpRef);
	  
	}
	for (int n = 0; n <= v; n++) {
	  float dx = velValues[n].x - (xmin + i*xGridsp);
	  if (fabs(dx) > sqrt(20 * falloff_x)) { continue; }

	  float dy = velValues[n].y - (ymin + j*xGridsp);
	  if (fabs(dy) > sqrt(20 * falloff_y)) { continue; }

	  float dz = velValues[n].z - (zmin + k*xGridsp);
	  if (fabs(dz) > sqrt(20 * falloff_z)) { continue; }
	  
	  float weight = exp(-(dx*dx)/falloff_x 
			     -(dy*dy)/falloff_y
			     -(dz*dz)/falloff_z);

	  
	  velWeight += weight;
	  float interpVel = bilinear(dx,dy,dz,1);
	  float interpSw = bilinear(dx,dy,dz,2);
	  sumVel += weight*(velValues[n].velValue - interpVel);
	  sumSw += weight*(velValues[n].swValue - interpSw);

	}
    
	if (refWeight > 0) {
	  cartGrid[0][i][j][k] += sumRef/refWeight;
	}
	if (velWeight > 0) {
	  cartGrid[1][i][j][k] += sumVel/velWeight;
	  cartGrid[2][i][j][k] += sumSw/velWeight;
	}	  	
      }
    }
  }
  
}

float CartesianData::bilinear(const float &x, const float &y,
			      const float &z, const int &param)
{

  // Do a bilinear interpolation from the nearest gridpoints
  int x0 = int((float(int(x/xGridsp))*xGridsp - xmin)/xGridsp);
  int y0 = int((float(int(y/yGridsp))*yGridsp - ymin)/yGridsp);
  int z0 = int((float(int(z/zGridsp))*zGridsp - zmin)/zGridsp);
  float dx = (x/xGridsp) - int(x/xGridsp);
  float dy = (y/yGridsp) - int(y/yGridsp);
  float dz = (z/zGridsp) - int(z/zGridsp);
  float omdx = 1 - dx;
  float omdy = 1 - dy;
  float omdz = 1 - dz;
  int x1 = 0;
  int y1 = 0;
  int z1 = 0;

  if (x0 < 0) {
    x0 = x1 = 0;
  }

  if (y0 < 0) {
    y0 = y1 = 0;
  }

  if (z0 < 0) {
    z0 = z1 = 0;
  }


  if (x0 >= xDim-1) {
    x1 = x0;
  } else {
    x1 = x0 + 1;
  }

  if (y0 >= yDim-1) {
    y1 = y0;
  } else {
    y1 = y0 + 1;
  }

  if (z0 >= zDim-1) {
    z1 = z0;
  } else {
    z1 = z0 + 1;
  }

  float interpValue = 0;
  if (cartGrid[param][x0][y0][z0] != -999) {
    interpValue += omdx*omdy*omdz*cartGrid[param][x0][y0][z0];
  }
  if (cartGrid[param][x0][y1][z0] != -999) {
    interpValue += omdx*dy*omdz*cartGrid[param][x0][y1][z0];
  }
  if (cartGrid[param][x1][y0][z0] != -999) {
    interpValue += dx*omdy*omdz*cartGrid[param][x1][y0][z0];
  }
  if (cartGrid[param][x1][y1][z0] != -999) {
    interpValue += dx*dy*omdz*cartGrid[param][x1][y1][z0];
  }
  if (cartGrid[param][x0][y0][z1] != -999) {
    interpValue += omdx*omdy*dz*cartGrid[param][x0][y0][z1];
  }
  if (cartGrid[param][x0][y1][z1] != -999) {
    interpValue += omdx*dy*dz*cartGrid[param][x0][y1][z1];
  }
  if (cartGrid[param][x1][y0][z1] != -999) {
    interpValue += dx*omdy*dz*cartGrid[param][x1][y0][z1];
  }
  if (cartGrid[param][x1][y1][z1] != -999) {
    interpValue += dx*dy*dz*cartGrid[param][x1][y1][z1];
  }

  return interpValue;
}
