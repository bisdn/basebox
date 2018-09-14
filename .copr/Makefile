srpm:
	INSTALL="" ;\
	which git || INSTALL="$$INSTALL git" ;\
	which envsubst || INSTALL="$$INSTALL gettext" ;\
	test -n "$$INSTALL" && dnf install $$INSTALL ; :

	if [ "$$(git rev-parse --abbrev-ref HEAD)" = "HEAD" ]; then \
		git checkout -b copr; \
	fi

	make -C $(dir $(spec)) outdir="$(outdir)" spec="$(spec)"
