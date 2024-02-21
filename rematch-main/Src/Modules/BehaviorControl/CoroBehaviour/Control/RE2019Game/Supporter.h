/**
 * @file Supporter.h
 *
 * This task implements the behaviour for supporter robots (defender,
 * supporter, forward).
 * 
 * It has been adapted from the RoboEireann 2019 behaviour (hattrick project) 
 * to fit the 2021 code release framework (rematch project) and coro behaviour 
 * engine.
 *
 * @author Rudi Villing
 */


#pragma once

#include "Modules/BehaviorControl/CoroBehaviour/2022/CoroBehaviour2022.h"


#include "Modules/BehaviorControl/CoroBehaviour/Skills/CommonSkills.h"
#include "Modules/BehaviorControl/CoroBehaviour/Skills/HeadSkills.h"



namespace CoroBehaviour
{
namespace RE2019
{
  CRBEHAVIOUR(GotoTacticPoseTask)
  {
    CRBEHAVIOUR_INIT(GotoTacticPoseTask) {}

    void operator()(void)
    {
      CRBEHAVIOUR_LOOP()
      {
        CR_CHECKPOINT(walk_to_tactic_pose);
        while (!walkToPoseAutoAvoidanceTask.isSuccess())
        {
          commonSkills.activityStatus(BehaviorStatus::reWalkToTacticPose);
          doHeadSkill();
          walkToPoseAutoAvoidanceTask(getTacticPose(), Pose2f(params.walkSpeed, params.walkSpeed, params.walkSpeed),
                                      /* keepTargetRotation */ false, params.distanceThreshold, params.angleThreshold);
          CR_YIELD();
        }

        CR_CHECKPOINT(stand_at_tactic_pose);
        while (!outOfPosition())
        {
          commonSkills.activityStatus(BehaviorStatus::reStandAtTacticPose);
          doHeadSkill();
          commonSkills.stand();
          CR_YIELD();
        }

      }
    }

  private:
    DEFINES_PARAMS(GotoTacticPoseTask,
    {,
      (float)(1.f) walkSpeed,
      (float)(1500.f) lookAtBallDistance, //mm
      (float)(50.f) distanceThreshold, // mm
      (float)(5_deg) angleThreshold,
      // (unsigned)(2000) minOutOfPositionDurationMs,
      (float)(100.f) outOfPositionDistance, // mm
      (float)(10_deg) outOfPositionAngle,
      (float)(1000.f) zoneHysteresis, // we need to be this far into next zone before making a change
      (float)(500.f) ballHysteresis,
    });

    // ENUM(Side,
    // {,
    //   LEFT, 
    //   RIGHT,
    // });

    ENUM(Zone,
    {,
      DEFENSE, 
      MIDDLE,
      OFFENSE,
    });

    ENUM(BallMovement,
    {,
      FORWARD, // towards the opponent goal 
      BACKWARD, // towards our goal
    });

    READS(RobotPose);
    READS(GameInfo);
    READS(TeamBehaviorStatus);
    READS(FieldBall);
    READS(FieldDimensions);
    READS(RobotInfo);
    
    CommonSkills commonSkills {env};
    HeadSkills headSkills {env};
    ObstacleSkills obstacleSkills {env};
    WalkToPoseAutoAvoidanceTask walkToPoseAutoAvoidanceTask {env};

    // Side activeSide = LEFT;
    Zone activeZone = MIDDLE;
    // BallMovement ballMovement = FORWARD;
    // float ballX = 0.f;

    // float defenseZoneEnd;
    // float offenseZoneStart;

    bool outOfPosition()
    {
      return !commonSkills.isPoseClose(getTacticPose(), params.outOfPositionDistance, params.outOfPositionAngle);
    }

    void doHeadSkill()
    {
      if (theFieldBall.positionRelative.squaredNorm() < sqr(params.lookAtBallDistance))
        headSkills.lookAtBall(/* mirrored */ false, /* forceOwnEstimate */ true);
      else
        headSkills.lookActive();
    }

    Pose2f getTacticPose()
    {
      // TODO: what are the tactic positions during kickoff

      // What direction is the ball moving (with some hysteresis to allow for duelling robots)
      // if (ballMovement == FORWARD) // was it previously going forwards?
      // {
      //   if (theFieldBall.endPositionOnField.x() > ballX)
      //     ballX = theFieldBall.endPositionOnField.x();
      //   else if (theFieldBall.endPositionOnField.x() < (ballX - params.ballHysteresis))
      //   {
      //     ballMovement = BACKWARD;
      //     ballX = theFieldBall.endPositionOnField.x();
      //   }
      // }
      // else // no, it was going backwards
      // {
      //   if (theFieldBall.endPositionOnField.x() < ballX)
      //     ballX = theFieldBall.endPositionOnField.x();
      //   else if (theFieldBall.endPositionOnField.x() > (ballX + params.ballHysteresis))
      //   {
      //     ballMovement = FORWARD;
      //     ballX = theFieldBall.endPositionOnField.x();
      //   }
      // }

      // // divide the field into 3 zones
      // defenseZoneEnd = theFieldDimensions.xPosOwnGroundLine / 3;
      // offenseZoneStart = theFieldDimensions.xPosOpponentGroundLine / 3;

      // if ((activeZone == DEFENSE) &&
      //     (theFieldBall.endPositionOnField.x() > (defenseZoneEnd + params.zoneHysteresis))) // change needed?
      //   activeZone = (theFieldBall.positionOnField.x() > offenseZoneStart) ? OFFENSE : MIDDLE;
      // else if ((activeZone == OFFENSE) &&
      //          (theFieldBall.endPositionOnField.x() < (offenseZoneStart - params.zoneHysteresis))) // change needed?
      //   activeZone = (theFieldBall.endPositionOnField.x() < defenseZoneEnd) ? DEFENSE : MIDDLE;
      // else if ((activeZone == MIDDLE) &&
      //          ((theFieldBall.endPositionOnField.x() < (defenseZoneEnd - params.zoneHysteresis)) ||
      //           (theFieldBall.endPositionOnField.x() > (offenseZoneStart + params.zoneHysteresis)))) // change needed?
      //   activeZone = (theFieldBall.endPositionOnField.x() < defenseZoneEnd) ? DEFENSE : OFFENSE;;


      // now choose the tactic pose based on the zone and the supporter number
      // Furthest back supporter is 0, farthest forward is 2 (if all 3 supporters active)
      int index = theTeamBehaviorStatus.role.supporterIndex();
      Pose2f tacticPose;

      if (index == 0)
        tacticPose = defenderPose(activeZone);
      else if ((index == 1) || (index == 2))
        tacticPose = wingerPose(activeZone);
      else
        tacticPose = Pose2f(0,0,0); // don't move

      // if (activeZone == OFFENSE)
      // {
      //   if (index == 0)
      //   else if (index == 1)
      //     return theRobotPose.toRobotCoordinates(
      //         Pose2f(0, theFieldDimensions.xPosOpponentGroundLine / 6,
      //                theFieldBall.endPositionOnField / 2)); // midfielder towards ball side of field
      //   else if (index == 2)
      //   {
      //     int side = theRobotPose.translation.y() > 0 ? 1 : -1;
      //     float x = Rangef(offenseZoneStart, theFieldDimensions.xPosOpponentGroundLine - 1000.f).limit(theFieldBall.endPositionOnField);
      //     float y = side * theFieldDimensions.yPosLeftGoalArea;

      //     return theRobotPose.toRobotCoordinates(
      //         Pose2f(0, x, y)); // forward in line with goal edge of goal area with x based on ball position
      //   }
      // }
      // else if (activeZone == MIDDLE)
      // {
      //   if (index == 0)
      //     return theRobotPose.toRobotCoordinates(Pose2f(0, defenseZoneEnd, 500.f)); // defender drops back a bit
      //   else if (index == 1)
      //     return theRobotPose.toRobotCoordinates(
      //         Pose2f(0, theFieldDimensions.xPosOpponentGroundLine / 6,
      //                theFieldBall.endPositionOnField / 2)); // midfielder towards ball side of field
      //   else if (index == 2)
      //   {
      //     int side = theRobotPose.translation.y() > 0 ? 1 : -1;
      //     float x = Rangef(offenseZoneStart, theFieldDimensions.xPosOpponentGroundLine - 1000.f).limit(theFieldBall.endPositionOnField);
      //     float y = side * theFieldDimensions.yPosLeftGoalArea;

      //     return theRobotPose.toRobotCoordinates(
      //         Pose2f(0, x, y)); // forward in line with goal edge of goal area with x based on ball position
      //   }
      // }
      // else // DEFENSE
      // {

      // }

      addActivationGraphOutput(fmt::format("tacticPose = {{{}, {}, {}}}", tacticPose.rotation, tacticPose.translation.x(), tacticPose.translation.y()));

      return tacticPose;
    }

    Pose2f defenderPose(Zone zone)
    {
      (void)zone;

      // if (zone == OFFENSE)
      //   return theRobotPose.toRobotCoordinates(Pose2f(0, theFieldDimensions.xPosOwnGroundLine * 0.4f, 500.f)); // defender pushes up
      // else // ((zone == MIDDLE) || (zone == DEFENSE))
      {
        const float goalAreaDepth = (theFieldDimensions.xPosOwnGoalArea - theFieldDimensions.xPosOwnGroundLine);
        Rangef ballXRange = Rangef(theFieldDimensions.xPosOwnGroundLine + goalAreaDepth * 1.5f, 1000.f);
        float ballXClipped = ballXRange.limit(theFieldBall.endPositionOnField.x());
        Rangef defenderXRange = Rangef(theFieldDimensions.xPosOwnGroundLine + goalAreaDepth * 0.8f, theFieldDimensions.xPosOwnGroundLine * 0.6f);

        // set up the defender based on the ball position, but putting more pressure on the ball the closer to goal it gets
        float x = defenderXRange.scale(ballXClipped, ballXRange);

        // the y position is on an intercept line between the ball and the near side of the goal
        bool left = theFieldBall.endPositionOnField.y() > 0;

        float yDist = left ? (theFieldBall.endPositionOnField.y() - (theFieldDimensions.yPosLeftGoal - 300.f))
                           : (theFieldBall.endPositionOnField.y() + (theFieldDimensions.yPosLeftGoal - 300.f));

        float scale = (x - theFieldDimensions.xPosOwnGroundLine) / (theFieldBall.endPositionOnField.x() - theFieldDimensions.xPosOwnGroundLine);
        float y = yDist * scale + (left ? (theFieldDimensions.yPosLeftGoal - 300.f) : -(theFieldDimensions.yPosLeftGoal - 300.f));

        // FIXME - head is woolly today and this seems muddled up
        // Vector2f tacticPosOnField = Vector2f(x,y);
        Pose2f tacticPoseOnField = Pose2f(theRobotPose.rotation, x, y);
        Vector2f ballRelativeTactic = tacticPoseOnField.inverse() * theFieldBall.endPositionOnField;
        // angle towards ball (at destination pose)
        // Angle ballAngle = (theFieldBall.endPositionOnField - tacticPosOnField).angle();

        // return Pose2f(ballAngle, theRobotPose.toRobotCoordinates(tacticPosOnField));

        return theRobotPose.toRobotCoordinates(tacticPoseOnField.rotate(ballRelativeTactic.angle()));
      }
    }

    Pose2f wingerPose(Zone zone)
    {
      (void)zone;

      bool left;
      if (theTeamBehaviorStatus.role.numOfActiveSupporters == 3)
        left = (theRobotInfo.number == 3);
      else
        left = (theRobotPose.translation.y() > 0);

      // setting the wingers up defensively, we'll leave about 2.5 m between them.
      // as the ball moves right or left they'll move a little but not too much.
      // we're happy to leave room along wings for the opposition.
      // we won't go further forward than half way.
      // we try to stay (our) goal side of the ball.

      Rangef ballXRange = Rangef(theFieldDimensions.xPosOwnGroundLine / 2, 1000.f);
      float ballXClipped = ballXRange.limit(theFieldBall.endPositionOnField.x());
      Rangef wingerXRange = Rangef(theFieldDimensions.xPosOwnPenaltyArea, theFieldDimensions.xPosOwnGroundLine * 0.4f);

      // choose x about 25% of field length back from ball (limited to range)
      float x = wingerXRange.scale(ballXClipped, ballXRange);

      if (theTeamBehaviorStatus.role.supporterIndex() == 2)
        // x += 200.f;
        x += 700.f;
      else
        // x -= 200.f;
        x -= 50.f;

      // set wingers a distance apart 
      // (should check obstacles again)
      Rangef ballYRange(-theFieldDimensions.yPosLeftSideline, theFieldDimensions.yPosLeftSideline);
      float ballY = ballYRange.limit(theFieldBall.endPositionOnField.y());

      // make the wingers even more compact as they are pushed back
      float maxY = Rangef(theFieldDimensions.yPosLeftGoal + 300.f, theFieldDimensions.yPosLeftGoal - 100.f).scale(ballY, ballYRange);

      Rangef wingerYRange = left ? Rangef(200.f, maxY) : Rangef(-maxY, -200.f);
      float y = wingerYRange.scale(ballY, ballYRange);

      // if there's a teammate near this spot, we should try the other side
      // if (obstacleSkills.areTeammatesNearPointOnField(Vector2f(x,y)))
      // {
      //   left = !left;
      //   wingerYRange = left ? Rangef(200.f, maxY) : Rangef(-maxY, -200.f);
      //   y = wingerYRange.scale(ballY, ballYRange);
      // }

      // FIXME - head is woolly today and this seems muddled up
      // Vector2f tacticPosOnField = Vector2f(x,y);
      Pose2f tacticPoseOnField = Pose2f(theRobotPose.rotation, x, y);
      Vector2f ballRelativeTactic = tacticPoseOnField.inverse() * theFieldBall.endPositionOnField;

      return theRobotPose.toRobotCoordinates(tacticPoseOnField.rotate(ballRelativeTactic.angle()));
    }
  };



  /**
   * This is the entry point task for the supporter playing state.
   */
  CRBEHAVIOUR(SupporterPlayingTask)
  {
    CRBEHAVIOUR_INIT(SupporterPlayingTask) {}

    void operator()(void)
    {
      CRBEHAVIOUR_LOOP()
      {
        // if the entire team has lost the ball for a while we need to do
        // some coordinated search
        // if (teamBallLost())
        //   coordinatedSearchForBallTask();
        // else  // otherwise go to tactic pose

        if (needsClearanceKick())
        {
          while (needsClearanceKick())
          {
            commonSkills.activityStatus(BehaviorStatus::reKickBestOption);
            gotoBallAndKickBestOptionTask();
            CR_YIELD();
          }
        }
        else
        {
          gotoTacticPoseTask();
          CR_YIELD();
        }
      }
    }

  private:
    DEFINES_PARAMS(SupporterPlayingTask,
    {,
      (unsigned)(3000) kickAwayBallSeenTimeoutMs, ///< we'll allow the ball to be obstructed briefly on the way to kicking it away
      (float)(1500.f) kickAwayDistance, ///< we'll allow the ball to be obstructed briefly on the way to kicking it away
    });

    READS(GameInfo);
    READS(TeamBehaviorStatus);
    READS(FieldBall);
    
    CommonSkills commonSkills {env};
    GotoTacticPoseTask gotoTacticPoseTask {env};
    GotoBallAndKickBestOptionTask gotoBallAndKickBestOptionTask {env};
    // SupporterClearanceTask supporterClearanceTask {env};


    /**
     * is the ball situation such that our best bet is to walk out to the ball and
     * kick it away?
     */
    bool needsClearanceKick()
    {
      // TODO: the current implementation is pretty naive and just looks at the
      // ball position. It would be better to consider things like, where
      // are the opponent players, where are our defensive players, before
      // committing to go 1v1

      // NOTE: that we also assume that if the goalie is the closest robot
      // to the ball it will role switch to become the ball player, in which
      // case that behaviour will run (and kick the ball) rather than this one

      bool ballSeen = theFieldBall.ballWasSeen(params.kickAwayBallSeenTimeoutMs);

      bool ballInRange = (theFieldBall.endPositionOnField.norm() < params.kickAwayDistance); // FIXME: dirty hack

      // TODO: check for striker

      return  ballSeen && ballInRange;
    }


    // bool teamBallLost()
    // {
    //   return false;
    // }
  };

} // RE2019
} // CoroBehaviour2022
