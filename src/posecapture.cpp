#include <QDebug>
#include <CppLowess/Lowess.h>
#include "posecapture.h"
#include "util.h"

#if USE_MOCAP

#define SAME_DIRECTION_CHECK_DOT_THRESHOLD      0.96

const int PoseCapture::PreEnterDuration = 3000;
const int PoseCapture::CapturingDuration = 3000;
const int PoseCapture::TargetFrames = 1;
const float PoseCapture::TargetSeconds = 0.0;

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

void PoseCapture::reduceFramesOfTrack(PoseCapture::Track &resultTrack, std::vector<qint64> &resultTimeline)
{
    if (resultTimeline.size() <= TargetFrames)
        return;
    PoseCapture::Track originalTrack = resultTrack;
    std::vector<qint64> originalTimeline = resultTimeline;
    resultTrack.clear();
    resultTimeline.clear();
    resultTrack.resize(TargetFrames);
    resultTimeline.resize(TargetFrames);
    if (1 == TargetFrames) {
        resultTrack[0] = originalTrack[originalTrack.size() - 1];
        resultTimeline[0] = originalTimeline[originalTimeline.size() - 1];
        return;
    }
    for (size_t i = 0; i < TargetFrames; ++i) {
        size_t sourceIndex = i * originalTrack.size() / TargetFrames;
        resultTrack[i] = originalTrack[sourceIndex];
        resultTimeline[i] = originalTimeline[sourceIndex];
    }
}

void PoseCapture::handleCapturingTimeout()
{
    qDebug() << "Capturing timeout, state:" << (int)m_state;
    if (m_state == State::Capturing) {
        commitCurrentTrack();
        if (Profile::Main == m_profile) {
            m_latestMainTimeline = m_currentCapturingTimeline;
        }
        if (!m_latestMainTimeline.empty() &&
                !m_latestLeftHandSideTrack.empty() &&
                !m_latestRightHandSideTrack.empty()) {
            PoseCapture::Track resultTrack;
            std::vector<qint64> resultTimeline;
            mergeProfileTracks(m_latestMainTrack, m_latestRightHandSideTrack, m_latestLeftHandSideTrack, m_latestMainTimeline,
                resultTrack, resultTimeline);
            reduceFramesOfTrack(resultTrack, resultTimeline);
            emit trackChanged(resultTrack, resultTimeline);
        }
        m_state = State::Idle;
        qDebug() << "State change to Idle";
        emit stateChanged(m_state);
    }
}

void PoseCapture::updateQVector3DsToAnimalPoserParameters(const QVector3D &from, const QVector3D &to,
        std::map<QString, QString> &parameters)
{
    parameters["fromX"] = QString::number(from.x());
    parameters["fromY"] = QString::number(-from.y());
    parameters["fromZ"] = QString::number(from.z());
    parameters["toX"] = QString::number(to.x());
    parameters["toY"] = QString::number(-to.y());
    parameters["toZ"] = QString::number(to.z());
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
    updateQVector3DsToAnimalPoserParameters(findFromPosition->second, findToPosition->second,
        parameters[paramName]);
}

void PoseCapture::updateSpineKeypointsToAnimalPoseParameters(const std::map<QString, QVector3D> &keypoints,
        std::map<QString, std::map<QString, QString>> &parameters)
{
    auto leftShoulderPosition = keypoints.find("leftShoulder");
    if (leftShoulderPosition == keypoints.end())
        return;
    
    auto rightShoulderPosition = keypoints.find("rightShoulder");
    if (rightShoulderPosition == keypoints.end())
        return;
    
    auto leftHipPosition = keypoints.find("leftHip");
    if (leftHipPosition == keypoints.end())
        return;
    
    auto rightHipPosition = keypoints.find("rightHip");
    if (rightHipPosition == keypoints.end())
        return;
    
    auto middleBetweenHipsPosition = (leftHipPosition->second + rightHipPosition->second) / 2;
    auto middleBetweenShouldersPosition = (leftShoulderPosition->second + rightShoulderPosition->second) / 2;
    auto middleOfHipAndShoulderPosition = (middleBetweenHipsPosition + middleBetweenShouldersPosition) / 2;
    updateQVector3DsToAnimalPoserParameters(middleBetweenHipsPosition, middleOfHipAndShoulderPosition,
        parameters["Spine1"]);
    updateQVector3DsToAnimalPoserParameters(middleOfHipAndShoulderPosition, middleBetweenShouldersPosition,
        parameters["Spine2"]);
    
    auto nosePosition = keypoints.find("nose");
    if (nosePosition == keypoints.end())
        return;
    updateQVector3DsToAnimalPoserParameters(middleBetweenShouldersPosition, nosePosition->second,
        parameters["Spine3"]);
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
    
    updateSpineKeypointsToAnimalPoseParameters(keypoints, parameters);
    
    updateFromKeypointsToAnimalPoserParameters(keypoints, parameters,
        "LeftLimb1_Joint1", "leftHip", "leftKnee");
    updateFromKeypointsToAnimalPoserParameters(keypoints, parameters,
        "LeftLimb1_Joint2", "leftKnee", "leftAnkle");
    updateFromKeypointsToAnimalPoserParameters(keypoints, parameters,
        "LeftLimb1_Joint3", "leftAnkle", "leftAnkle");
    updateFromKeypointsToAnimalPoserParameters(keypoints, parameters,
        "RightLimb1_Joint1", "rightHip", "rightKnee");
    updateFromKeypointsToAnimalPoserParameters(keypoints, parameters,
        "RightLimb1_Joint2", "rightKnee", "rightAnkle");
    updateFromKeypointsToAnimalPoserParameters(keypoints, parameters,
        "RightLimb1_Joint3", "rightAnkle", "rightAnkle");
}

bool PoseCapture::isMainTrackCaptured()
{
    return !m_latestMainTrack.empty();
}

bool PoseCapture::isRightHandTrackCaptured()
{
    return !m_latestRightHandSideTrack.empty();
}

bool PoseCapture::isLeftHandTrackCaptured()
{
    return !m_latestLeftHandSideTrack.empty();
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
            bool inverse = false;
            if (it.first.startsWith("Left")) {
                pickedSide = &leftHand;
                if (it.first.startsWith("LeftLimb2_"))
                    inverse = true;
            } else if (it.first.startsWith("Right")) {
                pickedSide = &rightHand;
            } else {
                parameters[it.first] = it.second;
                continue;
            }
            if (it.first.startsWith("LeftLimb1_") || it.first.startsWith("RightLimb1_"))
                inverse = true;
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
                change["fromZ"] = (inverse ? "-" : "") + valueOfKeyInMapOrEmpty(findSide->second, "fromX");
                change["toZ"] = (inverse ? "-" : "") + valueOfKeyInMapOrEmpty(findSide->second, "toX");
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

void PoseCapture::updateKeypoints(const std::map<QString, QVector3D> &keypoints, const QSizeF &imageSize)
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
                if (InvokePose::Seven == invokePose) {
                    qDebug() << "Invoke pose: Seven";
                    m_currentCapturingTrack = &m_latestRightHandSideTrack;
                } else {
                    qDebug() << "Invoke pose: FlippedSeven";
                    m_currentCapturingTrack = &m_latestLeftHandSideTrack;
                }
            }
            cleanupCurrentTrack();
            qDebug() << "State change to PreEnter";
            emit stateChanged(m_state);
            return;
        }
    } else if (State::Capturing == m_state) {
        if (imageSize.width() > 0 && imageSize.height() > 0) {
            std::map<QString, QVector3D> normalizedKeypoints = keypoints;
            for (auto &it: normalizedKeypoints) {
                it.second.setX(it.second.x() / imageSize.width());
                it.second.setY(it.second.y() / imageSize.height() - 0.5);
            }
            m_capturedKeypoints.push_back(normalizedKeypoints);
            m_capturedKeypointsTimeline.push_back(m_elapsedTimer.elapsed());
        }
    }
}

void PoseCapture::smoothQVector3DList(std::vector<QVector3D> &vectors)
{
    std::vector<float> xList(vectors.size());
    std::vector<float> yList(vectors.size());
    for (size_t i = 0; i < vectors.size(); ++i) {
        xList[i] = vectors[i].x();
        yList[i] = vectors[i].y();
    }
    smoothList(xList);
    smoothList(yList);
    for (size_t i = 0; i < vectors.size(); ++i) {
        vectors[i].setX(xList[i]);
        vectors[i].setY(yList[i]);
    }
}

void PoseCapture::smoothList(std::vector<float> &numbers)
{
    // https://github.com/hroest/CppLowess/blob/master/src/tests/testLowess.cpp
    CppLowess::TemplatedLowess<std::vector<double>, double> dlowess;
    
    std::vector<double> v_xval;
    std::vector<double> v_yval;
    for (size_t i = 0; i < numbers.size(); i++) {
        v_xval.push_back(i);
        v_yval.push_back(numbers[i]);
    }

    std::vector<double> out(numbers.size()), tmp1(numbers.size()), tmp2(numbers.size());
    dlowess.lowess(v_xval, v_yval, 0.25, 0, 0.0, out, tmp1, tmp2);
    for (size_t i = 0; i < numbers.size(); i++) {
        numbers[i] = out[i];
    }
}

void PoseCapture::smoothKeypointsList(std::vector<Keypoints> &keypointsList)
{
    std::map<QString, std::vector<QVector3D>> map;
    std::map<std::pair<QString, size_t>, size_t> mapDataPoseIndices;
    for (size_t i = 0; i < keypointsList.size(); ++i) {
        auto &keypoints = keypointsList[i];
        for (const auto &it: keypoints) {
            mapDataPoseIndices[{it.first, i}] = map[it.first].size();
            map[it.first].push_back(it.second);
        }
    }
    
    for (auto &it: map) {
        smoothQVector3DList(it.second);
    }
    
    for (size_t i = 0; i < keypointsList.size(); ++i) {
        auto &keypoints = keypointsList[i];
        for (auto &it: keypoints) {
            it.second = map[it.first][mapDataPoseIndices[{it.first, i}]];
        }
    }
}

void PoseCapture::commitCurrentTrack()
{
    if (m_capturedKeypointsTimeline.size() != m_capturedKeypoints.size()) {
        qDebug() << "Captured keypoints timeline length does not match with keypoints count";
        return;
    }
    smoothKeypointsList(m_capturedKeypoints);
    for (size_t i = 0; i < m_capturedKeypoints.size(); ++i) {
        const auto &keypoints = m_capturedKeypoints[i];
        std::map<QString, std::map<QString, QString>> parameters;
        keypointsToAnimalPoserParameters(keypoints, parameters);
        if (!parameters.empty())
            addFrameToCurrentTrack(m_capturedKeypointsTimeline[i], parameters);
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
    m_capturedKeypoints.clear();
    m_capturedKeypointsTimeline.clear();
}

PoseCapture::InvokePose PoseCapture::keypointsToInvokePose(const Keypoints &keypoints)
{
    bool isTpose = isLimbStraightAndParallelWith(keypoints, "left", (QVector3D(1, 0, 0)).normalized()) &&
        isLimbStraightAndParallelWith(keypoints, "right", (QVector3D(-1, 0, 0)).normalized());
    if (isTpose)
        return InvokePose::T;
    
    bool isRightHandSevenPose = (isLimbStraightAndParallelWith(keypoints, "left", (QVector3D(0, 1, 0)).normalized()) &&
        isLimbEndParallelWith(keypoints, "right", (QVector3D(-1, 0, 0)).normalized()));
    if (isRightHandSevenPose)
        return InvokePose::FlippedSeven;
    
    bool isLeftHandSevenPose = (isLimbStraightAndParallelWith(keypoints, "right", (QVector3D(0, 1, 0)).normalized()) &&
        isLimbEndParallelWith(keypoints, "left", (QVector3D(1, 0, 0)).normalized()));
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

bool PoseCapture::isLimbEndParallelWith(const Keypoints &keypoints,
        const QString &namePrefix, const QVector3D &referenceDirection)
{
    auto findElbow = keypoints.find(namePrefix + "Elbow");
    if (findElbow == keypoints.end())
        return false;
    auto findWrist = keypoints.find(namePrefix + "Wrist");
    if (findWrist == keypoints.end())
        return false;
    auto lowerLimb = (findWrist->second - findElbow->second).normalized();
    if (!isTwoQVector3DParallel(lowerLimb, referenceDirection))
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
