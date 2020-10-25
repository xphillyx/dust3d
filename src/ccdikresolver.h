#ifndef DUST3D_CCD_IK_SOLVER_H
#define DUST3D_CCD_IK_SOLVER_H
#include <vector>
#include <QVector3D>
#include <QQuaternion>

struct CcdIkNode
{
    QVector3D position;
};

class CcdIkSolver
{
public:
    CcdIkSolver();
    void setMaxRound(int maxRound);
    void setDistanceThreshod(float threshold);
    int addNodeInOrder(const QVector3D &position);
    void solveTo(const QVector3D &position);
    const QVector3D &getNodeSolvedPosition(int index);
    int getNodeCount(void);
private:
    void iterate();
private:
    std::vector<CcdIkNode> m_nodes;
    QVector3D m_destination;
    int m_maxRound;
    float m_distanceThreshold2;
    float m_distanceCeaseThreshold2;
};

#endif
