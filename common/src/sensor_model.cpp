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

#include <sensor_model.h>

// **************************************************************
// ********************* class sensor_model *********************
// **************************************************************




// **************************************************************
// ********************* class FOV_2D_model *********************
// **************************************************************


// standard constructor for derived class
FOV_2D_model::FOV_2D_model()
{
  setName("camera");
  setOpenAngles(0,0);
  setRange(0);
  setVelocity(0,0,0,0,0,0);
  setMaxVelocity(1,1,1, 30 * PI/180, 30 * PI/180, 30 * PI/180);
  setSensorPose(0,0,0,0,0,0,0);
}

// constructor with arguments
FOV_2D_model::FOV_2D_model(geometry_msgs::Twist new_vel, geometry_msgs::Twist new_max_vel, geometry_msgs::Pose new_pos, double new_range, double new_angle1, double new_angle2, std::string new_name)
{
  setName(new_name);
  setOpenAngles(new_angle1,new_angle2);
  setRange(new_range);
  setVelocity(new_vel);
  setMaxVelocity(new_max_vel);
  setSensorPose(new_pos);
}  

// destructor
FOV_2D_model::~FOV_2D_model(){}

// function to set the name
void FOV_2D_model::setName(std::string new_name)
{
  name_ = new_name;
}

// function to set actual velocity
void FOV_2D_model::setVelocity(double lin_x, double lin_y, double lin_z, double ang_x, double ang_y, double ang_z)
{

  vel_.linear.x = signum(lin_x) * std::min(fabs(lin_x), max_vel_.linear.x);
  vel_.linear.y = signum(lin_y) * std::min(fabs(lin_y), max_vel_.linear.y);
  vel_.linear.z = signum(lin_z) * std::min(fabs(lin_z), max_vel_.linear.z);
  vel_.angular.x = signum(ang_x) * std::min(fabs(ang_x), max_vel_.angular.x);
  vel_.angular.y = signum(ang_y) * std::min(fabs(ang_y), max_vel_.angular.y);
  vel_.angular.z = signum(ang_z) * std::min(fabs(ang_z), max_vel_.angular.z);
  
}

// function to set actual velocity
void FOV_2D_model::setVelocity(geometry_msgs::Twist new_vel)
{
  vel_.linear.x = signum(new_vel.linear.x) * std::min(fabs(new_vel.linear.x), max_vel_.linear.x);
  vel_.linear.y = signum(new_vel.linear.y) * std::min(fabs(new_vel.linear.y), max_vel_.linear.y);
  vel_.linear.z = signum(new_vel.linear.z) * std::min(fabs(new_vel.linear.z), max_vel_.linear.z);
  vel_.angular.x = signum(new_vel.angular.x) * std::min(fabs(new_vel.angular.x), max_vel_.angular.x);
  vel_.angular.y = signum(new_vel.angular.y) * std::min(fabs(new_vel.angular.y), max_vel_.angular.y);
  vel_.angular.z = signum(new_vel.angular.z) * std::min(fabs(new_vel.angular.z), max_vel_.angular.z);
}

// function to set maximal velocity
void FOV_2D_model::setMaxVelocity(double lin_x, double lin_y, double lin_z, double ang_x, double ang_y, double ang_z)
{

  max_vel_.linear.x = lin_x;
  max_vel_.linear.y = lin_y;
  max_vel_.linear.z = lin_z;
  max_vel_.angular.x = ang_x;
  max_vel_.angular.y = ang_y;
  max_vel_.angular.z = ang_z;
  
}

// function to set maximal velocity
void FOV_2D_model::setMaxVelocity(geometry_msgs::Twist new_max_vel)
{
  max_vel_ = new_max_vel;
}

// function to set general sensor pose
void FOV_2D_model::setSensorPose(double x, double y, double z, double quat_x, double quat_y, double quat_z, double quat_w)
{
  sensor_pose_.position.x = x;
  sensor_pose_.position.y = y;
  sensor_pose_.position.z = z;

  sensor_pose_.orientation.x = quat_x;
  sensor_pose_.orientation.y = quat_y;
  sensor_pose_.orientation.z = quat_z;
  sensor_pose_.orientation.w = quat_w;
}

// function to set general sensor pose
void FOV_2D_model::setSensorPose(double x, double y, double z, geometry_msgs::Quaternion orientation)
{
  sensor_pose_.position.x = x;
  sensor_pose_.position.y = y;
  sensor_pose_.position.z = z;

  sensor_pose_.orientation = orientation;
}

// function to set general sensor pose
void FOV_2D_model::setSensorPose(geometry_msgs::Point position, geometry_msgs::Quaternion orientation)
{
  sensor_pose_.position = position;
  sensor_pose_.orientation = orientation;
}

// function to set general sensor pose
void FOV_2D_model::setSensorPose(geometry_msgs::Pose new_pos)
{
  sensor_pose_ = new_pos;
}

// function to set sensor opening angles
void FOV_2D_model::setOpenAngles(double open_ang1, double open_ang2)
{
  if(open_angles_.empty())
  {
    open_angles_.push_back(open_ang1);
    open_angles_.push_back(open_ang2);
  }
  else
  {
    while(!open_angles_.empty())
    {
      open_angles_.pop_back();
    }
    open_angles_.push_back(open_ang1);
    open_angles_.push_back(open_ang2);
  }
}

// function to set sensor range
void FOV_2D_model::setRange(double new_range)
{
  range_ = new_range;
}

// function to get actual velocity
geometry_msgs::Twist FOV_2D_model::getVelocity()
{
  return vel_;
}

// functgion to get the maximal velocity
geometry_msgs::Twist FOV_2D_model::getMaxVelocity()
{
  return max_vel_;
}

geometry_msgs::Pose FOV_2D_model::getSensorPose()
{
  return sensor_pose_;
}

// functions to get sensor opening angles
std::vector<double> FOV_2D_model::getOpenAngles()
{
  return open_angles_;
}

// function to set sensor range
double FOV_2D_model::getRange()
{
  return range_;
}

// returns the visualization markers of the respective sensor model
visualization_msgs::MarkerArray FOV_2D_model::getVisualizationMarkers(unsigned int id)
{
  visualization_msgs::MarkerArray array;
  visualization_msgs::Marker border, area;

  // setup standard stuff
  border.header.frame_id = area.header.frame_id = "/map";
  border.header.stamp = area.header.stamp = ros::Time();
  border.ns = area.ns = name_ + boost::lexical_cast<std::string>(id);
  border.action = area.action = visualization_msgs::Marker::ADD;
  border.pose = area.pose = sensor_pose_;

  // setup for border of fov
  border.id = 0;
  border.type = visualization_msgs::Marker::LINE_STRIP;
  border.scale.x = 0.1;
  border.color.a = 1.0;
  border.color.r = 1.0;
  border.color.g = 0.0;
  border.color.b = 0.0;

  // setup for filling fov using triangle markers
  area.id = 1;
  area.type = visualization_msgs::Marker::TRIANGLE_LIST;
  area.scale.x = 1.0;
  area.scale.y = 1.0;
  area.scale.z = 1.0;
  area.color.a = 0.5;
  area.color.r = 0.8;
  area.color.g = 0.0;
  area.color.b = 0.0;

  // first point of border
  border.points.push_back(geometry_msgs::Point());

  // produce arc for visualization by discretizing it
  // TODO: expose as parameter or specify max angle or something
  unsigned int num_steps = 90;
  double step_size = open_angles_.front() / num_steps;
  geometry_msgs::Point p, last_p;
  for (unsigned int i = 0; i <= num_steps; i++)
  {
    p.x = range_ * cos(open_angles_.front() / 2 - i * step_size);
    p.y = range_ * sin(open_angles_.front() / 2 - i * step_size);
    // intermediate point of border
    border.points.push_back(p);

    // add another triangle
    if (i>0)
    {
      area.points.push_back(geometry_msgs::Point());
      area.points.push_back(last_p);
      area.points.push_back(p);
    }
    last_p = p;
  }

  // last point of border
  border.points.push_back(geometry_msgs::Point());

  array.markers.push_back(border);
  array.markers.push_back(area);

  return array;
}
