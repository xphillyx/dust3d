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

    PoseCapture(QObject *parent=nullptr);
    ~PoseCapture();
    State state();
    
signals:
    void stateChanged(State state);
    
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
    
    InvokePose keypointsToInvokePose(const Keypoints &keypoints);
    bool isLimbStraightAndParallelWith(const Keypoints &keypoints,
        const QString &namePrefix, const QVector3D &referenceDirection);
    bool isTwoQVector3DParallel(const QVector3D &first, const QVector3D &second);
};

#endif

#endif
