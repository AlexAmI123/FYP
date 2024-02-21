/**
 * @file PenaltyShotStriker.h
 *
 * This file contains the striker behaviours to use during an in-game
 * penalty shot or a penalty shoot out after normal time.
 * 
 * It has been adapted from the RoboEireann 2019 behaviour (hattrick project) 
 * to fit the 2021 code release framework (rematch project) and coro behaviour 
 * engine.
 *
 * @author Rudi Villing
 */


#pragma once

#include "Modules/BehaviorControl/CoroBehaviour/2022/CoroBehaviour2022.h"

#include "GotoBallSkills.h"

#include "Modules/BehaviorControl/CoroBehaviour/Skills/CommonSkills.h"
#include "Modules/BehaviorControl/CoroBehaviour/Skills/HeadSkills.h"
#include "Modules/BehaviorControl/CoroBehaviour/Skills/BallSkills.h"

#include "Representations/BehaviorControl/Skills.h"

#include "Representations/BehaviorControl/KickoffState.h"

#include "Tools/Math/Pose2f.h"


namespace CoroBehaviour
{
namespace RE2019
{
  /**
   * During this ready state, the striker should go to penalty kick position
   * which is just outside the penalty area facing the opponent goal
   */
  CRBEHAVIOUR(PenaltyShotStrikerReadyTask)
  {
    CRBEHAVIOUR_INIT(PenaltyShotStrikerReadyTask) {}

    void operator()(void)
    {
      CRBEHAVIOUR_BEGIN();

      CR_CHECKPOINT(walk_to_penalty_ready_pose);
      while (!walkToPoseAutoAvoidanceTask.isSuccess())
      {
        commonSkills.activityStatus(BehaviorStatus::reWalkToTacticPose);

        headSkills.lookActive();
        walkToPoseAutoAvoidanceTask(getPenaltyKickReadyPose(),
                                  Pose2f(params.speed, params.speed, params.speed),
                                  /* keepTargetRotation */ false, params.positionThreshold, params.angleThreshold);
        CR_YIELD();
      }

      CR_CHECKPOINT(stand_at_penalty_ready_pose);
      while (true)
      {
        commonSkills.activityStatus(BehaviorStatus::reStandAtTacticPose);

        headSkills.lookAtPoint(theRobotPose.toRobotVector3f(
            Vector2f(theFieldDimensions.xPosOpponentPenaltyMark, 0),
            theBallSpecification.radius)); // look at the penalty spot (the ball should be there)
        commonSkills.stand();
        CR_YIELD();
      }
    }

  private:
    DEFINES_PARAMS(PenaltyShotStrikerReadyTask,
    {,
      (float)(50.f) positionThreshold, 
      (float)(5_deg) angleThreshold,
      (float)(1.f) speed, 
    });

    READS(RobotPose);
    READS(FieldDimensions);
    READS(RobotDimensions);
    READS(BallSpecification);

    CommonSkills commonSkills {env};
    HeadSkills headSkills {env};
    WalkToPoseAutoAvoidanceTask walkToPoseAutoAvoidanceTask {env};

    // bool targetLeft;
    // Angle targetAngle;

    Pose2f getPenaltyKickReadyPose()
    {
      Pose2f poseOnField = Pose2f(0_deg, theFieldDimensions.xPosOpponentPenaltyArea - theRobotDimensions.soleToFrontEdgeLength, 0);

      return theRobotPose.toRobotCoordinates(poseOnField);
    }
  };




  /**
   * This behaviour is used whether in a penalty shoot out or taking a penalty
   * shot during a game. For penalty shots we look at the ball/penalty mark position
   */
  CRBEHAVIOUR(PenaltyShotStrikerSetStateTask)
  {
    CRBEHAVIOUR_INIT(PenaltyShotStrikerSetStateTask) {}

    void operator()(void)
    {
      commonSkills.activityStatus(BehaviorStatus::set);

      CRBEHAVIOUR_LOOP()
      {
        headSkills.lookAtPoint(theRobotPose.toRobotVector3f(
            Vector2f(theFieldDimensions.xPosOpponentPenaltyMark, 0),
            theBallSpecification.radius)); // look at the penalty spot (the ball should be there)
        commonSkills.stand();
        CR_YIELD();
      }
    }

  private:
    READS(FieldDimensions);
    READS(RobotPose);
    READS(BallSpecification);

    HeadSkills headSkills {env};
    CommonSkills commonSkills {env};
  };



  /**
   * This behaviour is used whether in a penalty shoot out or taking a penalty
   * shot during a game. We assume the same basic starting location in both
   * cases
   */
  CRBEHAVIOUR(PenaltyShotStrikerPlayingTask)
  {
    CRBEHAVIOUR_INIT(PenaltyShotStrikerPlayingTask) {}

    void operator()(void)
    {
      commonSkills.activityStatus(BehaviorStatus::rePenaltyShot);

      CRBEHAVIOUR_BEGIN();

      // look at penalty spot for some max time (or until we see the ball - whichever is first)
      CR_CHECKPOINT(look_at_pen_spot);
      while (!theFieldBall.ballWasSeen(params.ballSeenMs) && (getCheckpointDuration() < params.lookAtPenaltySpotMs))
      {
        headSkills.lookAtPoint(theRobotPose.toRobotVector3f(
            Vector2f(theFieldDimensions.xPosOpponentPenaltyMark, 0),
            theBallSpecification.radius)); // look at the penalty spot (the ball should be there)
        commonSkills.stand();
        CR_YIELD();
      }

      // walk forward to the stop and scan distance

      CR_CHECKPOINT(walk_to_stop_and_scan);
      while (!walkToPoseNoAvoidanceTask.isSuccess())
      {
        headSkills.lookAtPoint(theRobotPose.toRobotVector3f(
            Vector2f(theFieldDimensions.xPosOpponentPenaltyMark, 0),
            theBallSpecification.radius)); // look at the penalty spot (the ball should be there)
        walkToPoseNoAvoidanceTask(getStopAndScanTarget(),
                                  Pose2f(params.speed1, params.speed1, params.speed1),
                                  /* keepTargetRotation */ true, params.positionThreshold, params.angleThreshold);
        CR_YIELD();
      }

      // do a scan (fix our location and potentially detect the goalie)

      CR_CHECKPOINT(stop_and_scan);
      while (getCheckpointDuration() < params.scanDurationMs)
      {
        lookLeftAndRightTask();
        commonSkills.stand();
        CR_YIELD();
      }

      // OK, walk up and take the shot

      chooseShot();

      CR_CHECKPOINT(penalty_kick);
      while (!gotoBallAndKickTask.isSuccess())
      {
        // head is managed by gotoBallAndKick
        gotoBallAndKickTask(theRobotPose.toRobotCoordinates(shotTargetOnField).angle(), shotKickType,
                            /* alignPrecisely */ true, /* length */ 6000.f, /* preStepAllowed */ true,
                            /* turnKickAllowed */ false, Pose2f(params.speed2, params.speed2, params.speed2));
        CR_YIELD();
      }

      // wait until higher level behaviour switches out of this coroutine

      CR_CHECKPOINT(wait_after_kick);
      while (true)
      {
        headSkills.lookActive();
        commonSkills.stand();
        CR_YIELD();
      }
    }

  private:
    DEFINES_PARAMS(PenaltyShotStrikerPlayingTask,
    {,
      (float)(250.f) stopAndScanDistance, 
      (unsigned)(1000) lookAtPenaltySpotMs,
      (unsigned)(300) ballSeenMs, ///< must see the ball within this time
      (unsigned)(2000) scanDurationMs, 
      (float)(50.f) positionThreshold, 
      (float)(5_deg) angleThreshold,
      (float)(1.f) speed1, 
      (float)(0.7f) speed2,
      (float)(150.f) yInsideGoalPost, ///< the y distance inside the goal post to aim for - tradeoff
                                    ///< between difficult to save and the possibility of missing
    });

    READS(FieldBall);
    READS(FieldDimensions);
    READS(RobotPose);
    READS(BallSpecification);

    CommonSkills commonSkills{env};
    HeadSkills headSkills{env};
    WalkToPoseNoAvoidanceTask walkToPoseNoAvoidanceTask{env};
    LookLeftAndRightTask lookLeftAndRightTask{env};
    GotoBallAndKickTask gotoBallAndKickTask{env};

    Vector2f shotTargetOnField;
    KickInfo::KickType shotKickType;

    Pose2f getStopAndScanTarget()
    {
      return theRobotPose.toRobotCoordinates(
          Pose2f(0_deg, theFieldDimensions.xPosOpponentPenaltyMark - params.stopAndScanDistance, 0));
    }

    void chooseShot()
    {
      float side = Random::bernoulli() ? 1 : -1;

      shotTargetOnField = Vector2f(theFieldDimensions.xPosOpponentGroundLine, (theFieldDimensions.yPosLeftGoal - params.yInsideGoalPost) * side);

      // if shooting left, kick with the left, otherwise kick with the right
      shotKickType = (side == 1) ? KickInfo::forwardFastLeft : KickInfo::forwardFastRight;
    }
  };

} // RE2019
} // CoroBehaviour2022