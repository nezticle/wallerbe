#ifndef PLAYER_H
#define PLAYER_H

#include <Ogre.h>
#include <OVR_CAPI.h>

class Player
{
public:
    enum Eye {
        LeftEye = 0,
        RightEye = 1
    };

    Player(Ogre::SceneNode *headNode, Ogre::Camera *cameras[]);

    void updateFromTracking(const ovrTrackingState &trackingState, ovrEyeRenderDesc eyeRenderDesc[]);

private:
    enum {
        left = 0,
        right = 1
    };
    Ogre::SceneNode *m_playerNode;
    Ogre::SceneNode *m_camerasNode;
    Ogre::Camera *m_cameras[2];
};

#endif // PLAYER_H
