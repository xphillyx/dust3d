#ifndef DUST3D_PART_IMPORTER_H
#define DUST3D_PART_IMPORTER_H
#include <QByteArray>
#include <QString>

class PartImporter
{
public:
    PartImporter(const QByteArray *fileData);
    static QString getExtensionList();
};

#endif