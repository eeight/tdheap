CC = gcc
CFLAGS_ = -Werror -Wall -Wextra -Wpointer-arith -pipe
LFLAGS = -lm
CFILES = task1.c eval.c parse.c tree.c
OBJECTS = $(CFILES:.c=.o)
TARGET = calc

#DEBUG = 1
#JOKE_FAKE_CRASH = 1
#OUT_OF_MEM_HALT = 1
#SOLID_MONS = 1

#.if defined(DEBUG)
CFLAGS = ${CFLAGS_} -g # -DDEBUG
#.else
#CFLAGS += ${CFLAGS_} -O2
#.endif

#.if defined(JOKE_FAKE_CRASH)
#CFLAGS += -DFAKE_CRASH
#.endif

#.if defined (OUT_OF_MEM_HALT)
#CFLAGS += -DOUT_OF_MEM_HALT
#.endif

#.if defined (SOLID_MONS)
CFLAGS += -DSOLID_MONS
#.endif

.PHONY: clean rebuild

all: .depend $(TARGET)

.depend: $(CFILES)
	 $(CC) -MM $(CFILES) >.depend

clean:
	-rm -f $(OBJECTS) $(TARGET) .depend

rebuild: clean all

$(TARGET): $(OBJECTS)
	$(CC)  $(OBJECTS) -o $(TARGET) $(CFLAGS) $(LFLAGS)

#Rule for compiling single .c files
.c.o:
	$(CC)  $< -c $(CFLAGS) $(JOKES)

