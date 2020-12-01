# -------- environment variables
SST_REPO = sst-dir
GOOGLETEST_ARCHIVE_OUTPUT = google-test-dir
HOST=x86_64-linux-gnu
PREFIX=$(CURDIR)

# -------- standard targets

all : download-only sst

download-only : $(SST_REPO) $(GOOGLETEST_ARCHIVE_OUTPUT)

mrclean :
	rm -rf lib
	rm -rf include
	rm -rf share
	rm -rf $(SST_REPO)
	rm -rf $(GOOGLETEST_ARCHIVE_OUTPUT)
	rm -rf $(GOOGLETEST_ARCHIVE_OUTPUT).tar.gz
	rm -f *.success

# --------- Utility functions

define get_and_extract_tarball
	# ----- Target folder name is first parameter
	rm -rf $(1).tmp
	rm -rf $(1)
	mkdir $(1).tmp

	# ---- URL is 2nd parameter
	wget -O $(1).tar.gz $(2)

	# ---- Additional tar options is 3rd parameter
	tar -xvf $(1).tar.gz $(3) --directory $(1).tmp
	mv $(1).tmp $(1)
	rm $(1).tar.gz
endef


define gitclone_and_checkout
	# ---- Target directory is 1st parameter
	rm -rf $(1).tmp
	rm -rf $(1)

	# ---- Git(hub|lab) user 2nd parameter, project is 3rd parameter.
	# ---- Git hash is 3rd parameter
	if [ -n "$(CI_JOB_TOKEN)" ] ; then \
	  git clone "https://gitlab-ci-token:$(CI_JOB_TOKEN)@gitlab.stealthsoftwareinc.com/$(2)/$(3)" $(1).tmp ; \
		cd $(1).tmp && git checkout $(4) ; \
	elif [ '$(INTERNAL)' =  'true' ] ; then \
		git clone "git@gitlab.stealthsoftwareinc.com:$(2)/$(3)" $(1).tmp ; \
		cd $(1).tmp && git checkout $(4) ; \
	else \
		git clone "https://github.com/stealthsoftwareinc/$(3)" $(1).tmp ; \
		cd $(1).tmp && git checkout $(5) ; \
	fi

	mv $(1).tmp $(1)
endef

# --------- Google test Definitions
$(GOOGLETEST_ARCHIVE_OUTPUT) :
	$(call get_and_extract_tarball,$(GOOGLETEST_ARCHIVE_OUTPUT),https://github.com/google/googletest/archive/release-1.10.0.tar.gz,"--strip-components=1")



# ---------- SST Definitions
$(SST_REPO) :
	$(call gitclone_and_checkout,$(SST_REPO),stealth,sst.git,6213552700843ec4ac3d6857aad37e7c1043e28f,e5cb49bc8c6f4d95f54cffc89c42d459b7caa3ec)
	if [ '$(INTERNAL)' = 'true' -o -n "$(CI_JOB_TOKEN)" ] ; then \
		cd $(SST_REPO) && ./autogen ; \
  fi


sst.success : $(SST_REPO)
	cd $(SST_REPO) &&\
		./configure --host=$(HOST) --prefix=$(PREFIX) --with-build-groups=cpp &&\
	  make && \
	  make install \
  ;
	echo 'success' >  sst.success

sst: sst.success
