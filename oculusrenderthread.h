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

#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QVector>
#include <QtCore/QSize>
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>
#include <QtGui/QOpenGLContext>

#include <QtGui/QOpenGLTextureBlitter>

class QWindow;
class Renderer;
class QOpenGLFunctions_4_5_Core;

class OculusRenderThread : public QThread
{
    Q_OBJECT
    Q_PROPERTY(bool isActive READ isActive WRITE setIsActive NOTIFY isActiveChanged)
public:
    OculusRenderThread(QWindow *surface, QOpenGLContext *mirrorContext, QObject *parent = nullptr);
    ~OculusRenderThread();
    void stop();

    QOpenGLContext *openGLContext() const;
    ovrSession session() const;

    bool isActive() const;

    QSize mirrorTextureSize() const;
    unsigned int mirrorTextureId() const;

public slots:
    void setIsActive(bool isActive);

signals:
    void isActiveChanged(bool isActive);

protected:
    void run() override;

private:
    bool init();
    void cleanup();

    bool m_isActive;
    QMutex *m_mutex;
    ovrSession m_session;
    ovrTextureSwapChain m_textureSwapChain;
    QOpenGLContext *m_mirrorContext;
    QOpenGLContext *m_glContext;
    QWindow *m_surface;
    struct FramebufferObject {
        GLuint fbo;
        GLuint depthStencilBuffer;
        GLuint texture;
    };
    QSize m_outputSize;

    QVector<FramebufferObject> m_framebufferObjects;
    Renderer *m_renderer;
    QOpenGLFunctions_4_5_Core* m_gl;

    ovrEyeRenderDesc m_eyeRenderDesc[2];
    ovrVector3f m_hmdToEyeViewOffset[2];
    ovrHmdDesc m_hmdDesc;
    ovrLayerEyeFov m_layer;

    struct MirrorTexture {
        ovrMirrorTextureDesc description;
        ovrMirrorTexture texture;
        unsigned int id;
    } m_mirrorTexture;

    QOpenGLTextureBlitter *m_blitter;
};

#endif // RENDERTHREAD_H
