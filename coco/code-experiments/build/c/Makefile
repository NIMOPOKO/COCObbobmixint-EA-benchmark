## Makefile to build C example programs included with the COCO distribution
##
## NOTE: We have tried to make this Makefile as generic and portable
## as possible. However, there are many (incompatible) versions of
## make floating around. We regularly test using GNU make and BSD make
## from FreeBSD. If you have trouble compiling the examples, please
## try to use GNU make. 
##
## On Windows it is best to use either the included NMakefile by running
##
##   nmake -f NMakefile
##
## or installing Cygwin and running GNU make from within Cygwin.

LDFLAGS += -lm
CCFLAGS ?= -g -ggdb -std=c99 -pedantic -Wall -Wextra -Wstrict-prototypes -Wshadow -Wno-sign-compare -Wconversion
# LAMARCKIANとREVISEDオプションを追加
CCFLAGS += -DALGORITHM=$(ALGORITHM) -DENCODING=$(ENCODING) -DAPPROACH=$(APPROACH)
########################################################################
## Toplevel targets
all: example_experiment${NUM}

clean:
	rm -f coco*.o 
	rm -f example_experiment*.o 

########################################################################
## Programs
example_experiment${NUM}: example_experiment${NUM}.o coco${NUM}.o
	${CC} ${CCFLAGS} -o example_experiment${NUM} coco${NUM}.o example_experiment${NUM}.o ${LDFLAGS}  

########################################################################
## Additional dependencies
coco${NUM}.o: coco.h coco.c
	${CC} -c ${CCFLAGS} -o coco${NUM}.o coco.c
example_experiment${NUM}.o: coco.h coco.c example_experiment.c
	${CC} -c ${CCFLAGS} -o example_experiment${NUM}.o example_experiment.c
