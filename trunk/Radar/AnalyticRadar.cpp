/*
 *  AnalyticRadar.cpp
 *  VORTRAC
 *
 *  Created by Lisa Mauger on 02/07/06.
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "AnalyticRadar.h"
#include "IO/Message.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

 AnalyticRadar::AnalyticRadar(const QString& radarname, float lat,
			      float lon, const QString& configFile)
   : RadarData(radarname, lat, lon, configFile)
   
{
  numSweeps = 0;
  numRays = 0;
  vcp = 0;
  velNull = -999.;

  // Loads the configuration containing analytic radar parameters
  config = new Configuration();
  config->read(configFile);
  
  // Retrieves the parameters which indicates whether the analytic data
  // should be sampled or not

  QDomElement radar = config->getRoot().firstChildElement("analytic_radar");
  QString sampling = config->getParam(radar, "sample");
  
  if (sampling == QString("false") || sampling == QString("no")) {
    isDealiased(true);
  }
  else { 
    //Message::toScreen("Analytic Radar Not Dealaised");
    Message::toScreen("Sampling Analytic Storm with Analytic Radar");
    isDealiased(false);
  }

  // Configure Properties from theoretical radar

}

AnalyticRadar::~AnalyticRadar()
{
  delete data;
  delete elevations;
}

void AnalyticRadar::setConfigElement(QDomElement configRoot)
{
  masterRoot = configRoot;
}

bool AnalyticRadar::readVolume()
{
  
  if(isDealiased()) {
    // Then the analytic radar is not really needed,
    // a hollow radarData is constructed and passed along for 
    // algorithm consistancy

    if(skipReadVolume())
      return true;
  }
  else {
    // Sample the analytic data with the analytic radar

    if(readVolumeAnalytic()) {
      //testing Message::toScreen("Finished read Volume Analytic");
      return true;
    }
  }

return false;
}

Sweep AnalyticRadar::addSweep()
{
  
  // add Sweeps from theoretical radar volume
  Sweep *newSweep = new Sweep();

  // Needs a way to do different elevation angles -LM
  newSweep->setElevation(elevations[numSweeps]);
  newSweep->setSweepIndex(numSweeps);
  newSweep->setFirstRay(numRays);
  newSweep->setNyquist_vel( nyqVel );
  // newSweep->setFirst_ref_gate( );
  // newSweep->setFirst_vel_gate( );
  newSweep->setRef_gatesp( refGateSp );
  newSweep->setVel_gatesp( velGateSp );
  newSweep->setVcp( vcp );
  numSweeps++;
  return *newSweep;
  
}

Ray AnalyticRadar::addRay()
{
  
  // Add a new ray to the sweep
  Ray *newRay = new Ray();
  newRay->setSweepIndex( (numSweeps - 1) );
  newRay->setRayIndex(numRays);
  
  float elevAngle = Sweeps[numSweeps-1].getElevation();
 
  float metAzimAngle =  beamWidth*(float)(newRay->getRayIndex()-Sweeps[numSweeps-1].getFirstRay());
  // metAzimAngle is in meterological angle system (cw from 12 o'clock)
  
  float azimAngle = 450.0-metAzimAngle;
  // azimAngle is in math angle (ccw from 3 o'clock)
  if(azimAngle >= 360.0)
    azimAngle-=360.0;
  newRay->setAzimuth( metAzimAngle );
  newRay->setElevation( elevAngle );
  newRay->setNyquist_vel( nyqVel );

  int numPoints = data->getSphericalRangeLength(azimAngle, elevAngle);
  float *raw_ref_data = new float[numPoints];
  float *raw_ref_positions = new float[numPoints];

  QString dataType("DZ");
  raw_ref_data = data->getSphericalRangeData(dataType,
					     azimAngle, elevAngle);
  raw_ref_positions = data->getSphericalRangePosition(azimAngle, elevAngle);

  // Selects all points from data that are in the theoretical radar beam
  
  float furthestPosition = 0.0;
  for(int z = 0; z < numPoints; z++) {
    if(raw_ref_positions[z] > furthestPosition)
      furthestPosition = raw_ref_positions[z];
  }
  // determines the distance from the radar where there is sufficient data
  // for readings

  float *ref_data = new float[numRefGates];
  float gateBoundary = 0;
  
  // This loop checks the positions of all the points in the ray
  // for those within a certain gate and sums them together to get an average
  // reflectivity reading
  for(int gateNum = 0; gateNum < numRefGates; gateNum++ ) {
    if(gateBoundary < furthestPosition) {
      float refSum = 0;
      int count = 0;
      for(int p = 0; p < numPoints; p++) {
	if((raw_ref_positions[p] > gateBoundary) 
	   && (raw_ref_positions[p] <= (gateBoundary+refGateSp))) {
	  refSum+= raw_ref_data[p];
	  count++;
	}
      }
      if(count!=0)
	ref_data[gateNum] = refSum/(float)count;
      else
	ref_data[gateNum] = velNull;
    }
    else {
      ref_data[gateNum] = velNull;
    }
    gateBoundary+=refGateSp;
  }


  delete[] raw_ref_data;
  delete[] raw_ref_positions;
  
  // Selects all points in the theoretical radar beam that may contain valid 
  // velocity data
  
  float *raw_vel_data = new float[numPoints];
  float *raw_vel_positions = new float[numPoints];
  dataType = QString("VE");
  raw_vel_data = data->getSphericalRangeData(dataType,azimAngle, elevAngle);
  raw_vel_positions = data->getSphericalRangePosition(azimAngle, elevAngle);
  float *vel_data = new float[numVelGates];
  float *sw_data = new float[numVelGates];
  gateBoundary = 0;
  
  // This loop checks the positions of all the points in the ray
  // for those within a certain gate and sums them together to get an average
  // velocity reading with a measure of spectral width
  // here noise and aliasing are added if requested

  for(int gateNum = 0; gateNum < numVelGates; gateNum++ ) {
    if(gateBoundary < furthestPosition) {
      float velSum = 0;
      int count = 0;
      for(int p = 0; p < numPoints; p++) {
	if ((raw_vel_positions[p] > gateBoundary) 
	    && (raw_vel_positions[p] <= (gateBoundary+velGateSp))) {
	  velSum += raw_vel_data[p];
	  count++;
	}
      }
      if(count == 0) {
	vel_data[gateNum] = velNull;
	sw_data[gateNum] = velNull;
      }
      else {
	// Good velocity data in this gate
	vel_data[gateNum] = velSum*cos(elevAngle*deg2rad)/(float)count;

	// Add in noise factor, method borrowed from analyticTC
	// the variable noiseScale changes the relative magnetude of the noise
	// while the variable noisyGates holds the relative percentage of
	// gates that noise is applied to.
	
	//srand(time(NULL));  // initializes random number generator
	int percentOfGates = int(float(rand())/(RAND_MAX*.01));
	//Message::toScreen("Random gate = "+QString().setNum(percentOfGates));
	if(percentOfGates < noisyGates) {
	  //Message::toScreen("Got Noise");
	  srand(time(NULL));  // reinitializes random number generator
	  float noise = rand()%1000/1000.0 -.5;
	  vel_data[gateNum]+= noiseScale*noise;
	}

	// collect all the point's velocities for spectral width calculations
	// spectral width is the standard deviation of the velocity readings

	float *sw_points = new float[count];
	int countAgain = 0;
	for(int p = 0; p < numPoints; p++) {
	  if ((raw_vel_positions[p] > gateBoundary) 
	      && (raw_vel_positions[p] < gateBoundary+velGateSp)) {
	    sw_points[countAgain] = raw_vel_data[p];
	    countAgain++;
	  }
	}
	float swSum = 0;
	for(int q = 0; q < count; q++) {
      
	  swSum = (sw_points[q]-vel_data[gateNum])*(sw_points[q]-vel_data[gateNum]);
	}
	sw_data[gateNum] = sqrt(swSum/(float)count);
	
	// Velocity aliasing when applicable
	
       	while(fabs(vel_data[gateNum]) > nyqVel) {
	  if(vel_data[gateNum] > 0) {
	    vel_data[gateNum]-=2*nyqVel;
	  }
	  else {
	    vel_data[gateNum]+=2*nyqVel;
	  }
       	}
      }
    }
    else {
      vel_data[gateNum] = velNull;
      sw_data[gateNum] = velNull;
    } 
    gateBoundary+=velGateSp;
  }

  delete[] raw_vel_data;
  delete[] raw_vel_positions;
    
  newRay->setRefData( ref_data );
  newRay->setVelData( vel_data );
  newRay->setSwData( sw_data );
  newRay->setRef_numgates( numRefGates );
  newRay->setVel_numgates( numVelGates ); 
  newRay->setRef_gatesp( 1000*refGateSp );
  newRay->setVel_gatesp( 1000*velGateSp );  
 
  //newRay->setTi<me( );
  //newRay->setDate( );
 
  // Need more info about how these values are used before filling
  // With Useful info

  //newRay->setVelResolution( radarHeader->velocity_resolution );
  newRay->setUnambig_range( numVelGates*velGateSp);
  newRay->setFirst_ref_gate(0);
  newRay->setFirst_vel_gate(0);

  newRay->setVcp( vcp );
  numRays++;
  return *newRay;
}


bool AnalyticRadar::skipReadVolume()
{
  // This indicates that none of the RadarQC is used
  // This radarData is a dude, and goes straight to cappi
  
  numSweeps = -1;
  numRays = -1;

  return true;
}

bool AnalyticRadar::readVolumeAnalytic()
{

  // This section creates a griddedData to be sampled by the theoretical
  // radar and go through all the RadarQC
  
  vortexLat = masterRoot.firstChildElement("vortex").firstChildElement("lat").text().toFloat();
  vortexLon = masterRoot.firstChildElement("vortex").firstChildElement("lon").text().toFloat();


  QDomElement radar = config->getRoot().firstChildElement("analytic_radar");

  radarLat =  masterRoot.firstChildElement("radar").firstChildElement("lat").text().toFloat();
  radarLon =  masterRoot.firstChildElement("radar").firstChildElement("lon").text().toFloat(); 
 
  //  radarLat = config->getParam(radar, "radarY").toFloat();
  //  radarLon = config->getParam(radar, "radarX").toFloat();
  nyqVel= config->getParam(radar, "nyquistVel").toFloat();

  refGateSp = config->getParam(radar, "refgatesp").toFloat();
  velGateSp = config->getParam(radar, "velgatesp").toFloat();
  beamWidth = config->getParam(radar, "beamwidth").toFloat();
  noiseScale = config->getParam(radar, "noiseScale").toFloat();
  noisyGates = config->getParam(radar, "percent_noisy_gates").toFloat();
  int numGates = config->getParam(radar, "numgates").toInt();
  int totNumSweeps = config->getParam(radar, "numsweeps").toInt();
  
  QString sendToDealias = config->getParam(radar, "dealiasdata");
  if((sendToDealias == "true")||(sendToDealias == "True")
     ||(sendToDealias=="TRUE")){
    isDealiased(false);
  }
  else
    isDealiased(true);

  numRefGates = numGates;
  numVelGates = numGates;
  
  radarDateTime = QDateTime::currentDateTime();

  radarName = QString("Analytic Radar"); 
  
  GriddedFactory *factory = new GriddedFactory();
  data = factory->makeAnalytic(this, masterRoot.firstChildElement("cappi"), 
			       config, &vortexLat, &vortexLon, 
			       &radarLat, &radarLon);
  data->writeAsi();
  // writes out the base cappi data set before sampling

  data->setAbsoluteReferencePoint(radarLat,radarLon,0);
  // data->setCartesianReferencePoint(0,0,0);
 
  // Sample with theoretical radar
  // VCP 0 : flat cappi sample

  vcp = 32;
  // This needs to be connected to a bunch of more useful options
  // that correspond to the sweep angles chosen

  int numRaysPerSweep = (int)floor(360.0/beamWidth);
  // we use all the data we have aka radar has no distance limit
  // of its own

  //testing Message::toScreen(QString().setNum(totNumSweeps*numRaysPerSweep));
  // sweeps angles are pulled from array in configuration
  elevations = new float[totNumSweeps];

  elevations[0]  = 0;
  if(totNumSweeps > 1) {
    elevations[1] = 0.5;
    if(totNumSweeps > 2) {
      elevations[1] = 1.5;
      if(totNumSweeps > 3) {
	elevations[2] = 2.5;
	if(totNumSweeps > 4) {
	  elevations[3] = 3.5;
	  if(totNumSweeps > 5) {
	    elevations[4] = 4.5;
	  }}}}}
  
  Sweeps = new Sweep[totNumSweeps];
  Rays = new Ray[totNumSweeps*numRaysPerSweep]; 

  for(int n = 0; n < totNumSweeps; n++) {
    //testing Message::toScreen("Trying to Make Sweep Number "+QString().setNum(n));
    Message::toScreen("Made Sweep "+QString().setNum(n)+" of "+QString().setNum(totNumSweeps-1));
    Sweeps[numSweeps] = addSweep();
    for( int r = 0; r < numRaysPerSweep; r++) {
      // Message::toScreen("Trying to Make Ray Number "+QString().setNum(r));
      Rays[numRays] = addRay();
    }  
    Sweeps[n].setLastRay(numRays-1);
  }

  Message::toScreen("Left Analytic Radar");

  return true;
}

