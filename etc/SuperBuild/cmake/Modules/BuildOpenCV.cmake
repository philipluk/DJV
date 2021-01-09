include(ExternalProject)

ExternalProject_Add(
    OpenCV
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/OpenCV
    DEPENDS ZLIB
    GIT_REPOSITORY "https://github.com/opencv/opencv.git"
    GIT_TAG 4.5.0
    CMAKE_ARGS
        -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
        -DCMAKE_INSTALL_LIBDIR=lib
        -DCMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_POLICY_DEFAULT_CMP0091:STRING=NEW
        -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
        -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
        -DBUILD_LIST=calib3d,dnn,features2d,flann,gapi,imgproc,ml,objdetect,photo,stitching,videoio
        -DBUILD_ZLIB=FALSE
        -DBUILD_TIFF=FALSE
        -DBUILD_JASPER=FALSE
        -DBUILD_JPEG=FALSE
        -DBUILD_PNG=FALSE
        -DBUILD_OPENEXR=FALSE
        -DBUILD_WEBP=FALSE
        -DWITH_1394=FALSE
        -DWITH_AVFOUNDATION=FALSE
        -DWITH_CAP_IOS=FALSE
        -DWITH_VTK=FALSE
        -DWITH_FFMPEG=FALSE
        -DWITH_GSTREAMER=FALSE
        -DWITH_GTK=FALSE
        -DWITH_JASPER=FALSE
        -DWITH_OPENJPEG=FALSE
        -DWITH_JPEG=FALSE
        -DWITH_WEBP=FALSE
        -DWITH_OPENEXR=FALSE
        -DWITH_PNG=TRUE
        -DWITH_WIN32UI=FALSE
        -DWITH_TIFF=FALSE
        -DWITH_V4L=FALSE
        -DWITH_DSHOW=FALSE
        -DWITH_MSMF=FALSE
        -DWITH_IMGCODEC_HDR=FALSE
        -DWITH_IMGCODEC_SUNRASTER=FALSE
        -DWITH_IMGCODEC_PXM=FALSE
        -DWITH_IMGCODEC_PFM=FALSE)
