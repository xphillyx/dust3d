#include <QDebug>
#include "variablesxml.h"

void saveVariablesToXmlStream(const std::map<QString, QString> &variables, QXmlStreamWriter *writer)
{
    writer->setAutoFormatting(true);
    writer->writeStartDocument();
    
    writer->writeStartElement("variables");
        for (const auto &it: variables) {
            writer->writeStartElement("variable");
                writer->writeAttribute("name", it.first);
                writer->writeAttribute("value", it.second);
            writer->writeEndElement();
        }
    writer->writeEndElement();
    
    writer->writeEndDocument();
}

void loadVariablesFromXmlStream(std::map<QString, QString> *variables, QXmlStreamReader &reader)
{
    std::vector<QString> elementNameStack;
    while (!reader.atEnd()) {
        reader.readNext();
        if (!reader.isStartElement() && !reader.isEndElement()) {
            if (!reader.name().toString().isEmpty())
                qDebug() << "Skip xml element:" << reader.name().toString() << " tokenType:" << reader.tokenType();
            continue;
        }
        QString baseName = reader.name().toString();
        if (reader.isStartElement())
            elementNameStack.push_back(baseName);
        QStringList nameItems;
        for (const auto &nameItem: elementNameStack) {
            nameItems.append(nameItem);
        }
        QString fullName = nameItems.join(".");
        if (reader.isEndElement())
            elementNameStack.pop_back();
        if (reader.isStartElement()) {
            if (fullName == "variables.variable") {
                QString variableName;
                QString variableValue;
                foreach(const QXmlStreamAttribute &attr, reader.attributes()) {
                    auto attrNameString = attr.name().toString();
                    if ("name" == attrNameString) {
                        variableName = attr.value().toString();
                    } else if ("value" == attrNameString) {
                        variableValue = attr.value().toString();
                    }
                }
                if (!variableName.isEmpty()) {
                    (*variables)[variableName] = variableValue;
                }
            }
        }
    }
}
