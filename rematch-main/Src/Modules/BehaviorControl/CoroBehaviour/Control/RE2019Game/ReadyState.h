/**
 * @file ReadyStateTask.h
 *
 * This task implements the ready state behaviour (proceeding to kickoff positions).
 * It has been adapted to the 2021 rules regarding default start positions
 * for the robots.
 *
 * @author Rudi Villing
 */


#pragma once

#include "Modules/BehaviorControl/CoroBehaviour/2022/CoroBehaviour2022.h"

#include "PenaltyShotGoalie.h"
#include "PenaltyShotStriker.h"
#include "PenaltyShotOther.h"

#include "Modules/BehaviorControl/CoroBehaviour/Skills/CommonSkills.h"
#include "Modules/BehaviorControl/CoroBehaviour/Skills/HeadSkills.h"

#include "Representations/BehaviorControl/Skills.h"

#include "Tools/Math/Pose2f.h"


namespace CoroBehaviour
{
namespace RE2019
{
  CRBEHAVIOUR(ReadyStateKickoffTask)
  {
    CRBEHAVIOUR_INIT(ReadyStateKickoffTask) {}

    void operator()(void)
    {
      CRBEHAVIOUR_BEGIN();

      initReadyPose();

      CR_CHECKPOINT(look_around);
      // contine looking around while time is not up, while we don;t have great localisation, and while we're in our own half
      while (((getCoroDuration() < lookAroundMs) && (theRobotPose.quality != RobotPose::superb)) &&
             (theRobotPose.translation.x() < 0))
      {
        commonSkills.activityStatus(BehaviorStatus::rePositionForKickOff);
        headSkills.lookActive(/* withBall: */ false, /* ignoreBall: */ true);
        commonSkills.stand();
        CR_YIELD();
      }

      CR_CHECKPOINT(walk_to_kickoff);
      while (true)
      {
        commonSkills.activityStatus(BehaviorStatus::rePositionForKickOff);

        CALL_SKILL(WalkToKickoffPose)(readyPoseOnField);
        CR_YIELD();
      }
    }

  private:
    READS(RobotPose);
    READS(RobotInfo);
    READS(FieldDimensions);
    READS(GameInfo);
    
    CommonSkills commonSkills {env};
    HeadSkills headSkills {env};

    unsigned lookAroundMs = 3000;

    Pose2f readyPoseOnField;

    // for now just a simple implementation based on robot number
    // (we try to set positions that will scale to different field sizes)
    void initReadyPose()
    {
      if (theRobotInfo.isGoalkeeper()) // goalie?
        readyPoseOnField = Pose2f(0, theFieldDimensions.xPosOwnGroundLine, 0);
      else
      {        
        if (env.isOurTeamKick()) // is it our kickoff
        {
          // offensive kickoff positions
          switch (theRobotInfo.number)
          {
            // case 2: readyPoseOnField = Pose2f(0, theFieldDimensions.xPosOwnPenaltyArea + 500.f, theFieldDimensions.yPosRightGoalArea); break; // defender
            // case 3: readyPoseOnField = Pose2f(0, -500.f, theFieldDimensions.yPosLeftPenaltyArea); break; // left mid
            // case 4: readyPoseOnField = Pose2f(0, -500.f, theFieldDimensions.yPosRightPenaltyArea); break; // right mid

            case 2: readyPoseOnField = Pose2f(0, theFieldDimensions.xPosOwnPenaltyArea + 100.f, theFieldDimensions.yPosRightGoalArea/4.f); break; // defender
            // case 3: readyPoseOnField = Pose2f(0, theFieldDimensions.xPosOwnGroundLine / 2 + 800.f, theFieldDimensions.yPosLeftGoal); break; // left mid
            // case 4: readyPoseOnField = Pose2f(0, theFieldDimensions.xPosOwnGroundLine / 2 + 200.f, theFieldDimensions.yPosRightGoal); break; // right mid
            case 3: readyPoseOnField = Pose2f(0, theFieldDimensions.xPosOwnGroundLine / 2 + 1300.f, theFieldDimensions.yPosLeftGoal); break; // left mid
            case 4: readyPoseOnField = Pose2f(0, theFieldDimensions.xPosOwnGroundLine / 2 + 500.f, theFieldDimensions.yPosRightGoal); break; // right mid

            case 5: readyPoseOnField = Pose2f(0, -200.f, 0); break; // striker
            default: FAIL("Unknown robot number: " << theRobotInfo.number);
          }
        }
        else // no, it's the opponent kickoff
        {
          // defensive kickoff positions
          switch (theRobotInfo.number)
          {
            // case 2: readyPoseOnField = Pose2f(0, theFieldDimensions.xPosOwnPenaltyArea + 150, theFieldDimensions.yPosRightGoalArea/2); break; // defender
            // case 3: readyPoseOnField = Pose2f(0, -800.f, theFieldDimensions.yPosLeftGoalArea); break; // left mid
            // case 4: readyPoseOnField = Pose2f(0, -800.f, theFieldDimensions.yPosRightGoalArea); break; // right mid
            // case 5: readyPoseOnField = Pose2f(0, -theFieldDimensions.centerCircleRadius - 200.f, 0); break; // striker

            case 2: readyPoseOnField = Pose2f(0, theFieldDimensions.xPosOwnPenaltyArea + 100.f, theFieldDimensions.yPosRightGoalArea/4.f); break; // defender
            // case 3: readyPoseOnField = Pose2f(0, theFieldDimensions.xPosOwnGroundLine / 2, theFieldDimensions.yPosLeftGoal); break; // left mid
            case 3: readyPoseOnField = Pose2f(0, theFieldDimensions.xPosOwnGroundLine / 2 + 500.f, theFieldDimensions.yPosLeftGoal); break; // left mid
            case 4: readyPoseOnField = Pose2f(0, theFieldDimensions.xPosOwnGroundLine / 2, theFieldDimensions.yPosRightGoal); break; // right mid
            case 5: readyPoseOnField = Pose2f(0, -theFieldDimensions.centerCircleRadius - 400.f, 0); break; // striker

            default: FAIL("Unknown robot number: " << theRobotInfo.number);
          }
        }

      }

    }
  };




  // this is the main entry point for the ready state and it delegates to the appropriate
  // specialized behaviour to do the real work

  CRBEHAVIOUR(ReadyStateTask)
  {
    CRBEHAVIOUR_INIT(ReadyStateTask) {}

    void operator()(void)
    {
      CRBEHAVIOUR_LOOP()
      {
        if (theGameInfo.setPlay == SET_PLAY_PENALTY_KICK)
        {
          if (!commonSkills.isOurTeamKick() && theTeamBehaviorStatus.role.isGoalkeeper())
            penaltyShotGoalieReadyTask();
          else if (commonSkills.isOurTeamKick() && theTeamBehaviorStatus.role.playsTheBall())
            penaltyShotStrikerReadyTask();
          else
            penaltyShotOtherReadyTask();
        }
        else
          readyStateKickoffTask();

        CR_YIELD();
      }
    }

  private:
    READS(TeamBehaviorStatus);
    READS(GameInfo);

    CommonSkills commonSkills {env};

    PenaltyShotGoalieReadyTask penaltyShotGoalieReadyTask {env};
    PenaltyShotStrikerReadyTask penaltyShotStrikerReadyTask {env};
    PenaltyShotOtherReadyTask penaltyShotOtherReadyTask {env};
    ReadyStateKickoffTask readyStateKickoffTask {env};
  };


} // RE2019
} // CoroBehaviour2022
