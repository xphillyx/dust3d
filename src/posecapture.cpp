#include <QDebug>
#include "posecapture.h"
#include "util.h"

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

PoseCapture::Profile PoseCapture::profile()
{
    return m_profile;
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
        PoseCapture::Track resultTrack;
        mergeProfileTracks(m_latestMainTrack, m_latestSideTrack, resultTrack);
        m_state = State::Idle;
        qDebug() << "State change to Idle";
        emit stateChanged(m_state);
        emit trackMerged(resultTrack);
    }
}

void PoseCapture::updateFromKeypointsToAnimalPoserParameters(const std::map<QString, QVector3D> &keypoints,
        std::map<QString, std::map<QString, QString>> &parameters, const QString &paramName,
        const QString &fromName, const QString &toName)
{
    auto findFromPosition = keypoints.find(fromName);
    if (findFromPosition == keypoints.end())
        return;
    auto findToPosition = keypoints.find(toName);
    if (findToPosition == keypoints.end())
        return;
    auto &newParameters = parameters[paramName];
    newParameters["fromX"] = findFromPosition->second.x();
    newParameters["fromY"] = -findFromPosition->second.y();
    newParameters["fromZ"] = findFromPosition->second.z();
    newParameters["toX"] = findToPosition->second.x();
    newParameters["toY"] = -findToPosition->second.y();
    newParameters["toZ"] = findToPosition->second.z();
}

void PoseCapture::keypointsToAnimalPoserParameters(const std::map<QString, QVector3D> &keypoints,
        std::map<QString, std::map<QString, QString>> &parameters)
{
    updateFromKeypointsToAnimalPoserParameters(keypoints, parameters,
        "LeftLimb2_Joint1", "leftShoulder", "leftElbow");
    updateFromKeypointsToAnimalPoserParameters(keypoints, parameters,
        "LeftLimb2_Joint2", "leftElbow", "leftWrist");
    updateFromKeypointsToAnimalPoserParameters(keypoints, parameters,
        "LeftLimb2_Joint3", "leftWrist", "leftWrist");
    updateFromKeypointsToAnimalPoserParameters(keypoints, parameters,
        "RightLimb2_Joint1", "rightShoulder", "rightElbow");
    updateFromKeypointsToAnimalPoserParameters(keypoints, parameters,
        "RightLimb2_Joint2", "rightElbow", "rightWrist");
    updateFromKeypointsToAnimalPoserParameters(keypoints, parameters,
        "RightLimb2_Joint3", "rightWrist", "rightWrist");
}

void PoseCapture::mergeProfileTracks(const Track &main, const Track &side, Track &result)
{
    int trackLength = main.size();
    for (int mainIndex = 0; mainIndex < trackLength; ++mainIndex) {
        int sideIndex = side.size() * mainIndex / trackLength;
        if (sideIndex >= (int)side.size())
            continue;
        std::map<QString, std::map<QString, QString>> parameters;
        for (auto &it: main[mainIndex]) {
            auto &sideParameters = side[sideIndex];
            auto findSide = sideParameters.find(it.first);
            if (findSide == sideParameters.end())
                continue;
            parameters[it.first] = it.second;
            auto &change = parameters[it.first];
            change["fromZ"] = valueOfKeyInMapOrEmpty(findSide->second, "fromX");
            change["toZ"] = valueOfKeyInMapOrEmpty(findSide->second, "toX");
        }
        result.push_back(parameters);
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
            m_profile = InvokePose::T == invokePose ? Profile::Main : Profile::Side;
            if (m_profile == Profile::Main)
                m_latestMainTrack.clear();
            else
                m_latestSideTrack.clear();
            qDebug() << "State change to PreEnter";
            emit stateChanged(m_state);
            return;
        }
    } else if (State::Capturing == m_state) {
        std::map<QString, std::map<QString, QString>> parameters;
        keypointsToAnimalPoserParameters(keypoints, parameters);
        if (!parameters.empty()) {
            if (m_profile == Profile::Main)
                m_latestMainTrack.push_back(parameters);
            else
                m_latestSideTrack.push_back(parameters);
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
