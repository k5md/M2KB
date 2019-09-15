build: 
	docker build -t build-mingw-w64 .
	rm -rf ./bin
	mkdir ./bin
	docker create --name dummy build-mingw-w64 bash
	docker cp dummy:/app/bin .
	docker rm -fv dummy
	cp ./lib/*.dll ./bin

package:
	mkdir ./artefacts
	zip -rDj M2KB.zip ./bin
	mv M2KB.zip ./artefacts/M2KB.zip
