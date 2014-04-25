#ifndef SHOW_TOPONAV_MAP_H
#define SHOW_TOPONAV_MAP_H

#include <string>
#include <ros/ros.h>
#include <visualization_msgs/Marker.h>

#include "show_toponav_map.h"
#include "toponav_node.h"
#include "toponav_edge.h"

/*
 * @file show_toponav_map
 * @brief This class will take care of visualization of the Topological Navigation Map as build in the class TopoNavMap
 * @author Koen Lekkerkerker
 */

class ShowTopoNavMap
{

public:
  ShowTopoNavMap(ros::NodeHandle &n, const std::vector<TopoNavNode*> &nodes, const std::vector<TopoNavEdge*> &edges); //Constructor
  /**
   * Public Methods
   */
  void updateVisualization();

private:
  /**
   * Variables
   */
  ros::NodeHandle &n_;
  const std::vector<TopoNavNode*> &nodes_;
  const std::vector<TopoNavEdge*> &edges_;

  visualization_msgs::Marker nodes_marker_;
  visualization_msgs::Marker edges_marker_;
  visualization_msgs::Marker doors_marker_;

  ros::Publisher marker_pub_;

  /**
   * Private Methods
   */
  void visualizeNodes();
  void visualizeEdges();
};

#endif // SHOW_TOPONAV_MAP_H
