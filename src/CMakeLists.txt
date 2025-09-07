add_subdirectory(backend)

set(SOURCE ${SOURCE}
    src/main.cpp

    src/ModelRenderer.cpp
    src/ComputeSample.cpp
    src/Image.cpp
    src/Model.cpp
	PARENT_SCOPE
)

# Assumes include directories include .
set(HEADERS ${HEADERS}
    include/ComputeSample.h
    include/Image.h
    include/ModelRenderer.h
    include/Model.h
	PARENT_SCOPE
)

