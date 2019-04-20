#ifndef DUST3D_POSE_CAPTURE_H
#define DUST3D_POSE_CAPTURE_H
#include <QObject>
#include <QVector3D>
#include <QTimer>

#if USE_MOCAP

class PoseCapture : public QObject
{
    Q_OBJECT
public:
    using Track = std::vector<std::map<QString, std::map<QString, QString>>>;
    
    static const int PreEnterDuration;
    static const int CapturingDuration;

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
        Seven
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
    
signals:
    void stateChanged(State state);
    void trackMerged(Track track);
    
public slots:
    void updateKeypoints(const std::map<QString, QVector3D> &keypoints);
    
private slots:
    void handlePreEnterTimeout();
    void handleCapturingTimeout();
    
private:
    State m_state = State::Idle;
    QTimer *m_preEnterTimer = nullptr;
    QTimer *m_capturingTimer = nullptr;
    using Keypoints = std::map<QString, QVector3D>;
    Profile m_profile = Profile::Main;
    Track m_latestMainTrack;
    Track m_latestSideTrack;
    
    InvokePose keypointsToInvokePose(const Keypoints &keypoints);
    bool isLimbStraightAndParallelWith(const Keypoints &keypoints,
        const QString &namePrefix, const QVector3D &referenceDirection);
    bool isTwoQVector3DParallel(const QVector3D &first, const QVector3D &second);
    void updateFromKeypointsToAnimalPoserParameters(const std::map<QString, QVector3D> &keypoints,
        std::map<QString, std::map<QString, QString>> &parameters, const QString &paramName,
        const QString &fromName, const QString &toName);
    void keypointsToAnimalPoserParameters(const std::map<QString, QVector3D> &keypoints,
        std::map<QString, std::map<QString, QString>> &parameters);
    void mergeProfileTracks(const Track &main, const Track &side, Track &result);
};

#endif

#endif
