#! /usr/bin/env bash
find src/ -maxdepth 1 -name "*.cpp" -print > files
$XGETTEXT_QT --copyright-holder=This_file_is_part_of_KDE --msgid-bugs-address=https://bugs.kde.org --files-from=files -o $podir/phonon_vlc.pot
rm -f files
