#include <QDebug>
#include "posecapture.h"

#if USE_MOCAP

#define SAME_DIRECTION_CHECK_DOT_THRESHOLD      0.96

const int PoseCapture::PreEnterDuration = 3000;
const int PoseCapture::CapturingDuration = 5000;

PoseCapture::PoseCapture(QObject *parent) :
    QObject(parent)
{
}

PoseCapture::~PoseCapture()
{
    delete m_preEnterTimer;
    delete m_capturingTimer;
}

PoseCapture::State PoseCapture::state()
{
    return m_state;
}

void PoseCapture::handlePreEnterTimeout()
{
    qDebug() << "PreEnter timeout, state:" << (int)m_state;
    if (m_state == State::PreEnter) {
        delete m_capturingTimer;
        m_capturingTimer = new QTimer;
        m_capturingTimer->setSingleShot(true);
        connect(m_capturingTimer, &QTimer::timeout, this, &PoseCapture::handleCapturingTimeout);
        m_capturingTimer->start(CapturingDuration);
        m_state = State::Capturing;
        qDebug() << "State change to Capturing";
        emit stateChanged(m_state);
        return;
    }
}

void PoseCapture::handleCapturingTimeout()
{
    qDebug() << "Capturing timeout, state:" << (int)m_state;
    if (m_state == State::Capturing) {
        m_state = State::Idle;
        qDebug() << "State change to Idle";
        emit stateChanged(m_state);
    }
}

void PoseCapture::updateKeypoints(const std::map<QString, QVector3D> &keypoints)
{
    if (State::Idle == m_state) {
        auto invokePose = keypointsToInvokePose(keypoints);
        if (InvokePose::T == invokePose || InvokePose::Seven == invokePose) {
            delete m_preEnterTimer;
            m_preEnterTimer = new QTimer;
            m_preEnterTimer->setSingleShot(true);
            connect(m_preEnterTimer, &QTimer::timeout, this, &PoseCapture::handlePreEnterTimeout);
            m_preEnterTimer->start(PreEnterDuration);
            m_state = State::PreEnter;
            qDebug() << "State change to PreEnter";
            emit stateChanged(m_state);
            return;
        }
    }
}

PoseCapture::InvokePose PoseCapture::keypointsToInvokePose(const Keypoints &keypoints)
{
    bool isTpose = isLimbStraightAndParallelWith(keypoints, "left", (QVector3D(1, 0, 0)).normalized()) &&
        isLimbStraightAndParallelWith(keypoints, "right", (QVector3D(-1, 0, 0)).normalized());
    if (isTpose)
        return InvokePose::T;
    
    bool isRightHandSevenPose = (isLimbStraightAndParallelWith(keypoints, "left", (QVector3D(0, 1, 0)).normalized()) &&
        isLimbStraightAndParallelWith(keypoints, "right", (QVector3D(-1, 0, 0)).normalized()));
    if (isRightHandSevenPose)
        return InvokePose::Seven;
    
    bool isLeftHandSevenPose = (isLimbStraightAndParallelWith(keypoints, "right", (QVector3D(0, 1, 0)).normalized()) &&
        isLimbStraightAndParallelWith(keypoints, "left", (QVector3D(1, 0, 0)).normalized()));
    if (isLeftHandSevenPose)
        return InvokePose::Seven;
    
    return InvokePose::Unknown;
}

bool PoseCapture::isTwoQVector3DParallel(const QVector3D &first, const QVector3D &second)
{
    if (QVector3D::dotProduct(first, second) < SAME_DIRECTION_CHECK_DOT_THRESHOLD)
        return false;
    return true;
}

bool PoseCapture::isLimbStraightAndParallelWith(const Keypoints &keypoints,
        const QString &namePrefix, const QVector3D &referenceDirection)
{
    auto findShoulder = keypoints.find(namePrefix + "Shoulder");
    if (findShoulder == keypoints.end())
        return false;
    auto findElbow = keypoints.find(namePrefix + "Elbow");
    if (findElbow == keypoints.end())
        return false;
    auto findWrist = keypoints.find(namePrefix + "Wrist");
    if (findWrist == keypoints.end())
        return false;
    auto upperLimb = (findElbow->second - findShoulder->second).normalized();
    auto lowerLimb = (findWrist->second - findElbow->second).normalized();
    if (!isTwoQVector3DParallel(upperLimb, lowerLimb))
        return false;
    if (!isTwoQVector3DParallel(upperLimb, referenceDirection))
        return false;
    if (!isTwoQVector3DParallel(lowerLimb, referenceDirection))
        return false;
    return true;
}

#endif
