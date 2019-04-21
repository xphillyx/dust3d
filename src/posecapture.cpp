#include <QDebug>
#include "posecapture.h"
#include "util.h"

#if USE_MOCAP

#define SAME_DIRECTION_CHECK_DOT_THRESHOLD      0.96

const int PoseCapture::PreEnterDuration = 1000;
const int PoseCapture::CapturingDuration = 3000;

PoseCapture::PoseCapture(QObject *parent) :
    QObject(parent)
{
    m_elapsedTimer.start();
    connect(&m_stateCheckTimer, &QTimer::timeout, this, &PoseCapture::checkState);
    m_stateCheckTimer.start(500);
}

PoseCapture::~PoseCapture()
{
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
        m_stateBeginTime = m_elapsedTimer.elapsed();
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
        if (Profile::Main == m_profile) {
            m_latestMainTimeline = m_currentCapturingTimeline;
        }
        if (!m_latestMainTimeline.empty()) {
            PoseCapture::Track resultTrack;
            std::vector<qint64> resultTimeline;
            mergeProfileTracks(m_latestMainTrack, m_latestRightHandSideTrack, m_latestLeftHandSideTrack, m_latestMainTimeline,
                resultTrack, resultTimeline);
            emit trackChanged(resultTrack, resultTimeline);
        }
        m_state = State::Idle;
        qDebug() << "State change to Idle";
        emit stateChanged(m_state);
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
    newParameters["fromX"] = QString::number(findFromPosition->second.x());
    newParameters["fromY"] = QString::number(-findFromPosition->second.y());
    newParameters["fromZ"] = QString::number(findFromPosition->second.z());
    newParameters["toX"] = QString::number(findToPosition->second.x());
    newParameters["toY"] = QString::number(-findToPosition->second.y());
    newParameters["toZ"] = QString::number(findToPosition->second.z());
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

void PoseCapture::mergeProfileTracks(const Track &main, const Track &rightHand, const Track &leftHand,
    const std::vector<qint64> &timeline,
    Track &resultTrack, std::vector<qint64> &resultTimeline)
{
    int trackLength = main.size();
    if (trackLength > (int)timeline.size())
        trackLength = timeline.size();
    qDebug() << "Merging main:" << main.size() << "right:" << rightHand.size() << "left:" << leftHand.size() << "timeline:" << timeline.size();
    for (int mainIndex = 0; mainIndex < trackLength; ++mainIndex) {
        std::map<QString, std::map<QString, QString>> parameters;
        for (auto &it: main[mainIndex]) {
            const Track *pickedSide = nullptr;
            if (it.first.startsWith("Left")) {
                pickedSide = &leftHand;
            } else if (it.first.startsWith("Right")) {
                pickedSide = &rightHand;
            } else {
                qDebug() << "Encounter unsupported name:" << it.first;
            }
            if (nullptr != pickedSide) {
                parameters[it.first] = it.second;
                int sideIndex = pickedSide->size() * mainIndex / trackLength;
                if (sideIndex >= (int)pickedSide->size())
                    continue;
                auto &sideParameters = (*pickedSide)[sideIndex];
                auto findSide = sideParameters.find(it.first);
                if (findSide == sideParameters.end())
                    continue;
                auto &change = parameters[it.first];
                change["fromZ"] = valueOfKeyInMapOrEmpty(findSide->second, "fromX");
                change["toZ"] = valueOfKeyInMapOrEmpty(findSide->second, "toX");
            }
        }
        if (parameters.empty())
            continue;
        resultTrack.push_back(parameters);
        resultTimeline.push_back(timeline[mainIndex]);
    }
}

void PoseCapture::checkState()
{
    if (State::PreEnter == m_state) {
        if (m_stateBeginTime + PreEnterDuration < m_elapsedTimer.elapsed()) {
            handlePreEnterTimeout();
        }
    } else if (State::Capturing == m_state) {
        if (m_stateBeginTime + CapturingDuration < m_elapsedTimer.elapsed()) {
            handleCapturingTimeout();
        }
    }
}

void PoseCapture::updateKeypoints(const std::map<QString, QVector3D> &keypoints)
{
    checkState();
    if (State::Idle == m_state) {
        auto invokePose = keypointsToInvokePose(keypoints);
        if (InvokePose::T == invokePose || InvokePose::Seven == invokePose || InvokePose::FlippedSeven == invokePose) {
            m_stateBeginTime = m_elapsedTimer.elapsed();
            m_state = State::PreEnter;
            m_profile = InvokePose::T == invokePose ? Profile::Main : Profile::Side;
            if (m_profile == Profile::Main) {
                m_currentCapturingTrack = &m_latestMainTrack;
            } else {
                if (InvokePose::Seven == invokePose)
                    m_currentCapturingTrack = &m_latestLeftHandSideTrack;
                else
                    m_currentCapturingTrack = &m_latestRightHandSideTrack;
            }
            cleanupCurrentTrack();
            qDebug() << "State change to PreEnter";
            emit stateChanged(m_state);
            return;
        }
    } else if (State::Capturing == m_state) {
        std::map<QString, std::map<QString, QString>> parameters;
        keypointsToAnimalPoserParameters(keypoints, parameters);
        if (!parameters.empty())
            addFrameToCurrentTrack(m_elapsedTimer.elapsed(), parameters);
    }
}

void PoseCapture::addFrameToCurrentTrack(qint64 timestamp, const std::map<QString, std::map<QString, QString>> &parameters)
{
    for (const auto &it: parameters) {
        qDebug() << it.first;
        for (const auto &subIt: it.second) {
            qDebug() << subIt.first << subIt.second;
        }
    }
    m_currentCapturingTimeline.push_back(timestamp);
    m_currentCapturingTrack->push_back(parameters);
}

void PoseCapture::cleanupCurrentTrack()
{
    m_currentCapturingTrack->clear();
    m_currentCapturingTimeline.clear();
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
        return InvokePose::FlippedSeven;
    
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
