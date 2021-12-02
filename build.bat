docker build -t build-mingw-w64 .
mkdir bin
docker create --name dummy build-mingw-w64 bash
docker cp dummy:/app/bin .
docker rm -fv dummy
xcopy lib\*.dll bin /sy