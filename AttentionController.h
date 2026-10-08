#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QVector>

class AttentionController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(bool calibrated READ calibrated NOTIFY calibratedChanged)
    Q_PROPERTY(bool calibrating READ calibrating NOTIFY calibratingChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString screenTimeText READ screenTimeText NOTIFY totalsChanged)
    Q_PROPERTY(QString awayTimeText READ awayTimeText NOTIFY totalsChanged)
    Q_PROPERTY(QString currentAwayText READ currentAwayText NOTIFY currentAwayChanged)
    Q_PROPERTY(QString privacyState READ privacyState NOTIFY privacyStateChanged)

public:
    explicit AttentionController(QObject *parent = nullptr);

    bool running() const;
    bool calibrated() const;
    bool calibrating() const;
    QString statusText() const;
    QString screenTimeText() const;
    QString awayTimeText() const;
    QString currentAwayText() const;
    QString privacyState() const;

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void calibrateScreen();
    Q_INVOKABLE void resetTotals();
    Q_INVOKABLE void submitGaze(double irisX, double irisY, double headX, double headY,
                                bool faceDetected);
    Q_INVOKABLE void reportError(const QString &message);
    Q_INVOKABLE void trackerReady();
    Q_INVOKABLE void dismissPrivacyScreen();
    Q_INVOKABLE void setSessionLocked(bool locked);

signals:
    void startRequested();
    void stopRequested();
    void sessionLockChanged(bool locked);
    void runningChanged();
    void calibratedChanged();
    void calibratingChanged();
    void statusTextChanged();
    void totalsChanged();
    void currentAwayChanged();
    void privacyStateChanged();

private:
    struct GazeSample {
        double irisX;
        double irisY;
        double headX;
        double headY;
    };

    void setStatus(const QString &status);
    void setPrivacyState(const QString &state);
    void resetGazeState();
    void processGaze(bool isAway, qint64 nowMs);
    QString formatTime(qint64 milliseconds) const;

    bool m_running = false;
    bool m_calibrated = false;
    bool m_calibrating = false;
    bool m_locked = false;
    bool m_trackerReady = false;
    QString m_statusText = QStringLiteral("Start the camera, then calibrate screen center.");
    QString m_privacyState = QStringLiteral("hidden");
    GazeSample m_baseline{};
    QVector<GazeSample> m_calibrationSamples;
    QElapsedTimer m_clock;
    qint64 m_lastSampleMs = -1;
    qint64 m_awaySinceMs = -1;
    qint64 m_onScreenSinceMs = -1;
    qint64 m_screenTimeMs = 0;
    qint64 m_awayTimeMs = 0;
    qint64 m_currentAwayMs = 0;
};
