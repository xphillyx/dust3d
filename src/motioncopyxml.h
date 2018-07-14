#ifndef MOTION_COPY_XML_H
#define MOTION_COPY_XML_H
#include <QXmlStreamWriter>
#include "motioncopysnapshot.h"

void saveMotionCopyToXmlStream(MotionCopySnapshot *snapshot, QXmlStreamWriter *writer);
void loadMotionCopyFromXmlStream(MotionCopySnapshot *snapshot, QXmlStreamReader &reader);

#endif
