# - Try to find the THEORA libraries
# Derived from CMake Find Scripts from Visualization Toolkit Project
# 
# Copyright (c) 1993-2015 Ken Martin, Will Schroeder, Bill Lorensen
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
#  * Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 
#  * Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 
#  * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
#    of any contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


include(CMakeFindDependencyMacro)
find_dependency(OggVorbis)

find_path(THEORA_INCLUDE_DIR
  NAMES
    theora/theora.h
  DOC "theora include directory")
mark_as_advanced(THEORA_INCLUDE_DIR)

get_filename_component(computed_theora_root "${THEORA_INCLUDE_DIR}" DIRECTORY)

find_library(THEORA_LIBRARY
  NAMES
    theora
  HINTS
    "${computed_theora_root}/lib"
    "${computed_theora_root}/lib64"
  DOC "theora library")
mark_as_advanced(THEORA_LIBRARY)

find_library(THEORA_enc_LIBRARY
  NAMES
    theoraenc
  HINTS
    "${computed_theora_root}/lib"
    "${computed_theora_root}/lib64"
  DOC "theora encoding library")
mark_as_advanced(THEORA_enc_LIBRARY)

find_library(THEORA_dec_LIBRARY
  NAMES
    theoradec
  HINTS
    "${computed_theora_root}/lib"
    "${computed_theora_root}/lib64"
  DOC "theora decoding library")
mark_as_advanced(THEORA_dec_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(THEORA
  REQUIRED_VARS THEORA_LIBRARY THEORA_enc_LIBRARY THEORA_dec_LIBRARY THEORA_INCLUDE_DIR)

if (THEORA_FOUND)
  set(THEORA_LIBRARIES "${THEORA_LIBRARY}" "${THEORA_enc_LIBRARY}" "${THEORA_dec_LIBRARY}")
  set(THEORA_INCLUDE_DIRS "${THEORA_INCLUDE_DIR}")

  if (NOT TARGET THEORA::THEORA)
    add_library(THEORA::THEORA UNKNOWN IMPORTED)
    set_target_properties(THEORA::THEORA PROPERTIES
      IMPORTED_LOCATION "${THEORA_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES ${THEORA_INCLUDE_DIR}
      INTERFACE_LINK_LIBRARIES OGG::OGG)
  endif ()

  if (NOT TARGET THEORA::ENC)
    add_library(THEORA::ENC UNKNOWN IMPORTED)
    set_target_properties(THEORA::ENC PROPERTIES
      IMPORTED_LOCATION "${THEORA_enc_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES ${THEORA_INCLUDE_DIR}
      INTERFACE_LINK_LIBRARIES OGG::OGG)
  endif()

  if (NOT TARGET THEORA::DEC)
    add_library(THEORA::DEC UNKNOWN IMPORTED)
    set_target_properties(THEORA::DEC PROPERTIES
      IMPORTED_LOCATION "${THEORA_dec_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES ${THEORA_INCLUDE_DIR})
  endif()
endif ()
