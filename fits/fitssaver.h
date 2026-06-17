#pragma once

#include <ic4/ic4.h>
#include <QString>
#include <QDateTime>
#include <vector>

// ============================================================
//  Metadaten für einen einzelnen Frame
// ============================================================
struct FitsMetadata
{
    QString   cameraModel;
    QDateTime dateTime;
    double    longitude    = 0.0;   // Beobachterposition
    double    latitude     = 0.0;
    int       exposureTime = 0;     // microsekunden
    double    gain         = 0.0;
    int       offset       = 0;
    QString   bayerPattern;         // "RGGB","BGGR","GRBG","GBRG" – leer = kein Bayer
    QString   frameType;            // "Light", "Dark", "Flat", "Bias"
    QString   telescope;
    QString   username;
};

// ============================================================
//  FitsSaver
//
//  Drei öffentliche Methoden:
//    save()       – einzelner Frame  → 2D FITS
//    saveStack()  – Bildstack        → 3D FITS Cube  (gleiche Metadaten)
//    saveHDUs()   – Bildstack        → Multi-HDU FITS (individuelle Metadaten)
// ============================================================
class FitsSaver
{
public:
    /// Einzelnen Frame als 2D FITS speichern
    static bool save(const QString&           filename,
                     const ic4::ImageBuffer&  buffer,
                     const FitsMetadata&      meta);

    /// Bildstack als 3D Cube speichern (alle Frames gleiche Einstellungen)
    /// metas[0] wird als globaler Header verwendet
    static bool saveStack(const QString&                        filename,
                          const std::vector<ic4::ImageBuffer*>& frames,
                          const std::vector<FitsMetadata>&      metas);

    /// Bildstack als separate HDU Extensions speichern
    /// (unterschiedliche Belichtungen, Typen, etc.)
    static bool saveHDUs(const QString&                        filename,
                         const std::vector<ic4::ImageBuffer*>& frames,
                         const std::vector<FitsMetadata>&      metas);

private:
    /// FITS bitpix + CFitsIO datatype aus ic4 PixelFormat bestimmen
    static void pixelFormatToFitsType(ic4::PixelFormat fmt,
                                      int& bitpix,
                                      int& datatype);

    /// Bayer-Pattern aus PixelFormat ermitteln (leer wenn kein Bayer)
    static QString bayerPatternFromFormat(ic4::PixelFormat fmt);

    /// Gemeinsame Metadaten in aktuellen HDU schreiben
    static void writeMetaKeys(void* fptr,        // fitsfile*
                              const FitsMetadata& meta,
                              int frameIndex,
                              int totalFrames,
                              int& status);
};
