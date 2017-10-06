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

#include "mirrorrenderer.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLTextureBlitter>
#include <QtGui/QWindow>


MirrorRenderer::MirrorRenderer(QWindow *surface, QOpenGLContext *context, const QSize textureSize, unsigned int textureId, QObject *parent)
    : QObject(parent)
    , m_surface(surface)
    , m_context(context)
    , m_textureSize(textureSize)
    , m_mirrorTextureId(textureId)
    , m_isActive(false)
{
    m_updateTimer.setInterval(16);
    m_updateTimer.setSingleShot(false);
    m_context->makeCurrent(m_surface);
    m_gl = m_context->versionFunctions<QOpenGLFunctions_4_5_Core>();
    m_blitter = new QOpenGLTextureBlitter;
    if (!m_blitter->create()) {
        qWarning("Could not create blitter");
    }
    m_context->doneCurrent();

    connect(&m_updateTimer, &QTimer::timeout, this, &MirrorRenderer::render);
}

MirrorRenderer::~MirrorRenderer()
{
    delete m_blitter;
}

bool MirrorRenderer::isActive() const
{
    return m_isActive;
}

void MirrorRenderer::setIsActive(bool isActive)
{
    if (m_isActive == isActive)
        return;

    m_isActive = isActive;
    emit isActiveChanged(m_isActive);
    if (m_isActive)
        m_updateTimer.start();
    else
        m_updateTimer.stop();
}

void MirrorRenderer::render()
{
    m_context->makeCurrent(m_surface);

    // Clear window
    m_gl->glViewport(0, 0, m_surface->width(), m_surface->height());
    m_gl->glClearColor(0.f, 0.f, 0.f, 1.f);
    m_gl->glClear(GL_COLOR_BUFFER_BIT);

    if (m_blitter->isCreated()) {
        m_blitter->bind();
        QRect targetRect;
        float windowRatio = m_surface->width() / (float)m_surface->height();
        float textureRatio = m_textureSize.width() / (float)m_textureSize.height();
        if (windowRatio > textureRatio) {
            targetRect.setWidth(m_textureSize.width() * m_surface->height() / m_textureSize.height());
            targetRect.setHeight(m_surface->height());
            targetRect.setY(0);
            targetRect.setX(m_surface->width() / 2 - targetRect.width() / 2);
        } else {
            targetRect.setWidth(m_surface->width());
            targetRect.setHeight(m_textureSize.height() * m_surface->width() / m_textureSize.width());
            targetRect.setX(0);
            targetRect.setY(m_surface->height() / 2 - targetRect.height() / 2);
        }
        auto target = QOpenGLTextureBlitter::targetTransform(targetRect, QRect(0, 0, m_surface->width(), m_surface->height()));
        m_blitter->blit(m_mirrorTextureId, target, QOpenGLTextureBlitter::OriginTopLeft);
        m_blitter->release();
    }

    m_context->swapBuffers(m_surface);
    m_context->doneCurrent();
}
