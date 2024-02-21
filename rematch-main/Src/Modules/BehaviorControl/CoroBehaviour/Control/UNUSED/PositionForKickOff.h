// /**
//  * @file PositionForKickOff.h
//  *
//  * This file implements a basic kickoff behaviour.
//  *
//  * @author Rudi Villing
//  */

// #pragma once

// #include "Modules/BehaviorControl/CoroBehaviour/CoroBehaviour.h"

// #include "Modules/BehaviorControl/CoroBehaviour/Skills/HeadSkills.h"
// #include "Modules/BehaviorControl/CoroBehaviour/Skills/MotionSkills.h"
// #include "Modules/BehaviorControl/CoroBehaviour/Skills/GotoSkills.h"
// #include "Modules/BehaviorControl/CoroBehaviour/Skills/CommonSkills.h"

// #include "Tools/Debugging/Annotation.h"

// #include "Tools/Math/BHMath.h"
// #include "Tools/Math/Pose2f.h"



// namespace CoroBehaviour2022
// {

// class PositionForKickoffTask : public CoroBehaviour
// {
//     struct Params
//     {
//         float walkSpeed = 0.8f;
//         CoroTime initialWaitTime = 5000;
//         float positionThreshold = 100.0f;
//         // (Pose2f)({Angle(180_deg), 900.0f, 0.0f}) kickoffPose,
//     };

//     struct Params params;
//     headSkills.ScanSideToSideTask headScanSideToSideTask;

// public:
//     PositionForKickoffTask(BehaviourEnv &env) 
//         : CoroBehaviour(env, "PositionForKickoffTask"), 
//           headScanSideToSideTask(env)
//     {}


//     void operator() (void)
//     {
//         // pure local variables (reinitialized every time)
//         const Pose2f kickoffPose(0, -900.0f, 0.0f);
//         Pose2f targetPose = env.theRobotPose.inversePose + kickoffPose;

//         CRBEHAVIOUR_BEGIN()

//         CommonSkills::recordActivity(env, BehaviorStatus::codeReleasePositionForKickOff);

//         ANNOTATION("Behavior", "PositionForKickoff - Initial wait");

//         while (getCoroDuration() < params.initialWaitTime)
//         {
//             headScanSideToSideTask();
//             MotionSkills::standStill(env);
//             CR_YIELD();
//         }

//         ANNOTATION("Behavior", "PositionForKickoff - walk to kickoff");

//         // walk to kickoff position
//         while ((targetPose.translation.squaredNorm() > sqr(params.positionThreshold)) && (fabs(targetPose.rotation) > 10_deg))
//         {
//             headScanSideToSideTask();
//             // MotionSkills::walkToTarget(env, Pose2f(params.walkSpeed, params.walkSpeed, params.walkSpeed), targetPose);
//             GotoSkills::gotoTargetRel(env, Pose2f(params.walkSpeed, params.walkSpeed, params.walkSpeed), targetPose);
//             // thePathToTargetSkill(walkSpeed, kickoffPose);
//             CR_YIELD();
//         }

//         // // rotate to kickoff orientation
//         // while (fabs(targetPose.rotation) > 10_deg)
//         // {
//         //     headScanSideToSideTask();
//         //     MotionSkills::walkToTarget(env, Pose2f(params.walkSpeed, params.walkSpeed, params.walkSpeed), targetPose);
//         //     // thePathToTargetSkill(walkSpeed, kickoffPose);
//         //     CR_YIELD();
//         // }

//         // await the kickoff
//         while (true)
//         {
//             headSkills.lookForward(env);
//             MotionSkills::standStill(env);
//             CR_YIELD();
//         }

//         CRBEHAVIOUR_END()
//     }

// };

// } // CoroBehaviour2022