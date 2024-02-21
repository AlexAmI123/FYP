// /**
//  * @file SimpleStrikerOnlyBehaviour.h
//  *
//  * This file implements a basic kickoff behaviour.
//  *
//  * @author Rudi Villing
//  */

// #pragma once

// #include "Modules/BehaviorControl/CoroBehaviour/CoroBehaviour.h"

// #include "Modules/BehaviorControl/CoroBehaviour/Skills/HeadSkills.h"
// #include "Modules/BehaviorControl/CoroBehaviour/Skills/MotionSkills.h"
// #include "Modules/BehaviorControl/CoroBehaviour/Skills/InWalkKickSkills.h"
// #include "Modules/BehaviorControl/CoroBehaviour/Skills/GotoSkills.h"
// #include "Modules/BehaviorControl/CoroBehaviour/Skills/CommonSkills.h"

// #include "Tools/Debugging/Annotation.h"

// #include "Tools/Math/BHMath.h"
// #include "Tools/Math/Pose2f.h"


// namespace CoroBehaviour2022
// {

// // ============================================================================
// /**
//  * reposition the robot for kickoff. For this simple behaviour,
//  * We use the same position regardless of who has kickoff
//  */
// class SimpleStrikerGotoKickoffTask : public CoroBehaviour
// {
//     struct Params
//     {
//         float walkSpeed = 0.8f;
//         CoroTime initialWaitTime = 3000;
//         float positionThreshold = 100.0f;
//         // (Pose2f)({Angle(180_deg), 900.0f, 0.0f}) kickoffPose,
//     };

//     Params params;
//     headSkills.ScanSideToSideTask headScanSideToSideTask;

// public:
//     SimpleStrikerGotoKickoffTask(BehaviourEnv &env)
//         : CoroBehaviour(env, "SimpleStrikerGotoKickoffTask"),
//           headScanSideToSideTask(env)
//     {}


//     void operator() (void)
//     {
//         // the next variables are pure local and reinitialized every time the function is called
//         // before resuming within the behaviour block
//         const Pose2f kickoffPose(0, -900.0f, 0.0f); // FIXME: should be initialized from field model sizes
//         Pose2f targetPose = env.theRobotPose.inversePose + kickoffPose;

//         CommonSkills::recordActivity(env, BehaviorStatus::gotoReadyPosition);

//         CRBEHAVIOUR_BEGIN()

//         CR_CHECKPOINT(initial_wait);
//         while (getCoroDuration() < params.initialWaitTime)
//         {
//             headScanSideToSideTask();
//             MotionSkills::standStill(env);
//             CR_YIELD();
//         }

//         CR_CHECKPOINT(walk_to_kickoff_position);
//         while ((targetPose.translation.squaredNorm() > sqr(params.positionThreshold)) || (fabs(targetPose.rotation) > 5_deg))
//         {
//             headScanSideToSideTask();
//             // MotionSkills::walkToTargetRel(env, Pose2f(params.walkSpeed, params.walkSpeed, params.walkSpeed), targetPose);
//             GotoSkills::gotoTargetRel(env, params.walkSpeed, targetPose);
//             // thePathToTargetSkill(walkSpeed, kickoffPose);
//             CR_YIELD();
//         }

//         CR_CHECKPOINT(await_kickoff);
//         while (true)
//         {
//             headSkills.lookForward(env);
//             MotionSkills::standStill(env);
//             CR_YIELD();
//         }

//         CRBEHAVIOUR_END()
//     }

// };


// // ============================================================================
// /**
//  * Implement the playing state behaviour for the simple striker.
//  * Basically this striker just tries continuously to score a goal.
//  * It does not take account of obstacles.
//  */
// class SimpleStrikerGotoBallAndAct : public CoroBehaviour
// {
//     struct Params
//     {
//         float walkSpeed = 0.8f;
//         // Angle ballAlignThreshold = 5_deg;
//         float ballNearThreshold = 500.0f; //mm
//         Angle angleToGoalThreshold = 10_deg; // for alignment pose
//         float ballAlignOffsetX = 400.0f;
//         float ballYThreshold = 100.0f;
//         Angle angleToGoalPrecise = 2_deg;
//         float ballOffsetX = 150.0f; // for the actual kick pose
//         float ballOffsetY = 40.0f;
//         Rangef ballOffsetXRange = {140.0f, 170.0f};
//         Rangef ballOffsetYRange = {20.0f, 50.0f};
//         CoroTime minKickWaitMs = 10;
//         CoroTime maxKickWaitMs = 3000;
//         float dribbleX = 50.0f;
//     };

//     Params params;

//     headSkills.ScanSideToSideTask headScanSideToSideTask;
//     InWalkKickTask inWalkKickTask;

// public:
//     SimpleStrikerGotoBallAndAct(BehaviourEnv &env)
//         : CoroBehaviour(env, "SimpleStrikerGotoBallAndAct"),
//           headScanSideToSideTask(env),
//           inWalkKickTask(env)
//     {}

//     void operator()(void)
//     {
//         const Angle angleToGoal = calcAngleToGoal();

//         CRBEHAVIOUR_BEGIN()

//         // goto nearest point on a circle a bit out from the ball
//         CR_CHECKPOINT(goto_kick_perimeter);
//         while (env.theFieldBall.positionRelative.squaredNorm() > sqr(params.ballNearThreshold))
//         {
//             headScanSideToSideTask();
//             GotoSkills::gotoTargetRel(env, params.walkSpeed,
//                                       env.theFieldBall.positionRelative);
//             CR_YIELD();
//         }

//         // once at that circle rotate around the ball to the alignment pose
//         CR_CHECKPOINT(align_to_kick_direction);
//         while (std::abs(angleToGoal) > params.angleToGoalThreshold ||
//                std::abs(env.theFieldBall.positionRelative.y()) > params.ballYThreshold)
//         {
//             headSkills.lookForward(env);
//             MotionSkills::walkToTargetRel(env, 
//                                           Pose2f(params.walkSpeed, params.walkSpeed, params.walkSpeed),
//                                           Pose2f(angleToGoal,
//                                                  env.theFieldBall.positionRelative.x() - params.ballAlignOffsetX,
//                                                  env.theFieldBall.positionRelative.y()));
//             CR_YIELD();
//         }

//         // we're now behind the ball so get in close to the kick pose
//         CR_CHECKPOINT(approach_kick_point);
//         while (std::abs(angleToGoal) > params.angleToGoalPrecise || 
//                !params.ballOffsetXRange.isInside(env.theFieldBall.positionRelative.x()) || 
//                !params.ballOffsetYRange.isInside(env.theFieldBall.positionRelative.y()))
//         {
//             headSkills.lookForward(env);
//             MotionSkills::walkToTargetRel(env,
//                                           Pose2f(params.walkSpeed, params.walkSpeed, params.walkSpeed),
//                                           Pose2f(angleToGoal, 
//                                                  env.theFieldBall.positionRelative.x() - params.ballOffsetX, 
//                                                  env.theFieldBall.positionRelative.y() - params.ballOffsetY));
//             CR_YIELD();
//         }

//         // we're basically at the ball, so kick it
//         CR_CHECKPOINT(kick);
//         while (getCheckpointDuration() < params.maxKickWaitMs)
//         {
//             headSkills.lookForward(env);
//             inWalkKickTask(WalkKickVariant(WalkKicks::forward, Legs::left),                            
//                            Pose2f(angleToGoal, 
//                                   env.theFieldBall.positionRelative.x() - params.ballOffsetX, 
//                                   env.theFieldBall.positionRelative.y() - params.ballOffsetY));

//             CR_BREAK_OR_YIELD(inWalkKickTask.isSuccess() && getCheckpointDuration() > params.minKickWaitMs);
//         }

//         // alternatively could dribble it
//         // CR_CHECKPOINT(dribble);
//         // while (ballOffsetXRange.isInside(theFieldBall.positionRelative.x()) && ballOffsetYRange.isInside(theFieldBall.positionRelative.y()))
//         // {
//         //     headSkills.lookForward(env);
//         //     Motion::walkToTargetRel(Pose2f(params.walkSpeed, params.walkSpeed, params.walkSpeed), 
//         //                             Pose2f(angleToGoal, 
//         //                                    theFieldBall.positionRelative.x() - ballOffsetX + params.dribbleX, 
//         //                                    theFieldBall.positionRelative.y() - ballOffsetY));
//         //     CR_YIELD();
//         // }

//         CRBEHAVIOUR_END()
//     }

// private : 
//     Angle calcAngleToGoal() const
//     {
//         return (env.theRobotPose.inversePose * Vector2f(env.theFieldDimensions.xPosOpponentGroundline, 0.f)).angle();
//     }
// };


// // ============================================================================
// /**
//  * Implement the playing state behaviour for the simple striker.
//  * Basically this striker just tries continuously to score a goal.
//  * It does not take account of obstacles.
//  */
// class SimpleStrikerPlayTask : public CoroBehaviour
// {
//     struct Params
//     {
//         float walkSpeed = 0.8f;
//         CoroTime initialWaitMs = 1000;
//         CoroTime ballNotSeenTimeoutMs = 7000;
//     };

//     Params params;
//     SimpleStrikerGotoBallAndAct gotoBallAndAct;

// public:
//     SimpleStrikerPlayTask(BehaviourEnv &env)
//         : CoroBehaviour(env, "SimpleStrikerPlayTask"),
//           gotoBallAndAct(env)
//     {}

//     void operator()(void)
//     {
//         CRBEHAVIOUR_BEGIN()

//         CR_CHECKPOINT(initial_wait);
//         while (getCoroDuration() < params.initialWaitMs)
//         {
//             CommonSkills::recordActivity(env, BehaviorStatus::controlBall);

//             headSkills.lookForward(env); // FIXME scan
//             MotionSkills::standStill(env);
//             CR_YIELD();
//         }

//         while (true)
//         {
//             CR_CHECKPOINT(goto_ball_and_act);
//             while (env.theFieldBall.ballWasSeen(params.ballNotSeenTimeoutMs))
//             {
//                 CommonSkills::recordActivity(env, BehaviorStatus::controlBall);

//                 gotoBallAndAct();
//                 // if (gotoBallAndAct.isYielded())
//                     CR_YIELD();
//             }

//             // if we get here we need to search for the ball.
//             // For now, this is just a turn on the spot implementation
//             CR_CHECKPOINT(search_for_ball);
//             while (!env.theFieldBall.ballWasSeen())
//             {
//                 CommonSkills::recordActivity(env, BehaviorStatus::findBall);

//                 headSkills.lookForward(env); // FIXME scan
//                 MotionSkills::walkAtRelativeSpeed(env, Pose2f(params.walkSpeed, 0.f, 0.f));
//                 CR_YIELD();
//             }
//         }

//         CRBEHAVIOUR_END()
//     }
// };

// // ============================================================================
// /**
//  * this task manages the high level RoboCup game state transitions and 
//  * delegates to the appropriate sub tasks
//  */
// class SimpleStrikerGameControlTask : public CoroBehaviour
// {
// private:
//     SimpleStrikerGotoKickoffTask gotoKickoffTask;
//     SimpleStrikerPlayTask playTask;

// public:
//     SimpleStrikerGameControlTask(BehaviourEnv &env)
//         : CoroBehaviour(env, "SimpleStrikerGameControlTask"),
//           gotoKickoffTask(env),
//           playTask(env)
//     {
//     }

//     void operator()(void)
//     {
//         CRBEHAVIOUR_BEGIN()

//         while (true)
//         {
//             // we don't allow getup in the initial or final state
//             CR_CHECKPOINT(initial_or_finished);
//             while ((env.theGameInfo.state == STATE_INITIAL) || (env.theGameInfo.state == STATE_FINISHED))
//             {
//                 CommonSkills::recordActivity(env, 
//                     env.theGameInfo.state == STATE_INITIAL ? BehaviorStatus::initial : BehaviorStatus::finished);
//                 headSkills.lookForward(env);
//                 MotionSkills::standStill(env);
//                 CR_YIELD();
//             }

//             if (needsGetUp())
//             {
//                 annotation("Getting up.");
//                 CommonSkills::recordActivity(env, BehaviorStatus::fallen);

//                 CR_CHECKPOINT(getting_up);
//                 while (env.theFallDownState.state != FallDownState::upright)
//                 {
//                     MotionSkills::getUp(env);
//                     CR_YIELD();
//                 }
//             }

//             CR_CHECKPOINT(ready_state);
//             while (env.theGameInfo.state == STATE_READY)
//             {
//                 gotoKickoffTask();
//                 CR_YIELD();
//             }

//             CR_CHECKPOINT(set_state);
//             while (env.theGameInfo.state == STATE_SET)
//             {
//                 CommonSkills::recordActivity(env, BehaviorStatus::set);
//                 headSkills.lookForward(env); // could also scan side to side if we wanted
//                 MotionSkills::standStill(env);
//                 CR_YIELD();
//             }

//             CR_CHECKPOINT(playing_state);
//             while (env.theGameInfo.state == STATE_PLAYING)
//             {
//                 playTask();
//                 CR_YIELD();
//             }

//         }

//         CRBEHAVIOUR_END()
//     }

// private:
//     inline bool needsGetUp()
//     {
//         return env.theFallDownState.state == FallDownState::fallen ||
//                env.theFallDownState.state == FallDownState::squatting;
//     }
// };


// } // CoroBehaviour2022