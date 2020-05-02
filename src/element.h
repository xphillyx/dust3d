#ifndef DUST3D_ELEMENT_H
#define DUST3D_ELEMENT_H
#include <QString>

enum class Element
{
    Polygon,
    Voxel,
    Count
};
Element ElementFromString(const char *elementString);
#define IMPL_ElementFromString                                    \
Element ElementFromString(const char *elementString)              \
{                                                                 \
    QString element = elementString;                              \
    if (element == "Polygon")                                     \
        return Element::Polygon;                                  \
    if (element == "Voxel")                                       \
        return Element::Voxel;                                    \
    return Element::Polygon;                                      \
}
const char *ElementToString(Element element);
#define IMPL_ElementToString                                      \
const char *ElementToString(Element element)                      \
{                                                                 \
    switch (element) {                                            \
        case Element::Polygon:                                    \
            return "Polygon";                                     \
        case Element::Voxel:                                      \
            return "Voxel";                                       \
        default:                                                  \
            return "Polygon";                                     \
    }                                                             \
}
QString ElementToDispName(Element element);
#define IMPL_ElementToDispName                                    \
QString ElementToDispName(Element element)                        \
{                                                                 \
    switch (element) {                                            \
        case Element::Polygon:                                    \
            return QObject::tr("Polygon");                        \
        case Element::Voxel:                                      \
            return QObject::tr("Voxel");                          \
        default:                                                  \
            return QObject::tr("Polygon");                        \
    }                                                             \
}

#endif
