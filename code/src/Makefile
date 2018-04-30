include ../etc/pi-solar.conf
CC=gcc
CFLAGS= -O3 -Wall -g
AR=ar
ifndef BINDIR
	BINDIR="${pi-solar-dir}/bin"
endif

ALLBIN=getvictron daytcalc pvpower
ALLSH=solar-rrd.sh solar-data.sh solar-night.sh

all: ${ALLBIN}

install:
	@echo -n "Checking for bin directory ${BINDIR}:"
	@if test -d ${BINDIR}; then \
		echo " ..OK. ${BINDIR} exists."; \
	else \
		install -v -d ${BINDIR}; \
		echo " ..OK. ${BINDIR} created."; \
	fi
	install -v --mode=750 --owner=pi --strip ${ALLBIN} ${BINDIR}
	@echo
	@echo "Programs ${ALLBIN} installed in ${BINDIR}."
	@echo
	install -v --mode=750 --owner=pi ${ALLSH} ${BINDIR}
	@echo
	@echo "Scripts ${ALLSH} installed in ${BINDIR}."

clean:
	rm -f *.o ${ALLBIN}

getvictron: serial.o getvictron.o
	$(CC) serial.o getvictron.o -o getvictron

daytcalc: daytcalc.o
	$(CC) daytcalc.o -o daytcalc -lm

pvpower: pvpower.o
	$(CC) pvpower.o -o pvpower -lrrd
