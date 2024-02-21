/**
* @file RemoteControl.h
*
* This file implements a behaviour that ainms to control the robot remotely
*/

#pragma once

#include "TcpListener.h"
#include "Modules/BehaviorControl/CoroBehaviour/CoroBehaviourCommon.h"
#include "Modules/BehaviorControl/CoroBehaviour/Skills/CommonSkills.h"
#include "Modules/BehaviorControl/CoroBehaviour/Skills/SoundSkills.h"


namespace CoroBehaviour
{
    namespace RE2023
    {
        CRBEHAVIOUR(RemooteControl)
        {
            CRBEHAVIOUR_INIT(RemoteControl) {
                // Start the listener
                tcpListener.startListening(8080);
            }

            void RemoteControl::operator()(void) {
            std::string command;
            CRBEHAVIOUR_BEGIN();

            while (true) {
                if (tcpListener.dequeueCommand(command)) {
                    // Process command
                    if (command == "walk_forward") {
                        // Trigger walking forward
                        motionSkills.walkAtRelativeSpeed(Pose2f(0.f, 1.f, 0.f));
                    }
                    if (command == "end_remote") {
                        // End Remote control
                        break;
                    }
                    // Handle other commands...
                }
                CR_YIELD();
            }

            CRBEHAVIOUR_END();
            }

            private:
            // DEFINES_PARAMS(RemoteControl, 
            // {,
            // (unsigned)(5000) walkForwardMs,
            // });
            TCPListener tcpListener  {env};
            CommonSkills commonSkills  {env};
            HeadSkills headSkills {env};
            MotionSkills motionSkills {env};
        };
    } // RE2023
} // CoroBehaviour