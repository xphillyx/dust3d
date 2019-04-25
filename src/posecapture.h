#ifndef DUST3D_POSE_CAPTURE_H
#define DUST3D_POSE_CAPTURE_H
#include <QObject>
#include <QVector3D>
#include <QTimer>
#include <QElapsedTimer>
#include <QSizeF>

#if USE_MOCAP

class PoseCapture : public QObject
{
    Q_OBJECT
public:
    using Track = std::vector<std::map<QString, std::map<QString, QString>>>;
    
    static const int PreEnterDuration;
    static const int CapturingDuration;
    static const int TargetFrames;
    static const float TargetSeconds;

    enum class State
    {
        Idle,
        PreEnter,
        Capturing
    };
    
    enum class InvokePose
    {
        Unknown,
        T,
        Seven,
        FlippedSeven
    };
    
    enum class Profile
    {
        Main,
        Side
    };

    PoseCapture(QObject *parent=nullptr);
    ~PoseCapture();
    State state();
    Profile profile();
    bool isMainTrackCaptured();
    bool isRightHandTrackCaptured();
    bool isLeftHandTrackCaptured();
    
signals:
    void stateChanged(State state);
    void trackChanged(Track track, std::vector<qint64> timeline);
    
public slots:
    void updateKeypoints(const std::map<QString, QVector3D> &keypoints, const QSizeF &imageSize);
    
private slots:
    void handlePreEnterTimeout();
    void handleCapturingTimeout();
    void checkState();
    
private:
    State m_state = State::Idle;
    using Keypoints = std::map<QString, QVector3D>;
    Profile m_profile = Profile::Main;
    Track m_latestMainTrack;
    Track m_latestRightHandSideTrack;
    Track m_latestLeftHandSideTrack;
    Track *m_currentCapturingTrack = nullptr;
    std::vector<qint64> m_currentCapturingTimeline;
    std::vector<qint64> m_latestMainTimeline;
    QElapsedTimer m_elapsedTimer;
    qint64 m_stateBeginTime = 0;
    QTimer m_stateCheckTimer;
    std::vector<Keypoints> m_capturedKeypoints;
    std::vector<qint64> m_capturedKeypointsTimeline;
    
    InvokePose keypointsToInvokePose(const Keypoints &keypoints);
    bool isLimbStraightAndParallelWith(const Keypoints &keypoints,
        const QString &namePrefix, const QVector3D &referenceDirection);
    bool isLimbEndParallelWith(const Keypoints &keypoints,
        const QString &namePrefix, const QVector3D &referenceDirection);
    bool isTwoQVector3DParallel(const QVector3D &first, const QVector3D &second);
    void updateFromKeypointsToAnimalPoserParameters(const std::map<QString, QVector3D> &keypoints,
        std::map<QString, std::map<QString, QString>> &parameters, const QString &paramName,
        const QString &fromName, const QString &toName);
    void updateSpineKeypointsToAnimalPoseParameters(const std::map<QString, QVector3D> &keypoints,
        std::map<QString, std::map<QString, QString>> &parameters);
    void keypointsToAnimalPoserParameters(const std::map<QString, QVector3D> &keypoints,
        std::map<QString, std::map<QString, QString>> &parameters);
    void updateQVector3DsToAnimalPoserParameters(const QVector3D &from, const QVector3D &to,
        std::map<QString, QString> &parameters);
    void mergeProfileTracks(const Track &main, const Track &rightHand, const Track &leftHand,
        const std::vector<qint64> &timeline,
        Track &resultTrack, std::vector<qint64> &resultTimeline);
    void cleanupCurrentTrack();
    void commitCurrentTrack();
    void addFrameToCurrentTrack(qint64 timestamp, const std::map<QString, std::map<QString, QString>> &parameters);
    void smoothKeypointsList(std::vector<Keypoints> &keypointsList);
    void smoothQVector3DList(std::vector<QVector3D> &vectors);
    void smoothList(std::vector<float> &numbers);
    void reduceFramesOfTrack(PoseCapture::Track &resultTrack, std::vector<qint64> &resultTimeline);
};

#endif

#endif
