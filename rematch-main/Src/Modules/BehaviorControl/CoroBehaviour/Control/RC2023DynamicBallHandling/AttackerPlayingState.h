/**
 * @file PlayingState.h
 *
 * This task implements the top level playing state behaviour for all 
 * attacker robots in the dynamic ball handling challenge.
 *
 * @author Rudi Villing
 * @author Danny Ryan
 * @author Andy Lee Mitchell
 */


#pragma once

#include "Modules/BehaviorControl/CoroBehaviour/2023/CoroBehaviour2023.h"

#include "Modules/BehaviorControl/CoroBehaviour/Skills/CommonSkills.h"
#include "Modules/BehaviorControl/CoroBehaviour/Skills/HeadSkills.h"
#include "Modules/BehaviorControl/CoroBehaviour/Skills/ObstacleSkills.h"
#include "Modules/BehaviorControl/CoroBehaviour/Control/RC2023DynamicBallHandling/InterceptSkills.h"
#include "Modules/BehaviorControl/CoroBehaviour/Control/RC2023DynamicBallHandling/BallAndKickSkills.h"

#include "Tools/TextLogging.h"


namespace CoroBehaviour
{
namespace RC2023
{
  // ------------------------------------------------------------------------
  // Supporters/Wingers behaviour
  // ------------------------------------------------------------------------

  /**
  * Currently, the wingers check to see the position of defender 2 (the one on the edge of the opposition box).
  * If the defender is on the same side of the pitch as the winger, in relation to the y-axis, the winger will stay on the halfway line 
  * and wait for a pass from robot1, otherwise the winger will go forward into the opposition half to wait to receive the second pass
  * 
  * NOTE: we have to write this behaviour with the possibility that we might switch out of it to become
  * ball player and then switch back again, so decisions about which pass to execute next cannot be
  * based on simply going through the checkpoints in order
  */

  CRBEHAVIOUR(AttackingSupporterPlayingTask)
  {
    CRBEHAVIOUR_INIT(AttackingSupporterPlayingTask) {}

    void operator()(void)
    {
      CRBEHAVIOUR_BEGIN();

      // CR_CHECKPOINT(look_for_defender);
      // while (getCheckpointDuration() < params.lookForDefenderMs)
      // {
      //   headSkills.lookActive(/* withBall: */ false, /* ignoreBall: */ true);
      //   walkToPoseNoAvoidanceTask(getLookForDefenderPose(), Pose2f(params.fast, params.fast, params.fast),
      //                             /* keepTargetRotation */ false, params.positionThreshold, params.angleThreshold);
      //   CR_YIELD();
      // }

      // First pose
      // NOTE: we assume that going to the initial supporter pose will also allow
      // the supporter(s) to see where the defender is located

      if (theExtendedGameInfo.timeSincePlayingStarted < static_cast<int>(params.walkToStartingPoseMs))
      {
        CR_CHECKPOINT(walk_to_starting_pose);
        setStartingPose(); // set it once and don't adjust it any more
        while ((getCheckpointDuration() < params.walkToStartingPoseMs) && !walkToPoseAutoAvoidanceTask.isSuccess())
        {
          headSkills.lookActive();
          walkToPoseAutoAvoidanceTask(theRobotPose.toRobotCoordinates(supporterPose),
                                    Pose2f(params.fast, params.fast, params.fast),
                                    /* keepTargetRotation */ true, params.positionThreshold, params.angleThreshold);
          CR_YIELD();
        }

        CR_CHECKPOINT(wait_for_first_pass);
        while ((theTeamBehaviorStatus.passStatus.passReceiver == -1) ||
               (theTeamBehaviorStatus.passStatus.passReceiver == theRobotInfo.number) ||
               (theFieldBall.timeSinceTeamBallWasValid > 1000))
        {
          if (interceptSkills.isInterceptNeeded())
            interceptTask(InterceptTask::WALK_INTERCEPTS);
          else
          {
            headSkills.lookActive();          
            commonSkills.stand();
          }
          CR_YIELD();
        }
      }


      CR_CHECKPOINT(goto_next_pass_position);
      setNextReceivePose(); // set it once and don't adjust it any more
      walkToPoseAutoAvoidanceTask.reset(); // ensure that we don't just consider walk to first pose
      
      while (!walkToPoseAutoAvoidanceTask.isSuccess())
      {
        if (interceptSkills.isInterceptNeeded())
          interceptTask(InterceptTask::WALK_INTERCEPTS);
        else
        {
          headSkills.lookActive();
          walkToPoseAutoAvoidanceTask(theRobotPose.toRobotCoordinates(supporterPose),
                                    Pose2f(params.fast, params.fast, params.fast),
                                    /* keepTargetRotation */ true, params.positionThreshold, params.angleThreshold);
        }
        CR_YIELD();
      }
      
      // wait until higher level behaviour switches out of this coroutine,
      // e.g. by changing the robot role to ball player

      CR_CHECKPOINT(wait_for_ball);
      // While ball not seen, look straight ahead
      //while (!theFieldBall.ballWasSeen(params.ballSeenTimeoutMs))
      while (true)
      {
        if (interceptSkills.isInterceptNeeded())
          interceptTask(InterceptTask::WALK_INTERCEPTS);
        else
        {
          headSkills.lookActive();          
          commonSkills.stand();
        }
        CR_YIELD();
      }
    }

  private:
    DEFINES_PARAMS(AttackingSupporterPlayingTask,
    {,
      (unsigned)(5000) lookForDefenderMs,
      (unsigned)(7000) walkToStartingPoseMs,
      (float)(50.f) positionThreshold, 
      (float)(5_deg) angleThreshold,
      (float)(1.f) fast, 
    });
    
    READS(FrameInfo);
    READS(TeamBehaviorStatus);
    READS(RobotPose);
    READS(FieldDimensions);
    READS(FieldBall);
    READS(RobotInfo);
    READS(ExtendedGameInfo);
    
    CommonSkills commonSkills {env};
    MotionSkills motionSkills {env};
    HeadSkills headSkills {env};
    ObstacleSkills obstacleSkills {env};
    InterceptSkills interceptSkills {env};
    InterceptTask interceptTask {env};

    WalkToPoseAutoAvoidanceTask walkToPoseAutoAvoidanceTask {env};
    WalkToPoseNoAvoidanceTask walkToPoseNoAvoidanceTask {env};
    TurnToPointOdometryTask turnToPointOdometryTask {env};

    Vector2f shotTargetOnField;
    Pose2f supporterPose;


    // turn in slightly to scan across the penalty area
    // Pose2f getLookForDefenderPose()
    // {
    //   Vector2f target = theRobotPose.toRobotCoordinates(Vector2f(theFieldDimensions.xPosOpponentGroundLine, 0.f)); // centre of opponent goal

    //   return Pose2f(target.angle()); // rotate on the spot to facfe the target
    // }

    void setStartingPose()
    {
      float side = (theRobotPose.translation.y() > 0) ? 1.f : -1.f;

      float yOffset = (0.5f * (theFieldDimensions.yPosLeftSideline + theFieldDimensions.centerCircleRadius)) + 100.f; // extra room for ball inaccuracy

      supporterPose = Pose2f(-110_deg * side, 250.f, yOffset * side);
    }

    void setNextReceivePose()
    {
      float side = (theRobotPose.translation.y() > 0) ? 1.f : -1.f;

      // float xOffset1 = 0.6f * theFieldDimensions.xPosOpponentPenaltyMark;
      // float xOffset2 = 1.0f * theFieldDimensions.xPosOpponentPenaltyMark;
      float xOffset3 = 0.3f * theFieldDimensions.xPosOpponentPenaltyMark;

      supporterPose = Pose2f(-90_deg * side, xOffset3, 1.6f * theFieldDimensions.yPosLeftGoalArea * side);
      // // if the robot is close to the half way line, move to forward receive pose
      // if (theRobotPose.translation.x() < (xOffset1 * 0.8))
      //   supporterPose = Pose2f(-90_deg * side, xOffset1,
      //                         1.6f * theFieldDimensions.yPosLeftGoalArea * side);
      // else // third pose if needed
      //   supporterPose = Pose2f(-90_deg * side, xOffset2,
      //                         0.f * theFieldDimensions.yPosLeftGoalArea * side);
    }

    // Pose2f getSupporterPose()
    // {
    //   if (theRobotPose.translation.y() < 0) 
    //   {
    //     // If defender 2 is on the same side wait for a pass on the halfway line otherwise go forward
    //     if (obstacleSkills.areOpponentsNearPointOnField(Vector2f(theFieldDimensions.xPosOpponentPenaltyArea, theFieldDimensions.yPosLeftGoalArea)))
    //     {
    //       supporterPose = (Pose2f(90_deg, theFieldDimensions.xPosHalfWayLine, 0.75f*(theFieldDimensions.yPosRightSideline)));
    //     }
    //     else
    //     {
    //       supporterPose = (Pose2f(60_deg, theFieldDimensions.xPosOpponentPenaltyArea, 0.35f*(theFieldDimensions.yPosRightSideline)));
    //     }
    //   }
    //   else 
    //   {
    //     if (obstacleSkills.areOpponentsNearPointOnField(Vector2f(theFieldDimensions.xPosOpponentPenaltyArea, theFieldDimensions.yPosRightGoalArea)))
    //     {
    //       supporterPose = (Pose2f(270_deg, theFieldDimensions.xPosHalfWayLine, 0.75f*(theFieldDimensions.yPosLeftSideline)));
    //     }
    //     else
    //     {
    //       supporterPose = (Pose2f(210_deg, theFieldDimensions.xPosOpponentPenaltyArea, 0.35f*(theFieldDimensions.yPosLeftSideline)));
    //     }
    //   }

    //   return theRobotPose.toRobotCoordinates(supporterPose);
    // }
  };

  // ------------------------------------------------------------------------
  // Ball player behaviour
  // ------------------------------------------------------------------------

  /**
  * Robot 1 should look at the position of defender 2 (the one on the edge of the opposition box).
  * It should pass to the same side of the pitch that the defender 2, in relation to it's y-axis,
  * in order to give more space for the second supporter to have more space for a successful shot. 
  */

  CRBEHAVIOUR(AttackingBallPlayerPlayingTask)
  {
    CRBEHAVIOUR_INIT(AttackingBallPlayerPlayingTask) {}

    void operator()(int prevPassKicker, int prevPassReceiver)
    {
      // commonSkills.activityStatus(BehaviorStatus::unknown);

      CRBEHAVIOUR_LOOP()
      {
        addActivationGraphOutput(fmt::format("prevPassKicker {}, receiver {}", prevPassKicker, prevPassReceiver));
      

        if (theRobotInfo.number == 1)
        {
          // Robot 1 make the first pass to either winger
          CR_CHECKPOINT(pass_to_winger);

          // choose initial pass target and don't change once decided
          chooseFirstPassTarget();

          while (!gotoBallAndPassTask.isSuccess())
          {
            commonSkills.passTargetStatus(passTargetPlayer, passTargetOnField);
            gotoBallAndPassTask(theRobotPose.toRobotCoordinates(passTargetOnField));
            CR_YIELD();
          }
        }


        // while the ball is in the opposition half and we're the robot furthest forward, 
        // shoot at goal
        // While we're not the furthest forward robot, make the second pass

        // FIXME: I suspect this is fragile if the ball travels the wrong distance in the pass
        // if ((theRobotPose.translation.x() < 0.45f * theFieldDimensions.xPosOpponentPenaltyArea) &&
        //     (theRobotPose.translation.x() > -theFieldDimensions.centerCircleRadius))
        (void)prevPassReceiver; // force variable use
        if (prevPassKicker == 1) // if it kicked from robot1, we're clear to make the second pass
        {
          // CR_CHECKPOINT(wait_for_defender);
          // while (!obstacleSkills.nearestOpponentDistance(theRobotInfo.number)) 
          // //while (!obstacleSkills.isOpponentDangerNearBall())
          // {
          //   headSkills.lookActive();
          //   commonSkills.stand();
          //   CR_YIELD();
          // }

          //CR_CHECKPOINT(dribble_down_the_wing);


          CR_CHECKPOINT(make_second_pass);
          // chooseSecondPassTarget();

          while (!gotoBallAndPassTask.isSuccess())              
          {
            chooseAdaptivePassTarget();

            // gotoBallAndKickTask(theRobotPose
            //                         .toRobotCoordinates(Vector2f(0.9f * theFieldDimensions.xPosOpponentPenaltyArea,
            //                                                     side * 0.5f * (theFieldDimensions.yPosRightSideline)))
            //                         .angle(),
            //                     KickInfo::forwardFastRight,
            //                     /* alignPrecisely */ true, /* length */ 4500.f, /* preStepAllowed */ true,
            //                     /* turnKickAllowed */ false, Pose2f(params.speed1, params.speed1, params.speed1));
            commonSkills.passTargetStatus(passTargetPlayer, passTargetOnField);
            gotoBallAndPassTask(theRobotPose.toRobotCoordinates(passTargetOnField));
            CR_YIELD();
          }
        }

        CR_CHECKPOINT(optional_third_pass);
        checkOpponentPos();

        // while(!gotoBallAndPassTask.isSuccess())
        while(opponentInWay)
        {
            checkOpponentPos();
            if (!opponentInWay)
            {
              // need these to avoid meeks
              headSkills.lookActive();
              commonSkills.stand();
              CR_YIELD();
            }

            chooseAdaptivePassTarget();
            commonSkills.passTargetStatus(passTargetPlayer, passTargetOnField);
            gotoBallAndPassTask(theRobotPose.toRobotCoordinates(passTargetOnField));
            // goToBallAndPassToTeammateTask(passTargetPlayer);
            CR_YIELD();
        }

        // if we get this far, we're clear to shoot

        // while the ball is in the opposition half and we're the robot furthest forward, 
        // shoot at goal
        (void)prevPassReceiver; // force variable use
        if (prevPassKicker == (theRobotInfo.number == 2 ? 3 : 2)) // if it kicked from the other attacking robot we are clear to shoot
        {
          CR_CHECKPOINT(shoot_at_goal);
          // while ((theFieldBall.recentBallEndPositionOnField().x() > 0.45f*theFieldDimensions.xPosOpponentPenaltyArea))
          while (!theFieldDimensions.isInsideOpponentGoal(theFieldBall.endPositionOnField) && !opponentInWay)
          {
            checkOpponentPos();
            // commonSkills.activityStatus(BehaviorStatus::reKickAtGoal);
            // gotoBallAndKickBestOptionTask();
            gotoBallAndShootTask();
            CR_YIELD();
          }
        }

        // wait until higher level behaviour switches out of this coroutine

        CR_CHECKPOINT(wait_after_kick);
        while ((getCheckpointDuration() < 1000) || (theFieldDimensions.isInsideOpponentGoal(theFieldBall.endPositionOnField)))
        {
          headSkills.lookActive();
          commonSkills.stand();
          CR_YIELD();
        }
      }

    }

  private:
    DEFINES_PARAMS(AttackingBallPlayerPlayingTask, 
    {, 
      (float)(1.f) speed1, 
    });

    READS(RobotPose);
    READS(FieldBall);
    READS(FieldDimensions);
    READS(RobotInfo);
    READS(TeamData);
    
    CommonSkills commonSkills {env};
    HeadSkills headSkills{env};
    ObstacleSkills obstacleSkills{env};
    GotoBallAndPassTask gotoBallAndPassTask{env};
    GotoBallAndShootTask gotoBallAndShootTask{env};
    // GoToBallAndPassToTeammateTask goToBallAndPassToTeammateTask{env};
    // GotoBallAndKickTask gotoBallAndKickTask{env};
    

    int passTargetPlayer; // the player number we want to pass to
    Vector2f passTargetOnField;

    Vector2f shotTargetOnField;
    // KickInfo::KickType shotKickType;
    // float side;
    bool opponentInWay;


    void chooseFirstPassTarget()
    {
      // CAUTION: match with setStartingPose above
      //if (obstacleSkills.nearestOpponentDistance(2) < obstacleSkills.nearestOpponentDistance(3))
      float yOffset = (0.5f * (theFieldDimensions.yPosLeftSideline + theFieldDimensions.centerCircleRadius));
      
      // passTargetPlayer = 2;
      // passTargetOnField = Vector2f(250.f, yOffset - 150.f); // offset to place the ball in front of the player

      if (Random::bernoulli())
      {
        passTargetPlayer = 2;
        passTargetOnField = Vector2f(250.f, yOffset - 150.f); // offset to place the ball in front of the player
      }
      else
      {
        passTargetPlayer = 3;
        passTargetOnField = Vector2f(250.f, -(yOffset - 150.f));
      }
    }


    void chooseSecondPassTarget()
    {
      // CAUTION: match with setNextReceivePose
      float side = (theRobotPose.translation.y() > 0) ? 1.f : -1.f;

      float xOffset1 = 0.6f * theFieldDimensions.xPosOpponentPenaltyMark;
      float yOffset1 = 1.6f * theFieldDimensions.yPosLeftGoalArea;
      // if the robot is close to the half way line, move to forward receive pose


      passTargetPlayer = (theRobotInfo.number == 2) ? 3 : 2;
      passTargetOnField = Vector2f(xOffset1, (yOffset1 -250.f) * -side);
    }

    void chooseAdaptivePassTarget()
    {
      passTargetPlayer = (theRobotInfo.number == 2) ? 3 : 2;

      // get position of pass target

      for (const Teammate& teammate : theTeamData.teammates)
      {
        if (teammate.number == passTargetPlayer)
        {
          passTargetOnField = teammate.theRobotPose.translation;
          passTargetOnField.x() += 300.f; // pass ahead of the player
          return;
        }
      }

      chooseSecondPassTarget(); // fall back to hardcoded version
    }

    // void chooseShot()
    // {
    //   //float side = Random::bernoulli() ? 1 : -1;
    //   side = 1;
    //   // if (obstacleSkills.areOpponentsNearPointOnField(Vector2f(2850.0f, -1000.0f)))
    //   //   side = -1;

    //   // else
    //   //   side = 1;

    //   // if shooting left, kick with the left, otherwise kick with the right
    //   shotKickType = (side == -1) ? KickInfo::forwardFastLeft : KickInfo::forwardFastRight;
    //   shotTargetOnField = Vector2f(theFieldDimensions.xPosHalfWayLine, side*0.7f*(theFieldDimensions.yPosLeftSideline));
    // }

    void checkOpponentPos()
    {
      opponentInWay = obstacleSkills.isOpponentDangerNearBall(1500.f);
      // opponentInWay = obstacleSkills.areOpponentsNearPointOnField(Vector2f(theRobotPose.translation.x(),
      // theFieldDimensions.yPosRightGoalArea));

      // HACK - shoot if we're far enough up the pitch that making another pass wouldn't be worth it
      if (opponentInWay && (theRobotPose.translation.x() > (theFieldDimensions.xPosOpponentGoalArea - 500.f)))
        opponentInWay = false;

      if (opponentInWay)
      {
        const std::list<SectorWheel::Sector> &kickSectors =
            obstacleSkills.populateKickSectors(theRobotPose.translation);

        // std::list<SectorWheel::Sector> goalSectors = obstacleSkills.getGoalSectors(kickSectors, 5_deg);

        // if (goalSectors.size() < 2) // FIXME this should be a param
        //   opponentInWay = true;

        Rangea testRange = Rangea(-70_deg, 70_deg);
        std::tuple<Rangea, bool> bestSector = obstacleSkills.getBestFreeSector(kickSectors, testRange);

        if (std::get<1>(bestSector))
          opponentInWay = false;
      }
    }
  };

  /**
   * This is the entry point task for the playing state. It decides which
   * specialized behaviour to run
   */
  CRBEHAVIOUR(AttackingGoaliePlayingTask)
  {
    CRBEHAVIOUR_INIT(AttackingGoaliePlayingTask) {}

    void operator()(void)
    {
      CRBEHAVIOUR_LOOP()
      {
        headSkills.lookActive();
        commonSkills.stand();
        CR_YIELD();
      }
    }

  private:
    CommonSkills commonSkills {env};
    HeadSkills headSkills{env};
  };

  /**
   * This is the entry point task for the playing state. It decides which
   * specialized behaviour to run
   */
  CRBEHAVIOUR(AttackerPlayingStateTask)
  {
    CRBEHAVIOUR_INIT(AttackerPlayingStateTask) {}

    void operator()(void)
    {
      CRBEHAVIOUR_LOOP()
      {
        if (theTeamBehaviorStatus.role.playsTheBall())
          attackingBallPlayerPlayingTask(prevPassKicker, prevPassReceiver);
        else if (theTeamBehaviorStatus.role.isGoalkeeper())
          attackingGoaliePlayingTask();
        else
          attackingSupporterPlayingTask();

        if ((theTeamBehaviorStatus.passStatus.passReceiver != prevPassReceiver) && (theTeamBehaviorStatus.passStatus.passReceiver != -1))
        {
          prevPassKicker = theTeamBehaviorStatus.passStatus.passKicker;
          prevPassReceiver = theTeamBehaviorStatus.passStatus.passReceiver;
        }

        CR_YIELD();
      }
    }

  private:
    READS(TeamBehaviorStatus);

    AttackingBallPlayerPlayingTask attackingBallPlayerPlayingTask {env};
    AttackingGoaliePlayingTask attackingGoaliePlayingTask {env};
    AttackingSupporterPlayingTask attackingSupporterPlayingTask {env};

    int prevPassKicker = -1;
    int prevPassReceiver = -1;
  };
} // RC2023
} // CoroBehaviour2023
