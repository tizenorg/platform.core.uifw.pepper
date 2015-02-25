bin_PROGRAMS =

# Pepper server executable
bin_PROGRAMS += pepper

pepper_LDFLAGS = -export-dynamic
pepper_CFLAGS = $(COMPOSITOR_CFLAGS)
pepper_LDADD = $(COMPOSITOR_LIBS)
pepper_SOURCES = pepper.c
