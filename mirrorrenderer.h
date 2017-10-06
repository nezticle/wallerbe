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

#ifndef MIRRORRENDERER_H
#define MIRRORRENDERER_H

#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QTimer>

#include <QtGui/QOpenGLFunctions_4_5_Core>

class QWindow;
class QOpenGLContext;
class QOpenGLTextureBlitter;

class MirrorRenderer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isActive READ isActive WRITE setIsActive NOTIFY isActiveChanged)
public:
    explicit MirrorRenderer(QWindow *surface, QOpenGLContext *context, const QSize textureSize, unsigned int textureId, QObject *parent = nullptr);
    ~MirrorRenderer();

    bool isActive() const;

signals:
    void isActiveChanged(bool isActive);

public slots:
    void setIsActive(bool isActive);

private slots:
    void render();

private:
    QWindow *m_surface;
    QOpenGLContext *m_context;
    QSize m_textureSize;
    unsigned int m_mirrorTextureId;
    bool m_isActive;
    QOpenGLFunctions_4_5_Core* m_gl;
    QTimer m_updateTimer;
    QOpenGLTextureBlitter *m_blitter;
};

#endif // MIRRORRENDERER_H
