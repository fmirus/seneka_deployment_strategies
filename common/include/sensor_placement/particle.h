/****************************************************************
 *
 * Copyright (c) 2013
 *
 * Fraunhofer Institute for Manufacturing Engineering
 * and Automation (IPA)
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Project name: SeNeKa
 * ROS stack name: seneka
 * ROS package name: sensor_placement
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Author: Florian Mirus, email:Florian.Mirus@ipa.fhg.de
 *
 * Date of creation: April 2013
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Fraunhofer Institute for Manufacturing
 *     Engineering and Automation (IPA) nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License LGPL as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License LGPL for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License LGPL along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************/

#ifndef PARTICLE_H
#define PARTICLE_H

// standard includes
#include <iostream>
#include <vector>
#include <math.h>
#include <stdlib.h>

// ros includes
#include <ros/ros.h>
#include <tf/tf.h>
#include <ros/console.h>
#include <geometry_msgs/Polygon.h>
#include <geometry_msgs/PolygonStamped.h>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/Pose2D.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/Vector3.h>
#include <geometry_msgs/Point.h>
#include <nav_msgs/OccupancyGrid.h>
#include <nav_msgs/Path.h>
#include <std_msgs/String.h>
#include <visualization_msgs/MarkerArray.h>

// external includes
#include <sensor_model.h>
#include <seneka_utilities.h>


using namespace seneka_utilities;

class particle
{
private:

  // std-vector storing the sensors
  std::vector< FOV_2D_model > sensors_;

  // std-vector storing the sensors
  std::vector< FOV_2D_model > sol_sensors_;

  // std-vector storing the personal best solution of the particle
  std::vector< FOV_2D_model > pers_best_;

  // target vectors
  const std::vector<target_info_fix> * pTargets_with_info_fix_;
  std::vector<target_info_var> targets_with_info_var_;

  // number of sensors
  int sensor_num_;

  // number of targets
  int target_num_;
  int covered_targets_num_;

  // covered targets by GreedyPSO
  int gPSO_covered_targets_num_;

  // actual coverage
  double coverage_;

  // personal best coverage
  double pers_best_coverage_;

  // multiple coverage numbers
  int multiple_coverage_;
  int pers_best_multiple_coverage_;

  // actual area of interest to be covered by the sensor nodes
  const geometry_msgs::PolygonStamped * pArea_of_interest_;

  // forbidden area vector for the placement of sensors
  const std::vector<geometry_msgs::PolygonStamped> * pForbidden_poly_;

  // actual map
  const nav_msgs::OccupancyGrid * pMap_;


public:

  // standard constructor
  particle();

  // constructor with arguments
  particle(int num_of_sensors, int num_of_targets, FOV_2D_model sensor_model);

  // destructor
  ~particle();

  // ************************ getter functions ************************

  // function to get personal best solution
  std::vector< FOV_2D_model > getPersonalBest();

  // function to get actual solution
  std::vector< FOV_2D_model > getActualSolution();

  // function to get the sensor positions of the actual solution
  std::vector<geometry_msgs::Pose> getSolutionPositions();

  // function to get the sensor positions of the actual solution as nav_msgs::Path
  nav_msgs::Path getSolutionPositionsAsPath();

  // function to get the sensor positions of the personal best solution
  std::vector<geometry_msgs::Pose> getPersonalBestPositions();

  // function to get personal best coverage
  double getBestCoverage();

  // function to get actual coverage
  double getActualCoverage();

  // function to get multiple coverage index
  int getMultipleCoverageIndex();

  // function to get targets_info_var
  std::vector<target_info_var> getTargetsWithInfoVar();

  // function to get number of targets covered after call to updateTargetsInfoRaytracing_withlock
  unsigned int getNumOfTargetsCovered();


  // ************************ setter functions ************************

  // function to set solution
  void setSolutionSensors(FOV_2D_model sol_sensor);

  // function that sets the member variable sensor_num_ and reserves capacity for vector sensors_
  void setSensorNum(int num_of_sensors);

  // function to set the fix information for all targets
  void setTargetsWithInfoFix(const std::vector<target_info_fix> &targets_with_info_fix, int target_num);

  // function to set the variable information for all targets
  void setTargetsWithInfoVar(const std::vector<target_info_var> &targets_with_info_var);

  // function to reset the variable information for all targets
  void resetTargetsWithInfoVar();

  // function to reset the variable information for all targets
  void resetTargetsWithInfoVar2();

  // function that sets the map
  void setMap(const nav_msgs::OccupancyGrid & new_map);

  // function that sets the area of interest
  void setAreaOfInterest(const geometry_msgs::PolygonStamped & new_poly);

  // function that sets forbidden areas vector
  void setForbiddenAreaVec(const std::vector<geometry_msgs::PolygonStamped> & new_forbidden_area_vec_);

  // function that sets the opening angles for each sensor in the particle
  bool setOpenAngles(std::vector<double> new_angles);

  // function that sets the range for each sensor in the particle
  void setRange(double new_range);

  //function to create and set a lookup table for raytracing for each sensor in the particle
  void setLookupTable(const std::vector< std::vector<geometry_msgs::Point32> > * pLookup_table);


  // ************************ update functions ************************

  // function to place the sensors randomly on the perimeter
  void placeSensorsRandomlyOnPerimeter();

  // function to initialize the sensors on the perimeter
  void initializeSensorsOnPerimeter();

  // function to initialize given sensor on the perimeter
  void initializeOneSensorOnPerimeter(unsigned int sensor_index);

  // function to place all sensors at a given pose
  void placeSensorsAtPos(geometry_msgs::Pose new_pose);

  // function to initialize the sensors velocities randomly
  void initializeRandomSensorVelocities();

  // function to update particle during PSO
  void updateParticle(std::vector<geometry_msgs::Pose> global_best, double PSO_param_1, double PSO_param_2, double PSO_param_3);

  // function to update particle during PSO
  void updateParticle2(std::vector<geometry_msgs::Pose> global_best, double PSO_param_1, double PSO_param_2, double PSO_param_3);

  // function to update the targets_with_info variable
  void updateTargetsInfo(size_t sensor_index);

  //function to update the targets_with_info variable with raytracing (lookup table)
  void updateTargetsInfoRaytracing(size_t sensor_index);

  //function to update the targets_with_info variable with raytracing (lookup table); with option to save no reset info for covered targets
  void updateTargetsInfoRaytracing_withlock(size_t sensor_index, bool lock_targets);

  // function to calculate the actual  and personal best coverage
  void calcCoverage();

  // function to check coverage of given sensor and target
  bool checkCoverage(FOV_2D_model sensor, geometry_msgs::Point32 target);

  // function to check if the new sensor position is accepted
  bool newPositionAccepted(geometry_msgs::Pose new_pose_candidate);

  // function to check if the new sensor orientation is accepted
  bool newOrientationAccepted(size_t sensor_index, geometry_msgs::Pose new_pose_candidate);

  // function to update the original sensors vector; to show solution as path after GreedyPSO optimization step
  void updateOrigSensorsVec();

  // ************************* help functions *************************

  // helper function to find an uncovered target far away from a given sensor position
  // the return value is the index of that uncovered target
  unsigned int findFarthestUncoveredTarget(size_t sensor_index);

  // helper function to find a random uncovered and non occupied target not forbidden
  // the return value is the index of that uncovered target
  unsigned int randomFreeTarget();

  // helper function to check, if the sensor is facing outside the area of interest
  bool sensorBeamIntersectsPerimeter(size_t sensor_index, geometry_msgs::Pose new_pose_candidate);

  // helper function for the actual calculation step in sensorBeamIntersectsPerimeter function
  double intersectionCalculation(double v1, double v2, double x1, double x2, double y1, double y2);

  // returns all visualization markers of the particle
  visualization_msgs::MarkerArray getVisualizationMarkers();

  // returns all visualization markers of the particle
  visualization_msgs::MarkerArray getsolVisualizationMarkers();
};

#endif
