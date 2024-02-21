/**
 * @file SetPlay.h
 *
 * This file implements the main set plays, i.e. kick-in, free kick, goal
 * kick etc.
 * 
 * Parts of it have been adapted from the RoboEireann 2019 behaviour (hattrick project) 
 * to fit the 2021 code release framework (rematch project) and coro behaviour 
 * engine.
 *
 * @author Rudi Villing
 */


#pragma once

#include "Modules/BehaviorControl/CoroBehaviour/2022/CoroBehaviour2022.h"

#include "Modules/BehaviorControl/CoroBehaviour/Skills/CommonSkills.h"
#include "Modules/BehaviorControl/CoroBehaviour/Skills/HeadSkills.h"

#include "Tools/Math/Pose2f.h"


namespace CoroBehaviour
{
namespace RE2019
{
  // TODO add code here. Take a look at the original code from 2019
  // (commented out below) for inspiration but note that it does not work
  // without change in the framework. Look at it more to get a general
  // idea of how to approach things.


  /**
   * implement free kick, kick-in, possibly corner kick
   * The general approach is find the ball, go to the ball, kick to a teammate
   * since you cannot score directly
   * 
   * Not sure if goal kick should be here also since you would imagine that
   * should be the goalie rather than the striker taking it
   */
  CRBEHAVIOUR(SetPlayStrikerFreeKickTask)
  {
    CRBEHAVIOUR_INIT(SetPlayStrikerFreeKickTask) {}

    void operator()(void)
    {
      CRBEHAVIOUR_LOOP()
      {
        CR_CHECKPOINT(gotoBallAndKick);
        while (theFieldBall.ballWasSeen(params.ballSeenTimeoutMs))
        {
          commonSkills.activityStatus(BehaviorStatus::reIndirectKick);
          gotoBallAndKickBestOptionTask(/* indirect */ true);
          CR_YIELD();
        }

        CR_CHECKPOINT(searchForBall);
        while (!strikerSearchForBallTask.isEnded())
        {
          commonSkills.activityStatus(BehaviorStatus::reSearchForBall);
          strikerSearchForBallTask(theFieldBall.ballWasSeen(params.ballLostRecentlyTimeoutMs));        
          CR_YIELD();
        }
      }
    }

  private:
    DEFINES_PARAMS(SetPlayStrikerFreeKickTask, 
    {, 
      (unsigned)(5000) ballSeenTimeoutMs, // if the ball is not seen for this time we consider it lost
      (unsigned)(6000) ballLostRecentlyTimeoutMs, // if the ball *was* seen within this period we consider the ball to be recently lost
    });

    READS(FieldBall);
    CommonSkills commonSkills {env};

    StrikerSearchForBallTask strikerSearchForBallTask {env};
    GotoBallAndKickBestOptionTask gotoBallAndKickBestOptionTask {env};
  };


  // ==========================================================================

  CRBEHAVIOUR(SetPlayStrikerCornerSearchTask)
  {
    CRBEHAVIOUR_INIT(SetPlayStrikerCornerSearchTask) {}

    void operator()(void)
    {
      CRBEHAVIOUR_BEGIN();

      chooseCorner(true);

      CR_CHECKPOINT(GotoFirstCorner);
      while (!walkToPoseAutoAvoidanceTask.isEnded())
      {
        commonSkills.lookActive();
        walkToPoseAutoAvoidanceTask(cornerPose, Pose2f(params.speed, params.speed, params.speed));
        CR_YIELD();
      }

      chooseCorner(false);

      CR_CHECKPOINT(GotoSecondCorner);
      while (true)
      {
        commonSkills.lookActive();
        walkToPoseAutoAvoidanceTask(cornerPose, Pose2f(params.speed, params.speed, params.speed));
        CR_YIELD();
      }
    }

  private:
    DEFINES_PARAMS(SetPlayStrikerCornerSearchTask, 
    {, 
      (unsigned)(2000) waitMs, // if the ball is not seen for this time we consider it lost
      (float)(1.0f) speed,
    });

    READS(FieldBall);
    READS(FieldDimensions);
    READS(RobotPose);

    CommonSkills commonSkills {env};
    WalkToPoseAutoAvoidanceTask walkToPoseAutoAvoidanceTask {env};


    Pose2f cornerPose;
    Vector2f corner;

    void chooseCorner(bool closestFirst = false)
    {
      Vector2f cornerLeft = Vector2f(theFieldDimensions.xPosOpponentGroundLine, theFieldDimensions.yPosLeftSideline);
      Vector2f cornerRight = Vector2f(theFieldDimensions.xPosOpponentGroundLine, theFieldDimensions.yPosRightSideline);

      if (closestFirst)
      {
        if ((cornerLeft - theRobotPose.translation).squaredNorm() < (cornerRight - theRobotPose.translation).squaredNorm())
          corner = cornerLeft;
        else
          corner = cornerRight;
      }
      else if (corner == cornerLeft)
        corner = cornerRight;
      else
        corner = cornerLeft;

      // we have picked a corner, so choose pose to look at it

      if (corner.y() > 0) // left?
        cornerPose = Pose2f(45_deg, corner.x() - 2000.f, corner.y() - 2000.f);
      else
        cornerPose = Pose2f(-45_deg, corner.x() - 2000.f, corner.y() + 2000.f);
    }
  };

  // ==========================================================================

  CRBEHAVIOUR(SetPlayStrikerCornerKickTask)
  {
    CRBEHAVIOUR_INIT(SetPlayStrikerCornerKickTask) {}

    void operator()(void)
    {
      CRBEHAVIOUR_LOOP()
      {
        CR_CHECKPOINT(gotoBallAndKick);
        while (theFieldBall.ballWasSeen(params.ballSeenTimeoutMs))
        {
          commonSkills.activityStatus(BehaviorStatus::reIndirectKick);
          gotoBallAndKickBestOptionTask(/* indirect */ true);
          CR_YIELD();
        }

        // if we get here, the ball was not seen

        CR_CHECKPOINT(searchForCornerBall);
        while (!theFieldBall.ballWasSeen())
        {
          commonSkills.activityStatus(BehaviorStatus::reSearchForBall);
          setPlayStrikerCornerSearchTask();
          CR_YIELD();
        }
      }
    }

  private:
    DEFINES_PARAMS(SetPlayStrikerCornerKickTask, 
    {, 
      (unsigned)(5000) ballSeenTimeoutMs, // if the ball is not seen for this time we consider it lost
      (unsigned)(6000) ballLostRecentlyTimeoutMs, // if the ball *was* seen within this period we consider the ball to be recently lost
    });

    READS(FieldBall);
    CommonSkills commonSkills {env};

    SetPlayStrikerCornerSearchTask setPlayStrikerCornerSearchTask {env};
    StrikerSearchForBallTask strikerSearchForBallTask {env};
    GotoBallAndKickBestOptionTask gotoBallAndKickBestOptionTask {env};
  };


  // ==========================================================================



  /**
   * implement free kick, kick-in, possibly corner kick for supporters
   * The general approach is go to a decent position (at least one robot in
   * a position to receive the pass)
   */
  CRBEHAVIOUR(SetPlaySupporterPlayingTask)
  {
    CRBEHAVIOUR_INIT(SetPlaySupporterPlayingTask) {}

    void operator()(void)
    {
      commonSkills.activityStatus(BehaviorStatus::reSetPlayBackoffAndStand);

      CRBEHAVIOUR_LOOP()
      {
        // do we need to back up
        if (theFieldBall.positionRelative.norm() < params.exclusionRadius)
        {
          while (theFieldBall.positionRelative.norm() >= params.exclusionRadius)
          {
            commonSkills.lookActive();
            // FIXME - this does nothing to avoid walking into things or to choose a sensible position
            motionSkills.walkAtRelativeSpeed(Pose2f(0, -0.7f, 0));
          }
        }
        // TODO - doing nothing useful yet
        commonSkills.standLookActive();
        CR_YIELD(); 
      }
    }

  private:
    DEFINES_PARAMS(SetPlaySupporterPlayingTask,
    {,
      (float)(750.f)  exclusionRadius, ///< from the rules
    });

    READS(FieldBall);

    CommonSkills commonSkills {env};
    MotionSkills motionSkills {env};
  };


} // RE2019
} // CoroBehaviour2022



// class PlayingState
// {
//   private:
//   static LoggerPtr logger;

//   static float getBackoffDistance() { return 0.85f; }

//   public:
//   // ====================================================================
//   // FIXME: walks backwards without checking for field boundary or obstacles
//   class BackOffFreeKickTask : public CoroBehaviour
//   {
//   private:
//     HeadControl::ScanWithBallTask headScanWithBall;
//     HeadControl::SearchForBallTask headSearchForBall;
//     int startMs;

//   public:
//     BackOffFreeKickTask(BehaviourEnv &env) : CoroBehaviour(env), headScanWithBall(env), headSearchForBall(env) {}

//     void operator()(void)
//     {
//       CRBEHAVIOUR_BEGIN("Playing_BackoffFreeKick")

//       // ensure the robot has stopped walking
//       CRT_FOR_TIME_DO_YIELD(startMs, 1000, { Motion::standStill(output); });

//       // back off but keep looking around
//       CRT_FOR_TIME_DO_YIELD(startMs, 2000, {
//         backAwayAvoidingBoundary();
//         Motion::standStill(output);
//         if (input.ballInfo.ballAgeMs < 2000)
//           headScanWithBall();
//         else
//           headSearchForBall();
//       });

//       CRBEHAVIOUR_END()
//     }

//   private:
//     void backAwayAvoidingBoundary()
//     {
//       float backoffNeeded = getBackoffDistance() - input.ballInfo.ballModel.abs();

//       // do we even need to back off?
//       if (backoffNeeded < 0.0f)
//         return;

//       Vector2f boundaryDist = input.units->fieldModel.boundaryDistManhattan(input.robotPose.translation);
//       float minBoundaryDist = std::min(boundaryDist.x, boundaryDist.y);

//       if (minBoundaryDist < 0)
//       {
//         LOGE(logger, "boundary distance is negative means we are outside the field - give up");
//         return;
//       }

//       // weight how much we go backwards vs sideways by the available safe
//       // boundary distance. The safe boundary distance is the actual
//       // boundary distance less the field border width (so that we don't
//       // try to exit the playing area in general).
//       //   If the safeBoundary distance is more than the backoffNeeded
//       // we just reverse. Otherwise, we weight how much we can reverse
//       // by the room to backoff and compensate by moving sideways

//       float safeBoundaryDist = std::max(0.0f, minBoundaryDist - input.units->fieldModel.fieldBorderWidth);
//       float alpha = std::min(1.0f, safeBoundaryDist / backoffNeeded);

//       // should we sidestep left or right?
//       /*
//        * Orientations...
//        *
//        *       -pi/2
//        *         V
//        *      0-> <- pi/-pi
//        *         ^
//        *        pi/2
//        */

//       float leftRight;                     // 1 means left, -1 means right
//       if (boundaryDist.x < boundaryDist.y) // are we closer to the sideline?
//       {
//         // are we closer to goal?
//         if (input.ballInfo.ballModelField.x > input.robotPose.translation.x)
//           leftRight = -sgn(input.robotPose.rotation);
//         else
//           leftRight = sgn(input.robotPose.rotation);
//       }
//       else // closer to end line
//       {
//         // FIXME:
//         leftRight = -1.0; // always go right for now
//       }

//       // rotate to face ball
//       float rotation = sgn(input.ballInfo.ballModel.angle());

//       Motion::walk(output, -alpha, (1 - alpha) * leftRight, (1 - alpha) * rotation);
//     }
//   };

//   // ==== Corner kick ===============================================
//   class SearchForCornerBallTask : public CoroBehaviour
//   {
//   private:
//     GotoSkills::GotoPoseTask gotoPose;
//     Pose2D targetPose;
//     Pose2D localPose;
//     int threshold;

//   public:
//     SearchForCornerBallTask(BehaviourEnv &env)
//         : CoroBehaviour(env), gotoPose(env), threshold(1.5) // in meters, the abs dist away from the corner
//     {
//     }

//     void operator()()
//     {
//       CRBEHAVIOUR_BEGIN("SearchForCornerBall")

//       // go to cornerpose
//       // define the corner pose depending on which side pf the pitch youre on
//       targetPose.rotation = 0.0f;
//       targetPose.translation.x = -input.units->fieldModel.halfFieldLength; // always attacking half

//       // go to the nearest corner first
//       targetPose.translation.y = sgn(input.robotPose.translation.y) * input.units->fieldModel.halfFieldWidth;

//       LOGV(logger, "Go to pose on field: {}", targetPose);

//       LOGV(logger, "Robot Pose {}", input.robotPose);
//       CRT_DO
//       {
//         localPose = input.robotPose.invert() + targetPose; // convert to robot local
//         LOGI(logger, "Going to local {}, close enough? {}", localPose, (localPose.translation.abs() < threshold));
//         gotoPose(localPose, GotoSkills::ROUGH);
//       }
//       CRT_YIELD_UNTIL(localPose.translation.abs() < threshold);

//       CRT_YIELD(); // yield incase we've seen the ball again

//       // switch corner by changing target pose
//       targetPose.translation.y *= -1.0f;
//       CRT_DO
//       {
//         localPose = input.robotPose.invert() + targetPose; // convert to robot local
//         LOGI(logger, "Going to local {}, close enough? {}", localPose, (localPose.translation.abs() < threshold));
//         gotoPose(localPose, GotoSkills::ROUGH);
//       }
//       CRT_YIELD_UNTIL(localPose.translation.abs() < threshold);

//       CRBEHAVIOUR_END()
//     }
//   };

//   class CornerKickTask : public CoroBehaviour
//   {
//   private:
//     SearchForCornerBallTask searchForCornerBall;
//     BallHandling::GotoKickPoseAndKickTask gotoKickPoseAndKick;
//     GotoSkills::GotoPoseTask gotoPose;
//     HeadControl::ScanWithBallTask scanWithBall;
//     HeadControl::ScanVerticalSlowTask scanVerticalSlow;
//     Pose2D cornerKickApproachPose, targetCornerKickApproachPose;
//     Vector2f corner, vectFromCornerBall;
//     int startMs;
//     float offsetGoalLine, offsetSideLine;

//   public:
//     CornerKickTask(BehaviourEnv &env)
//         : CoroBehaviour(env), searchForCornerBall(env),
//           gotoKickPoseAndKick(env), // rotation, translation.x and translation.y
//           gotoPose(env), scanWithBall(env), scanVerticalSlow(env), cornerKickApproachPose(),
//           targetCornerKickApproachPose(), corner(), vectFromCornerBall(), offsetGoalLine(0.2), // for approach pose
//           offsetSideLine(0.2)
//     {
//     }

//     void operator()(void)
//     {
//       CRBEHAVIOUR_BEGIN("Playing_CornerKickTask")

//       CRT_WHILE(!input.ballInfo.seen)
//       {
//         searchForCornerBall();
//         scanWithBall();
//       }
//       CRT_YIELD_UNTIL_SUCCESS(searchForCornerBall);

//       /// debugging
//       LOGI(logger, "finished search for corner ball. was the ball seen? {}", input.ballInfo.seen);
//       ///----

//       // initialize the corner
//       corner.x = -input.units->fieldModel.halfFieldLength;
//       corner.y = sgn(input.robotPose.translation.y) * input.units->fieldModel.halfFieldWidth;
//       corner = input.robotPose.invert() * corner;

//       vectFromCornerBall.x = corner.x - 0.4;
//       vectFromCornerBall.y = corner.y + (sgn(input.robotPose.translation.y) * 0.4);

//       /// debugging
//       CRT_FOR_TIME_DO_YIELD(startMs, 2000, {
//         Motion::standStill(output);
//         if (input.ballInfo.seen)
//           HeadControl::lookAtBallModel(input, output);
//         else
//           HeadControl::lookAt(output, corner.x, corner.y);
//       });
//       ///---

//       // set the corner kick approach pose
//       targetCornerKickApproachPose.rotation = 0.0f; // face out onto the pitch initally
//       targetCornerKickApproachPose.translation.x = -input.units->fieldModel.halfFieldLength - offsetGoalLine;
//       targetCornerKickApproachPose.translation.y =
//           sgn(input.robotPose.translation.y) * (input.units->fieldModel.halfFieldWidth - offsetSideLine);

//       // go to corner kick approach pose

//       CRT_DO
//       {
//         cornerKickApproachPose = input.robotPose.invert() + targetCornerKickApproachPose; // convert to robot local
//         gotoPose(cornerKickApproachPose, GotoSkills::ROUGH);
//         if (input.ballInfo.seen)
//           HeadControl::lookAtBallModel(input, output);
//         else
//           scanWithBall();
//       }
//       CRT_YIELD_UNTIL_SUCCESS(gotoPose);

//       /// debugging
//       LOGI(logger, "finished approach 1");
//       ///----

//       /// debugging
//       CRT_FOR_TIME_DO_YIELD(startMs, 2000, {
//         Motion::standStill(output);
//         if (input.ballInfo.seen)
//           HeadControl::lookAtBallModel(input, output);
//         else
//           scanWithBall();
//       });
//       ///---

//       targetCornerKickApproachPose.rotation = 0.0f;                  // face closer to the goal
//       targetCornerKickApproachPose.translation = vectFromCornerBall; // get behind corner
//       targetCornerKickApproachPose = input.robotPose.invert() + targetCornerKickApproachPose;

//       // sidestep
//       CRT_FOR_TIME_AND_EXPR_DO_YIELD(startMs, 4000,
//                                      GotoSkills::isCloseEnough(targetCornerKickApproachPose, GotoSkills::ROUGH), {
//                                        Motion::walk(output, -0.2, sgn(input.robotPose.translation.y), 0);
//                                        if (input.ballInfo.seen)
//                                          HeadControl::lookAtBallModel(input, output);
//                                        else
//                                          scanWithBall();
//                                      });
//       /// debugging
//       LOGI(logger, "finished approach 2");
//       ///----

//       /*CRT_DO
//       {
//           cornerKickApproachPose = input.robotPose.invert() + targetCornerKickApproachPose; //convert to robot local
//           gotoPose(cornerKickApproachPose, GotoSkills::ROUGH);
//           if(input.ballInfo.seen)
//               HeadControl::lookAtBallModel(input, output);
//           else
//               scanWithBall();

//       }
//       CRT_YIELD_UNTIL_SUCCESS(gotoPose);*/

//       CRT_DO
//       {
//         gotoKickPoseAndKick(input.kickOffKickPose);
//         HeadControl::lookDown(output);
//       }
//       CRT_YIELD_UNTIL_SUCCESS(gotoKickPoseAndKick);

//       /// debugging
//       CRT_FOR_TIME_DO_YIELD(startMs, 5000, {
//         Motion::standStill(output);
//         scanVerticalSlow();
//       });
//       ///---

//       CRBEHAVIOUR_END()
//     }
//   };

//   // ====================================================================

//   class PlayTask : public CoroBehaviour
//   {
//   private:
//     // variables and subtasks go here
//     GotoSkills::GotoTacticPoseTask gotoTacticPose;
//     BackOffFreeKickTask backOffFreeKick;
//     CornerKickTask cornerKick;
//     PenaltyShootGoalie::Play penaltyShootGoaliePlay;
//     PenaltyShootStriker::Play penaltyShootStrikerPlay;
//     ExpGoalie::PlayTask expGoaliePlay;
//     Striker::PlayTask strikerPlay;
//     HeadControl::LookAtBallModelTask lookAtBallModel;

//   public:
//     PlayTask(BehaviourEnv &env)
//         : CoroBehaviour(env), gotoTacticPose(env), backOffFreeKick(env), cornerKick(env), penaltyShootGoaliePlay(env),
//           penaltyShootStrikerPlay(env), expGoaliePlay(env), strikerPlay(env), lookAtBallModel(env)
//     {
//     }

//     void operator()(void)
//     {
//       CRBEHAVIOUR_SCOPE("Play");

//       if (input.gameState.penaltyShoot)
//         doPenaltyShoot();
//       else
//         doNormalPlay();
//     }

//     void doPenaltyShoot()
//     {
//       LOGV(logger, "doPenaltyShoot");
//       if (input.role == RoleSelection::GOALIE)
//         penaltyShootGoaliePlay();
//       else
//         penaltyShootStrikerPlay();
//     }

//     void doNormalPlay()
//     {
//       LOGV(logger, "doNormalPlay: setPlay {}", GameControllerData::getName(input.gameState.setPlay));

//       // either normal (no set play) or else set play and we're kicking and not a corner
//       if ((input.gameState.setPlay == GameControllerData::NONE) ||
//           (input.gameState.kicking && (input.gameState.setPlay != GameControllerData::CORNER_KICK)))
//       {
//         if (input.role == RoleSelection::GOALIE)
//           expGoaliePlay();
//         else if (input.role == RoleSelection::DEFENDER || input.role == RoleSelection::SUPPORTER ||
//                  input.role == RoleSelection::FORWARD)
//           gotoTacticPose();
//         else if (input.role == RoleSelection::STRIKER)
//           strikerPlay();
//         else
//         {
//           LOGE(logger, "doNormalPlay ERROR: UNKNOWN ROLE");
//           Motion::standStill(output);
//           HeadControl::lookDown(output);
//         }
//       }
//       else // some sort of set play / free kick
//       {
//         // are we a goalie in the box? If so, then normal behaviour
//         if ((input.role == RoleSelection::GOALIE) &&
//             input.units->fieldModel.insidePenaltyBox(input.robotPose.translation))
//         {
//           expGoaliePlay();
//         }
//         // in set play, if we don't have the kick, we need to back up
//         // at least 0.75m but give a bit more to be safe
//         else if (!input.gameState.kicking && (input.ballInfo.ballModel.abs() < getBackoffDistance()))
//         {
//           LOGV(logger, "NormalPlay, setPlay awarded to the opposing team, back away");
//           backOffFreeKick();
//           // Motion::standStill(output);
//         }
//         else if (input.gameState.kicking && input.gameState.setPlay == GameControllerData::CORNER_KICK &&
//                  input.role == RoleSelection::STRIKER)
//         {
//           LOGV(logger, "CornerKick, corner awarded to our team");
//           cornerKick();
//         }
//         else
//         {
//           LOGV(logger, "NormalPlay, setPlay awarded to the opposing team, stay put");
//           // TODO - should we stay put or move to a better position
//           Motion::standStill(output);
//           // HeadControl::lookAtBallModel(input,output);
//           lookAtBallModel();
//         }
//       }
//     }
//   };
// };
