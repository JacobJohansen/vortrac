/*
 *  Sweep.cpp
 *  VORTRAC
 *
 *  Created by Michael Bell on 7/19/05.
 *  Copyright 2005 University Corporation for Atmospheric Research.
 *  All rights reserved.
 *
 */
#include <iostream>

#include "Radar/Sweep.h"

Sweep::Sweep()
{
}

Sweep::~Sweep()
{
}

void Sweep::setSweepIndex(const int &value) {
  sweepIndex = value;
}

void Sweep::setElevation(const float &value) {
  elevation = value;
}

void Sweep::setUnambig_range(const float &value) {
  unambig_range = value;
}

void Sweep::setNyquist_vel(const float &value) {
  nyquist_vel = value;
}

void Sweep::setFirst_ref_gate(const int &value) {
  first_ref_gate = value;
}

void Sweep::setFirst_vel_gate(const int &value) {
  first_vel_gate = value;
}

void Sweep::setRef_gatesp(const float &value) {
  ref_gatesp = value;
}

void Sweep::setVel_gatesp(const float &value) {
  vel_gatesp = value;
}

void Sweep::setRef_numgates(const int &value) {
  ref_numgates = value;
}

void Sweep::setVel_numgates(const int &value) {
  vel_numgates = value;
}

void Sweep::setVcp(const int &value) {
  vcp = value;
}

void Sweep::setFirstRay(const int &value) {
  firstRay = value;
}

void Sweep::setLastRay(const int &value) {
  lastRay = value;
}

// Get values

int Sweep::getSweepIndex() {
  return sweepIndex ;
}

float Sweep::getElevation() {
  return elevation ;
}

float Sweep::getUnambig_range() {
  return unambig_range ;
}

float Sweep::getNyquist_vel() {
  return nyquist_vel ;
}

int Sweep::getFirst_ref_gate() {
  return first_ref_gate ;
}

int Sweep::getFirst_vel_gate() {
  return first_vel_gate ;
}

float Sweep::getRef_gatesp() {
  return ref_gatesp ;
}

float Sweep::getVel_gatesp() {
  return vel_gatesp ;
}

int Sweep::getRef_numgates() {
  return ref_numgates ;
}

int Sweep::getVel_numgates() {
  return vel_numgates ;
}

int Sweep::getVcp() {
  return vcp ;
}

int Sweep::getFirstRay() {
  return firstRay ;
}

int Sweep::getLastRay() {
  return lastRay ;
}

int Sweep::getNumRays() {
  return (lastRay - firstRay + 1);
}

void Sweep::dump() {
  std::cout << "---------- Sweep " << getSweepIndex() << " ----------" << std::endl;
  std::cout << "\t       Elevation: " << getElevation() << std::endl;
  std::cout << "\t   Unambig Range: " << getUnambig_range() << std::endl;
  std::cout << "\t     Nyquist Vel: " << getNyquist_vel()  << std::endl;
  std::cout << "\t    1st ref gate: " << getFirst_ref_gate() << std::endl;
  std::cout << "\t    1st vel gate: " << getFirst_vel_gate() << std::endl;
  std::cout << "\t   Vel num gates: " << getVel_numgates() << std::endl;
  std::cout << "\t   Ref num gates: " << getRef_numgates() << std::endl;
  std::cout << "\tRef Gate Spacing: " << getRef_gatesp() << std::endl;
  std::cout << "\tVel Gate Spacing: " << getVel_gatesp() << std::endl;
  std::cout << "\t             VCP: " << getVcp() << std::endl;
  std::cout << "\t       First Ray: " << getFirstRay() << std::endl;
  std::cout << "\t        Last Ray: " << getLastRay() << std::endl;
  std::cout << "\t        Num Rays: " << getNumRays() << std::endl;
}
