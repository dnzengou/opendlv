/**
 * Copyright (C) 2015 Chalmers REVERE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include <ctype.h>
#include <cstring>
#include <cmath>
#include <iostream>

#include "opendavinci/odcore/data/Container.h"
#include "opendavinci/odcore/data/TimeStamp.h"

#include "opendlvdata/GeneratedHeaders_opendlvdata.h"

#include "setopticalrotation/setopticalrotation.hpp"

namespace opendlv {
namespace action {
namespace setopticalrotation {

/**
 * Constructor.
 *
 * @param a_argc Number of command line arguments.
 * @param a_argv Command line arguments.
 */
SetOpticalRotation::SetOpticalRotation(int32_t const &a_argc, char **a_argv)
    : DataTriggeredConferenceClientModule(a_argc, a_argv, 
        "action-setopticalrotation"),
    m_stimulusTime(),
    m_stimulus(),
    m_stimulusRate(),
    m_correctionTime(0, 0),
    m_correction(),
    m_correctionGain(0.17f),
    m_maxStimulusAge(1.0f),
    m_patienceDuration(0.1f),
    m_stimulusJerk(),
    m_stimulusJerkThreshold(0.02f),
    m_stimulusRateThreshold(0.017f),
    m_stimulusThreshold(0.01f)
{
}

SetOpticalRotation::~SetOpticalRotation()
{
}

/**
 * Receives aim-point angle error.
 * Sends a optical rotation (steer) command to Act.
 */
void SetOpticalRotation::nextContainer(odcore::data::Container &a_container)
{
  if (a_container.getDataType() == opendlv::perception::StimulusDirectionOfMovement::ID()) {

    // TODO: Should receive timestamp from sensors.
    std::cout << "Got stimulus." << std::endl;
    auto stimulusDirectionOfMovement = a_container.getData<opendlv::perception::StimulusDirectionOfMovement>();
    auto stimulusTime = stimulusDirectionOfMovement.getIdentified();
    AddStimulus(stimulusTime, stimulusDirectionOfMovement);
    Correct();
  }
}

void SetOpticalRotation::AddStimulus(odcore::data::TimeStamp const &a_stimulusTime, opendlv::perception::StimulusDirectionOfMovement const &a_stimulusDirectionOfMovement)
{
  odcore::data::TimeStamp now;

  std::vector<odcore::data::TimeStamp> stimulusTimeToSave;
  std::vector<float> stimulusToSave;
  std::vector<float> stimulusRateToSave;
  for (uint8_t i = 0; i < m_stimulusTime.size(); i++) {
    auto t0 = m_stimulusTime[i];
    auto stimulus = m_stimulus[i];
    auto stimulusRate = m_stimulusRate[i];

    float t1 = static_cast<float>(now.toMicroseconds() 
        - t0.toMicroseconds()) / 1000000.0f;

    if (t1 < m_maxStimulusAge) {
      stimulusTimeToSave.push_back(t0);
      stimulusToSave.push_back(stimulus);
      stimulusRateToSave.push_back(stimulusRate);
    }
  }
  
  m_stimulusTime = stimulusTimeToSave;
  m_stimulus = stimulusToSave;
  m_stimulusRate = stimulusRateToSave;

  float desiredDirectionOfMovementAzimuth = a_stimulusDirectionOfMovement.getDesiredDirectionOfMovement().getAzimuth();
  float directionOfMovementAzimuth = a_stimulusDirectionOfMovement.getDirectionOfMovement().getAzimuth();
  float stimulus = desiredDirectionOfMovementAzimuth - directionOfMovementAzimuth;

  float stimulusRate = 0.0f;
  if (m_stimulus.size() > 0) {
    odcore::data::TimeStamp firstStimulusTime = m_stimulusTime[0];
    float firstStimulus = m_stimulus[0];
    float firstStimulusRate = m_stimulusRate[0];

    float deltaTime = static_cast<float>(a_stimulusTime.toMicroseconds() 
        - firstStimulusTime.toMicroseconds()) / 1000000.0f;

    stimulusRate = (stimulus - firstStimulus) / deltaTime;
    m_stimulusJerk = (stimulusRate - firstStimulusRate) / deltaTime;
  } else {
    m_stimulusJerk = 0.0f;
  }

  m_stimulusTime.push_back(a_stimulusTime);
  m_stimulus.push_back(stimulus);
  m_stimulusRate.push_back(stimulusRate);

//  std::cout << "==============" << std::endl;
//  std::cout << "Stimulus (" << m_stimulus.size() << "): " << stimulus << std::endl;
//  std::cout << "Stimulus rate (" << m_stimulusRate.size() << "): " << stimulusRate << std::endl;
//  std::cout << "Stimulus jerk: " << m_stimulusJerk << std::endl;
}

void SetOpticalRotation::Correct()
{
  float priority = 0.2f;
  
  if (IsPatient()) {
    return;
  }

  std::cout << "<<< Start to correct!" << std::endl;

  float stimulus = m_stimulus.back();
  float stimulusRate = m_stimulusRate.back();
  
  std::cout << "Stimulus: " << stimulus << std::endl;
  std::cout << "Stimulus rate: " << stimulusRate << std::endl;

  if (std::abs(stimulus) > m_stimulusThreshold) {

    bool isStimulusRateZero = (std::abs(stimulusRate) < m_stimulusRateThreshold);
    bool isStimulusRateHelping = (static_cast<int>(std::copysign(1.0f, stimulus)) != static_cast<int>(std::copysign(1.0f, stimulusRate)));

    if (isStimulusRateZero) {
      std::cout << "Rate is zero!" << std::endl;
    }

    if (isStimulusRateHelping) {
      std::cout << "Rate is helping!" << std::endl;
    }


    if (isStimulusRateZero || (!isStimulusRateZero && !isStimulusRateHelping)) {

      float amplitude = m_correctionGain * stimulus;
      odcore::data::TimeStamp t0;
      opendlv::action::Correction correction(t0, "steering", false, amplitude, priority);
      odcore::data::Container container(correction);
      getConference().send(container);

      std::cout << "Sent steering amplitude: " << amplitude << std::endl;
      std::cout << "Correction: " << amplitude << std::endl;

      m_correctionTime = t0;
      m_correction = amplitude;
    }
  }
  
  std::cout << ">>> End correction!" << std::endl;
}

bool SetOpticalRotation::IsPatient() const
{
  odcore::data::TimeStamp now;

  float timeSinceCorrection = static_cast<float>(now.toMicroseconds() 
      - m_correctionTime.toMicroseconds()) / 1000000.0f;
  
  return (timeSinceCorrection < m_patienceDuration);
}

void SetOpticalRotation::setUp()
{
}

void SetOpticalRotation::tearDown()
{
}

} // setopticalrotation
} // action
} // opendlv
