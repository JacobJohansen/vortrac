/*
 *  RadarFactory.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/18/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */

#include "RadarFactory.h"
#include <iostream>
#include <QPushButton>

RadarFactory::RadarFactory(QDomElement radarConfig, QObject *parent)
  : QObject(parent)
{
  radarConfiguration = radarConfig;

  // Will poll for data and return radar objects in a queue
  radarQueue = new QQueue<QString>;

  // Get relevant configuration info
  radarName = radarConfig.firstChildElement("name").text();
  radarLat = radarConfig.firstChildElement("lat").text().toFloat();
  radarLon = radarConfig.firstChildElement("lon").text().toFloat();
  radarAlt = radarConfig.firstChildElement("alt").text().toFloat();
  // radarAltitude is given in meters, convert to km
  radarAlt = radarAlt/1000;

  QDate startDate = QDate::fromString(radarConfig.firstChildElement("startdate").text(),
				      Qt::ISODate);
  QDate endDate = QDate::fromString(radarConfig.firstChildElement("enddate").text(),
				    Qt::ISODate);
  QTime startTime = QTime::fromString(radarConfig.firstChildElement("starttime").text(),
				      "hh:mm:ss");
  QTime endTime = QTime::fromString(radarConfig.firstChildElement("endtime").text(),
				    "hh:mm:ss");
  startDateTime = QDateTime(startDate, startTime, Qt::UTC);
  endDateTime = QDateTime(endDate, endTime, Qt::UTC);

  QString path = radarConfig.firstChildElement("dir").text();
  dataPath = QDir(path);

  QString format = radarConfig.firstChildElement("format").text();
  if (format == "LDMLEVELII") {
    radarFormat = ldmlevelII;
  } else if (format == "NCDCLEVELII") {
	radarFormat = ncdclevelII;
  } else if (format == "MODEL") {
    radarFormat = model;
  }
  else { 
    // Will implement more later but give error for now
    emit log(Message("Data format not supported"));
  }

}

RadarFactory::~RadarFactory()
{

}

RadarData* RadarFactory::getUnprocessedData()
{
  
  // Get the latest files off the queue and make a radar object

  if (radarQueue->isEmpty()) {
    // Problem, shouldn't be here
    emit log(Message("Trying to get nonexistent radar data off queue"));
  }

  // Get the files off the queue
  QString fileName = dataPath.filePath(radarQueue->dequeue());
  Message::toScreen("Using "+fileName);
  // Mark it as processed
  fileAnalyzed[fileName] = true;
    
  // Now make a new radar object from that file and send it back
  switch(radarFormat) {
  case ncdclevelII :
    {
      NcdcLevelII *radarData = new NcdcLevelII(radarName, radarLat, 
											   radarLon, fileName);
      radarData->setAltitude(radarAlt);
      return radarData;
      break;
    }
  case ldmlevelII :
  {
      LdmLevelII *radarData = new LdmLevelII(radarName, radarLat, 
											 radarLon, fileName);
      radarData->setAltitude(radarAlt);
      return radarData;
      break;
  }  
  case model:
    {
      AnalyticRadar *radarData = new AnalyticRadar(radarName, 
						   radarLat, radarLon,
						   fileName);
      radarData->setAltitude(radarAlt);
      radarData->setConfigElement(radarConfiguration.parentNode().toElement());
      return radarData;
      break;
    }
  case dorade:
    {
      // Not yet implemented
      break;
    }
  case netcdf:
    {
      // Not yet implemented
      break;
    }
  }

  // If we get here theres a problem, return a null pointer
  emit log(Message("Problem with radar data Factory"));
  return 0;

}

bool RadarFactory::hasUnprocessedData()
{
  
  // Check the unprocessed list first, if it has files no need to reread directory yet
  if(!radarQueue->isEmpty()) {
    return true;
  }

  // Otherwise, check the directory for appropriate files

  switch(radarFormat) {
  case ncdclevelII:
    {
      // Should have filenames starting with radar ID
      dataPath.setNameFilters(QStringList(radarName + "*"));
      dataPath.setFilter(QDir::Files);
      dataPath.setSorting(QDir::Time | QDir::Reversed);
      QStringList filenames = dataPath.entryList();
      
      // Check to see which are in the time limits
      for (int i = 0; i < filenames.size(); ++i) {
		  QString file = filenames.at(i);
		  QString timepart = file;
		  // Replace the radarname so we just have timestamps
		  timepart.replace(radarName, "");
		  QStringList timestamp = timepart.split("_");
		  QDate fileDate = QDate::fromString(timestamp.at(0), "yyyyMMdd");
		  QTime fileTime = QTime::fromString(timestamp.at(1), "hhmmss");
		  QDateTime fileDateTime = QDateTime(fileDate, fileTime, Qt::UTC);
		  
		  if (fileDateTime >= startDateTime && fileDateTime <= endDateTime) {	
			  // Valid time and radar name, check to see if it has been processed
			  if (!fileAnalyzed[dataPath.filePath(file)]) {
				  // File has not been analyzed, add it to the queue
				  radarQueue->enqueue(file);
			  }
		  }
      }

    break;
    }
  case ldmlevelII:
  {
      // Should have filenames starting with radar ID
      dataPath.setNameFilters(QStringList(radarName + "*"));
      dataPath.setFilter(QDir::Files);
      dataPath.setSorting(QDir::Time | QDir::Reversed);
      QStringList filenames = dataPath.entryList();
      
      // Check to see which are in the time limits
      for (int i = 0; i < filenames.size(); ++i) {
		  QString file = filenames.at(i);
		  QString timepart = file;
		  // Replace the radarname so we just have timestamps
		  timepart.replace(radarName, "");
		  QStringList timestamp = timepart.split(".");
		  QDate fileDate = QDate::fromString(timestamp.at(1).left(8), "yyyyMMdd");
		  QTime fileTime = QTime::fromString(timestamp.at(1).right(6), "hhmmss");
		  QDateTime fileDateTime = QDateTime(fileDate, fileTime, Qt::UTC);
		  
		  if (fileDateTime >= startDateTime && fileDateTime <= endDateTime) {	
			  // Valid time and radar name, check to see if it has been processed
			  if (!fileAnalyzed[dataPath.filePath(file)]) {
				  // File has not been analyzed, add it to the queue
				  radarQueue->enqueue(file);
			  }
		  }
      }
	  
	  break;
  }
	  
  case model:
    {
      // Currently the GUI is set up to take the XML config in the 
      // radar directory parameter when analytic storm is selected as
      // radar format.
      
      // Here we will pass that xml file through the queue to be processed
      
      QString fileName = radarConfiguration.firstChildElement("dir").text();

      //Message::toScreen("Radar Factory:analytic config filename: "+fileName);
      
      if (!fileAnalyzed[fileName]) {
	// File has not been analyzed, add it to the queue
	radarQueue->enqueue(fileName);
      }
      
      break;
    }
    
  case dorade:
    {
      // Not yet implemented
      break;
    }  
  case netcdf:
    {
      // Not yet implemented
      break; 
    } 
    
  }

  // See if we added any new files to the queue
  if(!radarQueue->isEmpty()) {
    return true;
  }
  
  // We made it here so there must be nothing new
  return false;
  
}

void RadarFactory::catchLog(const Message& message)
{
  emit log (message);
}

void RadarFactory::updateDataQueue(const VortexList* list)
{
  if(!list->count()>0)
    return;
  int totalFiles = radarQueue->count();
  for(int i = totalFiles-1; i >= 0; i--)
    {
      switch(radarFormat) {
      case ncdclevelII:
	{
	  // Should have filenames starting with radar ID
      
	  // Check to see which are in the time limits
	  QString file = radarQueue->at(i);
	  QString timepart = file;
	  // Replace the radarname so we just have timestamps
	  timepart.replace(radarName, "");
	  QStringList timestamp = timepart.split("_");
	  QDate fileDate = QDate::fromString(timestamp.at(0), "yyyyMMdd");
	  QTime fileTime = QTime::fromString(timestamp.at(1), "hhmmss");
	  QDateTime fileDateTime = QDateTime(fileDate, fileTime, Qt::UTC);
	  
	  if (fileDateTime >= startDateTime && fileDateTime <= endDateTime){
	    // Valid time and radar name, check to see if it 
	    //  has been processed in vortexList
	    for(int j = 0; j < list->size(); j++) {
	      QDateTime processedTime = list->at(j).getTime();
	      if(abs(processedTime.secsTo(fileDateTime)) < 30) {
		if (!fileAnalyzed[dataPath.filePath(file)]) {
		  // File has been analyzed, remove it from the queue
		  radarQueue->removeAt(radarQueue->indexOf(file));
		  fileAnalyzed[dataPath.filePath(file)] = true; 
		}
	      }
	    }
	  }
	  break;
	}
      case ldmlevelII:
	{
	  // Should have filenames starting with radar ID
	  // Check to see which are already in vortexList
	  QString file = radarQueue->at(i);
	  QString timepart = file;
	  // Replace the radarname so we just have timestamps
	  timepart.replace(radarName, "");
	  QStringList timestamp = timepart.split(".");
	  QDate fileDate = QDate::fromString(timestamp.at(1).left(8),"yyyyMMdd");
	  QTime fileTime = QTime::fromString(timestamp.at(1).right(6), "hhmmss");
	  QDateTime fileDateTime = QDateTime(fileDate, fileTime, Qt::UTC);
	  if (fileDateTime >= startDateTime && fileDateTime <= endDateTime){
	    // Valid time and radar name, check to see if it 
	    //  has been processed in vortexList
	    for(int j = 0; j < list->size(); j++) {
	      QDateTime processedTime = list->at(j).getTime();
	      if(abs(processedTime.secsTo(fileDateTime)) < 30) {
		if (!fileAnalyzed[dataPath.filePath(file)]) {
		  // File has been analyzed, remove it from the queue
		  radarQueue->removeAt(radarQueue->indexOf(file));
		  fileAnalyzed[dataPath.filePath(file)] = true; 
		  Message::toScreen("Plucked file "+file+" from the list");
		}
	      }
	    }
	  }
	  break;
	}
      }
    }
}
