/**
 * @file ReadyOrSetStateTask.h
 *
 * This task implements a look around behaviour for Ready and Set state
 * in the dynamic ball handling challenge
 *
 * @author Rudi Villing
 */


#pragma once

#include "Modules/BehaviorControl/CoroBehaviour/2022/CoroBehaviour2022.h"

#include "Modules/BehaviorControl/CoroBehaviour/Skills/CommonSkills.h"
#include "Modules/BehaviorControl/CoroBehaviour/Skills/HeadSkills.h"


namespace CoroBehaviour
{
namespace RC2022
{

  CRBEHAVIOUR(ReadyOrSetStateTask)
  {
    CRBEHAVIOUR_INIT(ReadyOrSetStateTask) {}

    void operator()(void)
    {
      CRBEHAVIOUR_LOOP()
      {
        commonSkills.activityStatus(BehaviorStatus::set);
        
        headSkills.lookActive(/* withBall: */ false, /* ignoreBall: */ true);
        commonSkills.stand(/* high */ true);

        CR_YIELD();
      }
    }

  private:
    CommonSkills commonSkills {env};
    HeadSkills headSkills {env};
  };

} // RE2022

} // CoroBehaviour2022