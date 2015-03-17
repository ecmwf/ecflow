SET(PB_SRCS
    qtpropertybrowser.cpp
    qtbuttonpropertybrowser.cpp
    qtpropertymanager.cpp
    qteditorfactory.cpp
    qtvariantproperty.cpp
    qttreepropertybrowser.cpp
    qtbuttonpropertybrowser.cpp
    qtgroupboxpropertybrowser.cpp
    qtpropertybrowserutils.cpp
)

include(${QT_USE_FILE})

#We need it because the source includes teh genetrated moc files!!!!
include_directories(${CMAKE_CURRENT_BINARY_DIR})

#QT4_WRAP_CPP(PB_MOC qtpropertybrowserutils_p.h)

QT4_ADD_RESOURCES (PB_RES qtpropertybrowser.qrc )


# List of header that should go through moc
SET(PB_HEADERS_MOC_SRCS
    qtpropertybrowser.h
    qtpropertymanager.h
    qteditorfactory.h
    qtvariantproperty.h
    qttreepropertybrowser.h
    qtbuttonpropertybrowser.h
    qtgroupboxpropertybrowser.h
)

SET(PB_HEADERS_ONLY_MOC)

FOREACH(file ${PB_HEADERS_MOC_SRCS})
    get_filename_component(filename_we ${file} NAME_WE)
    SET(output_file moc_${filename_we}.cpp)
    QT4_GENERATE_MOC(${file} ${output_file})
    LIST(APPEND PB_HEADERS_ONLY_MOC ${output_file})
ENDFOREACH()

# List of cpp files that should go through moc
SET(PB_CPP_MOC_SRCS
    qtpropertymanager.cpp
    qteditorfactory.cpp
    qttreepropertybrowser.cpp
)

FOREACH(file ${PB_CPP_MOC_SRCS})
    get_filename_component(filename_we ${file} NAME_WE)
    SET(output_file ${filename_we}.moc)
    QT4_GENERATE_MOC(${file} ${output_file})
    LIST(APPEND PB_HEADERS_ONLY_MOC ${output_file})
ENDFOREACH()
    

set_source_files_properties(
  ${PB_HEADERS_ONLY_MOC}
  PROPERTIES
    HEADER_FILE_ONLY true
)

# List of header that should go through moc
SET(PB_HEADERS_OTHER_MOC_SRCS
    qtpropertybrowserutils_p.h
)

SET(PB_HEADERS_MOC)

FOREACH(file ${PB_HEADERS_OTHER_MOC_SRCS})
    get_filename_component(filename_we ${file} NAME_WE)
    SET(output_file moc_${filename_we}.cpp)
    QT4_GENERATE_MOC(${file} ${output_file})
    LIST(APPEND PB_HEADERS_MOC ${output_file})
ENDFOREACH()


set(
  PB_MOC
  ${PB_HEADERS_MOC}
  ${PB_HEADERS_ONLY_MOC}
)


ecbuild_add_library( TARGET  qtpropertybrowser 
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${PB_MOC} ${PB_SRCS} ${PB_RES} 
                     LIBS     
                     INCLUDES                               
                    )