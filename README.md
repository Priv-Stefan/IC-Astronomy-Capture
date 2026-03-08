# IC-Astronomy-Capture

Astronomy camera capture application built with **Qt** and **IC Imaging Control 4**.

Only [The Imaging Source](https://www.theimagingsource.com) cameras are suppoted

Images are saved in FITS format. 

The Qt user interface creation was supported by Claude AI.

This is the first version, an images are saved in application dir and it is named "test.fits"


## Features

- Live camera preview via IC Imaging Control 4
- Camera selection and configuration dialogs (IC4 native dialogs)
- Capture tab: project name, observer, telescope type, latitude/longitude, image count
- Camera tab: exposure (ms), gain (dB), flat-field capture
- Multi-language support: **English** and **German** (QTranslator)
- All settings persisted via **QSettings**
- Cross-platform: **Windows** and **Linux**

## Requirements

| Dependency | Version |
|---|---|
| Qt | 6.x (Widgets, LinguistTools) |
| IC Imaging Control 4 https://www.theimagingsource.com/en-us/support/ | 4.x |
| CMake | ≥ 3.16 |
| C++ | C++17 |

## Build

```bash
# Clone
git clone https://github.com/YOUR_USERNAME/IC-Astronomy-Capture.git
cd IC-Astronomy-Capture

# Configure (adjust IC4_ROOT if needed on Windows)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --parallel

# Run
./build/IC-Astronomy-Capture
```



## License

GNU General Public License v3.0 — see [LICENSE](LICENSE) for details.
