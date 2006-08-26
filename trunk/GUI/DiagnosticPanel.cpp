/*
 * DiagnosticPanel.cpp
 * VORTRAC
 *
 *  Created by Lisa Mauger on 04/09/06
 *  Copyright 2006 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 *
 */


#include "DiagnosticPanel.h"
#include <QDateTime>
#include <QGroupBox>
#include <QTime>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QBrush>
#include <QGradient>
#include <QPen>
#include <QSize>
#include <QColorDialog>

DiagnosticPanel::DiagnosticPanel(QWidget *parent)
  :QWidget(parent)

  /*
   * This panel is used to display widgets that provide diagnostic
   * information on the operational state of the vortrac algorithm.
   * This includes the vcp, problem indicator and messages, and the time.
   */

{

  // timer is used to create updates for the display clock

  timer = new QTimer(this);
  timer->setSingleShot(false);
  timer->setInterval(1000);
  timer->start();

  QGroupBox *clockBox = new QGroupBox("Current Time UTC");
  clockBox->setAlignment(Qt::AlignHCenter);
 
  clock = new QLCDNumber(clockBox);
  clock->setSegmentStyle(QLCDNumber::Flat);
  clock->resize(150,100);

  QGridLayout *clockLayout = new QGridLayout;
  clockLayout->addWidget(clock,0,1,1,1);
  //clockLayout->setRowMinimumHeight(1,100);
  clockLayout->setRowStretch(0,8);
  clockBox->setLayout(clockLayout);
  //clockBox->resize(230,150);
  

  connect(timer, SIGNAL(timeout()), 
	  this, SLOT(updateClock()), Qt::DirectConnection);
  
  updateClock();

  //QPushButton *color = new QPushButton("color", this);
  //connect(color, SIGNAL(pressed()), this, SLOT(pickColor()));

  //QPushButton *lightButton = new QPushButton("Next Signal Pattern");
  //connect(lightButton, SIGNAL(pressed()), this, SLOT(testLight()));
  
  // Stoplight used to show operational status of the vortrac algorithm

  lights = new StopLight(QSize(225,80), this);
  connect(lights, SIGNAL(log(const Message&)),
	  this, SLOT(catchLog(const Message&)));

  QHBoxLayout *lightsLayout = new QHBoxLayout;
  lightsLayout->addStretch();
  lightsLayout->addWidget(lights);
  lightsLayout->addStretch();

  // Displays current radar vcp

  QLabel *vcpLabel = new QLabel(tr("Current Radar VCP"));
  vcp = new QLineEdit(QString("No VCP available")); 
  //vcp = new QLineEdit(QString("121"));
  vcp->setReadOnly(true);

  // Displays warning message that may accompany the change in stoplight
  // signal

  warning = new QLineEdit;
  warning->setReadOnly(true);

  // Button and widget to display current Cappi
  cappiLaunch = new QPushButton("View Current CAPPI", this);
  connect(cappiLaunch, SIGNAL(pressed()), this, SLOT(launchCappi()));
  cappiDisplay = new CappiDisplay();
  
  QVBoxLayout *main = new QVBoxLayout();
  // main->addStretch();
  main->addWidget(clockBox);

  main->addWidget(vcpLabel);
  main->addWidget(vcp);
  
  main->addWidget(cappiLaunch);
  
  main->addStretch();

  main->addLayout(lightsLayout);
  main->addWidget(warning);

  //main->addWidget(color);
  //main->addWidget(lightButton);
  setLayout(main);
  
  // Used to set the initial color of the stoplight
  
  lights->changeColor(6);

  dummy = 0;
}

DiagnosticPanel::~DiagnosticPanel()
{
	delete cappiDisplay;
}

void DiagnosticPanel::updateClock()
{
  // Updates display clock
  QString displayTime;
  displayTime = QDateTime::currentDateTime().toUTC().toString("hh:mm");
  clock->display(displayTime);
  update();

}

void DiagnosticPanel::catchLog(const Message& message)
{
  emit log(message);
}

void DiagnosticPanel::pickColor()

{
  QColorDialog::getColor(); 
}

void DiagnosticPanel::updateVCP(const int newVCP)
{
  vcp->clear();
  vcp->insert(QString().setNum(newVCP));
}

void DiagnosticPanel::testLight()
{
  lights->changeColor(dummy);
  if(dummy < 6)
    dummy++;
  else 
    dummy = 0;
}

void DiagnosticPanel::launchCappi()
{
	// Open the floating widget to look at the Cappi
	cappiDisplay->show();
	
}
