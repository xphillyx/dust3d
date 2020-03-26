#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include "partimporter.h"

PartImporter::PartImporter(const QByteArray *fileData)
{
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFileFromMemory(fileData->constData(), fileData->size(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    // TODO:
}

QString PartImporter::getExtensionList()
{
    aiString extensionList;
    Assimp::Importer importer;
    importer.GetExtensionList(extensionList);
    return QString(extensionList.C_Str()).replace(QChar(';'), QChar(' '));
}