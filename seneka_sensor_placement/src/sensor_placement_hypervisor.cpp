/****************************************************************
 *
 * Copyright (c) 2014
 *
 * Fraunhofer Institute for Manufacturing Engineering
 * and Automation (IPA)
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Project name: SeNeKa
 * ROS stack name: seneka_deployment_strategies
 * ROS package name: seneka_sensor_placement
 *
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * Author: Muhammad Bilal Chughtai, email:Muhammad.Chughtai@ipa.fraunhofer.de
 *
 * Date of creation: October 2014
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


#include "sensor_placement_hypervisor.h"



sensor_placement_hypervisor::sensor_placement_hypervisor()
{
   ROS_INFO("sensor_placement_hypervisor created");

  nav_path_sub = n.subscribe(std::string("sensor_poses"), 1, &sensor_placement_hypervisor::navPathSubCB, this);                  // TODO: check suitable buffer size
  quanjo_maneuver_pub = n.advertise<std_msgs::String>(std::string("quanjo_maneuver"), 1, true);

}

sensor_placement_hypervisor::~sensor_placement_hypervisor(){}

void sensor_placement_hypervisor::navPathSubCB(const nav_msgs::Path)
{


  //TODO: save nav_path into a std::vector
  ROS_INFO("I heard: ....................");


  quanjo_maneuver_pub.publish(std::string("test_quanjo_maneuver_msg"));
}


int main(int argc, char **argv)
{

  ros::init(argc, argv, std::string("sensor_placement_hypervisor"));


  // create sensor_placement_hypervisor node object
  sensor_placement_hypervisor my_placement_hypervisor;

  //specify 10hz loop rate
  ros::Rate loop_rate(10);

  int count = 0;

  while (my_placement_hypervisor.n.ok())
  {

    // TODO: replace with Path+payload msg
    std_msgs::String msg;

    std::stringstream ss;
    ss << "this is quanjo_maneuver_msg message " << count;
    msg.data = ss.str();


    //ROS_INFO("%s", msg.data.c_str());
    //publish the msg on "quanjo_maneuver" topic
    //quanjo_maneuver_pub.publish(msg);

    ros::spinOnce();
    //ros::spin();

    loop_rate.sleep();
    ++count;
  }


  return 0;
}
