#ifndef SCENE_H
#define SCENE_H

#include <Ogre.h>
#include <QVector>

class Player;
class Scene
{
public:
    Scene();
    void init(Ogre::SceneManager *manager, Ogre::Root *root);
    void update(unsigned int time);

private:
    Player *m_player;
    Ogre::Root *m_root;
    Ogre::SceneManager *m_sceneManager;
    Ogre::SceneNode *m_rootSceneNode;
    QVector<Ogre::SceneNode *> m_lightNodes;
    QVector<Ogre::SceneNode *> m_sceneNodes;
};

#endif // SCENE_H
