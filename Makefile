
LIBS = -L/home/warmerdam/bld/lib -L/usr/local/lib -lgdal -ljson
INCLUDE = -I/home/warmerdam/bld/include -I/usr/include/gdal \
	-I/usr/include/json

OPTFLAGS = -Wall -g

CPPFLAGS = $(INCLUDE) $(OPTFLAGS)

OBJ =	src/plcinput.o \
	src/plcline.o \
	src/plccontext.o \
	src/plchistogram.o \
	src/json_util.o \
	\
	src/qualitymethodbase.o \
	src/linecompositor.o \
	src/darkestquality.o \
	src/greenestquality.o \
	src/scenemeasurequality.o \
	src/landsat8cloudquality.o \
	src/percentilequality.o \
	src/qualityfromfile.o

compositor:	src/compositor.o $(OBJ)
	g++ -o compositor src/compositor.o $(OBJ) $(LIBS)

$(OBJ):	src/compositor.h Makefile

clean:
	rm -f $(OBJ) src/compositor.o compositor

check:	compositor
	(cd tests; python small_tests.py)
	(cd tests; python saojose_tests.py)

