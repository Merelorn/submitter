CC = g++
CFLAGS = -std=c++11 -fPIC -static
LFLAGS = -L/home/wondrash/Dropbox/c++/lib
INCLUDES = -I/home/wondrash/Dropbox/c++/include
LIBS = -lprops
STATLIB = /home/wondrash/c++/lib/libprops.a
SRCS = core.cpp main.cpp addjob.cpp killjob.cpp statusjob.cpp slash.cpp commit.cpp
OBJS = $(SRCS:.cpp=.o)

default: submitter addjob killjob statusjob slash commit
	@echo "all binaries have been compiled"

test:
	$(CC) $(INCLUDES) $(CFLAGS) $(LFLAGS) test.cpp -o test

slash: $(OBJS)
	$(CC) $(INCLUDES) $(CFLAGS) $(LFLAGS) core.o slash.o $(STATLIB) -o slash
	
submitter: $(OBJS)
	$(CC) $(INCLUDES) $(CFLAGS) $(LFLAGS) core.o main.o $(STATLIB) -o submitter

commit:
	$(CC) $(INCLUDES) $(CFLAGS) $(LFLAGS) core.o commit.o $(STATLIB) -o commit

addjob:
	$(CC) $(INCLUDES) $(CFLAGS) $(LFLAGS) core.o addjob.o $(STATLIB) -o addjob

killjob:
	$(CC) $(INCLUDES) $(CFLAGS) $(LFLAGS) core.o killjob.o $(STATLIB) -o killjob

statusjob:
	$(CC) $(INCLUDES) $(CFLAGS) $(LFLAGS) core.o statusjob.o $(STATLIB) -o statusjob

%.o:%.cpp
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@
	
clean:
	rm submitter addjob killjob statusjob
	rm $(OBJS);
