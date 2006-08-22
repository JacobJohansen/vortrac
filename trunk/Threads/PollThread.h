/*
 *  PollThread.h
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/25/05.
 *  Copyright 2005 University Corporation for Atmospheric Research. 
 *	All rights reserved.
 *
 */

#ifndef POLLTHREAD_H
#define POLLTHREAD_H

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QList>
#include "Radar/RadarFactory.h"
#include "AnalysisThread.h"
#include "Config/Configuration.h"
#include "DataObjects/VortexList.h"
#include "DataObjects/SimplexList.h"
#include "Pressure/PressureFactory.h"
#include "Pressure/PressureList.h"

#include <QTextStream>

class PollThread : public QThread
{
  Q_OBJECT

  public:
	PollThread(QObject *parent = 0);
	~PollThread();
	void setConfig(Configuration *configPtr);
	
  public slots:
        void catchLog(const Message& message);
        void analysisDoneProcessing();
	void catchVCP(const int vcp);
	void abortThread();
	void setOnlyRunOnce(const bool newRunOnce = true);

  signals:
	void log(const Message& message);
	void newVCP(const int);
	void vortexListUpdate(VortexList* list);
	void dropListChanged(VortexList* list);
	
  protected:
	void run();

  private:
	QMutex mutex;
	bool runOnce;
	QWaitCondition waitForAnalysis;
	bool abort;
	RadarFactory *dataSource;
	PressureFactory *pressureSource;
	Configuration *configData;
	VortexList *vortexList;
	SimplexList *simplexList;
	PressureList *pressureList;
	PressureList *dropsondeList;
	AnalysisThread *analysisThread;
	Configuration *vortexConfig;
	Configuration *simplexConfig;
	Configuration *pressureConfig;
	
};

#endif
