# Find Phonon

# Copyright (c) 2010-2013, Harald Sitter <sitter@kde.org>
# Copyright (c) 2011, Alexander Neundorf <neundorf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

find_package(Phonon NO_MODULE)

if(PHONON_BUILDSYSTEM_DIR)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PHONON_BUILDSYSTEM_DIR})
    # Prevent double-include of internals, and make sure they are included
    # In Phonon <4.7 the internals were auto-included, in >=4.7 they are not.
    if(NOT COMMAND phonon_add_executable)
        include(${PHONON_BUILDSYSTEM_DIR}/FindPhononInternal.cmake )
    endif()
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Phonon  DEFAULT_MSG  Phonon_DIR )
