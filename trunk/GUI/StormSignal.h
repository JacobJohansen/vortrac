/*
 * StormSignal.h
 * VORTRAC
 *
 * Created by Lisa Mauger on 12/30/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */

#ifndef STORMSIGNAL_H
#define STORMSIGNAL_H

#include <QString>
#include <QPainter>
#include <QPainterPath>
#include <QWidget>
#include <QPen>
#include <QBrush>
#include <QSize>
#include <QTimer>
#include "Message.h"

class StormSignal:public QWidget
{

  Q_OBJECT
    
 public:
        StormSignal(QSize hint = QSize(225,80), QWidget *parent = 0);
        ~StormSignal();


	QSize sizeHint() const;

 protected:
	void paintEvent(QPaintEvent *event);
 
 private:
	bool red, yellow, green, flashing, on;
	QTimer *timer;
	QPen pen;
	QSize hint;
	QPainterPath *hurrSymbol;
 
 public slots:
        void catchLog(const Message& message);
        void changeStatus(StormSignalStatus status);
 
 signals:
        void log(const Message& message);

};
 
#endif
