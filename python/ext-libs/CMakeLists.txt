# See ../CMakeLists.txt for info on staged-plugins* and clean-staged-plugins targets

SET (QGIS_PYTHON_DIR ${QGIS_DATA_DIR}/python)
SET (PYTHON_OUTPUT_DIRECTORY ${QGIS_OUTPUT_DIRECTORY}/python)

SET (WITH_PYSPATIALITE FALSE CACHE BOOL "Determines whether PYSPATIALITE should be built")
IF (WITH_PYSPATIALITE)
  ADD_SUBDIRECTORY(pyspatialite)
ENDIF (WITH_PYSPATIALITE)

MACRO(EXT_PYLIB lib enable_by_default)
  STRING(TOUPPER ${lib} ulib)
  SET (WITH_INTERNAL_${ulib} ${enable_by_default} CACHE BOOL "Determines whether python ${lib} should be included")
  IF(WITH_INTERNAL_${ulib})
    INSTALL(DIRECTORY ${lib} DESTINATION "${QGIS_PYTHON_DIR}")

    ADD_CUSTOM_TARGET(py${lib})

    FILE(GLOB_RECURSE files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${lib}/*)
    ADD_CUSTOM_COMMAND(TARGET py${lib}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${lib} "${PYTHON_OUTPUT_DIRECTORY}/${lib}"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS ${files}
    )
    PY_COMPILE(py${lib} "${PYTHON_OUTPUT_DIRECTORY}/${lib}")

    ADD_DEPENDENCIES(staged-plugins py${lib})
    ADD_CUSTOM_COMMAND(TARGET clean-staged-plugins
      COMMAND ${CMAKE_COMMAND} -E remove_directory "${PYTHON_OUTPUT_DIRECTORY}/${lib}"
    )

  ENDIF(WITH_INTERNAL_${ulib})
ENDMACRO(EXT_PYLIB lib)

# Our yaml package is not compatible with python 3
# Fallback to system package by default
EXT_PYLIB(yaml FALSE)

FOREACH(pkg httplib2 jinja2 markupsafe owslib pygments dateutil pytz nose2 requests)
  EXT_PYLIB(${pkg} TRUE)
ENDFOREACH(pkg)

SET (WITH_INTERNAL_SIX TRUE CACHE BOOL "Determines whether python six should be included")
IF(WITH_INTERNAL_SIX)
  INSTALL(FILES six.py DESTINATION "${QGIS_PYTHON_DIR}")

  ADD_CUSTOM_TARGET(pysix)

  ADD_CUSTOM_COMMAND(TARGET pysix
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy six.py "${PYTHON_OUTPUT_DIRECTORY}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS six.py
  )
  PY_COMPILE(pysix "${PYTHON_OUTPUT_DIRECTORY}/six.py")

  ADD_DEPENDENCIES(staged-plugins pysix)
  ADD_CUSTOM_COMMAND(TARGET clean-staged-plugins
    COMMAND ${CMAKE_COMMAND} -E remove "${PYTHON_OUTPUT_DIRECTORY}/six.py"
  )

ENDIF(WITH_INTERNAL_SIX)

