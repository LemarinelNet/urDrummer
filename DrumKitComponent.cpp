/*
 * DrumKitComponent.cpp
 *
 *  Created on: 13 juil. 2016
 *      Author: ludovic
 */

#include "DrumKitComponent.h"

DrumKitComponent::DrumKitComponent() {

	m_associatedTrigger=NULL;
	m_choosenInstrument=NULL;
	m_balance=0;
	m_pitch=1;
	m_replayGain=1;

}

DrumKitComponent::~DrumKitComponent() {

}

