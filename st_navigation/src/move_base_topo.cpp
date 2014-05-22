/**
 * @file move_base_topo
 * @brief Navigate a robot based on a topological map and topological goal
 * @author Koen Lekkerkerker
 */

/**
   The move_base_topo ros node is an action server that can receive a topological node as a goal.
   It is actually *not* a nav_core global_planner. It only generates goals to pass to the global_planner through
   the move_base system. So move_base_topo generates a topological path (which has a similar role as the normal global plan),
   and passes a series of goals to move_base, which then uses the global and local planner to execute it.
 */

#include <st_navigation/move_base_topo.h>

MoveBaseTopo::MoveBaseTopo(std::string name):
    action_server_mbt_(nh_, name, boost::bind(&MoveBaseTopo::executeCB, this, _1), false),
    action_name_mbt_(name),
    move_base_client_("move_base", true)
  {
	ros::NodeHandle private_nh("~");

	private_nh.param("goal_frame_id", goal_frame_id_, std::string("map"));

	toponav_map_topic_ = "topological_navigation_mapper/topological_navigation_map";
	toponavmap_sub_ = nh_.subscribe(toponav_map_topic_, 1, &MoveBaseTopo::toponavmapCB, this);
	action_server_mbt_.start();

  }

void MoveBaseTopo::toponavmapCB(const st_topological_mapping::TopologicalNavigationMapConstPtr& toponav_map)
{
		ROS_DEBUG("Received map with %lu nodes and %lu edges",
				toponav_map->nodes.size(), toponav_map->edges.size());
		toponavmap_=*toponav_map;
}

void MoveBaseTopo::executeCB(const st_navigation::GotoNodeGoalConstPtr& goal) //CB stands for CallBack...
  {
	ROS_INFO("Waiting for move_base action server");
	move_base_client_.waitForServer();
	ROS_INFO("Waiting for move_base action server -- Finished");

	// helper variables
    bool success = false;
    std::vector<int> path_nodes;
    std::vector<int> path_edges;
    int start_node_id;
    tf::Pose robot_pose_tf;
    double dist_tolerance_intermediate = 1.0; // a intermediate node is regarded 'reached' when it is within this distance [m]. New goal will then be passed.

    // Calculate the topological path
    start_node_id = getCurrentAssociatedNode();
    path_nodes = st_shortest_paths::shortestPath(toponavmap_,start_node_id,goal->target_node_id);
    path_edges=nodesPathToEdgesPath(path_nodes);

    // Action server feedback
    feedback_.route_node_ids=path_nodes;
    feedback_.route_edge_ids=path_edges;
    action_server_mbt_.publishFeedback(feedback_);

    // Variables for generating move base goals
    geometry_msgs::PoseStamped node_pose;
	move_base_msgs::MoveBaseGoal move_base_goal;
	tf::Pose robot_pose = getCurrentPose();

	// Generate mapping to find nodes by node id from toponavmsg
	std::map<int, int> nodes_id2vecpos_map;
	for (int i = 0; i < toponavmap_.nodes.size(); i++)
	{
		nodes_id2vecpos_map[toponavmap_.nodes.at(i).node_id] = i;
	}

	node_pose.pose.orientation.w  = 1.0; //position x,y,z default to 0 for now...
	node_pose.header.frame_id = goal_frame_id_;

    // start executing the action
    int i = 0;
	while (!success && ros::ok()) //you need to add ros::ok(), otherwise the loop will never finish
    {
	  // pass new goal node if within 1 m of current goal node, or if it is the first goal. Only check for passing new goals if last is not passed already
	  if ((i==0 || calcDistance(node_pose.pose, getCurrentPose()) < dist_tolerance_intermediate) && i!=path_nodes.size())
	  {
		  ROS_DEBUG("Navigating to goal node #%d, with NodeID %d", i+1, path_nodes.at(i));
		  node_pose.pose.position = toponavmap_.nodes.at(nodes_id2vecpos_map[path_nodes.at(i)]).pose.position;
		  move_base_goal.target_pose = node_pose;
		  move_base_client_.sendGoal(move_base_goal);
		  i++; // current i is always +1 compared to the current goal node vector position in path_nodes
	  }
      ROS_DEBUG("Distance between robot and intermediate Goal NodeID %d = %f", path_nodes.at(i-1), calcDistance(node_pose.pose, getCurrentPose()));

	  // check that preempt has not been requested by the client -> a preempt (cancelation) will automatically be triggered if a new goal is send!
      if (action_server_mbt_.isPreemptRequested() || !ros::ok())
      {
        ROS_INFO("%s: Preempted", action_name_mbt_.c_str());
        // set the action state to preempted
        result_.success = false;
        action_server_mbt_.setPreempted(result_);
        break;
      }

      // TODO : Add some handling of the move_base_client_.getState() -> if it fails, this should be noticed and handled.

      // check if the final goal has been reached
      if (i == path_nodes.size() && move_base_client_.getState() == actionlib::SimpleClientGoalState::SUCCEEDED){
    	  success = true;
    	  ROS_INFO("Hooray: the final topological goal has been reached");
      }
      ros::Rate(1).sleep(); //to decrease CPU load...
    }

    if(success)
    {
      result_.success = true;
      ROS_INFO("%s: Succeeded", action_name_mbt_.c_str());
      // set the action state to succeeded
      action_server_mbt_.setSucceeded(result_);
    }
    else
	{
		result_.success = false;
		ROS_INFO("%s: Failed", action_name_mbt_.c_str());
		//do not set succeeded or abort here: it might be that a new action was requested and thus the action is still alive in an active status! I guess (KL)...
	}
  }

/*!
 * getCurrentPose
 */
tf::Pose MoveBaseTopo::getCurrentPose() {
	tf::TransformListener tf_listener;
	tf::Pose robot_pose_tf;
	tf::StampedTransform robot_transform_tf;

	try {
		tf_listener.waitForTransform("/map", "/base_link", ros::Time(0),
				ros::Duration(10));
		tf_listener.lookupTransform("/map", "/base_link", ros::Time(0),
				robot_transform_tf);
	} catch (tf::TransformException &ex) {
		ROS_ERROR("Error looking up transformation\n%s", ex.what());
	}

	robot_pose_tf.setOrigin(robot_transform_tf.getOrigin());
	robot_pose_tf.setRotation(robot_transform_tf.getRotation());

	return robot_pose_tf;
}

int MoveBaseTopo::getCurrentAssociatedNode(){
	int associated_node;
	tf::Pose robot_pose_tf = getCurrentPose();
	double closest_dist=DBL_MAX;
	double tentative_closest_dist;

	//FIXME This loop picks the closest node, not even taking into account blocking obstacles or e.g. the direction in which the robot should travel eventually.
	ROS_WARN_ONCE("Currently, the starting node is the closest node to robot, which could even go through walls. This method should be improved. This warning will only print once.");
	for(int i = 0 ; i < toponavmap_.nodes.size() ; i++){
		tentative_closest_dist = calcDistance(toponavmap_.nodes.at(i).pose,robot_pose_tf);
		if (tentative_closest_dist < closest_dist)
		{
			closest_dist = tentative_closest_dist;
			associated_node = toponavmap_.nodes.at(i).node_id;
		}
	}

	ROS_INFO("Associated Node ID=%d. Pose is x=%f, y=%f, theta=%f", associated_node, robot_pose_tf.getOrigin().x(),
			robot_pose_tf.getOrigin().y(),
			tf::getYaw(robot_pose_tf.getRotation()));

	return associated_node;
}

std::vector<int> MoveBaseTopo::nodesPathToEdgesPath(const std::vector<int>& path_nodes)
{
	std::vector<int> path_edges;

	for (int i = 0; i < path_nodes.size()-1; i++)
	{
		for (int j = 0; j < toponavmap_.edges.size(); j++)
		{
			if((toponavmap_.edges.at(j).start_node_id==path_nodes.at(i) && toponavmap_.edges.at(j).end_node_id==path_nodes.at(i+1)) ||
					(toponavmap_.edges.at(j).end_node_id==path_nodes.at(i) && toponavmap_.edges.at(j).start_node_id==path_nodes.at(i+1)))
			{
				path_edges.push_back(toponavmap_.edges.at(j).edge_id);
				break;
			}
		}
	}
	return path_edges;
}

/*!
 * Main
 */
int main(int argc, char** argv) {
	ros::init(argc, argv, "move_base_topo");
	ros::NodeHandle n;

	ros::Rate r(1);

	MoveBaseTopo move_base_topo(ros::this_node::getName());

	//Set default logger level for this ROS Node...
	/*if( ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug) ) {
		ros::console::notifyLoggerLevelsChanged();
	}*/

	while (n.ok()) {

		ros::spinOnce();
		r.sleep();
	}

	return 0;
}
