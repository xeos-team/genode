# identify the Qt5 repository by searching for a file that is unique for Qt5
QT5_REP_DIR := $(call select_from_repositories,lib/import/import-qt5.inc)
QT5_REP_DIR := $(realpath $(dir $(QT5_REP_DIR))../..)

include $(QT5_REP_DIR)/lib/mk/qt5_version.inc

QT5_PORT_DIR := $(call select_from_ports,qt5)
QT5_CONTRIB_DIR := $(QT5_PORT_DIR)/src/lib/qt5/$(QT5)

QMAKE_PROJECT_PATH = $(QT5_CONTRIB_DIR)/qtbase/examples/gui/analogclock
QMAKE_PROJECT_FILE = $(QMAKE_PROJECT_PATH)/analogclock.pro

INC_DIR += $(QT5_CONTRIB_DIR)/qtbase/examples/gui/rasterwindow

vpath % $(QMAKE_PROJECT_PATH)
vpath % $(QT5_CONTRIB_DIR)/qtbase/examples/gui/rasterwindow

include $(QT5_REP_DIR)/src/app/qt5/tmpl/target_defaults.inc

include $(QT5_REP_DIR)/src/app/qt5/tmpl/target_final.inc

LIBS += qt5_gui qt5_qpa_nitpicker qt5_widgets