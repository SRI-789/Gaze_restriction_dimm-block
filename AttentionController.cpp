#include "AttentionController.h"

#include <QtGlobal>
#include <algorithm>
#include <cmath>

namespace {
constexpr int CalibrationSampleCount = 45;
constexpr qint64 MaximumCountedFrameGapMs = 250;
constexpr qint64 DimDelayMs = 3000;
constexpr qint64 RestrictDelayMs = 10000;
constexpr qint64 OnScreenResetGraceMs = 300;
constexpr double HorizontalLimit = 0.10;
constexpr double VerticalLimit = 0.10;
}

AttentionController::AttentionController(QObject *parent)
    : QObject(parent)
{
    m_clock.start();
}

bool AttentionController::running() const { return m_running; }
bool AttentionController::calibrated() const { return m_calibrated; }
bool AttentionController::calibrating() const { return m_calibrating; }
QString AttentionController::statusText() const { return m_statusText; }
QString AttentionController::screenTimeText() const { return formatTime(m_screenTimeMs); }
QString AttentionController::awayTimeText() const { return formatTime(m_awayTimeMs); }
QString AttentionController::currentAwayText() const
{
    return QStringLiteral("%1.%2 s").arg(m_currentAwayMs / 1000).arg((m_currentAwayMs % 1000) / 100, 1, 10, QLatin1Char('0'));
}
QString AttentionController::privacyState() const { return m_privacyState; }

void AttentionController::start()
{
    if (m_running)
        return;

    m_running = true;
    m_locked = false;
    emit runningChanged();
    setStatus(QStringLiteral("Loading face tracker and requesting camera access..."));
    if (m_trackerReady)
        emit startRequested();
}

void AttentionController::stop()
{
    if (!m_running)
        return;

    emit stopRequested();
    m_running = false;
    m_calibrated = false;
    m_calibrating = false;
    m_calibrationSamples.clear();
    resetGazeState();
    emit runningChanged();
    emit calibratedChanged();
    emit calibratingChanged();
    emit currentAwayChanged();
    setStatus(QStringLiteral("Camera stopped."));
}

void AttentionController::calibrateScreen()
{
    if (!m_running || m_locked)
        return;

    m_calibrated = false;
    m_calibrating = true;
    m_calibrationSamples.clear();
    resetGazeState();
    emit calibratedChanged();
    emit calibratingChanged();
    emit currentAwayChanged();
    setStatus(QStringLiteral("Look at the center of the screen for about two seconds..."));
}

void AttentionController::resetTotals()
{
    m_screenTimeMs = 0;
    m_awayTimeMs = 0;
    emit totalsChanged();
}

void AttentionController::reportError(const QString &message)
{
    if (m_running) {
        emit stopRequested();
        m_running = false;
        m_calibrated = false;
        m_calibrating = false;
        resetGazeState();
        emit runningChanged();
        emit calibratedChanged();
        emit calibratingChanged();
        emit currentAwayChanged();
    }
    setStatus(message);
}

void AttentionController::trackerReady()
{
    m_trackerReady = true;
    if (m_running)
        emit startRequested();
}

void AttentionController::dismissPrivacyScreen()
{
    setPrivacyState(QStringLiteral("hidden"));
}

void AttentionController::setSessionLocked(bool locked)
{
    if (m_locked == locked)
        return;

    m_locked = locked;
    resetGazeState();
    emit currentAwayChanged();
    emit sessionLockChanged(m_locked);

    if (m_locked) {
        setStatus(QStringLiteral("Paused while Windows is locked."));
    } else if (m_running && m_calibrated) {
        setStatus(QStringLiteral("Unlocked. Tracking resumed."));
    }
}

void AttentionController::submitGaze(double irisX, double irisY, double headX, double headY,
                                     bool faceDetected)
{
    if (!m_running || m_locked)
        return;

    if (!faceDetected) {
        resetGazeState();
        emit currentAwayChanged();
        if (m_calibrating)
            setStatus(QStringLiteral("Face not detected. Keep your face visible while calibrating."));
        else
            setStatus(QStringLiteral("Face not detected."));
        return;
    }

    const GazeSample sample{
        .irisX = irisX,
        .irisY = irisY,
        .headX = headX,
        .headY = headY
    };
    if (m_calibrating) {
        m_calibrationSamples.append(sample);
        if (m_calibrationSamples.size() < CalibrationSampleCount) {
            setStatus(QStringLiteral("Calibrating screen center: %1 / %2")
                          .arg(m_calibrationSamples.size())
                          .arg(CalibrationSampleCount));
            return;
        }

        GazeSample sum{};
        for (const auto &item : m_calibrationSamples) {
            sum.irisX += item.irisX;
            sum.irisY += item.irisY;
            sum.headX += item.headX;
            sum.headY += item.headY;
        }
        const double count = static_cast<double>(m_calibrationSamples.size());
        m_baseline = {
            .irisX = sum.irisX / count,
            .irisY = sum.irisY / count,
            .headX = sum.headX / count,
            .headY = sum.headY / count
        };
        m_calibrated = true;
        m_calibrating = false;
        m_lastSampleMs = -1;
        emit calibratedChanged();
        emit calibratingChanged();
        setStatus(QStringLiteral("Screen calibrated. Looking down or above the display counts as away."));
        return;
    }

    if (!m_calibrated)
        return;

    const double horizontal = (sample.irisX - m_baseline.irisX)
        + 0.35 * (sample.headX - m_baseline.headX);
    const double vertical = (sample.irisY - m_baseline.irisY)
        + 0.35 * (sample.headY - m_baseline.headY);
    const bool isAway = std::abs(horizontal) > HorizontalLimit
        || std::abs(vertical) > VerticalLimit;
    processGaze(isAway, m_clock.elapsed());
}

void AttentionController::processGaze(bool isAway, qint64 nowMs)
{
    if (m_lastSampleMs >= 0) {
        const qint64 elapsed = std::min(nowMs - m_lastSampleMs, MaximumCountedFrameGapMs);
        if (isAway)
            m_awayTimeMs += elapsed;
        else
            m_screenTimeMs += elapsed;
        emit totalsChanged();
    }
    m_lastSampleMs = nowMs;

    if (isAway) {
        m_onScreenSinceMs = -1;
        if (m_awaySinceMs < 0)
            m_awaySinceMs = nowMs;
    } else if (m_awaySinceMs < 0) {
        m_currentAwayMs = 0;
        setPrivacyState(QStringLiteral("hidden"));
        setStatus(QStringLiteral("Looking at screen."));
        emit currentAwayChanged();
        return;
    } else {
        if (m_onScreenSinceMs < 0)
            m_onScreenSinceMs = nowMs;
        if (nowMs - m_onScreenSinceMs >= OnScreenResetGraceMs) {
            m_awaySinceMs = -1;
            m_onScreenSinceMs = -1;
            m_currentAwayMs = 0;
            setPrivacyState(QStringLiteral("hidden"));
            setStatus(QStringLiteral("Looking at screen."));
            emit currentAwayChanged();
            return;
        }
    }

    m_currentAwayMs = nowMs - m_awaySinceMs;
    emit currentAwayChanged();
    if (m_currentAwayMs >= RestrictDelayMs) {
        setPrivacyState(QStringLiteral("restricted"));
        setStatus(isAway ? QStringLiteral("Looking away — restricted screen.")
                         : QStringLiteral("Confirming return to screen."));
    } else if (m_currentAwayMs >= DimDelayMs) {
        setPrivacyState(QStringLiteral("warning"));
        setStatus(isAway ? QStringLiteral("Looking away — screen dimmed.")
                         : QStringLiteral("Confirming return to screen."));
    } else {
        setStatus(isAway ? QStringLiteral("Possibly looking away.")
                         : QStringLiteral("Confirming return to screen."));
    }
}

void AttentionController::resetGazeState()
{
    m_lastSampleMs = -1;
    m_awaySinceMs = -1;
    m_onScreenSinceMs = -1;
    m_currentAwayMs = 0;
    setPrivacyState(QStringLiteral("hidden"));
}

void AttentionController::setStatus(const QString &status)
{
    if (m_statusText == status)
        return;
    m_statusText = status;
    emit statusTextChanged();
}

void AttentionController::setPrivacyState(const QString &state)
{
    if (m_privacyState == state)
        return;
    m_privacyState = state;
    emit privacyStateChanged();
}

QString AttentionController::formatTime(qint64 milliseconds) const
{
    const qint64 totalSeconds = milliseconds / 1000;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 seconds = totalSeconds % 60;
    return QStringLiteral("%1:%2:%3")
        .arg(hours, 2, 10, QLatin1Char('0'))
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}
