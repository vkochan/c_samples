CC=gcc
RM=rm -f

LIBS=-lcurses
OBJS=matrix.o
TARGET=matrix

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(WFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS) $(LIBS)

c.o.:
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

clean:
	$(RM) $(TARGET)
	$(RM) *.o
