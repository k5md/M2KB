FROM ubuntu
USER root
RUN apt-get update -y && \
    apt-get install -y \
    mingw-w64

ADD ./src /app/src

ADD ./lib/*.h /usr/lib/gcc/i686-w64-mingw32/7.3-win32/include/
ADD ./lib/*.lib /usr/lib/gcc/i686-w64-mingw32/7.3-win32/lib/
ADD ./lib/*.dll /usr/lib/gcc/i686-w64-mingw32/7.3-win32/bin/
ADD ./lib /app

WORKDIR /app

RUN mkdir bin && \
  i686-w64-mingw32-g++-win32 \
  -o bin/M2KB.exe ./src/M2KB.cpp \
  -lpdcurses \
  -lwinmm -L./ \
  --static
