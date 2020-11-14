#ifndef DUST3D_TEXTURE_TRANSFER_H
#define DUST3D_TEXTURE_TRANSFER_H
#include <vector>
#include <QVector3D>
#include <QImage>

class TextureTransfer
{
public:
    TextureTransfer(const std::vector<QVector3D> *sourceVertices,
            const std::vector<std::vector<size_t>> *sourceTriangles,
            const std::vector<QVector3D> *sourceTriangleNormals,
            const std::vector<std::vector<QVector2D>> *sourceUvs,
            const QImage *sourceImage,
            const std::vector<QVector3D> *destinationVertices,
            const std::vector<std::vector<size_t>> *destinationTriangles,
            const std::vector<QVector3D> *destinationTriangleNormals,
            const std::vector<std::vector<QVector2D>> *destinationUvs,
            QImage *destinationImage) :
        m_sourceVertices(sourceVertices),
        m_sourceTriangles(sourceTriangles),
        m_sourceTriangleNormals(sourceTriangleNormals),
        m_sourceUvs(sourceUvs),
        m_sourceImage(sourceImage),
        m_destinationVertices(destinationVertices),
        m_destinationTriangles(destinationTriangles),
        m_destinationTriangleNormals(destinationTriangleNormals),
        m_destinationUvs(destinationUvs),
        m_destinationImage(destinationImage)
    {
    }
    void transfer();
private:
    const std::vector<QVector3D> *m_sourceVertices = nullptr;
    const std::vector<std::vector<size_t>> *m_sourceTriangles = nullptr;
    const std::vector<QVector3D> *m_sourceTriangleNormals = nullptr;
    const std::vector<std::vector<QVector2D>> *m_sourceUvs = nullptr;
    const QImage *m_sourceImage = nullptr;
    
    const std::vector<QVector3D> *m_destinationVertices = nullptr;
    const std::vector<std::vector<size_t>> *m_destinationTriangles = nullptr;
    const std::vector<QVector3D> *m_destinationTriangleNormals = nullptr;
    const std::vector<std::vector<QVector2D>> *m_destinationUvs = nullptr;
    QImage *m_destinationImage = nullptr;
    
    void transferTriangle(size_t destinationTriangleIndex);
    QVector2D findUvInSource(const QVector3D &position, const QVector3D &normal);
};

#endif
