noinst_DATA = $(top_srcdir)/doc/html/index.html

$(noinst_DATA): $(top_srcdir)/doc/doxygen/pepper.doxygen
	cd $(top_srcdir)/doc/doxygen && $(DOXYGEN) pepper.doxygen

clean-local:
	rm -rf $(top_srcdir)/doc/html
