#pragma once

#include <QMainWindow>
#include <QSettings>
#include <QLabel>
#include <QTranslator>
#include <ic4/ic4.h> 
#include <ic4-interop/interop-Qt.h>
#include <string>

class QTabWidget;
class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QPushButton;
class QToolBar;
class QStatusBar;
class VideoWidget;


/**
 * @brief The MainWindow class
 *
 * Main application window for IC-Astronomy-Capture.
 * Layout:
 *   Left  – VideoWidget (live camera preview)
 *   Right – QTabWidget
 *             Tab 0: Capture settings (project, user, telescope, location, image count)
 *             Tab 1: Camera settings  (exposure, gain, flat-field capture)
 */
class MainWindow : public QMainWindow, ic4::QueueSinkListener
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // Camera actions
    void onSelectCamera();
    void onCameraConfiguration();

    // Menu actions
    void onOptions();
    void onAbout();

    // Capture tab
    void onStartCapture();

    // Camera tab
    void onFlatFieldCapture();

    // Language change
    void retranslateUi();

private:
    void createActions();
    void createMenuBar();
    void createToolBar();
    void createStatusBar();
    void createCentralWidget();
    QWidget* createCaptureTab();
    QWidget* createCameraTab();
    QString createImageFileName();

    void readSettings();
    void writeSettings();

    void startstopstream();
    bool sinkConnected(ic4::QueueSink& sink, const ic4::ImageType& imageType, size_t min_buffers_required) final;
    void framesQueued(ic4::QueueSink& sink) final;

    void onExposureChanged(int value);
    void onGainChanged(double value);


    // ---- Actions ----
    QAction *m_actSelectCamera    = nullptr;
    QAction *m_actCameraConfig    = nullptr;
    QAction *m_actOptions         = nullptr;
    QAction *m_actAbout           = nullptr;
    QAction *m_actQuit            = nullptr;

    // ---- Toolbar ----
    QToolBar *m_toolBar = nullptr;

    // ---- Central layout ----
    VideoWidget *m_videoWidget = nullptr;
    QTabWidget  *m_tabWidget   = nullptr;

    // ---- Capture tab widgets ----
    QLineEdit    *m_editSaveDir     = nullptr;
    QLineEdit    *m_editProjectName = nullptr;
    QLineEdit    *m_editUserName    = nullptr;
    QLineEdit    *m_editTelescope   = nullptr;
    QDoubleSpinBox *m_spinLatitude  = nullptr;
    QDoubleSpinBox *m_spinLongitude = nullptr;
    QSpinBox     *m_spinNumImages   = nullptr;
    QPushButton  *m_btnCapture      = nullptr;

    // ---- Camera tab widgets ----
    QSpinBox *m_spinExposure  = nullptr;
    QDoubleSpinBox *m_spinGain      = nullptr;
    QPushButton    *m_btnFlatField  = nullptr;

    // ---- Status bar labels ----
    QLabel *m_lblCameraStatus  = nullptr;
    QLabel *m_lblCaptureStatus = nullptr;

    // ---- Settings ----
    QSettings m_settings;

    ic4::PropertyMap _devicePropertyMap;
    ic4::Grabber _grabber;
    std::shared_ptr<ic4::Display> _display;
    std::shared_ptr<ic4::QueueSink> _queuesink;
    std::string _devicefile;    
    int _savedImageCounter = 0;

};
