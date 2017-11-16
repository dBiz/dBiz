
SOURCES = $(wildcard src/*.cpp) \

include ../../plugin.mk


dist: all
ifndef VERSION
	$(error VERSION is not set.)
endif
	mkdir -p dist/dBiz
	cp LICENSE* dist/dBiz/
	cp plugin.* dist/dBiz/
	cp -R res dist/dist/
	cd dist && zip -5 -r dBiz-$(VERSION)-$(ARCH).zip dBiz
