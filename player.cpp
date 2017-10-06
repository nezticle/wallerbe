/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
****************************************************************************/

#include "player.h"

Player::Player(Ogre::SceneNode *headNode, Ogre::Camera *cameras[])
    : m_camerasNode(headNode)
{
    m_camerasNode->setPosition(0.f, 3.f, 0.f);
    m_cameras[0] = cameras[0];
    m_cameras[1] = cameras[1];
}


void Player::updateFromTracking(const ovrTrackingState &trackingState, ovrEyeRenderDesc eyeRenderDesc[])
{
    auto cameraPosition = m_camerasNode->getPosition();
    auto cameraOrientation = m_camerasNode->getOrientation();
    ovrQuatf oculusOrient = trackingState.HeadPose.ThePose.Orientation;
    ovrVector3f oculusPosition = trackingState.HandPoses->ThePose.Position;
    for (int i = 0; i < 2; ++i) {
        m_cameras[i]->setOrientation(cameraOrientation * Ogre::Quaternion(oculusOrient.w, oculusOrient.x, oculusOrient.y, oculusOrient.z));
        m_cameras[i]->setPosition(cameraPosition // "gameplay" position
                                  + (m_cameras[i]->getOrientation()
                                     * Ogre::Vector3(eyeRenderDesc[i].HmdToEyeOffset.x,
                                                     eyeRenderDesc[i].HmdToEyeOffset.y,
                                                     eyeRenderDesc[i].HmdToEyeOffset.z)
                                     + cameraOrientation
                                     * Ogre::Vector3(oculusPosition.x,
                                                     oculusPosition.y,
                                                     oculusPosition.z)));
    }
    // TODO Update the position of Hands
}
