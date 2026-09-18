#!/bin/bash
# Builds AU + VST3 (Release, universal x86_64/arm64) and packages them into
# build/DarkFader.pkg, an installer that puts the plugin in the standard
# system-wide locations:
#   /Library/Audio/Plug-Ins/Components/DarkFader.component
#   /Library/Audio/Plug-Ins/VST3/DarkFader.vst3
#
# The plugin is ad-hoc signed (no Apple Developer Program membership needed)
# by JUCE's build step automatically - fine for local use, since a locally
# built file never gets the quarantine flag that triggers Gatekeeper's
# "unidentified developer" block. If you ever move this .pkg to another Mac
# via download/AirDrop/email, that copy WILL get quarantined and Gatekeeper
# will refuse to run it until you right-click > Open (or remove the flag
# with `xattr -cr DarkFader.pkg`) and confirm past the warning.
set -e
export PATH="/Applications/CMake.app/Contents/bin:$PATH"
cd "$(dirname "$0")"

cmake --build build --target DarkFader_AU DarkFader_VST3 -j"$(sysctl -n hw.ncpu)"

packagesbuild DarkFader.pkgproj

echo ""
echo "Installer ready: build/DarkFader.pkg"
echo "Double-click it in Finder to install (will ask for your admin password)."
