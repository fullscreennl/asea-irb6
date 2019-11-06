# define the C compiler to use
CC = gcc

MAIN = irb6-driver
CFLAGS = -Wall -g
#INCLUDES = -I/usr/xenomai/include/cobalt -I/usr/xenomai/include -march=armv7-a -mfloat-abi=hard -mfpu=neon -ffast-math -D_GNU_SOURCE -D_REENTRANT -fasynchronous-unwind-tables -D__COBALT__
INCLUDES = -I/usr/xenomai/include/cobalt -I/usr/xenomai/include -march=armv7-a -mfloat-abi=hard -mfpu=neon -ffast-math -D_GNU_SOURCE -D_REENTRANT -fasynchronous-unwind-tables -D__COBALT__ -I/usr/xenomai/include/alchemy -Ibcm2835 -I/usr/local/include -L/usr/local/lib
LFLAGS = -L/usr/lib/xenomai
#LIBS = -Wl,--no-as-needed -Wl,@/usr/xenomai/lib/modechk.wrappers  /usr/xenomai/lib/xenomai/bootstrap.o -Wl,--wrap=main -Wl,--dynamic-list=/usr/xenomai/lib/dynlist.ld -L/usr/xenomai/lib -lcobalt -lmodechk -lpthread -lrt
LIBS = -Wl,--no-as-needed -Wl,@/usr/xenomai/lib/modechk.wrappers -lalchemy -lcopperplate /usr/xenomai/lib/xenomai/bootstrap.o -Wl,--wrap=main -Wl,--dynamic-list=/usr/xenomai/lib/dynlist.ld -L/usr/xenomai/lib -lcobalt -lmodechk -lpthread -lrt -lwiringPi
SRCS = driver.c bcm2835/bcm2835.c


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
