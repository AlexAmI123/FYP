/**
* @file RemoteControl.h
*
* This file implements a behaviour that ainms to control the robot remotely
*/

#pragma once

#include "TCPListener.h"
#include "Modules/BehaviorControl/CoroBehaviour/CoroBehaviourCommon.h"
#include "Modules/BehaviorControl/CoroBehaviour/Skills/CommonSkills.h"
#include "Modules/BehaviorControl/CoroBehaviour/Skills/SoundSkills.h"


namespace CoroBehaviour
{
    namespace RE2023
    {
        CRBEHAVIOUR(RemoteControl)
        {
            CRBEHAVIOUR_INIT(RemoteControl) {
                // Start the listener
                tcpListener.startListening(8080);
            }

            void operator()(void) {
            std::string command;
            CRBEHAVIOUR_BEGIN();

            while (true) {
                if (tcpListener.dequeueCommand(command)) {
                    // Process commands
                    if (command == "walk_forward") {
                        // Trigger walking forward
                        motionSkills.walkAtRelativeSpeed(Pose2f(0.f, 1.f, 0.f));
                    }
                    // Trigger crab walking left
                    // Trigger crab walking right
                    // Trigger rotate clockwise
                    // Trigger rotate anticlockwise
                    // Trigger kick ball
                    //etc
                    if (command == "end_remote") {
                        // End Remote control
                        break;
                    }
                }
                CR_YIELD(); break;
            }

            // CRBEHAVIOUR_END();
            }

            private:
            // DEFINES_PARAMS(RemoteControl, 
            // {,
            // (unsigned)(5000) walkForwardMs,
            // });
            TCPListener tcpListener;
            CommonSkills commonSkills  {env};
            HeadSkills headSkills {env};
            MotionSkills motionSkills {env};
        };
    } // RE2023
} // CoroBehaviour