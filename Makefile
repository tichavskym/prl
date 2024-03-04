NUMBERS=8
PROC=$(shell python -c "import math; print(math.ceil(math.log($(NUMBERS))/math.log(2) + 1))")


.PHONY: build generate run calc

all: build

build:
	mpic++ --prefix /usr/local/share/OpenMPI -o pms pms.cpp

generate:
	dd if=/dev/random bs=1 count=$(NUMBERS) of=numbers 2> /dev/null

run: build
	# TODO use-hwthread-cpus is my addition to the command from assignment
	mpirun --prefix /usr/local/share/OpenMPI --use-hwthread-cpus -np $(PROC) pms

clean:
	rm numbers pms