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

#include <sensor_placement_node.h>

// constructor
sensor_placement_node::sensor_placement_node()
{
  // create node handles
  nh_ = ros::NodeHandle();
  pnh_ = ros::NodeHandle("~");

  // ros subscribers

  AoI_sub_ = nh_.subscribe("in_AoI_poly", 1,
                           &sensor_placement_node::AoICB, this);
  forbidden_area_sub_ = nh_.subscribe("in_forbidden_area", 1,
                                      &sensor_placement_node::forbiddenAreaCB, this);

  // ros publishers
  nav_path_pub_ = nh_.advertise<nav_msgs::Path>("out_path",1,true);
  marker_array_pub_ = nh_.advertise<visualization_msgs::MarkerArray>("out_marker_array",1,true);
  map_pub_ = nh_.advertise<nav_msgs::OccupancyGrid>("out_cropped_map",1,true);
  map_meta_pub_ = nh_.advertise<nav_msgs::MapMetaData>("out_cropped_map_metadata",1,true);
  offset_AoI_pub_ = nh_.advertise<geometry_msgs::PolygonStamped>("offset_AoI", 1,true);
  GS_targets_grid_pub_ = nh_.advertise<visualization_msgs::MarkerArray>("GS_targets_grid",1,true);
  fa_marker_array_pub_ = nh_.advertise<visualization_msgs::MarkerArray>("fa_marker_array",1,false);


  // ros service servers
  ss_start_PSO_ = nh_.advertiseService("StartPSO", &sensor_placement_node::startPSOCallback, this);
  ss_test_ = nh_.advertiseService("TestService", &sensor_placement_node::testServiceCallback, this);
  ss_start_GS_ = nh_.advertiseService("StartGS", &sensor_placement_node::startGSCallback, this);
  ss_start_GS_with_offset_ = nh_.advertiseService("StartGS_with_offset_polygon", &sensor_placement_node::startGSCallback2, this);
  ss_clear_fa_vec_ = nh_.advertiseService("ClearForbiddenAreas", &sensor_placement_node::clearFACallback, this);

  // ros service clients
  sc_get_map_ = nh_.serviceClient<nav_msgs::GetMap>("static_map");

  // get parameters from parameter server if possible
  getParams();

  // initialize best coverage
  best_cov_ = 0;

  // initialize global best multiple coverage
  global_best_multiple_coverage_ = 0;

  // initialize number of targets
  target_num_ = 0;

  // initialize best particle index
  best_particle_index_ = 0;

  // initialize other variables
  map_received_ = false;
  AoI_received_ = false;
  targets_saved_ = false;
  fa_received_ = false;
  polygon_offset_val_received_=false;

}

// destructor
sensor_placement_node::~sensor_placement_node(){}

// function to get the ROS parameters from yaml-file
void sensor_placement_node::getParams()
{
  if(!pnh_.hasParam("number_of_sensors"))
  {
    ROS_WARN("No parameter number_of_sensors on parameter server. Using default [5]");
  }
  pnh_.param("number_of_sensors",sensor_num_,5);

  if(!pnh_.hasParam("max_sensor_range"))
  {
    ROS_WARN("No parameter max_sensor_range on parameter server. Using default [5.0 in m]");
  }
  pnh_.param("max_sensor_range",sensor_range_,5.0);

  double open_angle_1, open_angle_2;

  if(!pnh_.hasParam("open_angle_1"))
  {
    ROS_WARN("No parameter open_angle_1 on parameter server. Using default [1.5708 in rad]");
  }
  pnh_.param("open_angle_1",open_angle_1,1.5708);

  open_angles_.push_back(open_angle_1);

  if(!pnh_.hasParam("open_angle_2"))
  {
    ROS_WARN("No parameter open_angle_2 on parameter server. Using default [0.0 in rad]");
  }
  pnh_.param("open_angle_2",open_angle_2,0.0);

  open_angles_.push_back(open_angle_2);

  if(!pnh_.hasParam("max_linear_sensor_velocity"))
  {
    ROS_WARN("No parameter max_linear_sensor_velocity on parameter server. Using default [1.0]");
  }
  pnh_.param("max_linear_sensor_velocity",max_lin_vel_,1.0);

  if(!pnh_.hasParam("max_angular_sensor_velocity"))
  {
    ROS_WARN("No parameter max_angular_sensor_velocity on parameter server. Using default [0.5236]");
  }
  pnh_.param("max_angular_sensor_velocity",max_ang_vel_,0.5236);

  if(!pnh_.hasParam("number_of_particles"))
  {
    ROS_WARN("No parameter number_of_particles on parameter server. Using default [20]");
  }
  pnh_.param("number_of_particles",particle_num_,20);

  if(!pnh_.hasParam("max_num_iterations"))
  {
    ROS_WARN("No parameter max_num_iterations on parameter server. Using default [400]");
  }
  pnh_.param("max_num_iterations",iter_max_,400);

  if(!pnh_.hasParam("min_coverage_to_stop"))
  {
    ROS_WARN("No parameter min_coverage_to_stop on parameter server. Using default [0.95]");
  }
  pnh_.param("min_coverage_to_stop",min_cov_,0.95);

  // get PSO configuration parameters
  if(!pnh_.hasParam("c1"))
  {
    ROS_WARN("No parameter c1 on parameter server. Using default [0.729]");
  }
  pnh_.param("c1",PSO_param_1_,0.729);

  if(!pnh_.hasParam("c2"))
  {
    ROS_WARN("No parameter c2 on parameter server. Using default [1.49445]");
  }
  pnh_.param("c2",PSO_param_2_,1.49445);

  if(!pnh_.hasParam("c3"))
  {
    ROS_WARN("No parameter c3 on parameter server. Using default [1.49445]");
  }
  pnh_.param("c3",PSO_param_3_,1.49445);

  // get Greedy Search algorithm parameters
  double slice_open_angle_1, slice_open_angle_2;

  if(!pnh_.hasParam("slice_open_angle_1"))
  {
    ROS_WARN("No parameter slice_open_angle_1 on parameter server. Using default [1.5708 in rad]");
  }
  pnh_.param("slice_open_angle_1",slice_open_angle_1,1.5708);

  slice_open_angles_.push_back(slice_open_angle_1);

  if(!pnh_.hasParam("slice_open_angle_2"))
  {
    ROS_WARN("No parameter slice_open_angle_2 on parameter server. Using default [0.0 in rad]");
  }
  pnh_.param("slice_open_angle_2",slice_open_angle_2,0.0);

  slice_open_angles_.push_back(slice_open_angle_2);

  if(!pnh_.hasParam("GS_target_offset"))
  {
    ROS_WARN("No parameter GS_target_offset on parameter server. Using default [5.0 in m]");
  }
  pnh_.param("GS_target_offset",GS_target_offset_, 5.0);
}


bool sensor_placement_node::getTargets()
{
  // initialize result
  bool result = false;
  // intialize local variable
  size_t num_of_fa = forbidden_area_vec_.size();

  if(map_received_ == true)
  {
    // only if we received a map, we can get targets
    target_info_fix dummy_target_info_fix;
    targets_with_info_fix_.assign(map_.info.width * map_.info.height, dummy_target_info_fix);
    target_info_var dummy_target_info_var;
    targets_with_info_var_.assign(map_.info.width * map_.info.height, dummy_target_info_var);
    dummy_target_info_var.covered_by_sensor.assign(sensor_num_, false);

    if(AoI_received_ == false)
    {
      for(unsigned int i = 0; i < map_.info.width; i++)
      {
        for(unsigned int j = 0; j < map_.info.height; j++)
        {
          // calculate world coordinates from map coordinates of given target
          geometry_msgs::Pose2D world_Coord;
          world_Coord.x = mapToWorldX(i, map_);
          world_Coord.y = mapToWorldY(j, map_);
          world_Coord.theta = 0;

          //fix information
          dummy_target_info_fix.world_pos.x = mapToWorldX(i, map_);
          dummy_target_info_fix.world_pos.y = mapToWorldY(j, map_);
          dummy_target_info_fix.world_pos.z = 0;

          dummy_target_info_fix.forbidden = false;    //all targets are allowed unless found in forbidden area
          dummy_target_info_fix.occupied = true;
          dummy_target_info_fix.potential_target = -1;

          //variable information
          dummy_target_info_var.covered = false;
          dummy_target_info_var.multiple_covered = false;

          if(map_.data.at( j * map_.info.width + i) == 0)
          {
            //check all forbidden areas
            for (size_t k=0; k<num_of_fa; k++)
            {
              if((pointInPolygon(world_Coord, forbidden_area_vec_.at(k).polygon) >= 1) && (fa_received_==true))
              {
                // the given position is on the forbidden area
                dummy_target_info_fix.forbidden = true;
                break;
              }
            }
            dummy_target_info_fix.occupied = false;
            dummy_target_info_fix.potential_target = 1;

            target_num_++;
          }
        }
      }
      result = true;
    }
    else
    {
      // if area of interest polygon was specified, we consider the non-occupied grid cells within the polygon as targets
      for(unsigned int i = 0; i < map_.info.width; i++)
      {
        for(unsigned int j = 0; j < map_.info.height; j++)
        {
          // calculate world coordinates from map coordinates of given target
          geometry_msgs::Pose2D world_Coord;
          world_Coord.x = mapToWorldX(i, map_);
          world_Coord.y = mapToWorldY(j, map_);
          world_Coord.theta = 0;


          //fix information
          dummy_target_info_fix.world_pos.x = world_Coord.x;
          dummy_target_info_fix.world_pos.y = world_Coord.y;
          dummy_target_info_fix.world_pos.z = 0;

          dummy_target_info_fix.forbidden = false;    //all targets are allowed unless found in forbidden area
          dummy_target_info_fix.occupied = true;
          dummy_target_info_fix.potential_target = -1;
          dummy_target_info_fix.map_data = map_.data.at( j * map_.info.width + i);

          //variable information
          dummy_target_info_var.covered = false;
          dummy_target_info_var.multiple_covered = false;

          // the given position lies withhin the polygon
          if(pointInPolygon(world_Coord, area_of_interest_.polygon) == 2)
          {
            //check all forbidden areas
            for (size_t k=0; k<num_of_fa; k++)
            {
              if((pointInPolygon(world_Coord, forbidden_area_vec_.at(k).polygon) >= 1) && (fa_received_==true))
              {
                // the given position is on the forbidden area
                dummy_target_info_fix.forbidden = true;
                break;
              }
            }
            dummy_target_info_fix.potential_target = 1;

            if(map_.data.at( j * map_.info.width + i) == 0)
            {
              target_num_++;

              dummy_target_info_fix.occupied = false;
            }
          }
          // the given position lies on the perimeter
          else if( pointInPolygon(world_Coord, area_of_interest_.polygon) == 1)
          {
            //check all forbidden areas
            for (size_t k=0; k<num_of_fa; k++)
            {
              if((pointInPolygon(world_Coord, forbidden_area_vec_.at(k).polygon) >= 1) && (fa_received_==true))
              {
                // the given position is on the forbidden area
                dummy_target_info_fix.forbidden = true;
                break;
              }
            }
            dummy_target_info_fix.potential_target = 0;

            if(map_.data.at( j * map_.info.width + i) == 0)
            {
              dummy_target_info_fix.occupied = false;
            }
          }
          // save the target information
          targets_with_info_var_.at(j * map_.info.width + i) = dummy_target_info_var;
          targets_with_info_fix_.at(j * map_.info.width + i) = dummy_target_info_fix;
        }
      }
      result = true;
    }
  }
  else
  {
    ROS_WARN("No map received! Not able to propose sensor positions.");
  }
  return result;
}



// function to initialize PSO-Algorithm
void sensor_placement_node::initializePSO()
{
  // initialize pointer to dummy sensor_model
  FOV_2D_model dummy_2D_model;
  dummy_2D_model.setMaxVelocity(max_lin_vel_, max_lin_vel_, max_lin_vel_, max_ang_vel_, max_ang_vel_, max_ang_vel_);

  // initialize dummy particle
  particle dummy_particle = particle(sensor_num_, target_num_, dummy_2D_model);

  dummy_particle.setMap(map_);
  dummy_particle.setAreaOfInterest(area_of_interest_);
  dummy_particle.setForbiddenAreaVec(forbidden_area_vec_);
  dummy_particle.setOpenAngles(open_angles_);
  dummy_particle.setRange(sensor_range_);
  dummy_particle.setTargetsWithInfoVar(targets_with_info_var_);
  dummy_particle.setTargetsWithInfoFix(targets_with_info_fix_, target_num_);
  dummy_particle.setLookupTable(& lookup_table_);

  // initialize particle swarm with given number of particles containing given number of sensors
  particle_swarm_.assign(particle_num_,dummy_particle);
  // initialize the global best solution
  global_best_ = dummy_particle;

  double actual_coverage = 0;

  // initialize sensors randomly on perimeter for each particle with random velocities
  if(AoI_received_)
  {
    for(size_t i = 0; i < particle_swarm_.size(); i++)
    {
      // initialize sensor poses randomly on perimeter
      particle_swarm_.at(i).initializeSensorsOnPerimeter();
      // initialize sensor velocities randomly
      particle_swarm_.at(i).initializeRandomSensorVelocities();
      // get calculated coverage
      actual_coverage = particle_swarm_.at(i).getActualCoverage();
      // check if the actual coverage is a new global best
      if(actual_coverage > best_cov_)
      {
        best_cov_ = actual_coverage;
        global_best_ = particle_swarm_.at(i);
      }
    }
    // after the initialization step we're looking for a new global best solution
    getGlobalBest();

    // publish the actual global best visualization
    marker_array_pub_.publish(global_best_.getVisualizationMarkers());
  }
}



// function for the actual particle-swarm-optimization
void sensor_placement_node::PSOptimize()
{
  //clock_t t_start;
  //clock_t t_end;
  //double  t_diff;

  // PSO-iterator
  int iter = 0;
  std::vector<geometry_msgs::Pose> global_pose;

  // iteration step
  // continue calculation as long as there are iteration steps left and actual best coverage is
  // lower than mininmal coverage to stop
  while(iter < iter_max_ && best_cov_ < min_cov_)
  {
    global_pose = global_best_.getSolutionPositions();
    // update each particle in vector
    for(size_t i=0; i < particle_swarm_.size(); i++)
    {
      //t_start = clock();
      particle_swarm_.at(i).resetTargetsWithInfoVar();
      //t_end = clock();
      //t_diff = (double)(t_end - t_start) / (double)CLOCKS_PER_SEC;
      //ROS_INFO( "reset: %10.10f \n", t_diff);

      // now we're ready to update the particle
      //t_start = clock();
      particle_swarm_.at(i).updateParticle(global_pose, PSO_param_1_, PSO_param_2_, PSO_param_3_);
      //t_end = clock();
      //t_diff = (double)(t_end - t_start) / (double)CLOCKS_PER_SEC;
      //ROS_INFO( "updateParticle: %10.10f \n", t_diff);
    }
    // after the update step we're looking for a new global best solution
    getGlobalBest();

    // publish the actual global best visualization
    marker_array_pub_.publish(global_best_.getVisualizationMarkers());

    ROS_INFO_STREAM("iteration: " << iter << " with coverage: " << best_cov_);

    // increment PSO-iterator
    iter++;
  }

}

void sensor_placement_node::getGlobalBest()
{
  // a new global best solution is accepted if
  // (1) the coverage is higher than the old best coverage or
  // (2) the coverage is equal to the old best coverage but there are less targets covered by multiple sensors
  for(size_t i=0; i < particle_swarm_.size(); i++)
  {
    if(particle_swarm_.at(i).getActualCoverage() > best_cov_)
    {
      best_cov_ = particle_swarm_.at(i).getActualCoverage();
      global_best_ = particle_swarm_.at(i);
      global_best_multiple_coverage_ = particle_swarm_.at(i).getMultipleCoverageIndex();
      best_particle_index_ = i;
    }
    else
    {
      if( (particle_swarm_.at(i).getActualCoverage() == best_cov_) && (particle_swarm_.at(i).getMultipleCoverageIndex() < global_best_multiple_coverage_ ))
      {
        best_cov_ = particle_swarm_.at(i).getActualCoverage();
        global_best_ = particle_swarm_.at(i);
        global_best_multiple_coverage_ = particle_swarm_.at(i).getMultipleCoverageIndex();
        best_particle_index_ = i;
      }
    }
  }
}

// callback function for the start PSO service
bool sensor_placement_node::startPSOCallback(std_srvs::Empty::Request& req, std_srvs::Empty::Response& res)
{
  // call static_map-service from map_server to get the actual map
  sc_get_map_.waitForExistence();

  nav_msgs::GetMap srv_map;

  if(sc_get_map_.call(srv_map))
  {
    ROS_INFO("Map service called successfully");
    map_received_ = true;

    if(AoI_received_)
    {
      // get bounding box of area of interest
      geometry_msgs::Polygon bound_box = getBoundingBox2D(area_of_interest_.polygon, srv_map.response.map);
      // cropMap to boundingBox
      cropMap(bound_box, srv_map.response.map, map_);
    }
    else
    {
      map_ = srv_map.response.map;

      // if no AoI was specified, we consider the whole map to be the AoI
      area_of_interest_.polygon = getBoundingBox2D(geometry_msgs::Polygon(), map_);
    }

    // publish map
    map_.header.stamp = ros::Time::now();
    map_pub_.publish(map_);
    map_meta_pub_.publish(map_.info);
  }
  else
  {
    ROS_INFO("Failed to call map service");
  }

  if(map_received_)
  {
    ROS_INFO("Received a map");

    // now create the lookup table based on the range of the sensor and the resolution of the map
    int radius_in_cells = floor(sensor_range_ / map_.info.resolution);
    lookup_table_ = createLookupTableCircle(radius_in_cells);
  }

  ROS_INFO("getting targets from specified map and area of interest!");

  targets_saved_ = getTargets();

  if(targets_saved_)
  {
    ROS_INFO_STREAM("Saved " << target_num_ << " targets in std-vector");

    ROS_INFO_STREAM("Saved " << targets_with_info_fix_.size() << " targets with info in std-vectors");
  }


  ROS_INFO("Initializing particle swarm");
  initializePSO();

  // publish global best visualization
  marker_array_pub_.publish(global_best_.getVisualizationMarkers());

  ROS_INFO("Particle swarm Optimization step");
  PSOptimize();

  // get the PSO result as nav_msgs::Path in UTM coordinates and publish it
  PSO_result_ = particle_swarm_.at(best_particle_index_).particle::getSolutionPositionsAsPath();

  nav_path_pub_.publish(PSO_result_);

  ROS_INFO_STREAM("Print the best solution as Path: " << PSO_result_);

  // publish global best visualization
  marker_array_pub_.publish(global_best_.getVisualizationMarkers());

  ROS_INFO("Clean up everything");
  particle_swarm_.clear();

  targets_with_info_fix_.clear();
  targets_with_info_var_.clear();

  target_num_ = 0;
  best_cov_ = 0;
  best_particle_index_ = 0;

  ROS_INFO("PSO terminated successfully");

  return true;

}


// callback function for the test service
bool sensor_placement_node::testServiceCallback(std_srvs::Empty::Request& req, std_srvs::Empty::Response& res)
{
  // call static_map-service from map_server to get the actual map
  sc_get_map_.waitForExistence();

  nav_msgs::GetMap srv_map;

  if(sc_get_map_.call(srv_map))
  {
    ROS_INFO("Map service called successfully");

    if(AoI_received_)
    {
      // get bounding box of area of interest
      geometry_msgs::Polygon bound_box = getBoundingBox2D(area_of_interest_.polygon, srv_map.response.map);
      // cropMap to boundingBox
      cropMap(bound_box, srv_map.response.map, map_);
      // publish cropped map
      map_.header.stamp = ros::Time::now();
      map_pub_.publish(map_);
      map_meta_pub_.publish(map_.info);
      map_received_ = true;
    }
    else
    {
      // if no AoI was specified, we consider the whole map to be the AoI
      area_of_interest_.polygon = getBoundingBox2D(geometry_msgs::Polygon(), map_);
      map_ = srv_map.response.map;
      map_pub_.publish(map_);
      map_meta_pub_.publish(map_.info);
      map_received_ = true;
    }
  }
  else
  {
    ROS_INFO("Failed to call map service");
  }

  if(map_received_)
  {
    ROS_INFO("Received a map");

    // now create the lookup table based on the range of the sensor and the resolution of the map
    int radius_in_cells = floor(5 / map_.info.resolution);
    lookup_table_ = createLookupTableCircle(radius_in_cells);
  }

  ROS_INFO("getting targets from specified map and area of interest!");

  targets_saved_ = getTargets();
  if(targets_saved_)
  {
    ROS_INFO_STREAM("Saved " << target_num_ << " targets in std-vector");

    ROS_INFO_STREAM("Saved " << targets_with_info_fix_.size() << " targets with info in std-vectors");

  }

  // initialize pointer to dummy sensor_model
  FOV_2D_model dummy_2D_model;
  dummy_2D_model.setMaxVelocity(max_lin_vel_, max_lin_vel_, max_lin_vel_, max_ang_vel_, max_ang_vel_, max_ang_vel_);

  particle_num_ = 1;
  sensor_num_ = 1;
  open_angles_.at(0) = 1.5 * PI;

  // initialize dummy particle
  particle dummy_particle = particle(sensor_num_, target_num_, dummy_2D_model);
  // initialize particle swarm with given number of particles containing given number of sensors

  dummy_particle.setMap(map_);
  dummy_particle.setAreaOfInterest(area_of_interest_);
  dummy_particle.setForbiddenAreaVec(forbidden_area_vec_);
  dummy_particle.setOpenAngles(open_angles_);
  dummy_particle.setRange(5);

  dummy_particle.setTargetsWithInfoFix(targets_with_info_fix_, target_num_);
  dummy_particle.setTargetsWithInfoVar(targets_with_info_var_);

  ROS_INFO_STREAM("creating lookup tables for dummy particle..");
  dummy_particle.setLookupTable(& lookup_table_);
  ROS_INFO_STREAM("lookup tables created.");



  particle_swarm_.assign(particle_num_,dummy_particle);

  // initialze the global best solution
  global_best_ = dummy_particle;

  double actual_coverage = 0;

  // initialize sensors randomly on perimeter for each particle with random velocities
  if(AoI_received_)
  {
    for(size_t i = 0; i < particle_swarm_.size(); i++)
    {
      geometry_msgs::Pose test_pos = geometry_msgs::Pose();
      test_pos.position.x = area_of_interest_.polygon.points.at(0).x+5;
      test_pos.position.y = area_of_interest_.polygon.points.at(0).y+5;
      test_pos.orientation = tf::createQuaternionMsgFromYaw(PI/4);

      particle_swarm_.at(i).placeSensorsAtPos(test_pos);

      global_best_ = particle_swarm_.at(i);

      actual_coverage = global_best_.getActualCoverage();

      ROS_INFO_STREAM("coverage: " << actual_coverage);

      ROS_INFO_STREAM("orientation: " << test_pos.orientation);
      ROS_INFO_STREAM("orientation-map: " << tf::getYaw(map_.info.origin.orientation));
    }
  }
  // publish global best visualization
  marker_array_pub_.publish(global_best_.getVisualizationMarkers());

  return true;
}



// get greedy search targets
bool sensor_placement_node::getGSTargets()
{
  // initialize result
  bool result = false;
  // intialize local variables
  size_t num_of_fa = forbidden_area_vec_.size();
  // initialize flag to identify that a given point is in forbidden area
  bool fa_flag = false;
  // intialize cell offset (number of cells to be skipped)
  unsigned int cell_offset = (unsigned int) round(GS_target_offset_/map_.info.resolution);

  if(cell_offset==0)
  {
    cell_offset=1;
    ROS_INFO("evaluated cell_offset = 0, forcing cell_offset = 1");
  }

  if(map_received_ == true)
  {
    //only if we received a map, we can get targets
    point_info dummy_point_info;
    point_info_vec_.assign(map_.info.width * map_.info.height, dummy_point_info);
    //create dummy GS_point_info to save information in the pool
    GS_point_info dummy_GS_point_info;

    if(AoI_received_ == false)
    {
      for(unsigned int i = 0; i < map_.info.width; i++)
      {
        for(unsigned int j = 0; j < map_.info.height; j++)
        {
          //calculate world coordinates from map coordinates of given target
          geometry_msgs::Pose2D world_Coord;
          world_Coord.x = mapToWorldX(i, map_);
          world_Coord.y = mapToWorldY(j, map_);
          world_Coord.theta = 0;

          dummy_point_info.occupied = true;
          dummy_point_info.potential_target = -1;

          //check if the given point is occupied or not; mark non-occupied ones as potential targets
          if(map_.data.at( j * map_.info.width + i) == 0)
          {
            dummy_point_info.occupied = false;
            dummy_point_info.potential_target = 1;
            target_num_++;
            //check if the given point lies on the grid
            if ((i%cell_offset==0) && (j%cell_offset==0))
            {
              if (fa_received_==true)
              {
                //check all forbidden areas
                for (size_t k=0; k<num_of_fa; k++)
                {
                  if(pointInPolygon(world_Coord, forbidden_area_vec_.at(k).polygon) >= 1)
                  {
                    fa_flag=true;
                    break;
                  }
                }
                //allow only targets outside all forbidden areas to be saved as GS_target
                if(fa_flag==false)
                {
                  //given point is not occupied AND on the grid AND not in any forbidden area. So, save it as GS_target
                  dummy_GS_point_info.p.x=i;
                  dummy_GS_point_info.p.y=j;
                  dummy_GS_point_info.max_targets_covered=0;
                  GS_pool_.push_back(dummy_GS_point_info);
                  fa_flag=false;
                }
                else
                {
                  //skip the given point and make the flag false again
                  fa_flag=false;
                }
              }
              else
              {
                //given point is not occupied AND on the grid. So, save it as GS_target
                dummy_GS_point_info.p.x=i;
                dummy_GS_point_info.p.y=j;
                dummy_GS_point_info.max_targets_covered=0;
                GS_pool_.push_back(dummy_GS_point_info);
              }
            }
          }
          // saving point information
          point_info_vec_.at(j * map_.info.width + i) = dummy_point_info;
        }
      }
      result = true;
    }
    else
    {
      //AoI received, check if polygon offset value is received
      if(polygon_offset_val_received_==true)
      {
        //****************get offset polygon****************
        geometry_msgs::PolygonStamped offset_AoI;
        double  perimeter_min = 3*map_.info.resolution;     // define minimum perimiter in meters
        double offset_val = clipper_offset_value_;          // load offset value

        // set a lower bound on offset value according to perimeter
        if (offset_val < perimeter_min)
          offset_val = perimeter_min;

        // now get an offsetted polygon from area of interest
        offset_AoI = offsetAoI(offset_val);

        if (!offset_AoI.polygon.points.empty())
        {
          //publish offset_AoI
          offset_AoI.header.frame_id = "/map";
          offset_AoI_pub_.publish(offset_AoI);
        }
        else
        {
          ROS_ERROR("No offset polygon returned");
          return false;
        }
        //****************now get targets******************
        for(unsigned int i = 0; i < map_.info.width; i++)
        {
          for(unsigned int j = 0; j < map_.info.height; j++)
          {
            // calculate world coordinates from map coordinates of given target
            geometry_msgs::Pose2D world_Coord;
            world_Coord.x = mapToWorldX(i, map_);
            world_Coord.y = mapToWorldY(j, map_);
            world_Coord.theta = 0;

            dummy_point_info.occupied = true;
            dummy_point_info.potential_target = -1;

            // the given point lies withhin the polygon
            if(pointInPolygon(world_Coord, area_of_interest_.polygon) == 2)
            {
              dummy_point_info.potential_target = 1;

              if(map_.data.at( j * map_.info.width + i) == 0)
              {
                dummy_point_info.occupied = false;
                target_num_++;
                //check if the given point lies on the grid
                if ((i%cell_offset==0) && (j%cell_offset==0))
                {
                  if(pointInPolygon(world_Coord, offset_AoI.polygon) == 0)
                  {
                    if (fa_received_ == true)
                    {
                      //check all forbidden areas
                      for (size_t k=0; k<num_of_fa; k++)
                      {
                        if(pointInPolygon(world_Coord, forbidden_area_vec_.at(k).polygon) >= 1)
                        {
                          fa_flag=true;
                          break;
                        }
                      }
                       //allow only targets outside all forbidden areas to be saved as GS_target
                      if(fa_flag==false)
                      {
                        //given point is not occupied AND on the grid AND outside offset_AoI, all forbidden areas. So, save it as GS_target
                        dummy_GS_point_info.p.x=i;
                        dummy_GS_point_info.p.y=j;
                        dummy_GS_point_info.max_targets_covered=0;
                        GS_pool_.push_back(dummy_GS_point_info);

                      }
                      else
                      {
                        //skip the given point and make the flag false again
                        fa_flag=false;
                      }
                    }
                    else
                    {
                      //given point is not occupied AND on the grid AND outside offset_AoI. So, save it as GS_target
                      dummy_GS_point_info.p.x=i;
                      dummy_GS_point_info.p.y=j;
                      dummy_GS_point_info.max_targets_covered=0;
                      GS_pool_.push_back(dummy_GS_point_info);
                    }
                  }
                }
              }
            }
            //the given point lies on the perimeter
            else if(pointInPolygon(world_Coord, area_of_interest_.polygon) == 1)
            {
              dummy_point_info.potential_target = 0;
              //check if given point is occupied or not
              if(map_.data.at( j * map_.info.width + i) == 0)
              {
                dummy_point_info.occupied = false;
                //check if the given point lies on the grid
                if ((i%cell_offset==0) && (j%cell_offset==0))
                {
                  if(pointInPolygon(world_Coord, offset_AoI.polygon) == 0)
                  {
                    if (fa_received_ == true)
                    {
                      //check all forbidden areas
                      for (size_t k=0; k<num_of_fa; k++)
                      {
                        if(pointInPolygon(world_Coord, forbidden_area_vec_.at(k).polygon) >= 1)
                        {
                          fa_flag=true;
                          break;
                        }
                      }
                      //allow only targets outside all forbidden areas to be saved as GS_target
                      if(fa_flag==false)
                      {
                        //given point is not occupied AND on the grid AND not in any forbidden area. So, save it as GS_target
                        dummy_GS_point_info.p.x=i;
                        dummy_GS_point_info.p.y=j;
                        dummy_GS_point_info.max_targets_covered=0;
                        GS_pool_.push_back(dummy_GS_point_info);
                      }
                      else
                      {
                        //skip the given point and make the flag false again
                        fa_flag=false;
                      }
                    }

                    else
                    {
                      //given point is not occupied AND on the grid. So, save it as GS_target
                      dummy_GS_point_info.p.x=i;
                      dummy_GS_point_info.p.y=j;
                      dummy_GS_point_info.max_targets_covered=0;
                      GS_pool_.push_back(dummy_GS_point_info);
                    }
                  }
                }
              }
            }
            // save information
            point_info_vec_.at(j * map_.info.width + i) = dummy_point_info;
          }
        }
        result = true;
      }
      else
      {
        //polygon_offset_val_received_ not recieved, get targets on whole AoI
        for(unsigned int i = 0; i < map_.info.width; i++)
        {
          for(unsigned int j = 0; j < map_.info.height; j++)
          {
            // calculate world coordinates from map coordinates of given target
            geometry_msgs::Pose2D world_Coord;
            world_Coord.x = mapToWorldX(i, map_);
            world_Coord.y = mapToWorldY(j, map_);
            world_Coord.theta = 0;

            dummy_point_info.occupied = true;
            dummy_point_info.potential_target = -1;

            // the given point lies withhin the polygon
            if(pointInPolygon(world_Coord, area_of_interest_.polygon) == 2)
            {
              dummy_point_info.potential_target = 1;
              // check if given point is occupied or not
              if(map_.data.at( j * map_.info.width + i) == 0)
              {
                dummy_point_info.occupied = false;
                target_num_++;
                //check if the given point lies on the grid
                if ((i%cell_offset==0) && (j%cell_offset==0))
                {
                  if (fa_received_ == true)
                  {
                    //check all forbidden areas
                    for (size_t k=0; k<num_of_fa; k++)
                    {
                      if(pointInPolygon(world_Coord, forbidden_area_vec_.at(k).polygon) >= 1)
                      {
                        fa_flag=true;
                        break;
                      }
                    }
                    //allow only targets outside all forbidden areas to be saved as GS_target
                    if(fa_flag==false)
                    {
                      //given point is not occupied AND on the grid AND not in any forbidden area. So, save it as GS_target
                      dummy_GS_point_info.p.x=i;
                      dummy_GS_point_info.p.y=j;
                      dummy_GS_point_info.max_targets_covered=0;
                      GS_pool_.push_back(dummy_GS_point_info);
                    }
                    else
                    {
                      //skip the given point and make the flag false again
                      fa_flag=false;
                    }
                  }
                  else
                  {
                    //given point is not occupied AND on the grid. So, save it as GS_target
                    dummy_GS_point_info.p.x=i;
                    dummy_GS_point_info.p.y=j;
                    dummy_GS_point_info.max_targets_covered=0;
                    GS_pool_.push_back(dummy_GS_point_info);
                  }
                }
              }
            }
            // the given point lies on the perimeter
            else if(pointInPolygon(world_Coord, area_of_interest_.polygon) == 1)
            {
              dummy_point_info.potential_target = 0;
              // check if given point is occupied or not
              if(map_.data.at( j * map_.info.width + i) == 0)
              {
                dummy_point_info.occupied = false;
                //check if the given point lies on the grid
                if ((i%cell_offset==0) && (j%cell_offset==0))
                {
                  if (fa_received_ == true)
                  {
                    //check all forbidden areas
                    for (size_t k=0; k<num_of_fa; k++)
                    {
                      if(pointInPolygon(world_Coord, forbidden_area_vec_.at(k).polygon) >= 1)
                      {
                        fa_flag=true;
                        break;
                      }
                    }
                    //allow only targets outside all forbidden areas to be saved as GS_target
                    if(fa_flag==false)
                    {
                      //given point is not occupied AND on the grid AND not in any forbidden area. So, save it as GS_target
                      dummy_GS_point_info.p.x=i;
                      dummy_GS_point_info.p.y=j;
                      dummy_GS_point_info.max_targets_covered=0;
                      GS_pool_.push_back(dummy_GS_point_info);
                    }
                    else
                    {
                      //skip the given point and make the flag false again
                      fa_flag=false;
                    }
                  }
                  else
                  {
                    //given point is not occupied AND on the grid. So, save it as GS_target
                    dummy_GS_point_info.p.x=i;
                    dummy_GS_point_info.p.y=j;
                    dummy_GS_point_info.max_targets_covered=0;
                    GS_pool_.push_back(dummy_GS_point_info);
                  }
                }
              }
            }
            // save information
            point_info_vec_.at(j * map_.info.width + i) = dummy_point_info;
          }
        }
        result = true;
      }
    }
  }
  else
  {
    ROS_WARN("No map received! Not able to propose sensor positions.");
  }
  return result;
}


geometry_msgs::PolygonStamped sensor_placement_node::offsetAoI(double offset)
{
  //intializations
  ClipperLib::Polygon cl_AoI;
  ClipperLib::IntPoint cl_point;
  ClipperLib::Polygons in_polys;
  ClipperLib::Polygons out_polys;
  ClipperLib::JoinType jointype = ClipperLib::jtMiter;
  double miterLimit = 0.0;
  geometry_msgs::Point32 p;
  geometry_msgs::PolygonStamped offset_AoI;

  offset = doubleToInt(offset);

  // check if offset input is valid for deflating the area of interest
  if(offset>0)
  {
    //save AoI as a clipper library polygon
    for (size_t i=0; i<area_of_interest_.polygon.points.size(); i++)
    {
      cl_point.X = doubleToInt(area_of_interest_.polygon.points.at(i).x);
      cl_point.Y = doubleToInt(area_of_interest_.polygon.points.at(i).y);
      cl_AoI.push_back(cl_point);
    }

    //save the AoI
    in_polys.push_back(cl_AoI);

    //apply offset function to get an offsetted polygon in out_polys
    ClipperLib::OffsetPolygons(in_polys, out_polys, -1*offset, jointype, miterLimit);

    //check if the offset function returned only one polygon (if more, then in_polys is set incorrectly)
    if(out_polys.size()==1)
    {
      //save the offsetted polygon as geometry_msgs::PolygonStamped type
      for (size_t i=0; i<out_polys.at(0).size(); i++)
      {
        p.x = intToDouble(out_polys.at(0).at(i).X);
        p.y = intToDouble(out_polys.at(0).at(i).Y);
        p.z = 0;
        offset_AoI.polygon.points.push_back(p);
      }
    }
    else
    {
      ROS_ERROR("wrong output from ClipperLib::OffsetPolygons function");
    }

    // show output -b-
    for (int i=0; i<offset_AoI.polygon.points.size(); i++)
    {
      ROS_INFO_STREAM("offset_AoI point " << i << " (" << offset_AoI.polygon.points.at(i).x << "," << offset_AoI.polygon.points.at(i).y << ")" ) ;
    }
  }
  // offset input is invalid
  else
    ROS_ERROR("wrong offset input! Enter a positive offset value for deflated polygon");

  return offset_AoI;
}


// function to initialize GS-Algorithm
void sensor_placement_node::initializeGS()
{
  // initialize pointer to dummy sensor_model
  FOV_2D_model dummy_GS_2D_model;
  dummy_GS_2D_model.setMaxVelocity(max_lin_vel_, max_lin_vel_, max_lin_vel_, max_ang_vel_, max_ang_vel_, max_ang_vel_);

  // initialize dummy greedySearch object
  greedySearch gs_dummy = greedySearch(sensor_num_, target_num_, dummy_GS_2D_model);

  GS_solution = gs_dummy;
  GS_solution.setMap(map_);
  GS_solution.setAreaOfInterest(area_of_interest_);
  GS_solution.setOpenAngles(open_angles_);
  GS_solution.setSliceOpenAngles(slice_open_angles_);
  GS_solution.setRange(sensor_range_);
  GS_solution.setPointInfoVec(point_info_vec_, target_num_);
  GS_solution.setGSpool(GS_pool_);
  GS_solution.setLookupTable(& lookup_table_);

  if (!GS_pool_.empty())
  {
    //publish the GS_targarts grid
    GS_targets_grid_pub_.publish(GS_solution.getGridVisualizationMarker());
  }
  else
    ROS_ERROR("No targets in GS_pool_");

}


// function to run Greedy Search Algorithm
void sensor_placement_node::runGS()
{
  //initialization
  double GS_coverage;
  ros::Time start_time;
  ros::Duration end_time;
  std::vector<double> open_angles(2,0);

  //start placing sensors one by one according to greedy algorithm
  for(size_t sensor_index = 0; sensor_index < sensor_num_; sensor_index++)
  {
    //note start time for greedy search
    start_time = ros::Time::now();
    //do Greedy Search and place sensor on the max coverage pose
    GS_solution.newGreedyPlacement(sensor_index);
    //note end time for greedy_search
    end_time= ros::Time::now() - start_time;
    //publish the solution
    marker_array_pub_.publish(GS_solution.getVisualizationMarkers());
    //calculate the current coverage
    GS_coverage = GS_solution.calGScoverage();

    ROS_INFO_STREAM("Sensors placed: " << sensor_index+1 << " coverage: " << GS_coverage);
    ROS_INFO_STREAM("Time taken: " << end_time << "[s]");
  }

}


// callback function for the start GS service
bool sensor_placement_node::startGSCallback(std_srvs::Empty::Request& req, std_srvs::Empty::Response& res)
{

  // call static_map-service from map_server to get the actual map
  sc_get_map_.waitForExistence();

  nav_msgs::GetMap srv_map;

  if(sc_get_map_.call(srv_map))
  {
    ROS_INFO("Map service called successfully");
    map_received_ = true;

    if(AoI_received_)
    {
      // get bounding box of area of interest
      geometry_msgs::Polygon bound_box = getBoundingBox2D(area_of_interest_.polygon, srv_map.response.map);
      // cropMap to boundingBox
      cropMap(bound_box, srv_map.response.map, map_);
    }
    else
    {
      map_ = srv_map.response.map;

      // if no AoI was specified, we consider the whole map to be the AoI
      area_of_interest_.polygon = getBoundingBox2D(geometry_msgs::Polygon(), map_);
    }

    // publish map
    map_.header.stamp = ros::Time::now();
    map_pub_.publish(map_);
    map_meta_pub_.publish(map_.info);
  }
  else
  {
    ROS_INFO("Failed to call map service");
  }

  if(map_received_)
  {
    ROS_INFO("Received a map");

    // now create the lookup table based on the range of the sensor and the resolution of the map
    int radius_in_cells = floor(sensor_range_ / map_.info.resolution);
    lookup_table_ = createLookupTableCircle(radius_in_cells);
  }

  ROS_INFO("getting targets from specified map and area of interest!");

  targets_saved_ = getGSTargets();

  if(targets_saved_)
  {
  ROS_INFO_STREAM("Saved " << GS_pool_.size() << " targets in GS pool");
  ROS_INFO_STREAM("Saved " << target_num_ << " all targets");
  ROS_INFO_STREAM("Saved " << point_info_vec_.size() << " all points on map");
  }
  else
  {
    ROS_ERROR("No targets received! Error in getGSTargets function");
    return false;
  }

  ROS_INFO("Initializing Greedy Search algorithm");
  initializeGS();

  // now start the actual Greedy Search  Algorithm
  ROS_INFO("Running Greedy Search Algorithm");
  runGS();

  ROS_INFO("Clean up everything");

  GS_pool_.clear();
  point_info_vec_.clear();

  target_num_ = 0;

  ROS_INFO("Greedy Search Algorithm terminated successfully");

  return true;

}


// callback function for the start GS service with offset parameter
bool sensor_placement_node::startGSCallback2(sensor_placement::polygon_offset::Request& req, sensor_placement::polygon_offset::Response& res)
{
  // save offset value received
  clipper_offset_value_ = req.offset_value;
  polygon_offset_val_received_=true;

  // call static_map-service from map_server to get the actual map
  sc_get_map_.waitForExistence();

  nav_msgs::GetMap srv_map;

  if(sc_get_map_.call(srv_map))
  {
    ROS_INFO("Map service called successfully");
    map_received_ = true;

    if(AoI_received_)
    {
      // get bounding box of area of interest
      geometry_msgs::Polygon bound_box = getBoundingBox2D(area_of_interest_.polygon, srv_map.response.map);
      // cropMap to boundingBox
      cropMap(bound_box, srv_map.response.map, map_);
    }
    else
    {
      map_ = srv_map.response.map;

      // if no AoI was specified, we consider the whole map to be the AoI
      area_of_interest_.polygon = getBoundingBox2D(geometry_msgs::Polygon(), map_);
    }

    // publish map
    map_.header.stamp = ros::Time::now();
    map_pub_.publish(map_);
    map_meta_pub_.publish(map_.info);
  }
  else
  {
    ROS_INFO("Failed to call map service");
  }

  if(map_received_)
  {
    ROS_INFO("Received a map");

    // now create the lookup table based on the range of the sensor and the resolution of the map
    int radius_in_cells = floor(sensor_range_ / map_.info.resolution);
    lookup_table_ = createLookupTableCircle(radius_in_cells);
  }

  ROS_INFO("getting targets from specified map and area of interest!");

  targets_saved_ = getGSTargets();

  if(targets_saved_)
  {
  ROS_INFO_STREAM("Saved " << GS_pool_.size() << " targets in GS pool");
  ROS_INFO_STREAM("Saved " << target_num_ << " all targets");
  ROS_INFO_STREAM("Saved " << point_info_vec_.size() << " all points on map");
  }
  else
  {
    ROS_ERROR("No targets received! Error in getGSTargets function");
    return false;
  }

  ROS_INFO("Initializing Greedy Search algorithm");
  initializeGS();

  // now start the actual Greedy Search  Algorithm
  ROS_INFO("Running Greedy Search Algorithm");
  runGS();

  ROS_INFO("Clean up everything");

  res.success = true;
  GS_pool_.clear();
  point_info_vec_.clear();

  target_num_ = 0;

  ROS_INFO("Greedy Search Algorithm terminated successfully");

  return true;

}

// callback function for clearing all forbidden areas
bool sensor_placement_node::clearFACallback(std_srvs::Empty::Request& req, std_srvs::Empty::Response& res)
{
  forbidden_area_vec_.clear();
  visualization_msgs::MarkerArray empty_marker;
  fa_marker_array_pub_.publish(empty_marker);
  fa_received_=false;
  return true;
}

// callback function saving the AoI received
void sensor_placement_node::AoICB(const geometry_msgs::PolygonStamped::ConstPtr &AoI)
{
  area_of_interest_ = *AoI;
  AoI_received_ = true;
}

// callback function saving the forbidden area received
void sensor_placement_node::forbiddenAreaCB(const geometry_msgs::PolygonStamped::ConstPtr &forbidden_area)
{
  forbidden_area_vec_.push_back(*forbidden_area);
  fa_received_ = true;
  //NOTE: get visualization function called again when a polygon is received -b-
  fa_marker_array_pub_.publish(getPolygonVecVisualizationMarker(forbidden_area_vec_, "forbidden_area"));
}

// returns the visualization markers of a vector of polygons
visualization_msgs::MarkerArray sensor_placement_node::getPolygonVecVisualizationMarker(std::vector<geometry_msgs::PolygonStamped> polygons, std::string polygon_name)
{
  visualization_msgs::MarkerArray polygon_marker_array;
  visualization_msgs::Marker line_strip;
  geometry_msgs::Point p;

  for (size_t j=0; j<polygons.size(); j++)
  {
    // setup standard stuff
    line_strip.header.frame_id = "/map";
    line_strip.header.stamp = ros::Time();
    line_strip.ns = polygon_name + boost::lexical_cast<std::string>(j);;
    line_strip.action = visualization_msgs::Marker::ADD;
    line_strip.pose.orientation.w = 1.0;
    line_strip.id = 0;
    line_strip.type = visualization_msgs::Marker::LINE_STRIP;
    line_strip.scale.x = 0.3;
    line_strip.scale.y = 0.0;
    line_strip.scale.z = 0.0;
    line_strip.color.a = 1.0;
    line_strip.color.r = 1.0;
    line_strip.color.g = 0.1;
    line_strip.color.b = 0.0;

    for (size_t k=0; k<polygons.at(j).polygon.points.size(); k++)
    {
      p.x = polygons.at(j).polygon.points.at(k).x;
      p.y = polygons.at(j).polygon.points.at(k).y;
      line_strip.points.push_back(p);
    }
    //enter first point again to get a closed shape
    line_strip.points.push_back(line_strip.points.at(0));
    //save the marker
    polygon_marker_array.markers.push_back(line_strip);
    //clear line strip point for next polygon
    line_strip.points.clear();
  }
  return polygon_marker_array;
}


//######################
//#### main program ####
int main(int argc, char **argv)
{
  // initialize ros and specify node name
  ros::init(argc, argv, "sensor_placement_node");

  // create Node Class
  sensor_placement_node my_placement_node;

  // initialize random seed for all underlying classes
  srand(time(NULL));

  // initialize loop rate
  ros::Rate loop_rate(10);

  while(my_placement_node.nh_.ok())
  {
     //can add this later to the funtion below
    ros::spinOnce();

    loop_rate.sleep();
  }

}
