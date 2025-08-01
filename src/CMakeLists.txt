add_subdirectory(backend)

set(SOURCE ${SOURCE}
    src/main.cpp
    src/App.cpp
    src/Image.cpp
	PARENT_SCOPE
)

# Assumes include directories include .
set(HEADERS ${HEADERS}
    include/App.h
    include/Image.h
	PARENT_SCOPE
)

