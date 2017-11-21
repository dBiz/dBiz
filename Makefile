
SOURCES = $(wildcard src/*.cpp) \

include ../../plugin.mk

 
dist: all

	mkdir -p dist/dBiz
	cp LICENSE* dist/dBiz/
	cp plugin.* dist/dBiz/
	cp -R res dist/dBiz/
	cd dist && zip -5 -r dBiz-$(VERSION)-$(ARCH).zip dBiz
