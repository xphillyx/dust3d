#include "motioncopyxml.h"

void saveMotionCopyToXmlStream(MotionCopySnapshot *snapshot, QXmlStreamWriter *writer)
{
    writer->setAutoFormatting(true);
    writer->writeStartDocument();
    
    writer->writeStartElement("motion");
        std::map<QString, QString>::iterator motionIterator;
        for (motionIterator = snapshot->motion.begin(); motionIterator != snapshot->motion.end(); motionIterator++) {
            writer->writeAttribute(motionIterator->first, motionIterator->second);
        }
    
        writer->writeStartElement("trackNodes");
        std::map<QString, std::map<QString, QString>>::iterator trackNodeIterator;
        for (trackNodeIterator = snapshot->trackNodes.begin(); trackNodeIterator != snapshot->trackNodes.end(); trackNodeIterator++) {
            std::map<QString, QString>::iterator trackNodeAttributeIterator;
            writer->writeStartElement("trackNode");
            for (trackNodeAttributeIterator = trackNodeIterator->second.begin(); trackNodeAttributeIterator != trackNodeIterator->second.end(); trackNodeAttributeIterator++) {
                writer->writeAttribute(trackNodeAttributeIterator->first, trackNodeAttributeIterator->second);
            }
            writer->writeEndElement();
        }
        writer->writeEndElement();
    
        writer->writeStartElement("frames");
            for (const auto &frame: snapshot->framesTrackNodeInstances) {
                writer->writeStartElement("frame");
                    writer->writeStartElement("trackNodeInstances");
                    for (const auto &trackNodeInstanceIterator: frame) {
                        writer->writeStartElement("trackNodeInstance");
                        for (const auto &trackNodeInstanceAttributeIterator: trackNodeInstanceIterator.second) {
                            writer->writeAttribute(trackNodeInstanceAttributeIterator.first, trackNodeInstanceAttributeIterator.second);
                        }
                        writer->writeEndElement();
                    }
                    writer->writeEndElement();
                writer->writeEndElement();
            }
        writer->writeEndElement();
    
    writer->writeEndElement();
    
    writer->writeEndDocument();
}

void loadMotionCopyFromXmlStream(MotionCopySnapshot *snapshot, QXmlStreamReader &reader)
{
    int currentFrame = -1;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (reader.name() == "motion") {
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    snapshot->motion[attr.name().toString()] = attr.value().toString();
                }
            } else if (reader.name() == "trackNode") {
                QString trackNodeId = reader.attributes().value("id").toString();
                if (trackNodeId.isEmpty())
                    continue;
                std::map<QString, QString> *trackNodeMap = &snapshot->trackNodes[trackNodeId];
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    (*trackNodeMap)[attr.name().toString()] = attr.value().toString();
                }
            } else if (reader.name() == "frame") {
                std::map<QString, std::map<QString, QString>> instances;
                currentFrame = snapshot->framesTrackNodeInstances.size();
                snapshot->framesTrackNodeInstances.push_back(instances);
            } else if (reader.name() == "trackNodeInstance") {
                QString trackNodeInstanceId = reader.attributes().value("id").toString();
                if (trackNodeInstanceId.isEmpty())
                    continue;
                if (-1 == currentFrame)
                    continue;
                std::map<QString, QString> *trackNodeInstanceMap = &snapshot->framesTrackNodeInstances[currentFrame][trackNodeInstanceId];
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    (*trackNodeInstanceMap)[attr.name().toString()] = attr.value().toString();
                }
            }
        }
    }
}
