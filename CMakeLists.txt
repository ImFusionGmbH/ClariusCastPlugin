cmake_minimum_required(VERSION 3.2.0)
project(ClariusCastPlugin)
message(STATUS "> Configuring ${PROJECT_NAME}")

find_package(ImFusionLib COMPONENTS ImFusionUS ImFusionStream REQUIRED)

###########################################################
# define and configure the targets
###########################################################
set(Sources
		ClariusStream.cpp
		ClariusController.cpp
		ClariusStreamIoAlgorithm.cpp
		ClariusPlugin.cpp
		ClariusCastApi.cpp)

set(Headers
		ClariusStream.h
		ClariusController.h
		ClariusStreamIoAlgorithm.h
		ClariusPlugin.h
		ClariusApi.h)

if (WIN32)
	list(APPEND Headers WindowsFirewall.h)
	list(APPEND Sources WindowsFirewall.cpp)
endif ()

add_library(${PROJECT_NAME} ${Sources} ${Headers})
target_include_directories(${PROJECT_NAME}
		PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
		)
target_link_libraries(${PROJECT_NAME} PRIVATE ImFusionLib ImFusionUS ImFusionStream)

imfusion_set_common_target_properties(Plugin)


###########################################################
# Gather external dependencies
###########################################################

set(CAST_VERSION 11.2.0)

set_property(
	SOURCE ClariusCastApi.cpp
	APPEND PROPERTY COMPILE_DEFINITIONS CLARIUS_CAST_VERSION="${CAST_VERSION}"
)

if (WIN32)
	set(CLARIUS_CAST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ext/Cast/${CAST_VERSION}/windows" CACHE PATH "Path to the Clarius Cast SDK" FORCE)
	target_link_libraries(${PROJECT_NAME} PRIVATE ${CLARIUS_CAST_PATH}/lib/cast.lib)
	imfusion_installer_register_dll("${CLARIUS_CAST_PATH}/lib/cast.dll" "${CLARIUS_CAST_PATH}/lib/cast.dll")
elseif (UNIX AND NOT APPLE)
	execute_process(COMMAND lsb_release -rs OUTPUT_VARIABLE UbuntuVersion OUTPUT_STRIP_TRAILING_WHITESPACE)
	string(SUBSTRING "${UbuntuVersion}" 0 2 UBUNTU_YEAR)
	set(LINUX_FOLDER_NAME "ubuntu${UBUNTU_YEAR}")
	set(CLARIUS_CAST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ext/Cast/${CAST_VERSION}/${LINUX_FOLDER_NAME}" CACHE PATH "Path to the Clarius Cast SDK")
	target_link_libraries(${PROJECT_NAME} PRIVATE ${CLARIUS_CAST_PATH}/lib/libcast.so)
	install(FILES "${CLARIUS_CAST_PATH}/lib/libcast.so" DESTINATION "${ImFusionLibraryInstallDir}/ImFusionLib")
else ()
	message( FATAL_ERROR "Clarius not supported for MacOS so far." )
endif ()
target_include_directories(${PROJECT_NAME} PRIVATE ${CLARIUS_CAST_PATH}/include)

set(CLARIUS_PLUGIN_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "Path to the headers of the Clarius Plugin")

imfusion_provide_ide_instructions()




