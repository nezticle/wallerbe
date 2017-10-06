#ifndef RENDERER_H
#define RENDERER_H

#include <OVR_CAPI.h>

#include <QObject>
#include <Ogre.h>

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

#include <QtCore/QSize>


class QOpenGLContext;
class QWindow;
class Scene;

class Renderer : public QObject
{
    Q_OBJECT
public:
    explicit Renderer(QWindow *surface, QOpenGLContext *glContext, const QSize &size, QObject *parent = nullptr);

    unsigned int render(const ovrTrackingState &trackingState, ovrEyeRenderDesc eyeRenderDesc[], qint64 timeDelta);

    void setEyeMatrices(ovrMatrix4f left, ovrMatrix4f right);

private:
    void init();
    void loadResources();
    void setupCameras();
    void setupRenderTarget();
    void setupCompositor();
    enum {
        left = 0,
        right = 1
    };

    QWindow *m_offscreenSurface;
    QOpenGLContext *m_glContext;
    QSize m_size;
    Ogre::Root *m_root;
    Ogre::RenderWindow *m_renderWindow;
    Ogre::SceneManager *m_sceneManager;

    Ogre::SceneNode *m_camerasNode;
    Ogre::Camera *m_cameras[2];

    Ogre::CompositorWorkspace *m_workspaces[2];
    Ogre::String m_pluginsPath;
    Ogre::String m_writeAccessFolder;
    Ogre::String m_resourcePath;
    Ogre::ViewPoint *m_viewports[2];
    Ogre::Real m_nearClippingDistance;
    Ogre::Real m_farClippingDistance;
    Ogre::TexturePtr m_renderTexturePtr;
    Ogre::RenderTexture* m_renderTexture;
    Scene *m_scene;

    double m_currentFrameDisplayTime;
    double m_lastFrameDisplayTime;
};

#endif // RENDERER_H
