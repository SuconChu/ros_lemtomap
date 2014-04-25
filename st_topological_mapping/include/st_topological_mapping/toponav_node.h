#ifndef TOPONAV_NODE_H
#define TOPONAV_NODE_H

#include "ros/ros.h"
#include "tf/transform_datatypes.h"
#include <vector>
#include <algorithm> //std::find


/*
 * The TopoNavNode class can be used to create TopoNavNode objects.
 * These objects form the nodes and all their properties that
 * together with the edges make the TopoNavMap.
 * TODO p3 - Possibly, using a struct or so and integrate it  in TopoNavMap class instead of separate TopoNavNode class makes more sense...
 */

typedef int node_id_int; //This can be used to help function signatures see the difference between a node_id form any int

class TopoNavNode
{

public:
  TopoNavNode(tf::Pose pose, bool is_door, int area_id, std::vector<TopoNavNode*> &nodes);
  ~TopoNavNode();

  /**
   * Public Methods
   */

  // get Methods
  const node_id_int getNodeID() const { return node_id_; }
  const ros::Time getLastUpdatedTime() const { return last_updated_; }
  const int getAreaID() const { return area_id_; }
  const tf::Pose  getPose() const { return pose_; }
  const bool getIsDoor() const { return is_door_; }
  // set Methods
  // void setNodeID(int node_id)  { node_id_=node_id; last_updated_=ros::Time::now();} //NodeID should be static, so set method is n.a.
  void setAreaID(int area_id)  { area_id_=area_id; last_updated_=ros::Time::now();}
  void setPose(tf::Pose  pose)  { pose_=pose; last_updated_=ros::Time::now();}
  //void setPose(double x, double y, double theta) // @TODO: implement this set method, if desired.
  void setIsDoor(bool is_door)  { is_door_=is_door; last_updated_=ros::Time::now();}

private:
  /**
   * Variables
   */
  node_id_int node_id_; //node_ids should never be changed!
  ros::Time last_updated_;
  int area_id_; //an area is a collection of nodes, in general areas would be rooms. But in future, large spaces or outdoor spaces could be divided in smaller areas, like the coffee corner, lunch corner and sitting area in the TU Delft Aula building.
  tf::Pose pose_;
  bool is_door_;
  // @TODO add some "Properties of space" msg type that I still need to implement. This should get objects, local room size (by local I mean: as measured at this node), local room shape. All defined using semantic/symbolic labels with a prob. distribution of the relevant collection of such labels (e.g. room size 0.9 large, 0.07 medium, 0.04 small).
  // @TODO add room_type, also as a semantic/symbolic probability distribution of all possible room types.

  std::vector<TopoNavNode*> &nodes_;
  /**
   * Private Methods
   */

};

#endif // TOPONAV_NODE_H

