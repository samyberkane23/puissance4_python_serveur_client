CFLAGS :=  -Wall -Wextra -Wpedantic  
EXE := server client

all: $(EXE)

client: utils.o tcp.o message.o puissance.o
server: utils.o tcp.o message.o puissance.o

puissance.o: puissance.h
utils.o: utils.h
tcp.o: utils.h tcp.h
message.o: message.h utils.h tcp.h puissance.h


clean:
	$(RM) $(EXE) *~ *.o

.PHONY: all clean
