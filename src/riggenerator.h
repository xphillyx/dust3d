#ifndef RIG_GENERATOR_H
#define RIG_GENERATOR_H
#include <QObject>
#include <QThread>
#include "meshresultcontext.h"
#include "meshloader.h"

class RigGenerator : public QObject
{
    Q_OBJECT
public:
    RigGenerator(const MeshResultContext &meshResultContext);
    ~RigGenerator();
    MeshLoader *takeResultMesh();
signals:
    void finished();
public slots:
    void process();
private:
    MeshResultContext *m_meshResultContext = nullptr;
    MeshLoader *m_resultMesh = nullptr;
};

#endif
