#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
APP="$PWD/ios-output/Payload/h1pNoise.app"
mkdir -p "$APP"
SDK="$(xcrun --sdk iphoneos --show-sdk-path)"
xcrun --sdk iphoneos swiftc ios/App.swift -parse-as-library -O -swift-version 5 -sdk "$SDK" -target arm64-apple-ios17.0 -framework SwiftUI -framework UIKit -framework WebKit -o "$APP/h1pNoise"
cat > "$APP/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleExecutable</key><string>h1pNoise</string>
<key>CFBundleIdentifier</key><string>com.h1pnoise.controller</string>
<key>CFBundleName</key><string>h1pNoise</string>
<key>CFBundleDisplayName</key><string>h1pNoise</string>
<key>CFBundlePackageType</key><string>APPL</string>
<key>CFBundleShortVersionString</key><string>0.1.0</string>
<key>CFBundleVersion</key><string>1</string>
<key>MinimumOSVersion</key><string>17.0</string>
<key>CFBundleSupportedPlatforms</key><array><string>iPhoneOS</string></array>
<key>UIDeviceFamily</key><array><integer>1</integer><integer>2</integer></array>
<key>UILaunchScreen</key><dict/>
<key>NSLocalNetworkUsageDescription</key><string>Ligar à h1pNoise na tua PS4 para controlar as transferências.</string>
<key>NSAppTransportSecurity</key><dict><key>NSAllowsArbitraryLoadsInWebContent</key><true/><key>NSAllowsLocalNetworking</key><true/></dict>
<key>UISupportedInterfaceOrientations</key><array><string>UIInterfaceOrientationPortrait</string><string>UIInterfaceOrientationLandscapeLeft</string><string>UIInterfaceOrientationLandscapeRight</string></array>
</dict></plist>
PLIST
plutil -lint "$APP/Info.plist"
cd ios-output
zip -qr h1pNoise-iPhone-unsigned.ipa Payload
