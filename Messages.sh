#! /usr/bin/env bash
$EXTRACT_TR_STRINGS $(find . -name "*.cpp" -o -name "*.h") -o $podir/phonon_vlc_qt.pot
