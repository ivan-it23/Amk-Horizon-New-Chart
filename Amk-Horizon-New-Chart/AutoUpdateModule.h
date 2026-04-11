#pragma once

#include "framework.h"
#include "AppVersion.h"
#include <string>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")

// ===================================================================
// AutoUpdateModule — HTTP-based auto-update via ivanbohg.beget.tech
//
// Server layout:
//   http://ivanbohg.beget.tech/FilesForProgramVisualisation/version.txt
//   http://ivanbohg.beget.tech/FilesForProgramVisualisation/<exe_name>
//
// version.txt format (UTF-8, two lines):
//   2.20.0
//   Amk-Horizon-New-Chart.exe
// ===================================================================

namespace AutoUpdate {

    // Parsed version triplet
    struct Version {
        int major = 0;
        int minor = 0;
        int patch = 0;

        bool operator>(const Version& o) const {
            if (major != o.major) return major > o.major;
            if (minor != o.minor) return minor > o.minor;
            return patch > o.patch;
        }
        bool operator==(const Version& o) const {
            return major == o.major && minor == o.minor && patch == o.patch;
        }
    };

    // Result of the update check
    struct UpdateInfo {
        bool available = false;
        Version remoteVersion;
        std::string exeFileName;       // e.g. "Amk-Horizon-New-Chart.exe"
        std::wstring downloadUrl;      // full URL to the exe
        std::wstring errorMessage;     // non-empty if check failed
    };

    // Check for updates (blocking, call from background thread or startup).
    // Returns UpdateInfo with available=true if a newer version exists.
    UpdateInfo CheckForUpdate();

    // Download the new exe to a temp folder. Returns the local path to the
    // downloaded file, or empty string on failure.
    // hWndParent is used for progress messages (can be NULL).
    std::wstring DownloadUpdate(const UpdateInfo& info, HWND hWndParent);

    // Apply the update: creates a helper batch script that waits for this
    // process to exit, replaces the exe, and restarts it. Returns true if
    // the script was launched successfully. The caller should exit the app
    // after this returns true.
    bool ApplyUpdate(const std::wstring& downloadedExePath);

    // High-level: check + prompt + download + apply.
    // If silent=true, no dialog is shown when the app is already up-to-date.
    // hWndParent — owner window for message boxes.
    void RunUpdateCheck(HWND hWndParent, bool silent);

    // Parse "major.minor.patch" string into Version struct.
    Version ParseVersion(const std::string& s);

    // Get the current compiled-in version.
    inline Version GetCurrentVersion() {
        return { APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH };
    }

} // namespace AutoUpdate
