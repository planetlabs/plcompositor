
LIBS = -L/home/warmerdam/bld/lib -L/usr/local/lib -lgdal
INCLUDE = -I/home/warmerdam/bld/include -I/usr/include/gdal

CPPFLAGS = $(INCLUDE) -Wall -g

OBJ =	src/plcinput.o \
	src/plcline.o \
	src/plccontext.o \
	src/plchistogram.o \
	\
	src/qualitymethodbase.o \
	src/qualitylinecompositor.o \
	src/medianlinecompositor.o \
	src/darkestquality.o \
	src/greenestquality.o \
	src/scenemeasurequality.o \
	src/landsat8cloudquality.o 

compositor:	src/compositor.o $(OBJ)
	g++ -o compositor src/compositor.o $(OBJ) $(LIBS)

$(OBJ):	src/compositor.h Makefile

clean:
	rm -f $(OBJ) src/compositor.o compositor

check:	compositor
	(cd tests; python compositor_tests.py)


