#include "scene.h"
#include "player.h"

#include <Hlms/Pbs/OgreHlmsPbsDatablock.h>
#include <OgreHlmsSamplerblock.h>
#include <OgreHlmsTextureManager.h>
#include <OgreHlmsManager.h>
#include <Hlms/Pbs/OgreHlmsPbs.h>
#include <OgreMeshManager.h>
#include <OgreMeshManager2.h>
#include <OgreMesh2.h>
#include <OgreItem.h>

Scene::Scene()
    : m_player(nullptr)
    , m_root(nullptr)
    , m_sceneManager(nullptr)
    , m_rootSceneNode(nullptr)
{
    // not valid unti after init is called
}

void Scene::init(Ogre::SceneManager *manager, Ogre::Root *root, Ogre::SceneNode *headNode, Ogre::Camera *cameras[])
{
    m_root = root;
    m_sceneManager = manager;
    m_rootSceneNode = m_sceneManager->getRootSceneNode(Ogre::SCENE_DYNAMIC);

    const float armsLength = 2.5f;

    // Create player (with cameras attached to head)
    m_player = new Player(headNode, cameras);


//    Ogre::Item *item = m_sceneManager->createItem( "Cube_d.mesh",
//                                                 Ogre::ResourceGroupManager::
//                                                 AUTODETECT_RESOURCE_GROUP_NAME,
//                                                 Ogre::SCENE_DYNAMIC );

//    auto sceneNode = m_sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->createChildSceneNode( Ogre::SCENE_DYNAMIC );

//    sceneNode->attachObject( item );
//    m_sceneNodes.append(sceneNode);

    // Create floor

    Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane( "Plane v1",
                                                                                       Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                                                                       Ogre::Plane( Ogre::Vector3::UNIT_Y, 1.0f ), 50.0f, 50.0f,
                                                                                       1, 1, true, 1, 4.0f, 4.0f, Ogre::Vector3::UNIT_Z,
                                                                                       Ogre::v1::HardwareBuffer::HBU_STATIC,
                                                                                       Ogre::v1::HardwareBuffer::HBU_STATIC );

    Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createManual(
                "Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

    planeMesh->importV1( planeMeshV1.get(), true, true, true );

    {
        Ogre::Item *item = m_sceneManager->createItem( planeMesh, Ogre::SCENE_DYNAMIC );
        item->setDatablock( "Marble" );
        Ogre::SceneNode *sceneNode = m_sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                createChildSceneNode( Ogre::SCENE_DYNAMIC );
        sceneNode->setPosition( 0, -1, 0 );
        sceneNode->attachObject( item );

        //Change the addressing mode of the roughness map to wrap via code.
        //Detail maps default to wrap, but the rest to clamp.
        assert( dynamic_cast<Ogre::HlmsPbsDatablock*>( item->getSubItem(0)->getDatablock() ) );
        Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                    item->getSubItem(0)->getDatablock() );
        //Make a hard copy of the sampler block
        Ogre::HlmsSamplerblock samplerblock( *datablock->getSamplerblock( Ogre::PBSM_ROUGHNESS ) );
        samplerblock.mU = Ogre::TAM_WRAP;
        samplerblock.mV = Ogre::TAM_WRAP;
        samplerblock.mW = Ogre::TAM_WRAP;
        //Set the new samplerblock. The Hlms system will
        //automatically create the API block if necessary
        datablock->setSamplerblock( Ogre::PBSM_ROUGHNESS, samplerblock );
    }

    for( int i=0; i<4; ++i )
    {
        for( int j=0; j<4; ++j )
        {
            Ogre::String meshName;

            if( i == j )
                meshName = "Sphere1000.mesh";
            else
                meshName = "Cube_d.mesh";

            Ogre::Item *item = m_sceneManager->createItem( meshName,
                                                         Ogre::ResourceGroupManager::
                                                         AUTODETECT_RESOURCE_GROUP_NAME,
                                                         Ogre::SCENE_DYNAMIC );
            if( i % 2 == 0 )
                item->setDatablock( "Rocks" );
            else
                item->setDatablock( "Marble" );

            item->setVisibilityFlags( 0x000000001 );

            size_t idx = i * 4 + j;
            auto sceneNode = m_rootSceneNode->createChildSceneNode( Ogre::SCENE_DYNAMIC );

            sceneNode->setPosition( (i - 1.5f) * armsLength,
                                          2.0f,
                                          (j - 1.5f) * armsLength );
            sceneNode->setScale( 0.65f, 0.65f, 0.65f );
            sceneNode->roll( Ogre::Radian( (Ogre::Real)idx ) );
            sceneNode->attachObject( item );
            m_sceneNodes.append(sceneNode);
        }
    }

    {
        size_t numItems = 0;
        Ogre::HlmsManager *hlmsManager = m_root->getHlmsManager();
        Ogre::HlmsTextureManager *hlmsTextureManager = hlmsManager->getTextureManager();

        //assert( dynamic_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );

        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlmsManager->getHlms(Ogre::HLMS_PBS) );

        const int numX = 8;
        const int numZ = 8;

        const float armsLength = 1.0f;
        const float startX = (numX-1) / 2.0f;
        const float startZ = (numZ-1) / 2.0f;

        for( int x=0; x<numX; ++x )
        {
            for( int z=0; z<numZ; ++z )
            {
                Ogre::String datablockName = "Test" + Ogre::StringConverter::toString( numItems++ );
                Ogre::HlmsPbsDatablock *datablock = static_cast<Ogre::HlmsPbsDatablock*>(
                            hlmsPbs->createDatablock( datablockName,
                                                      datablockName,
                                                      Ogre::HlmsMacroblock(),
                                                      Ogre::HlmsBlendblock(),
                                                      Ogre::HlmsParamVec() ) );

                Ogre::HlmsTextureManager::TextureLocation texLocation = hlmsTextureManager->
                        createOrRetrieveTexture( "SaintPetersBasilica.dds",
                                                 Ogre::HlmsTextureManager::TEXTURE_TYPE_ENV_MAP );

                datablock->setTexture( Ogre::PBSM_REFLECTION, texLocation.xIdx, texLocation.texture );
                datablock->setDiffuse( Ogre::Vector3( 0.0f, 1.0f, 0.0f ) );

                datablock->setRoughness( std::max( 0.02f, x / Ogre::max( 1, (float)(numX-1) ) ) );
                datablock->setFresnel( Ogre::Vector3( z / Ogre::max( 1, (float)(numZ-1) ) ), false );

                Ogre::Item *item = m_sceneManager->createItem( "Sphere1000.mesh",
                                                             Ogre::ResourceGroupManager::
                                                             AUTODETECT_RESOURCE_GROUP_NAME,
                                                             Ogre::SCENE_DYNAMIC );
                item->setDatablock( datablock );
                item->setVisibilityFlags( 0x000000002 );

                Ogre::SceneNode *sceneNode = m_sceneManager->getRootSceneNode( Ogre::SCENE_DYNAMIC )->
                        createChildSceneNode( Ogre::SCENE_DYNAMIC );
                sceneNode->setPosition( Ogre::Vector3( armsLength * x - startX,
                                                       1.0f,
                                                       armsLength * z - startZ ) );
                sceneNode->attachObject( item );
            }
        }
    }

    // Create lights
    Ogre::Light *light = m_sceneManager->createLight();
    Ogre::SceneNode *lightNode = m_rootSceneNode->createChildSceneNode();
    lightNode->attachObject( light );
    light->setPowerScale( 97.0f );
    light->setType( Ogre::Light::LT_DIRECTIONAL );
    light->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );

    m_lightNodes.append(lightNode);

    m_sceneManager->setAmbientLight( Ogre::ColourValue( 0.3f, 0.5f, 0.7f ) * 0.1f * 0.75f * 60.0f,
                                   Ogre::ColourValue( 0.6f, 0.45f, 0.3f ) * 0.065f * 0.75f * 60.0f,
                                     -light->getDirection() + Ogre::Vector3::UNIT_Y * 0.2f );
}



void Scene::update(qint64 time)
{

}

Player *Scene::player() const
{
    return m_player;
}
