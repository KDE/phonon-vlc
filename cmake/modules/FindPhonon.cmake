# Find Phonon

# Copyright (c) 2010, Harald Sitter <sitter@kde.org>
# Copyright (c) 2011, Alexander Neundorf <neundorf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

find_package(Phonon NO_MODULE)
if(PHONON_FOUND)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PHONON_BUILDSYSTEM_DIR})
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Phonon  DEFAULT_MSG  Phonon_DIR )


