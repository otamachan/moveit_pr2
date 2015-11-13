/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2012, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/* Author: Sachin Chitta */

//##include <boost/pointer_cast.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

#include <pluginlib/class_loader.h>
#include <ros/ros.h>

// MoveIt!
#include <moveit/robot_model_loader/robot_model_loader.h>
#include <moveit/planning_interface/planning_interface.h>
#include <moveit/planning_scene/planning_scene.h>
#include <moveit/kinematic_constraints/utils.h>
#include <moveit_msgs/DisplayTrajectory.h>
#include <moveit_msgs/PlanningScene.h>

#include <moveit/ompl_interface/model_based_planning_context.h>
#include <moveit/ompl_interface/ompl_interface.h>
#include <moveit/ompl_interface/detail/constrained_goal_sampler.h>

#include <ompl/util/Console.h>

#include <ros/console.h>

namespace ompl_interface
{

  class ModelBasedPlanningContext;
  class SimpleGoalSampler : public ompl::base::GoalLazySamples
  {
  public:
    SimpleGoalSampler(const ModelBasedPlanningContext *pc, robot_state::RobotState& state)
      : ompl::base::GoalLazySamples(pc->getOMPLSimpleSetup()->getSpaceInformation(),
                                    boost::bind(&SimpleGoalSampler::sampler, this, _1, _2), false)
      , planning_context_(pc)
      , state_(state)
    {
    }
  private:
    bool sampler(const ompl::base::GoalLazySamples *gls, ompl::base::State *new_goal) {
      if (gls->hasStates()) {
        return false;
      }
      planning_context_->getOMPLStateSpace()->copyToOMPLState(new_goal, state_);
      return true;
    }
    const ModelBasedPlanningContext *planning_context_;
    robot_state::RobotState&   state_;
  };
}



ompl::msg::OutputHandlerSTD handler;


int main(int argc, char **argv)
{
  ros::init (argc, argv, "move_group_tutorial");
  ros::AsyncSpinner spinner(1);
  spinner.start();
  ros::NodeHandle node_handle("~");

  ompl::msg::useOutputHandler(&handler);
  ompl::msg::setLogLevel(ompl::msg::LOG_DEBUG);
  ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug);

  robot_model_loader::RobotModelLoader robot_model_loader("robot_description");
  robot_model::RobotModelPtr robot_model = robot_model_loader.getModel();
  planning_scene::PlanningScenePtr planning_scene(new planning_scene::PlanningScene(robot_model));

  boost::scoped_ptr<ompl_interface::OMPLInterface> ompl_interface_(new ompl_interface::OMPLInterface(robot_model, node_handle));

  planning_interface::MotionPlanRequest req;
  planning_interface::MotionPlanResponse res;


  robot_state::RobotState& robot_state = planning_scene->getCurrentStateNonConst();
  const robot_state::JointModelGroup* joint_model_group = robot_state.getJointModelGroup("right_arm");
  //robot_state::RobotState goal_state(robot_model);
  moveit_msgs::Constraints joint_goal = kinematic_constraints::constructGoalConstraints(robot_state, joint_model_group);
  req.goal_constraints.clear();
  req.goal_constraints.push_back(joint_goal);
#if 0
  geometry_msgs::PoseStamped pose;
  pose.header.frame_id = "torso_lift_link";
  pose.pose.position.x = 0.75;
  pose.pose.position.y = 0.0;
  pose.pose.position.z = 0.0;
  pose.pose.orientation.w = 1.0;
  std::vector<double> tolerance_pose(3, 0.01);
  std::vector<double> tolerance_angle(3, 0.01);
  moveit_msgs::Constraints pose_goal = kinematic_constraints::constructGoalConstraints("r_wrist_roll_link", pose, tolerance_pose, tolerance_angle);
  req.goal_constraints.push_back(pose_goal);
#endif

  req.group_name = "right_arm";
  req.planner_id = "RRTkConfigDefault";

  ompl_interface::PlanningContextManager& context_manager_ = ompl_interface_->getPlanningContextManager();
  ompl_interface::ModelBasedPlanningContextPtr ctx = context_manager_.getPlanningContext(planning_scene, req, res.error_code_);
  ompl::geometric::SimpleSetupPtr ss = ctx->getOMPLSimpleSetup();

  ompl::base::GoalPtr goal = ctx->getOMPLSimpleSetup()->getGoal();
  ompl_interface::SimpleGoalSampler simple(ctx.get(), robot_state);
  ctx->getOMPLSimpleSetup()->setGoal(ompl::base::GoalPtr(static_cast<ompl::base::Goal*>(&simple)));

  std::vector<std::string> link_names;
  std::
  link_names.push_back(joint_model_group->getSolverInstance()->getTipFrame());

  ROS_ERROR(joint_model_group->getEndEffectorName().c_str());
  joint_model_group->printGroupInfo();
  ss->getSpaceInformation()->printSettings();
  ss->getSpaceInformation()->printProperties();

  ctx->solve(res);

  if(res.error_code_.val != res.error_code_.SUCCESS)
  {
    ROS_ERROR("Could not compute plan successfully");
    return 0;
  }
  //ss->getProblemDefinition()->print();
  return 0;
}

