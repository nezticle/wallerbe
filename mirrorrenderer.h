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
