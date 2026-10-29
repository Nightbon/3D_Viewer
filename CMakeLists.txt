cmake_minimum_required(VERSION 3.16)
project(3DVisualizer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGLWidgets)

# 5. Исходники
set(SOURCES
    src/main.cpp
    src/Model/Model.cpp
    src/Controller/Controller.cpp
    src/View/View.cpp
    src/View/Renderer/Render.cpp
    src/View/SettingsManager.cpp
    src/View/TransformState.cpp
    src/View/GifRecorder.cpp
    src/Includes/s21_matrix_oop.cpp
    src/View/Camera.cpp
    src/Model/ObjParser.cpp
    src/View/RayTracer.cpp
    src/Includes/AnalyticPrimitive.cpp
    src/Model/AnalyticMesh.cpp 
)

set(HEADERS
    src/Model/TypesModel.h
    src/Model/Model.h
    src/Model/ObjParser.h
    src/Controller/Controller.h
    src/View/TypesView.h
    src/View/View.h
    src/View/Renderer/Render.h
    src/View/Renderer/Shaders.h
    src/View/Camera.h
    src/Includes/gif.h
    src/View/SettingsManager.h
    src/View/TransformState.h
    src/View/GifRecorder.h
    src/Includes/s21_matrix_oop.h
    src/View/RayTracer.h
    src/Includes/AnalyticPrimitive.h
    src/Model/AnalyticMesh.h 
    src/Includes/RTTypes.h 
)

option(Release "Build" OFF)
if(Release)
    # 6. Создание исполняемого файла
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

    target_include_directories(${PROJECT_NAME} PRIVATE src/Includes)

    # 7. Линковка
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::OpenGLWidgets
    )
endif()

# ================ ТЕСТЫ ========================
option(BUILD_TESTING "Build tests" OFF)

if(BUILD_TESTING)

    find_package(GTest REQUIRED)

    add_executable(run_tests
        src/Model/TestModel.cpp
        src/Model/Model.cpp
        src/Model/ObjParser.cpp
        src/Includes/s21_matrix_oop.cpp
        src/Model/TypesModel.h
        src/Model/Model.h
        src/Includes/s21_matrix_oop.h
        src/Model/AnalyticMesh.cpp
        src/Includes/AnalyticPrimitive.cpp
    )

    target_include_directories(run_tests PRIVATE src/Includes)

    target_link_libraries(run_tests PRIVATE
        GTest::gtest
        GTest::gtest_main
        Qt6::Core
        Qt6::Widgets
        Qt6::OpenGLWidgets
    )

    enable_testing()
    add_test(NAME AllTests COMMAND run_tests)

endif()

# ================ Документация ========================
option(BUILD_DVI "Build docs" OFF)

if(BUILD_DVI)

    find_package(Doxygen)

    if (DOXYGEN_FOUND)
        # Настройки Doxygen
        set(DOXYGEN_PROJECT_NAME "3DVisualizer")
        set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs)
        set(DOXYGEN_GENERATE_HTML YES)
        set(DOXYGEN_GENERATE_LATEX NO)
        set(DOXYGEN_EXTRACT_ALL YES)
        set(DOXYGEN_OUTPUT_LANGUAGE Russian)
        set(DOXYGEN_GENERATE_TREEVIEW YES)
        set(DOXYGEN_CALL_GRAPH YES)
        set(DOXYGEN_HAVE_DOT YES)
        set(JAVADOC_AUTOBRIEF YES)
        set(SOURCE_BROWSER YES)
        set(QUIET = YES)

        set(DOXYGEN_EXCLUDE_PATTERNS "*Test*")
        set(DOXYGEN_EXCLUDE
            ${CMAKE_SOURCE_DIR}/src/Includes/gif.h
        )

        set(DOXYGEN_EXCLUDE  ${CMAKE_SOURCE_DIR}/src/Includes/glm)

        # Создаем команду 'doxygen'
        doxygen_add_docs(doxygen
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ALL
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
        )
    endif()
endif()