# h1pNoise iPhone prototype

Native SwiftUI connection screen and embedded WKWebView for the existing console interface. This first prototype requires iOS 17 or newer. It is not an App Store submission.

The GitHub Actions workflow builds an unsigned physical-device IPA on a standard macOS runner. Install and re-sign it with AltStore Classic on Windows using your own Apple account. Never submit Apple credentials to this repository or a workflow. Free-account installations require renewal every seven days and have feature/app-count limits.

Open h1pNoise on the PS4 (or the PC test host), then enter its private IPv4 HTTP address including port. Allow Local Network access. Enter the pairing code in the console page; alternatively paste the QR URL. Only the address without the code is saved. Web storage is session-only. The app controls the console; it does not perform the downloads itself. The native shell limits navigation to the chosen origin. This prototype renders the interface served by the local console and should only connect to a trusted device.

Validation on a physical iPhone is still required: launch, local-network permission, connection, incorrect code, torrent file picker, progress, reconnect, and disconnect. A working Swift compilation does not prove those device behaviours.

