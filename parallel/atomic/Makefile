CC=gcc
RM=rm -f

LIBS=-lpthread
OBJS=increment.o
TARGET=increment

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(WFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS) $(LIBS)

c.o.:
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

clean:
	$(RM) $(TARGET)
	$(RM) *.o
