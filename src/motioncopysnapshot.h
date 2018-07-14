#ifndef MOTION_COPY_SNAPSHOT_H
#define MOTION_COPY_SNAPSHOT_H
#include <vector>
#include <map>
#include <QString>

class MotionCopySnapshot
{
public:
    std::map<QString, QString> motion;
    std::vector<std::map<QString, std::map<QString, QString>>> framesTrackNodeInstances;
    std::map<QString, std::map<QString, QString>> trackNodes;
};

#endif
