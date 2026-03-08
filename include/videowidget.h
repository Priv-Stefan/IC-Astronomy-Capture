#pragma once

#include <QWidget>

/**
 * @brief VideoWidget
 *
 * A plain QWidget used as the render surface for IC Imaging Control 4's
 * live video stream.  No extra class logic is required beyond providing a
 * native window handle (winId()) to the IC4 sink.
 *
 * The widget paints a dark background with a placeholder text when no camera
 * is connected.
 */
class VideoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoWidget(QWidget *parent = nullptr);

    /// Call this when a camera stream is active / inactive so the
    /// placeholder text is shown/hidden.
    void setStreamActive(bool active);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    bool m_streamActive = false;
};
