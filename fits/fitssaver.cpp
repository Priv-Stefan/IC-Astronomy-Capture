#include "fitssaver.h"
#include <fitsio.h>
#include <QDebug>

// ============================================================
//  Private Hilfsfunktionen
// ============================================================

void FitsSaver::pixelFormatToFitsType(ic4::PixelFormat fmt,
                                       int& bitpix,
                                       int& datatype)
{
    switch (fmt)
    {
    // 8-bit Mono und Bayer
    case ic4::PixelFormat::Mono8:
    case ic4::PixelFormat::BayerRG8:
    case ic4::PixelFormat::BayerBG8:
    case ic4::PixelFormat::BayerGR8:
    case ic4::PixelFormat::BayerGB8:
        bitpix   = BYTE_IMG;
        datatype = TBYTE;
        return;

    // 16-bit Mono und Bayer
    case ic4::PixelFormat::Mono16:
    case ic4::PixelFormat::BayerRG16:
    case ic4::PixelFormat::BayerBG16:
    case ic4::PixelFormat::BayerGR16:
    case ic4::PixelFormat::BayerGB16:
        bitpix   = USHORT_IMG;
        datatype = TUSHORT;
        return;

    // 32-bit Mono (float)
    case ic4::PixelFormat::BGRa8:
        bitpix   = LONG_IMG;
        datatype = TUINT;
        return;

    // 64-bit Mono (double)
    case ic4::PixelFormat::BGRa16:
        bitpix   = LONGLONG_IMG;
        datatype = TLONGLONG;
        return;

    // Fallback: 16-bit
    default:
        bitpix   = USHORT_IMG;
        datatype = TUSHORT;
        return;
    }
}

QString FitsSaver::bayerPatternFromFormat(ic4::PixelFormat fmt)
{
    switch (fmt)
    {
    case ic4::PixelFormat::BayerRG8:
    case ic4::PixelFormat::BayerRG16:  return QStringLiteral("RGGB");
    case ic4::PixelFormat::BayerBG8:
    case ic4::PixelFormat::BayerBG16:  return QStringLiteral("BGGR");
    case ic4::PixelFormat::BayerGR8:
    case ic4::PixelFormat::BayerGR16:  return QStringLiteral("GRBG");
    case ic4::PixelFormat::BayerGB8:
    case ic4::PixelFormat::BayerGB16:  return QStringLiteral("GBRG");
    default:                           return QString();
    }
}

void FitsSaver::writeMetaKeys(void*              fptr_void,
                               const FitsMetadata& meta,
                               int                frameIndex,
                               int                totalFrames,
                               int&               status)
{
    fitsfile* fptr = reinterpret_cast<fitsfile*>(fptr_void);

    // Datum / Zeit
    QByteArray dateBA = meta.dateTime.toString(Qt::ISODate).toLocal8Bit();
    fits_write_key(fptr, TSTRING, "DATE-OBS",
                   dateBA.data(), "Observation UTC", &status);

    // Kameramodell
    QByteArray camBA = meta.cameraModel.toLocal8Bit();
    fits_write_key(fptr, TSTRING, "INSTRUME",
                   camBA.data(), "Camera model", &status);

    // Aufnahmeparameter
    fits_write_key(fptr, TDOUBLE, "EXPTIME",
                   const_cast<double*>(&meta.exposureTime),
                   "Exposure time [s]", &status);

    fits_write_key(fptr, TINT, "GAIN",
                   const_cast<int*>(&meta.gain),
                   "Gain", &status);

    fits_write_key(fptr, TINT, "OFFSET",
                   const_cast<int*>(&meta.offset),
                   "Offset", &status);

    // Beobachterposition
    fits_write_key(fptr, TDOUBLE, "LONG-OBS",
                   const_cast<double*>(&meta.longitude),
                   "Observer longitude [deg]", &status);

    fits_write_key(fptr, TDOUBLE, "LAT-OBS",
                   const_cast<double*>(&meta.latitude),
                   "Observer latitude [deg]", &status);

    // Frame-Typ (Light, Dark, Flat, Bias)
    if (!meta.frameType.isEmpty()) {
        QByteArray typeBA = meta.frameType.toLocal8Bit();
        fits_write_key(fptr, TSTRING, "IMAGETYP",
                       typeBA.data(), "Frame type", &status);
    }

    // Bayer-Pattern  – aus Metadaten ODER automatisch aus PixelFormat
    // (meta.bayerPattern hat Vorrang; wird in saveStack/saveHDUs gesetzt)
    if (!meta.bayerPattern.isEmpty()) {
        QByteArray bayerBA = meta.bayerPattern.toLocal8Bit();
        fits_write_key(fptr, TSTRING, "BAYERPAT",
                       bayerBA.data(), "Bayer color pattern", &status);
        int zero = 0;
        fits_write_key(fptr, TINT, "XBAYROFF",
                       &zero, "Bayer X offset", &status);
        fits_write_key(fptr, TINT, "YBAYROFF",
                       &zero, "Bayer Y offset", &status);
    }

    // Stack-Infos (nur wenn mehr als ein Frame)
    if (totalFrames > 1) {
        fits_write_key(fptr, TINT, "NFRAMES",
                       &totalFrames, "Total frames in stack", &status);
        int idx = frameIndex + 1;   // 1-basiert
        fits_write_key(fptr, TINT, "FRAMENUM",
                       &idx, "Frame number (1-based)", &status);
    }
}

// ============================================================
//  save() – einzelner Frame → 2D FITS
// ============================================================
bool FitsSaver::save(const QString&          filename,
                     const ic4::ImageBuffer& buffer,
                     const FitsMetadata&     meta)
{
    fitsfile* fptr  = nullptr;
    int       status = 0;

    auto layout = buffer.imageType();
    long naxes[2] = { (long)layout.width(), (long)layout.height()};

    int bitpix, datatype;
    pixelFormatToFitsType(layout.pixel_format(), bitpix, datatype);

    // Bayer-Pattern automatisch ermitteln falls nicht explizit gesetzt
    FitsMetadata m = meta;
    if (m.bayerPattern.isEmpty())
        m.bayerPattern = bayerPatternFromFormat(layout.pixel_format());

    fits_create_file(&fptr, filename.toLocal8Bit().data(), &status);
    fits_create_img (fptr, bitpix, 2, naxes, &status);

    writeMetaKeys(fptr, m, 0, 1, status);

    long fpixel = 1;
    fits_write_img(fptr, datatype, fpixel,
                   layout.width() * layout.height(),
                   const_cast<void*>(static_cast<const void*>(buffer.ptr())),
                   &status);

    fits_close_file(fptr, &status);

    if (status) {
        fits_report_error(stderr, status);
        return false;
    }
    return true;
}

// ============================================================
//  saveStack() – Bildstack als 3D Cube
//  Alle Frames müssen gleiche Größe und gleiches PixelFormat haben.
//  metas[0] liefert die globalen Header-Infos.
//  Individuelle DATE-OBS pro Slice werden als COMMENT geschrieben.
// ============================================================
bool FitsSaver::saveStack(const QString&                        filename,
                           const std::vector<ic4::ImageBuffer*>& frames,
                           const std::vector<FitsMetadata>&      metas)
{
    if (frames.empty()) return false;
    if (frames.size() != metas.size()) {
        qWarning() << "FitsSaver::saveStack: frames und metas müssen gleich groß sein";
        return false;
    }

    fitsfile* fptr   = nullptr;
    int       status = 0;

    auto layout = frames[0]->imageType();
    long naxes[3] = {
        (long)layout.width(),
        (long)layout.height(),
        (long)frames.size()
    };

    int bitpix, datatype;
    pixelFormatToFitsType(layout.pixel_format(), bitpix, datatype);

    FitsMetadata m0 = metas[0];
    if (m0.bayerPattern.isEmpty())
        m0.bayerPattern = bayerPatternFromFormat(layout.pixel_format());

    fits_create_file(&fptr, filename.toLocal8Bit().data(), &status);
    fits_create_img (fptr, bitpix, 3, naxes, &status);      // NAXIS=3

    // Globaler Header aus Frame 0
    writeMetaKeys(fptr, m0, 0, (int)frames.size(), status);

    // Individuelle Timestamps als COMMENT (können nicht als echte Keywords
    // pro Slice im Cube abgelegt werden)
    for (size_t i = 0; i < metas.size(); i++) {
        QString comment = QString("FRAME %1 DATE-OBS %2")
                            .arg(i + 1)
                            .arg(metas[i].dateTime.toString(Qt::ISODate));
        QByteArray ba = comment.toLocal8Bit();
        fits_write_comment(fptr, ba.data(), &status);
    }

    // Frames sequenziell schreiben
    long pixelsPerFrame = (long)(layout.width() * layout.height());
    for (size_t i = 0; i < frames.size(); i++) {
        long fpixel = 1 + (long)i * pixelsPerFrame;
        fits_write_img(fptr, datatype, fpixel, pixelsPerFrame,
                       const_cast<void*>(
                           static_cast<const void*>(frames[i]->ptr())),
                       &status);
        if (status) {
            qWarning() << "FitsSaver::saveStack: Fehler bei Frame" << i;
            break;
        }
    }

    fits_close_file(fptr, &status);

    if (status) {
        fits_report_error(stderr, status);
        return false;
    }
    return true;
}

// ============================================================
//  saveHDUs() – Bildstack als separate HDU Extensions
//  Jeder Frame bekommt seinen eigenen Header mit vollen Metadaten.
//  Ideal für gemischte Serien (Light/Dark/Flat) oder unterschiedliche
//  Belichtungszeiten.
// ============================================================
bool FitsSaver::saveHDUs(const QString&                        filename,
                          const std::vector<ic4::ImageBuffer*>& frames,
                          const std::vector<FitsMetadata>&      metas)
{
    if (frames.empty()) return false;
    if (frames.size() != metas.size()) {
        qWarning() << "FitsSaver::saveHDUs: frames und metas müssen gleich groß sein";
        return false;
    }

    fitsfile* fptr   = nullptr;
    int       status = 0;

    fits_create_file(&fptr, filename.toLocal8Bit().data(), &status);

    for (size_t i = 0; i < frames.size(); i++) {
        auto layout = frames[i]->imageType();
        long naxes[2] = { (long)layout.width(), (long)layout.height()};

        int bitpix, datatype;
        pixelFormatToFitsType(layout.pixel_format(), bitpix, datatype);

        FitsMetadata m = metas[i];
        if (m.bayerPattern.isEmpty())
            m.bayerPattern = bayerPatternFromFormat(layout.pixel_format());

        // fits_create_img wechselt beim zweiten Aufruf automatisch
        // zur nächsten Extension HDU
        fits_create_img(fptr, bitpix, 2, naxes, &status);

        writeMetaKeys(fptr, m, (int)i, (int)frames.size(), status);

        long fpixel = 1;
        long nPixels = (long)(layout.width() * layout.height());
        fits_write_img(fptr, datatype, fpixel, nPixels,
                       const_cast<void*>(
                           static_cast<const void*>(frames[i]->ptr())),
                       &status);

        if (status) {
            qWarning() << "FitsSaver::saveHDUs: Fehler bei Frame" << i;
            break;
        }
    }

    fits_close_file(fptr, &status);

    if (status) {
        fits_report_error(stderr, status);
        return false;
    }
    return true;
}
