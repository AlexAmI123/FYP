/**
 * @file WhistleHandler.cpp
 * @author Andreas Stolpmann
 * @author Thomas Röfer
 */

#include "WhistleHandler.h"
#include "Platform/SystemCall.h"
#include "Tools/Settings.h"

#include "Tools/Debugging/Annotation.h"


MAKE_MODULE(WhistleHandler, infrastructure);


// OVERVIEW:
// guessedGameState == STATE_NOT_GUESSED is used to mean that there is no active guess
//
// We use the whistle to guess at transitions from SET to PLAYING (at kickoff,
// for a set play penalty kick, or for a penalty shootout penalty kick) and from
// PLAYING to READY (after a goal has been scored except in a penalty shootout)
//
// We check for a bad guess at the SET to PLAYING transition by checking if
// any of our team is penalised for illegal motion in set.
// We check for a bad guess at the PLAYING to READY transition by checking if
// the READY state was not signalled by the game controller after the expected
// 15 sec delay plus additional operator delay

// we'll process the whistle in all states because it might be used for custom
// features in behaviour
void WhistleHandler::update(GameInfo& theGameInfo)
{ 
  uint8_t lastGuessedGameState = guessedGameState;

  /* Get robot to say if whistle is detected */
  if (sayWhistleDetected && theWhistle.whistleDetected)
  { 
    SystemCall::say("Whistle confirmed");
  }

  /* Only receive WD output if last state change was a while ago */
  if (theFrameInfo.getTimeSince(timeOfLastStateChange) > detTimeoutMs)
  {
    whistleProcessed.detected = theWhistle.whistleDetected;
  }
  else
  {
    whistleProcessed.detected = false;
  }

  /* Copy game state sent by GameController.
   * State and kicking team may be overwritten below.
   */
  theGameInfo = theRawGameInfo;

  /* Do we need to do any further processing? */
  if (!useWhistleForKickOff && !useWhistleAfterGoal)
    return;

  // Remember the time when the effective game state changed or we wrongly detected a change.
  if(lastRawGameState != theRawGameInfo.state
     || (theRawGameInfo.state == STATE_SET && checkForIllegalMotionPenalty()))
  {
    // Update timestamp, unless the raw game state is now the one already guessed.
    if(theRawGameInfo.state != guessedGameState)
      timeOfLastStateChange = theFrameInfo.time;
    guessedGameState = STATE_NOT_GUESSED;

    ANNOTATION("WhistleHandler", fmt::format("GC state changed to {} or motion in set penalty => back to GC state", theRawGameInfo.state));

  }
  lastRawGameState = theRawGameInfo.state;

    // Switching SET --> PLAYING:
  if (useWhistleForKickOff && (theRawGameInfo.state == STATE_SET && guessedGameState == STATE_NOT_GUESSED))
  {
    // If the GameController is sending SET, and we are not guessing yet, and we heard a whistle.
    if (whistleProcessed.detected)
    {
      guessedGameState = STATE_PLAYING;
      timeOfLastStateChange =
          theFrameInfo.time +
          (theRawGameInfo.setPlay == SET_PLAY_PENALTY_KICK ? ignoreWhistleAfterPenaltyKick : ignoreWhistleAfterKickOff);

      if (theRawGameInfo.gamePhase != GAME_PHASE_PENALTYSHOOT || theRawGameInfo.setPlay == SET_PLAY_PENALTY_KICK)
        SystemCall::say("Kickoff");
    }
  }
  // Switching fromPLAYING (actual/guessed) to READY because a goal was scored:
  else if (useWhistleAfterGoal && ((theRawGameInfo.state == STATE_PLAYING && guessedGameState == STATE_NOT_GUESSED) ||
                                   (theRawGameInfo.state == STATE_SET && guessedGameState == STATE_PLAYING)))
  {
    // we don't do this in penalty shoot outs
    // is the ball in the goal and we heard a whistle?
    if (theRawGameInfo.gamePhase != GAME_PHASE_PENALTYSHOOT && checkForBallPosition() && whistleProcessed.detected)
    {
      guessedGameState = STATE_READY;
      timeOfLastStateChange = theFrameInfo.time;

      // No check for any validity of the ball position, because the ball could be hidden for a while
      // before the referee finally whistles.
      kickingTeam = theBallInGoal.inOwnGoal ? theOwnTeamInfo.teamNumber : theOpponentTeamInfo.teamNumber;
      SystemCall::say((std::string("Goal for ") + TypeRegistry::getEnumName(static_cast<Settings::TeamColor>(
        kickingTeam == theOwnTeamInfo.teamNumber ? theOpponentTeamInfo.fieldPlayerColor : theOwnTeamInfo.fieldPlayerColor))).c_str());
    }
  }

  // Check GameController messages for wrong switch to READY. If the GameController does not
  // send READY after a while or set plays are active, we cannot be in READY state.
  if (guessedGameState == STATE_READY &&
      (theFrameInfo.getTimeSince(timeOfLastStateChange) > gameControllerDelay + gameControllerOperatorDelay ||
       (theRawGameInfo.setPlay != SET_PLAY_NONE &&
        theFrameInfo.getTimeSince(timeOfLastStateChange) > gameControllerOperatorDelay)))
  {
    guessedGameState = STATE_NOT_GUESSED;
    timeOfLastStateChange = std::max(timeOfLastStateChange, theFrameInfo.time - acceptPastWhistleDelay);
    SystemCall::say("Back to playing");
    ANNOTATION("WhistleHandler", "READY state guess was incorrect => back to PLAYING");
 }

  // override state, kickingTeam in the game info sent by the GameController if necessary.
  // (NOTE: the rawGameInfo is copied to gameInfo above)
  if (guessedGameState != STATE_NOT_GUESSED)
  {
    theGameInfo.state = guessedGameState;
    if (guessedGameState == STATE_READY)
      theGameInfo.kickingTeam = kickingTeam;

    if (guessedGameState != lastGuessedGameState)
      ANNOTATION("WhistleHandler",
                 fmt::format("Overriding state {} with guessed state {}", theRawGameInfo.state, guessedGameState));
  }

  // Stop overriding when both game states match.
  if (theRawGameInfo.state == theGameInfo.state)
  {
    guessedGameState = STATE_NOT_GUESSED;

    if (guessedGameState != lastGuessedGameState)
      ANNOTATION("WhistleHandler", fmt::format("GC state {} now matches our guess", theRawGameInfo.state));
  }
  
}

void WhistleHandler::update(WhistleProcessed& theWhistleProcessed)
{
  theWhistleProcessed = whistleProcessed;
}

bool WhistleHandler::checkForIllegalMotionPenalty()
{
  penaltyTimes.resize(MAX_NUM_PLAYERS, 0);

  for(size_t i = 0; i < penaltyTimes.size(); ++i)
    if(theOwnTeamInfo.players[i].penalty != PENALTY_SPL_ILLEGAL_MOTION_IN_SET)
      penaltyTimes[i] = 0;
    else if(penaltyTimes[i] == 0)
      penaltyTimes[i] = theFrameInfo.time;

  for(unsigned penaltyTime : penaltyTimes)
    if(penaltyTime > timeOfLastStateChange)
      return true;

  return false;
}

bool WhistleHandler::checkForBallPosition()
{
  //was in goal in the last 5 seconds
  return !checkBallForGoal || theBallInGoal.timeSinceLastInGoal <= acceptBallInGoalDelay;
}
