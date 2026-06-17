#include "mainwindow.h"
#include "videowidget.h"
#include "optionsdialog.h"
#include "version.h"

#include <QApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QSplitter>
#include <QTabWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QAction>
#include <QIcon>
#include <QMessageBox>
#include <QCloseEvent>
#include <QTranslator>
#include <QSettings>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>


#include "DeviceSelectionDialog.h"
#include "PropertyDialog.h"
#include "fits/fitssaver.h"

// ============================================================
//  Constructor / Destructor
// ============================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_settings(QApplication::organizationName(), QApplication::applicationName())
{
    setWindowTitle(tr(APP_NAME));
    setMinimumSize(900, 600);

    createActions();
    createMenuBar();
    createToolBar();
    createCentralWidget();
    createStatusBar();

    auto appDataDirectory = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QString apppath = QDir::cleanPath(appDataDirectory +
        QDir::separator() +
        "ic-astronomy-capture");
    QDir(appDataDirectory).mkdir(apppath);
    _devicefile += apppath.toStdString() + "/ic4-astronomy.json";

    try
    {
        WId wid = m_videoWidget->winId();
        _display = ic4::Display::create(ic4::DisplayType::Default, (ic4::WindowHandle)wid);
        _display->setRenderPosition(ic4::DisplayRenderPosition::StretchCenter);
    }
    catch (const ic4::IC4Exception& ex)
    {
        QMessageBox::information(this, {}, ex.what());
    }

    readSettings();

    m_lblCameraStatus->setText(tr("No camera connected"));

    // Create the sink for accessing images.
    _queuesink = ic4::QueueSink::create( *this );

    if (QFileInfo::exists(_devicefile.c_str()))
    {
        ic4::Error err;

        // Try to load the last used device.
        if (!_grabber.deviceOpenFromState(_devicefile, err))
        {
            auto message = tr("Loading last used device failed: ") + QString::fromStdString(err.message());
            QMessageBox::information(this, {}, message);
        }
        else
        {
            startstopstream();
        }
    }

}

MainWindow::~MainWindow()
{
    //writeSettings();
}

// ============================================================
//  UI construction
// ============================================================

void MainWindow::createActions()
{
    // Camera actions
    m_actSelectCamera = new QAction(QIcon(":/icons/camera_select.png"), tr("Select Camera"), this);
    m_actSelectCamera->setStatusTip(tr("Open camera selection dialog"));
    connect(m_actSelectCamera, &QAction::triggered, this, &MainWindow::onSelectCamera);

    m_actCameraConfig = new QAction(QIcon(":/icons/camera_config.png"), tr("Camera Configuration"), this);
    m_actCameraConfig->setStatusTip(tr("Open camera configuration dialog"));
    connect(m_actCameraConfig, &QAction::triggered, this, &MainWindow::onCameraConfiguration);

    // Options
    m_actOptions = new QAction(QIcon(":/icons/options.png"), tr("Options"), this);
    m_actOptions->setStatusTip(tr("Open application options"));
    connect(m_actOptions, &QAction::triggered, this, &MainWindow::onOptions);

    // About
    m_actAbout = new QAction(QIcon(":/icons/about.png"), tr("About"), this);
    m_actAbout->setStatusTip(tr("About " APP_NAME));
    connect(m_actAbout, &QAction::triggered, this, &MainWindow::onAbout);

    // Quit
    m_actQuit = new QAction(QIcon(":/icons/quit.png"), tr("Quit"), this);
    m_actQuit->setShortcut(QKeySequence::Quit);
    m_actQuit->setStatusTip(tr("Quit the application"));
    connect(m_actQuit, &QAction::triggered, this, &QMainWindow::close);
}

void MainWindow::createMenuBar()
{
    // File
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_actOptions);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actQuit);

    // Camera
    QMenu *cameraMenu = menuBar()->addMenu(tr("&Camera"));
    cameraMenu->addAction(m_actSelectCamera);
    cameraMenu->addAction(m_actCameraConfig);

    // Help
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(m_actAbout);
}

void MainWindow::createToolBar()
{
    m_toolBar = addToolBar(tr("Main Toolbar"));
    m_toolBar->setObjectName("mainToolBar");
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    m_toolBar->addAction(m_actSelectCamera);
    m_toolBar->addAction(m_actCameraConfig);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_actOptions);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_actQuit);
}

void MainWindow::createStatusBar()
{
    m_lblCameraStatus  = new QLabel(this);
    m_lblCaptureStatus = new QLabel(this);

    statusBar()->addWidget(m_lblCameraStatus, 1);
    statusBar()->addPermanentWidget(m_lblCaptureStatus);
}

void MainWindow::createCentralWidget()
{
    // ---------- Left: live video ----------
    m_videoWidget = new VideoWidget(this);
    m_videoWidget->setMinimumSize(640, 480);
    m_videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // ---------- Right: tab panel ----------
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setMinimumWidth(300);
    m_tabWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    m_tabWidget->addTab(createCaptureTab(), tr("Capture"));
    m_tabWidget->addTab(createCameraTab(),  tr("Camera"));

    // ---------- Splitter ----------
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_videoWidget);
    splitter->addWidget(m_tabWidget);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    setCentralWidget(splitter);
}

QWidget* MainWindow::createCaptureTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(tab);

    // --- Project group ---
    QGroupBox *grpProject = new QGroupBox(tr("Project"), tab);
    QFormLayout *frmProject = new QFormLayout(grpProject);

    m_editSaveDir = new QLineEdit(grpProject);
    frmProject->addRow(tr("Image Directory:"), m_editSaveDir);

    m_editProjectName = new QLineEdit(grpProject);
    m_editProjectName->setPlaceholderText(tr("e.g. M42_Orion_Nebula"));
    frmProject->addRow(tr("Project Name:"), m_editProjectName);

    m_editUserName = new QLineEdit(grpProject);
    m_editUserName->setPlaceholderText(tr("Observer name"));
    frmProject->addRow(tr("User Name:"), m_editUserName);

    m_editTelescope = new QLineEdit(grpProject);
    m_editTelescope->setPlaceholderText(tr("e.g. Celestron C8"));
    frmProject->addRow(tr("Telescope:"), m_editTelescope);

    // --- Location group ---
    QGroupBox *grpLocation = new QGroupBox(tr("Location"), tab);
    QFormLayout *frmLocation = new QFormLayout(grpLocation);

    m_spinLatitude = new QDoubleSpinBox(grpLocation);
    m_spinLatitude->setRange(-90.0, 90.0);
    m_spinLatitude->setDecimals(6);
    m_spinLatitude->setSuffix(tr(" °"));
    frmLocation->addRow(tr("Latitude:"), m_spinLatitude);

    m_spinLongitude = new QDoubleSpinBox(grpLocation);
    m_spinLongitude->setRange(-180.0, 180.0);
    m_spinLongitude->setDecimals(6);
    m_spinLongitude->setSuffix(tr(" °"));
    frmLocation->addRow(tr("Longitude:"), m_spinLongitude);

    // --- Capture group ---
    QGroupBox *grpCapture = new QGroupBox(tr("Image Capture"), tab);
    QFormLayout *frmCapture = new QFormLayout(grpCapture);

    m_spinNumImages = new QSpinBox(grpCapture);
    m_spinNumImages->setRange(1, 9999);
    m_spinNumImages->setValue(10);
    frmCapture->addRow(tr("Number of Images:"), m_spinNumImages);

    m_btnCapture = new QPushButton(QIcon(":/icons/capture.png"), tr("Start Capture"), grpCapture);
    m_btnCapture->setMinimumHeight(36);
    connect(m_btnCapture, &QPushButton::clicked, this, &MainWindow::onStartCapture);
    frmCapture->addRow(m_btnCapture);

    mainLayout->addWidget(grpProject);
    mainLayout->addWidget(grpLocation);
    mainLayout->addWidget(grpCapture);
    mainLayout->addStretch();

    return tab;
}

QWidget* MainWindow::createCameraTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(tab);

    // --- Exposure & Gain group ---
    QGroupBox *grpCamera = new QGroupBox(tr("Camera Settings"), tab);
    QFormLayout *frmCamera = new QFormLayout(grpCamera);

    m_spinExposure = new QSpinBox(grpCamera);
    m_spinExposure->setRange(1, 60000);
    m_spinExposure->setValue(100.0);
    m_spinExposure->setSuffix(tr(" µs"));
    m_spinExposure->setSingleStep(10.0);
    connect(m_spinExposure, &QSpinBox::valueChanged, this, &MainWindow::onExposureChanged);
    
    frmCamera->addRow(tr("Exposure:"), m_spinExposure);

    m_spinGain = new QDoubleSpinBox(grpCamera);
    m_spinGain->setRange(0.0, 48.0);
    m_spinGain->setDecimals(1);
    m_spinGain->setValue(0.0);
    m_spinGain->setSuffix(tr(" dB"));
    m_spinGain->setSingleStep(0.5);
    connect(m_spinGain, &QDoubleSpinBox::valueChanged, this, &MainWindow::onGainChanged);

    frmCamera->addRow(tr("Gain:"), m_spinGain);

    // --- Flat field group ---
    QGroupBox *grpFlat = new QGroupBox(tr("Flat Field"), tab);
    QVBoxLayout *layFlat = new QVBoxLayout(grpFlat);

    QLabel *lblFlatInfo = new QLabel(
        tr("Point the telescope at a uniform light source,\n"
           "then press the button to capture flat field frames."),
        grpFlat);
    lblFlatInfo->setWordWrap(true);

    m_btnFlatField = new QPushButton(QIcon(":/icons/flat_field.png"), tr("Capture Flat Field"), grpFlat);
    m_btnFlatField->setMinimumHeight(36);
    connect(m_btnFlatField, &QPushButton::clicked, this, &MainWindow::onFlatFieldCapture);

    layFlat->addWidget(lblFlatInfo);
    layFlat->addWidget(m_btnFlatField);

    mainLayout->addWidget(grpCamera);
    mainLayout->addWidget(grpFlat);
    mainLayout->addStretch();

    return tab;
}

// ============================================================
//  Slots
// ============================================================

void MainWindow::onSelectCamera()
{
    DeviceSelectionDialog cDlg(this, &_grabber);
    if (cDlg.exec() == 1)
    {
        if (_grabber.isDeviceValid())
        {
            _grabber.deviceSaveState(_devicefile);
        }

        // Remember the device's property map for later use
        _devicePropertyMap = _grabber.devicePropertyMap(ic4::Error::Ignore());

        startstopstream();
    }
}

void MainWindow::onCameraConfiguration()
{
    PropertyDialog cDlg(_grabber, this, tr("Device Properties"));
    if (cDlg.exec() == 1)
    {
        _grabber.deviceSaveState(_devicefile);
    }
}

void MainWindow::onOptions()
{
    OptionsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        // Language change requires restart
        QString lang = dlg.selectedLanguage();
        m_settings.setValue("options/language", lang);
        QMessageBox::information(this, tr("Language Changed"),
            tr("The language change will take effect after restarting the application."));
    }
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("About ") + APP_NAME,
        QString("<b>%1</b> v%2<br><br>"
                "%3<br><br>"
                "License: %4<br>"
                "<a href='%5'>%5</a>")
            .arg(APP_NAME)
            .arg(APP_VERSION)
            .arg(APP_DESCRIPTION)
            .arg(APP_LICENSE)
            .arg(APP_URL));
}

void MainWindow::onStartCapture()
{
    // Validate inputs
    if (m_editProjectName->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Capture"), tr("Please enter a project name."));
        return;
    }

    // TODO: Implement capture sequence using IC Imaging Control 4 API
    // Example flow:
    //   1. Set up ic4::SnapSink or ic4::QueueSink
    //   2. Start grabber stream
    //   3. Capture m_spinNumImages->value() frames
    //   4. Save with project metadata

    _savedImageCounter = m_spinNumImages->value();

    //m_lblCaptureStatus->setText(tr("Capturing %1 image(s) …").arg(numImages));
    //QMessageBox::information(this, tr("Capture"),
    //    tr("Capture sequence for %1 image(s) would start here.\n\n"
    //       "Project : %2\nUser    : %3\nTelescope: %4\nLat/Lon : %5 / %6")
    //        .arg(numImages)
    //        .arg(m_editProjectName->text())
    //        .arg(m_editUserName->text())
    //        .arg(m_editTelescope->text())
    //        .arg(m_spinLatitude->value())
    //        .arg(m_spinLongitude->value()));
    //m_lblCaptureStatus->setText(tr("Idle"));
}

void MainWindow::onFlatFieldCapture()
{
    // TODO: Implement flat field capture using IC Imaging Control 4 API
    QMessageBox::information(this, tr("Flat Field Capture"),
        tr("Flat field capture sequence will be implemented here.\n\n"
           "Exposure: %1 ms  |  Gain: %2 dB")
            .arg(m_spinExposure->value())
            .arg(m_spinGain->value()));
}

void MainWindow::retranslateUi()
{
    setWindowTitle(tr(APP_NAME));
    // Tab titles
    m_tabWidget->setTabText(0, tr("Capture"));
    m_tabWidget->setTabText(1, tr("Camera"));
    // Action texts
    m_actSelectCamera->setText(tr("Select Camera"));
    m_actCameraConfig->setText(tr("Camera Configuration"));
    m_actOptions->setText(tr("Options"));
    m_actAbout->setText(tr("About"));
    m_actQuit->setText(tr("Quit"));
}

// ============================================================
//  Settings persistence
// ============================================================

void MainWindow::readSettings()
{
    m_settings.beginGroup("mainwindow");
    restoreGeometry(m_settings.value("geometry").toByteArray());
    restoreState(m_settings.value("state").toByteArray());
    m_settings.endGroup();

    m_settings.beginGroup("capture");
  
    m_editSaveDir->setText(m_settings.value("saveDir").toString());
    m_editProjectName->setText(m_settings.value("projectName").toString());
    m_editUserName->setText(m_settings.value("userName").toString());
    m_editTelescope->setText(m_settings.value("telescope").toString());
    m_spinLatitude->setValue(m_settings.value("latitude", 0.0).toDouble());
    m_spinLongitude->setValue(m_settings.value("longitude", 0.0).toDouble());
    m_spinNumImages->setValue(m_settings.value("numImages", 10).toInt());
    m_settings.endGroup();

    if (m_editSaveDir->text() == "")
    {
        m_editSaveDir->setText(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    }

    m_settings.beginGroup("camera");
    m_spinExposure->setValue(m_settings.value("exposure", 100.0).toDouble());
    m_spinGain->setValue(m_settings.value("gain", 0.0).toDouble());
    m_settings.endGroup();
}

void MainWindow::writeSettings()
{
    m_settings.beginGroup("mainwindow");
    m_settings.setValue("geometry", saveGeometry());
    m_settings.setValue("state", saveState());
    m_settings.endGroup();

    m_settings.beginGroup("capture");
    qDebug() << m_editSaveDir->text();
    m_settings.setValue("saveDir",    m_editSaveDir->text());
    m_settings.setValue("projectName", m_editProjectName->text());
    m_settings.setValue("userName",    m_editUserName->text());
    m_settings.setValue("telescope",   m_editTelescope->text());
    m_settings.setValue("latitude",    m_spinLatitude->value());
    m_settings.setValue("longitude",   m_spinLongitude->value());
    m_settings.setValue("numImages",   m_spinNumImages->value());
    m_settings.endGroup();

    m_settings.beginGroup("camera");
    m_settings.setValue("exposure", m_spinExposure->value());
    m_settings.setValue("gain",     m_spinGain->value());
    m_settings.endGroup();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (_grabber.isStreaming())
    {
        _grabber.streamStop();
        _grabber.deviceClose();
    }

    writeSettings();
    event->accept();
}

/// <summary>
/// Start and stop the live video stream
/// </summary>
void MainWindow::startstopstream()
{
    try
    {
        if (_grabber.isDeviceValid())
        {
            if (_grabber.isStreaming())
            {
                _grabber.streamStop();
            }
            else
            {
                _grabber.streamSetup(_queuesink);
                auto map = _grabber.devicePropertyMap();
                map.setValue(ic4::PropId::TriggerMode, "Off");
                map.setValue(ic4::PropId::GainAuto, "Off");
                map.setValue(ic4::PropId::ExposureAuto, "Off");


                auto exposure = map.find(ic4::PropId::ExposureTime);
                m_spinExposure->blockSignals(true);
                m_spinExposure->setRange(exposure.minimum(), exposure.maximum());
                m_spinExposure->setValue(exposure.getValue());
                m_spinExposure->blockSignals(false);
                auto gain = map.find(ic4::PropId::Gain);

                m_spinGain->blockSignals(true);
                m_spinGain->setRange(gain.minimum(), gain.maximum());
                m_spinGain->setValue(gain.getValue());
                m_spinGain->blockSignals(false);
            }
        }
    }
    catch (ic4::IC4Exception ex)
    {
        QMessageBox::warning(this, {}, ex.what());
    }
}

void MainWindow::onExposureChanged(int value)
{
    if (_grabber.isDeviceValid())
    {
        _grabber.devicePropertyMap().setValue(ic4::PropId::ExposureTime, value);
    }
}

void MainWindow::onGainChanged(double value)
{
    if (_grabber.isDeviceValid())
    {
        _grabber.devicePropertyMap().setValue(ic4::PropId::Gain, value);
    }
}


// <summary>
/// Listener related. Allocate image buffers.
/// </summary>
/// <param name="sink"></param>
/// <param name="imageType"></param>
/// <param name="min_buffers_required"></param>
/// <returns></returns>
bool MainWindow::sinkConnected(ic4::QueueSink& sink, const ic4::ImageType& imageType, size_t min_buffers_required)
{
    // Allocate more buffers than suggested, because we temporarily take some buffers
    // out of circulation when saving an image or video files.
    sink.allocAndQueueBuffers(min_buffers_required + 2);
    return true;
};



/// <summary>
/// Listener related: Callback for new frames.
/// </summary>
/// <param name="sink"></param>
void MainWindow::framesQueued(ic4::QueueSink& sink)
{
    ic4::Error err;
    auto buffer = sink.popOutputBuffer(err);
    if (buffer)
    {
        if (_savedImageCounter > 0)
        {
            _savedImageCounter--;
            FitsMetadata meta;
            meta.cameraModel = QString::fromStdString(_grabber.deviceInfo().modelName());
            meta.exposureTime = _grabber.devicePropertyMap().getValueDouble(ic4::PropId::ExposureTime);
            meta.gain = _grabber.devicePropertyMap().getValueDouble(ic4::PropId::Gain);
            meta.telescope = m_editTelescope->text();
            meta.username = m_editUserName->text();

            QString filename = createImageFileName();
            auto x = filename.toLocal8Bit().data();

            FitsSaver::save(filename, *buffer, meta);
        }

        _display->displayBuffer(buffer);
    }

    return;
}

QString MainWindow::createImageFileName()
{
    QString dirname =QDir::cleanPath( m_editSaveDir->text() +
        QDir::separator() +
        m_editProjectName->text());
    QDir().mkdir(dirname);

    QString filename;
    int filecounter = 0;
    do
    {
        filename = QDir::cleanPath(dirname + QDir::separator() +
            m_editProjectName->text() + QString("_%1.fits").arg(filecounter, 5, 10, '0'));
        filecounter++;
    } while (QFileInfo::exists(filename));
    return filename;
}