/**
 * @file GotoSkills.h
 *
 * This file contains some RE2019 based goto skills.
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
#include "Modules/BehaviorControl/CoroBehaviour/Skills/BallSkills.h"
#include "Modules/BehaviorControl/CoroBehaviour/Skills/ObstacleSkills.h"


namespace CoroBehaviour
{
namespace RE2019
{

  // =====================================================================

  /**
   * Main entry point of the striker behaviour in playing state
   */
  CRBEHAVIOUR(GotoBallAndKickBestOptionTask)
  {
    CRBEHAVIOUR_INIT(GotoBallAndKickBestOptionTask) {}

    void operator()(bool indirect = false)
    {
      // we assume that a higher level behaviour will switch away from this
      // skill if the ball has not been seen for too long, so we just use
      // the current ball model as the real position and don't bother checking
      // when we saw it last
      CRBEHAVIOUR_LOOP()
      {
        // try not to walk into goal
        if (theFieldDimensions.isInsideOpponentGoal(theFieldBall.endPositionOnField))
        {
          commonSkills.standLookActive();
          CR_EXIT_SUCCESS();
        }

        updateKickParams(indirect);
        gotoBallAndKickTask(kickAngle, kickType, kickAlignPrecisely);

        if (gotoBallAndKickTask.isSuccess())
          CR_EXIT_SUCCESS();
        else if (gotoBallAndKickTask.isFailure())
          CR_EXIT_FAILED();
        else
          CR_YIELD();
      }
    }

  private:
    DEFINES_PARAMS(GotoBallAndKickBestOptionTask,
    {,
      // (Rangef)(2000.f, 4000.f) forwardFastLongKickRange, ///< In this range the kick would reach the target
      // (Rangef)(1500.f, 2500.f) forwardFastKickRange, ///< In this range the kick would reach the target
      // (Rangef)(1000.f, 2000.f) walkForwardsLongKickRange, ///< In this range the kick would reach the target
      // (Rangef)(0.f, 1500.f) walkForwardsKickRange, ///< In this range the kick would reach the target
      (Rangef)(2000.f, 5000.f) forwardFastLongKickRange, ///< In this range the kick would reach the target
      (Rangef)(1600.f, 2500.f) forwardFastKickRange, ///< In this range the kick would reach the target
      (Rangef)(800.f, 2000.f) walkForwardsLongKickRange, ///< In this range the kick would reach the target
      (Rangef)(0.f, 1000.f) walkForwardsKickRange, ///< In this range the kick would reach the target
      (Angle)(10_deg) minKickSectorSize,
    });

    READS(RobotPose);
    READS(FieldDimensions);
    READS(ObstacleModel);
    READS(FieldBall);

    Angle kickAngle;
    float kickDistance;
    KickInfo::KickType kickType;
    bool kickAlignPrecisely;

    CommonSkills commonSkills {env};
    ObstacleSkills obstacleSkills {env};
    GotoBallAndKickTask gotoBallAndKickTask {env};

    void updateKickParams(bool indirect)
    {
      if (indirect)
      {
        // FIXME - should try to pass
        setGoalKickAngleAndDistance();
        setIndirectKickType();
      }
      else if (isKickTowardsGoalBestOption())
      {
        setGoalKickAngleAndDistance();
        setGoalKickType();
      }
    }

    bool isKickTowardsGoalBestOption() const
    {
      // TODO: needs a real implementation
      return true;
    }

    /// angle to goal in robot relative coordinates
    void setGoalKickAngleAndDistance()
    {
      Vector2f goalCenterOnField {theFieldDimensions.xPosOpponentGroundLine, 0.f};
      Vector2f goalCenter = theRobotPose.toRobotCoordinates(goalCenterOnField);

      kickAngle = goalCenter.angle();
      kickDistance = goalCenter.norm() + 1000.f; // approximation wtih margin

      if (goalCenter.norm() < 6000.f) // FIXME HACK
      {
        const std::list<SectorWheel::Sector>& kickSectors = obstacleSkills.populateKickSectorsAtBall();
        (void)kickSectors;
        Rangea bestGoalSector = obstacleSkills.getBestGoalSector(kickSectors);

        if (bestGoalSector.getSize() > params.minKickSectorSize)
        {
          kickAngle = (bestGoalSector.max + bestGoalSector.min) / 2; // mid angle = average
          kickDistance = goalCenter.norm() + 1000.f; // approximation wtih margin
        }
      }

      addActivationGraphOutput(fmt::format("kickAngle = {}_deg", toDegrees(kickAngle)));
      // addActivationGraphOutput(fmt::format("goalCenterAngle = {}_deg", toDegrees(goalCenter.angle())));
    }

    void setIndirectKickType()
    {
      bool left;

      if (kickAngle > 15_deg) // kicking with right foot (toward our left)
        left = true;
      else if (kickAngle < 15_deg) // kicking with left foot (toward our right)
        left = false;
      else
        left = Random::bernoulli();

      kickAlignPrecisely = false;

      // how strong to make the kick?
      kickType = left ? KickInfo::walkForwardsLeft : KickInfo::walkForwardsRight;
    }

    void setGoalKickType()
    {
      bool left;

      if (kickAngle > 15_deg) // kicking with right foot (toward our left)
        left = true;
      else if (kickAngle < 15_deg) // kicking with left foot (toward our right)
        left = false;
      else
        left = Random::bernoulli();

      kickAlignPrecisely = false;

      // how strong to make the kick?
      if (obstacleSkills.isOpponentDangerNearBall())
        kickType = left ? KickInfo::walkForwardsLeft : KickInfo::walkForwardsRight;
      else if (params.forwardFastLongKickRange.isInside(kickDistance))
      {
        kickAlignPrecisely = true;
        kickType = left ? KickInfo::forwardFastLeftLong : KickInfo::forwardFastRightLong;
      }
      else if (params.forwardFastKickRange.isInside(kickDistance))
        kickType = left ? KickInfo::forwardFastLeft : KickInfo::forwardFastRight;
      else if (params.walkForwardsLongKickRange.isInside(kickDistance))
        kickType = left ? KickInfo::walkForwardsLeftLong : KickInfo::walkForwardsRightLong;
      else
        kickType = left ? KickInfo::walkForwardsLeft : KickInfo::walkForwardsRight;
    }
  };


} // RE2019
} // CoroBehaviour2022
