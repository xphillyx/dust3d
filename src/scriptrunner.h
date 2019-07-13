#ifndef DUST3D_SCRIPT_RUNNER_H
#define DUST3D_SCRIPT_RUNNER_H
#include <QObject>
#include "snapshot.h"

class ScriptRunner : public QObject
{
    Q_OBJECT
public:
    ScriptRunner();
    ~ScriptRunner();
    void run();
    void setScript(QString *script);
    void setVariables(std::map<QString, QString> *variables);
    Snapshot *takeResultSnapshot();
    std::map<QString, QString> *takeDefaultVariables();
    const QString &scriptError();
    static void mergeVaraibles(std::map<QString, QString> *target, const std::map<QString, QString> &source);
signals:
    void finished();
public slots:
    void process();
private:
    QString *m_script = nullptr;
    Snapshot *m_resultSnapshot = nullptr;
    std::map<QString, QString> *m_defaultVariables = nullptr;
    std::map<QString, QString> *m_variables = nullptr;
    QString m_scriptError;
};

#endif
