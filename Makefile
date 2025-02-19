SHELL := /bin/bash
VERSION := $(shell cat VERSION)
BUILDDIR ?= .build

.DEFAULT_GOAL := build

.PHONY: all build bumpversionminor bumpversionmajor clean format install scan-build srpm srpm-release tag

$(BUILDDIR):
	@echo $(BUILDDIR) not existing run \'meson $(BUILDDIR)\'
	@exit -1


all: format build

build: $(BUILDDIR)
	ninja -C $(BUILDDIR)

clean: $(BUILDDIR)
	ninja -C $(BUILDDIR) clean

install: $(BUILDDIR)
	DESTDIR=$(DESTDIR) ninja -C $(BUILDDIR) install

format: $(BUILDDIR)
	ninja -C $(BUILDDIR) clang-format

scan-build: $(BUILDDIR)
	ninja -C $(BUILDDIR) scan-build

srpm: $(BUILDDIR)
	make -C pkg/testing/rpm outdir=$(CURDIR)

srpm-release: $(BUILDDIR)
	make -C pkg/release/rpm outdir=$(CURDIR)

bumpversionminor bumpversionmajor:
	$(eval BUMP = $(shell echo $@ | sed 's/bumpversion//'))
	@IFS='.' read -r -a ver <<< "$(VERSION)"; \
	if [ "$(BUMP)" = "major" ]; then \
	  ver[0]=$$(($${ver[0]} + 1)); \
	  ver[1]=0; \
	  ver[2]=0; \
	elif [ "$(BUMP)" = "minor" ]; then \
	  ver[1]=$$(($${ver[1]} + 1)); \
	  ver[2]=0; \
	else \
	  echo invalid bump target \'$(BUMP)\'; \
	  exit -1; \
	fi; \
	BUMPED_VER="$${ver[0]}.$${ver[1]}.$${ver[2]}"; \
	echo "$$BUMPED_VER" > VERSION; \
	git commit -vsam "Bump version to $$BUMPED_VER"
	@echo "Don't forget to run 'make tag'"

tag:
	git tag -sa $(VERSION) -m "$(VERSION)"
	@echo "now run \'git push origin $(VERSION)\'"

