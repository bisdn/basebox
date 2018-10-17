FULLVERSION := $(shell git describe --always --tags)
LASTTAG := $(shell git rev-list --tags --max-count=1)
VERSION := $(shell git describe --tags $(LASTTAG) | tr -d '-')
NCOMMITS := $(shell git rev-list $(LASTTAG)..HEAD --count)
COMMIT := $(shell git rev-parse HEAD)
SHORTCOMMIT := $(shell echo $(COMMIT) | cut -c1-7)
BRANCH := $(shell git rev-parse --abbrev-ref HEAD)

BASEDIR := "$(CURDIR)/../../../"

PROJECT := "baseboxd"
outdir ?= $(CURDIR)

build: clean dist spec
	# srpm
	rpmbuild -bs ./$(PROJECT).spec --define "_sourcedir $(CURDIR)" --define "_srcrpmdir $(outdir)"

dist:
	# clean copy to not affect the current workdir
	(cd $(CURDIR) && git clone --recursive $(BASEDIR) $(PROJECT)-$(COMMIT) -b $(BRANCH))
	tar czf $(PROJECT)-$(SHORTCOMMIT).tar.gz $(PROJECT)-$(COMMIT)
	rm -rf $(PROJECT)-$(COMMIT)

spec:
	# create spec
	FULLVERSION=$(FULLVERSION) \
	  VERSION=$(VERSION) \
	  NCOMMITS=$(NCOMMITS) \
	  COMMIT=$(COMMIT) \
	  SHORTCOMMIT=$(SHORTCOMMIT) \
	  envsubst < ./$(PROJECT).spec.envsubst > ./$(PROJECT).spec

clean:
	rm -rf $(PROJECT)-* $(PROJECT).spec

.PHONY: build clean spec dist
