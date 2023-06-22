ifeq ($(TARGET),cross)
	CC=riscv64-unknown-linux-gnu-g++
	AR=riscv64-unknown-linux-gnu-ar
else
	CC=g++
	AR=ar
endif

# default to use code of scheduling, set the scheduler policy in monitor header

OPTIONS=-static -Dscheduler

ifneq ($(BIND),no)	
	OPTIONS+=-Dbinding 
endif

OPTIONS=-static -Dbinding -Dscheduler -std=c++11

OBJ_FOLDER=	./obj
SRC_FOLDER=	./src
DOCS_FOLDER= ./docs
BIN_FOLDER=	./bin

all:example 


lib: src/monitor.cc src/monitor.h src/worker.cc src/worker.h
	mkdir -p $(OBJ_FOLDER)
	$(CC) $(OPTIONS) -c -o $(OBJ_FOLDER)/monitor.o $(SRC_FOLDER)/monitor.cc -I$(SRC_FOLDER)
	$(CC) $(OPTIONS) -c -o $(OBJ_FOLDER)/worker.o $(SRC_FOLDER)/worker.cc -I$(SRC_FOLDER)
	$(AR) rvs $(OBJ_FOLDER)/lib.a $(OBJ_FOLDER)/monitor.o $(OBJ_FOLDER)/worker.o

libd: src/monitor.cc src/monitor.h src/worker.cc src/worker.h
	mkdir -p $(OBJ_FOLDER)
	$(CC) $(OPTIONS) -Ddebug -c -o $(OBJ_FOLDER)/monitor.o $(SRC_FOLDER)/monitor.cc -I$(SRC_FOLDER)
	$(CC) $(OPTIONS) -c -o $(OBJ_FOLDER)/worker.o $(SRC_FOLDER)/worker.cc -I$(SRC_FOLDER) 
	$(AR) rvs $(OBJ_FOLDER)/lib.a $(OBJ_FOLDER)/monitor.o $(OBJ_FOLDER)/worker.o

libinstr: src/monitor.cc src/monitor.h src/worker.cc src/worker.h
	mkdir -p $(OBJ_FOLDER)
	$(CC) $(OPTIONS) -Dinstr -c -o $(OBJ_FOLDER)/monitor.o $(SRC_FOLDER)/monitor.cc -I$(SRC_FOLDER)
	$(CC) $(OPTIONS) -c -o $(OBJ_FOLDER)/worker.o $(SRC_FOLDER)/worker.cc -I$(SRC_FOLDER) 
	$(AR) rvs $(OBJ_FOLDER)/lib.a $(OBJ_FOLDER)/monitor.o $(OBJ_FOLDER)/worker.o

libactive: src/monitor.cc src/monitor.h src/worker.cc src/worker.h
	mkdir -p $(OBJ_FOLDER)
	$(CC) $(OPTIONS) -Dactive -c -o $(OBJ_FOLDER)/monitor.o $(SRC_FOLDER)/monitor.cc -I$(SRC_FOLDER)
	$(CC) $(OPTIONS) -c -o $(OBJ_FOLDER)/worker.o $(SRC_FOLDER)/worker.cc -I$(SRC_FOLDER) 
	$(AR) rvs $(OBJ_FOLDER)/lib.a $(OBJ_FOLDER)/monitor.o $(OBJ_FOLDER)/worker.o

libactiveinstr: src/monitor.cc src/monitor.h src/worker.cc src/worker.h
	mkdir -p $(OBJ_FOLDER)
	$(CC) $(OPTIONS) -Dactive -Dinstr -c -o $(OBJ_FOLDER)/monitor.o $(SRC_FOLDER)/monitor.cc -I$(SRC_FOLDER)
	$(CC) $(OPTIONS) -c -o $(OBJ_FOLDER)/worker.o $(SRC_FOLDER)/worker.cc -I$(SRC_FOLDER) 
	$(AR) rvs $(OBJ_FOLDER)/lib.a $(OBJ_FOLDER)/monitor.o $(OBJ_FOLDER)/worker.o

libdemo: src/monitor.cc src/monitor.h src/worker.cc src/worker.h
	mkdir -p $(OBJ_FOLDER)
	$(CC) $(OPTIONS) -Ddemo -c -o $(OBJ_FOLDER)/monitor.o $(SRC_FOLDER)/monitor.cc -I$(SRC_FOLDER)
	$(CC) $(OPTIONS) -Ddemo -c -o $(OBJ_FOLDER)/worker.o $(SRC_FOLDER)/worker.cc -I$(SRC_FOLDER)
	$(AR) rvs $(OBJ_FOLDER)/lib.a $(OBJ_FOLDER)/monitor.o $(OBJ_FOLDER)/worker.o

example:lib
	mkdir -p $(BIN_FOLDER)
	$(CC) -O0 -o $(BIN_FOLDER)/example \
	$(SRC_FOLDER)/example.cc \
	$(OBJ_FOLDER)/lib.a

debugexample: libd
	mkdir -p $(BIN_FOLDER)
	$(CC) -O0 -o $(BIN_FOLDER)/debugexample \
	$(SRC_FOLDER)/example.cc \
	$(OBJ_FOLDER)/lib.a

instrexample:libinstr
	mkdir -p $(BIN_FOLDER)
	$(CC) -O0 -o $(BIN_FOLDER)/instrexample \
	$(SRC_FOLDER)/example.cc \
	$(OBJ_FOLDER)/lib.a

activeexample: libactive
	mkdir -p $(BIN_FOLDER)
	$(CC) -O0 -o $(BIN_FOLDER)/activeexample \
	$(SRC_FOLDER)/example.cc \
	$(OBJ_FOLDER)/lib.a

activeinstrexample: libactiveinstr
	mkdir -p $(BIN_FOLDER)
	$(CC) -O0 -o $(BIN_FOLDER)/activeinstrexample \
		$(SRC_FOLDER)/example.cc \
		$(OBJ_FOLDER)/lib.a

demoexample: libdemo
	mkdir -p $(BIN_FOLDER)
	$(CC) -O0 -o $(BIN_FOLDER)/demoexample \
	$(SRC_FOLDER)/example.cc \
	$(OBJ_FOLDER)/lib.a

demo: libdemo
	$(CC) $(OPTIONS) -O0 -o $(BIN_FOLDER)/demo \
	$(SRC_FOLDER)/demo.cc \
	$(OBJ_FOLDER)/lib.a

docs:
	doxygen Doxyfile
	cd ./docs/latex && make all

.PHONY: clean
clean:
	rm -f $(OBJ_FOLDER)/*
	rm -f $(BIN_FOLDER)/*
	rm -fr $(DOCS_FOLDER)
