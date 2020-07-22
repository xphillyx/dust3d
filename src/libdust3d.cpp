#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <iostream>
#include <QVector3D>
#include "dust3d.h"
#include "meshgenerator.h"
#include "snapshot.h"
#include "snapshotxml.h"
#include "model.h"
#include "version.h"

struct _dust3d
{
    void *userData = nullptr;
    GeneratedCacheContext *cacheContext = nullptr;
    Model *resultMesh = nullptr;
    Snapshot *snapshot = nullptr;
    int error = DUST3D_ERROR;
    std::string obj;
};

int dust3dError(dust3d *ds3)
{
    return ds3->error;
}

void dust3dClose(dust3d *ds3)
{
    delete ds3->resultMesh;
    ds3->resultMesh = nullptr;
    
    delete ds3->cacheContext;
    ds3->cacheContext = nullptr;
    
    delete ds3->snapshot;
    ds3->snapshot = nullptr;
    
    delete ds3;
}

void dust3dInitialize(void)
{
    // void
}

dust3d *dust3dOpenFromMemory(const char *documentType, const char *buffer, int size)
{
    dust3d *ds3 = new dust3d;
    
    ds3->error = DUST3D_ERROR;
    
    if (0 == strcmp(documentType, "xml")) {
        QByteArray data(buffer, size);
        
        QXmlStreamReader stream(data);
        
        delete ds3->snapshot;
        ds3->snapshot = new Snapshot;
        
        loadSkeletonFromXmlStream(ds3->snapshot, stream);
        
        ds3->error = DUST3D_OK;
    } else {
        ds3->error = DUST3D_UNSUPPORTED;
    }
    
    return ds3;
}

dust3d *dust3dOpen(const char *fileName)
{
    std::string name = fileName;
    auto dotIndex = name.rfind('.');
    if (std::string::npos == dotIndex)
        return nullptr;

    std::string extension = name.substr(dotIndex + 1);

    std::ifstream is(name, std::ifstream::binary);
    if (!is)
        return nullptr;
    
    is.seekg (0, is.end);
    int length = is.tellg();
    is.seekg (0, is.beg);

    char *buffer = new char[length + 1];
    is.read(buffer, length);
    is.close();
    buffer[length] = '\0';
    
    dust3d *ds3 = dust3dOpenFromMemory(extension.c_str(), buffer, length);
    
    delete[] buffer;
    
    return ds3;
}

void dust3dSetUserData(dust3d *ds3, void *userData)
{
    ds3->userData = userData;
}

void *dust3dGetUserData(dust3d *ds3)
{
    return ds3->userData;
}

const char *dust3dVersion(void)
{
    return APP_NAME " " APP_HUMAN_VER;
}

int dust3dGenerateMesh(dust3d *ds3)
{
    ds3->error = DUST3D_ERROR;
    
    ds3->obj.clear();
    
    if (nullptr == ds3->snapshot)
        return ds3->error;
    
    if (nullptr == ds3->cacheContext)
        ds3->cacheContext = new GeneratedCacheContext();
    
    Snapshot *snapshot = ds3->snapshot;
    ds3->snapshot = nullptr;
    
    MeshGenerator *meshGenerator = new MeshGenerator(snapshot);
    meshGenerator->setGeneratedCacheContext(ds3->cacheContext);
    meshGenerator->generate();
    
    delete ds3->resultMesh;
    ds3->resultMesh = meshGenerator->takeResultMesh();
    
    std::stringstream stream;
    stream << "# " << APP_NAME << " " << APP_HUMAN_VER << endl;
    stream << "# " << APP_HOMEPAGE_URL << endl;
    for (std::vector<QVector3D>::const_iterator it = ds3->resultMesh->vertices().begin() ; it != ds3->resultMesh->vertices().end(); ++it) {
        stream << "v " << (*it).x() << " " << (*it).y() << " " << (*it).z() << endl;
    }
    for (std::vector<std::vector<size_t>>::const_iterator it = ds3->resultMesh->faces().begin() ; it != ds3->resultMesh->faces().end(); ++it) {
        stream << "f";
        for (std::vector<size_t>::const_iterator subIt = (*it).begin() ; subIt != (*it).end(); ++subIt) {
            stream << " " << (1 + *subIt);
        }
        stream << endl;
    }
    ds3->obj = stream.str();
    
    if (meshGenerator->isSuccessful())
        ds3->error = DUST3D_OK;
    
    delete meshGenerator;
    
    return ds3->error;
}

const char *dust3dGetMeshAsObj(dust3d *ds3)
{
    return ds3->obj.c_str();
}


