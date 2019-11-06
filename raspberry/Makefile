# define the C compiler to use
CC = gcc

CFLAGS_EXTRA = -I/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/arm-linux-gnueabihf/include

MAIN = irb6-driver
CFLAGS = -Wall -g
INCLUDES = -I/usr/include/xenomai
LFLAGS = -L/usr/lib/xenomai
LIBS = -lnative -lxenomai -lm -llua -ldl
SRCS = driver.c


# define the C object files 
#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
OBJS = $(SRCS:.c=.o)

#
# The following part of the makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

.PHONY: depend clean test pi pi_cc preview

$(MAKECMDGOALS):$(MAIN)
		@echo sketchy has been compiled

$(MAIN):$(OBJS) 
		$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
.c.o:
		$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
		$(RM) *.o *~ sketchy test sketchy-driver sketchy-preview

install:
		install sketchy-driver ../build

installp:
		install sketchy-preview ../build

depend: $(SRCS)
		makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it
