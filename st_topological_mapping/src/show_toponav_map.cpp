/**
 * @file show_toponav_map
 * @brief Publish markers showing the Topological Navigation Map to a visualization topic for RVIZ.
 * @author Koen Lekkerkerker
 */

#include <st_topological_mapping/show_toponav_map.h>

ShowTopoNavMap::ShowTopoNavMap(ros::NodeHandle &n, const std::map<NodeID, TopoNavNode*> &nodes,
                               const std::map<EdgeID, TopoNavEdge*> &edges) :
    n_(n), // this way, ShowTopoNavMapis aware of the NodeHandle of this ROS node, just as TopoNavMap...
    nodes_(nodes), edges_(edges)
{
  ROS_DEBUG("ShowTopoNavMap object is constructed");

  std::string movebasetopo_feedback_topic = "move_base_topo/feedback";
  movebasetopo_feedback_sub_ = n_.subscribe(movebasetopo_feedback_topic, 1, &ShowTopoNavMap::moveBaseTopoFeedbackCB, this);

  markers_pub_ = n_.advertise<visualization_msgs::MarkerArray>("toponavmap_markerarray", 1,true);

  // Set all general marker properties to a marker
  nodes_marker_template_.header.frame_id = "/map";
  nodes_marker_template_.header.stamp = ros::Time::now();
  nodes_marker_template_.action = visualization_msgs::Marker::ADD;
  nodes_marker_template_.pose.orientation.w;
  nodes_marker_template_.lifetime = ros::Duration(1.5); //it will take up to this much time until deleted markers will disappear...

  // Equal other markers with this 'template' marker.
  edges_marker_template_ = nodes_marker_template_;
  doors_marker_template_ = nodes_marker_template_;

  // Set marker specific properties
  nodes_marker_template_.ns = "nodes";
  nodes_marker_template_.type = visualization_msgs::Marker::CYLINDER;
  nodes_marker_template_.color.r = 0.0;
  nodes_marker_template_.color.g = 0.0;
  nodes_marker_template_.color.b = 1.0;
  nodes_marker_template_.color.a = 0.5;
  nodes_marker_template_.scale.x = 0.5;
  nodes_marker_template_.scale.y = 0.5;
  nodes_marker_template_.scale.z = 0.001;

  doors_marker_template_.ns = "doors";
  nodes_marker_template_.type = visualization_msgs::Marker::CYLINDER;
  doors_marker_template_.color.r = 1.0;
  doors_marker_template_.color.g = 0.0;
  doors_marker_template_.color.b = 0.0;
  doors_marker_template_.color.a = 1;
  doors_marker_template_.scale.x = 0.2;
  doors_marker_template_.scale.y = 0.2;
  doors_marker_template_.scale.z = 0.001;

  edges_marker_template_.ns = "edges";
  edges_marker_template_.type = visualization_msgs::Marker::LINE_STRIP;
  edges_marker_template_.scale.x = 0.05;
  edges_marker_template_.color.r = 0.0;
  edges_marker_template_.color.g = 0.0;
  edges_marker_template_.color.b = 0.0;
  edges_marker_template_.color.a = 1.0;
}

void ShowTopoNavMap::updateVisualization()
{
  toponavmap_ma_.markers.clear();
  visualizeNodes();
  visualizeEdges();
  markers_pub_.publish(toponavmap_ma_);

}

void ShowTopoNavMap::visualizeNodes()
{
  visualization_msgs::Marker node_marker;
  visualization_msgs::Marker door_marker;

	//if having gcc4.7 or higher and enable c++11, you could use: auto it=nodes_.begin(); it!=nodes_.end(); it++
  for(std::map<NodeID, TopoNavNode*>::const_iterator it=nodes_.begin(); it!=nodes_.end(); it++) //TODO: This visualizes every time for all nodes! Maybe only updated nodes should be "revizualized".
  {
	node_marker = nodes_marker_template_;
	door_marker = doors_marker_template_;
	//If it is part of the navigation path: give it a nice color
	if(std::find(topo_path_nodes_.begin(), topo_path_nodes_.end(), it->second->getNodeID()) != topo_path_nodes_.end())
	{
	  if (it->second->getIsDoor()==true)
		door_marker.color.r = 1.0;
	  else
	    node_marker.color.r = 1.0;
	}

	//Check if it is a door node
    if(it->second->getIsDoor()==true)
    {
      door_marker.id = it->second->getNodeID(); //it->second->getNodeID() should equal it->first
      poseTFToMsg(it->second->getPose(),door_marker.pose);
      toponavmap_ma_.markers.push_back(door_marker);
    }
    else
    {
      node_marker.id = it->second->getNodeID(); // as there can only be one per ID: updated nodes are automatically moved if pose is updated, without the need to remove the old one...
      poseTFToMsg(it->second->getPose(),node_marker.pose);
      toponavmap_ma_.markers.push_back(node_marker);
    }


  }
}

void ShowTopoNavMap::visualizeEdges ()
{
  for(std::map<EdgeID, TopoNavEdge*>::const_iterator it=edges_.begin(); it!=edges_.end(); it++) //TODO: This visualizes every time for all edges! Maybe only updated edges should be "revizualized".
  {
	  visualization_msgs::Marker edge_marker = edges_marker_template_;
	  edge_marker.id =it->second->getEdgeID();
      edge_marker.points.resize(2); // each line_list exits of one line

      //Source
      edge_marker.points[0].x = it->second->getStartNode().getPose().getOrigin().getX();
      edge_marker.points[0].y = it->second->getStartNode().getPose().getOrigin().getY();
      edge_marker.points[0].z = 0.0f;

      //Destination
      edge_marker.points[1].x = it->second->getEndNode().getPose().getOrigin().getX();
      edge_marker.points[1].y = it->second->getEndNode().getPose().getOrigin().getY();
      edge_marker.points[1].z = 0.0f;

      //If it is part of the navigation path: give it a nice color
      if(std::find(topo_path_edges_.begin(), topo_path_edges_.end(), edge_marker.id) != topo_path_edges_.end())
      {
    	  edge_marker.color.r = 1.0;
      }

      toponavmap_ma_.markers.push_back(edge_marker);
  }
}

void ShowTopoNavMap::moveBaseTopoFeedbackCB (const st_navigation::GotoNodeActionFeedback feedback)
{
 topo_path_nodes_=feedback.feedback.route_node_ids;
 topo_path_edges_=feedback.feedback.route_edge_ids;

 //turn it into a string and print it
 std::stringstream path_stringstream;
 std::string path_string;
 std::copy(topo_path_nodes_.begin(), topo_path_nodes_.end(), std::ostream_iterator<int>(path_stringstream, ", "));
 path_string=path_stringstream.str();
 path_string=path_string.substr(0, path_string.size()-2);
 ROS_INFO("Received Topological Path for visualization (node ids): [%s]",path_string.c_str());

 path_stringstream.str(std::string());
 path_string.clear();
 std::copy(topo_path_edges_.begin(), topo_path_edges_.end(), std::ostream_iterator<int>(path_stringstream, ", "));
 path_string=path_stringstream.str();
 path_string=path_string.substr(0, path_string.size()-2);
 ROS_INFO("Received Topological Path for visualization (edge ids): [%s]",path_string.c_str());
}
