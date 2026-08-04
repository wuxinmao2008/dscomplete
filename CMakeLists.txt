cmake_minimum_required(VERSION 3.21)

project(DsComplete LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(QtCreator_DIR "C:/Qt/Tools/qtcreator/lib/cmake/QtCreator"
    CACHE PATH "Directory containing QtCreatorConfig.cmake")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/staging"
      CACHE PATH "DsComplete staging directory" FORCE)
endif()

if(WIN32 AND NOT TARGET Qt6::TaskTree)
  add_library(Qt6::TaskTree INTERFACE IMPORTED)
endif()

find_package(Qt6 REQUIRED COMPONENTS Network Widgets)
find_package(QtCreator REQUIRED COMPONENTS Core TextEditor)

set(DSCOMPLETE_PLUGIN_VERSION "20.0.0" CACHE STRING "DsComplete plugin version")
set(DSCOMPLETE_PLUGIN_COMPAT_VERSION "20.0.0"
  CACHE STRING "Minimum compatible Qt Creator plugin version")
set(DSCOMPLETE_DEPENDENCY_VERSION "20.0.0"
  CACHE STRING "Required Core and TextEditor plugin version")

set_property(TARGET QtCreator::Core PROPERTY QTC_PLUGIN_VERSION
       "${DSCOMPLETE_DEPENDENCY_VERSION}")
set_property(TARGET QtCreator::TextEditor PROPERTY QTC_PLUGIN_VERSION
       "${DSCOMPLETE_DEPENDENCY_VERSION}")

option(WITH_TESTS "Build DsComplete tests" OFF)

if(WITH_TESTS)
  find_package(Qt6 REQUIRED COMPONENTS Test)
  set(IMPLICIT_DEPENDS Qt::Test)
  enable_testing()
endif()

add_qtc_plugin(DsComplete
  VERSION "${DSCOMPLETE_PLUGIN_VERSION}"
  COMPAT_VERSION "${DSCOMPLETE_PLUGIN_COMPAT_VERSION}"
  PLUGIN_DEPENDS
    QtCreator::Core
    QtCreator::TextEditor
  DEPENDS
    Qt::Network
    Qt::Widgets
    QtCreator::ExtensionSystem
    QtCreator::Utils
  SOURCES
    dscompleteclient.cpp
    dscompleteclient.h
    dscompleteconstants.h
    dscompleteplugin.cpp
    dscompleteprotocol.cpp
    dscompleteprotocol.h
    dscompletesettings.cpp
    dscompletesettings.h
    dscompletetr.h
)

if(WITH_TESTS)
  add_qtc_test(tst_dscompleteprotocol
    SOURCES
      dscompleteprotocol.cpp
      dscompleteprotocol.h
      tst_dscompleteprotocol.cpp
    DEPENDS
      Qt::Network
      Qt::Test
  )
endif()

get_target_property(QtCreatorCorePath QtCreator::Core LOCATION)
find_program(QtCreatorExecutable
  NAMES qtcreator
  PATHS "${QtCreatorCorePath}/../../../bin"
  NO_DEFAULT_PATH
)

if(QtCreatorExecutable)
  add_custom_target(RunQtCreator
    COMMAND "${QtCreatorExecutable}" -pluginpath $<TARGET_FILE_DIR:DsComplete>
    DEPENDS DsComplete
  )
endif()