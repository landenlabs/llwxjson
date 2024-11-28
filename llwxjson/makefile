
SRCS = llwxjson.cpp json.cppß
OBJS = $(SRCS:.cpp=.o)
LDFLAGS = 
	  
llwxjson : $(OBJS)
	g++ -o llwxjson $(OBJS)  $(LDFLAGS)

	
$(OBJS) : $(SRCS)
	g++  -c $(SRCS)
	
	
all :
	${MAKE} llwxjson
	
