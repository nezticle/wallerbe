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
