cmake_minimum_required (VERSION 3.4)

find_package (Qt5 REQUIRED COMPONENTS Core Widgets Network LinguistTools)

set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)
set (CMAKE_AUTOUIC ON)
set (CMAKE_INCLUDE_CURRENT_DIR ON)

# files that are used both in tests and the app
set(GUI_COMMON_SOURCE_FILES
    src/Action.cpp
    src/Hotkey.cpp
    src/KeySequence.cpp
)

set(GUI_COMMON_HEADER_FILES
    src/Action.h
    src/Hotkey.h
    src/KeySequence.h
)

set(GUI_SOURCE_FILES
    src/AboutDialog.cpp
    src/ActionDialog.cpp
    src/AddClientDialog.cpp
    src/AppConfig.cpp
    src/BarrierLocale.cpp
    src/BaseConfig.cpp
    src/CommandProcess.cpp
    src/DataDownloader.cpp
    src/DisplayIsValid.cpp
    src/FingerprintAcceptDialog.cpp
    src/HotkeyDialog.cpp
    src/IpcClient.cpp
    src/Ipc.cpp
    src/IpcReader.cpp
    src/KeySequenceWidget.cpp
    src/LogWindow.cpp
    src/main.cpp
    src/MainWindow.cpp
    src/NewScreenWidget.cpp
    src/QInputLeapApplication.cpp
    src/QUtility.cpp
    src/Screen.cpp
    src/ScreenSettingsDialog.cpp
    src/ScreenSetupModel.cpp
    src/ScreenSetupView.cpp
    src/ServerConfig.cpp
    src/ServerConfigDialog.cpp
    src/SettingsDialog.cpp
    src/SetupWizard.cpp
    src/SslCertificate.cpp
    src/TrashScreenWidget.cpp
    src/VersionChecker.cpp
    src/ZeroconfBrowser.cpp
    src/ZeroconfRegister.cpp
    src/ZeroconfServer.cpp
    src/ZeroconfService.cpp
    src/ZeroconfThread.cpp
)

set(GUI_HEADER_FILES
    src/AboutDialog.h
    src/ActionDialog.h
    src/AddClientDialog.h
    src/AppConfig.h
    src/BarrierLocale.h
    src/BaseConfig.h
    src/CommandProcess.h
    src/DataDownloader.h
    src/DisplayIsValid.h
    src/ElevateMode.h
    src/HotkeyDialog.h
    src/IpcClient.h
    src/Ipc.h
    src/IpcReader.h
    src/KeySequenceWidget.h
    src/LogWindow.h
    src/MainWindow.h
    src/NewScreenWidget.h
    src/ProcessorArch.h
    src/QInputLeapApplication.h
    src/QUtility.h
    src/Screen.h
    src/ScreenSettingsDialog.h
    src/ScreenSetupModel.h
    src/ScreenSetupView.h
    src/ServerConfigDialog.h
    src/ServerConfig.h
    src/SettingsDialog.h
    src/SetupWizard.h
    src/SslCertificate.h
    src/TrashScreenWidget.h
    src/VersionChecker.h
    src/ZeroconfBrowser.h
    src/ZeroconfRecord.h
    src/ZeroconfRegister.h
    src/ZeroconfServer.h
    src/ZeroconfService.h
    src/ZeroconfThread.h
)

set(GUI_UI_FILES
    src/AboutDialogBase.ui
    src/ActionDialogBase.ui
    src/AddClientDialogBase.ui
    src/FingerprintAcceptDialog.ui
    src/HotkeyDialogBase.ui
    src/LogWindowBase.ui
    src/MainWindowBase.ui
    src/ScreenSettingsDialogBase.ui
    src/ServerConfigDialogBase.ui
    src/SettingsDialogBase.ui
    src/SetupWizardBase.ui
)

set(TS_FILES
    res/lang/gui_af-ZA.ts
    res/lang/gui_ar.ts
    res/lang/gui_bg-BG.ts
    res/lang/gui_ca-AD.ts
    res/lang/gui_cs-CZ.ts
    res/lang/gui_cy.ts
    res/lang/gui_da.ts
    res/lang/gui_de.ts
    res/lang/gui_es.ts
    res/lang/gui_et-EE.ts
    res/lang/gui_fi.ts
    res/lang/gui_fr.ts
    res/lang/gui_gl.ts
    res/lang/gui_grk.ts
    res/lang/gui_he.ts
    res/lang/gui_hi.ts
    res/lang/gui_hr-HR.ts
    res/lang/gui_hu-HU.ts
    res/lang/gui_id.ts
    res/lang/gui_is-IS.ts
    res/lang/gui_it.ts
    res/lang/gui_ja-JP.ts
    res/lang/gui_ko.ts
    res/lang/gui_lt.ts
    res/lang/gui_lv.ts
    res/lang/gui_mr.ts
    res/lang/gui_nl-NL.ts
    res/lang/gui_no.ts
    res/lang/gui_pes-IR.ts
    res/lang/gui_pl-PL.ts
    res/lang/gui_pt-BR.ts
    res/lang/gui_pt-PT.ts
    res/lang/gui_ro.ts
    res/lang/gui_ru.ts
    res/lang/gui_si.ts
    res/lang/gui_sk-SK.ts
    res/lang/gui_sl-SI.ts
    res/lang/gui_sq-AL.ts
    res/lang/gui_sr.ts
    res/lang/gui_sv.ts
    res/lang/gui_th-TH.ts
    res/lang/gui_tr-TR.ts
    res/lang/gui_uk.ts
    res/lang/gui_ur.ts
    res/lang/gui_vi.ts
    res/lang/gui_zh-CN.ts
    res/lang/gui_zh-TW.ts
)

set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/res/lang")
qt5_add_translation(QM_FILES ${TS_FILES} OPTIONS -silent)

if (WIN32)
    set (GUI_RC_FILES res/win/Barrier.rc)
    set_property(SOURCE res/win/Barrier.rc PROPERTY COMPILE_FLAGS /nologo)
endif()

add_executable (barrier WIN32
    ${GUI_COMMON_SOURCE_FILES}
    ${GUI_COMMON_HEADER_FILES}
    ${GUI_SOURCE_FILES}
    ${GUI_HEADER_FILES}
    ${GUI_UI_FILES}
    ${GUI_RC_FILES}
    res/Barrier.qrc
)

include_directories (./src)

target_link_libraries(barrier net base io Qt5::Core Qt5::Widgets Qt5::Network ${OPENSSL_LIBS})
target_compile_definitions (barrier PRIVATE -DINPUTLEAP_VERSION_STAGE="${INPUTLEAP_VERSION_STAGE}")
target_compile_definitions (barrier PRIVATE -DINPUTLEAP_REVISION="${INPUTLEAP_REVISION}")

if (BUILD_MSWINDOWS)
    include_directories ($ENV{BONJOUR_SDK_HOME}/Include)
    find_library (DNSSD_LIB dnssd.lib
                  HINTS ENV BONJOUR_SDK_HOME
                  PATH_SUFFIXES "Lib/x64")
    set_target_properties (barrier PROPERTIES LINK_FLAGS "/NODEFAULTLIB:LIBCMT")
    target_link_libraries (barrier ${DNSSD_LIB})
endif()
if (BUILD_CARBON)
    find_library(APPSERVICES_LIB ApplicationServices)
    target_link_libraries(barrier ${APPSERVICES_LIB})
endif()
if (BUILD_XWINDOWS)
    target_link_libraries (barrier dns_sd X11)
endif()

target_link_libraries (barrier common)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    install(TARGETS barrier DESTINATION ${INPUTLEAP_BUNDLE_BINARY_DIR})
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "BSD")
    install (TARGETS barrier DESTINATION bin)
endif()

if(INPUTLEAP_BUILD_TESTS)
    set(GUI_TEST_SOURCE_FILES
        test/KeySequenceTests.cpp
        test/HotkeyTests.cpp
        test/main.cpp
    )

    add_executable(guiunittests
        ${GUI_TEST_SOURCE_FILES}
        ${GUI_COMMON_SOURCE_FILES}
        ${GUI_COMMON_HEADER_FILES}
    )

    add_test(NAME guiunittests
             COMMAND guiunittests
             WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

    target_include_directories(guiunittests PUBLIC ../../ext)
    target_link_libraries(guiunittests gtest gmock Qt5::Core Qt5::Widgets Qt5::Network ${libs})
endif()
