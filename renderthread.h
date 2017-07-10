#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QtCore/QThread>

class RenderThread : public QThread
{
public:
    RenderThread(QObject *parent = nullptr);
protected:
    void run() override;
};

#endif // RENDERTHREAD_H
