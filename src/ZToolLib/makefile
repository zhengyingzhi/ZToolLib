CC=gcc
# CC_FLAG=-std=gnu11 -fPIC -O2 -D__STDC_FORMAT_MACROS -Wno-unused-parameter
CC_FLAG=-fPIC -O2 -D__STDC_FORMAT_MACROS -Wno-unused-parameter

#this is your target name
TARGET=libztoollib.a

#declare the sources and compile objects
SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

#the public includes description
INCLUDE=
LIBS_L=

#compile all .cpp sources to .o files
%.o:%.c
	$(CC) $(CC_FLAG) $(INCLUDE) -c $< -o $@

#make target file
$(TARGET) : $(OBJECTS)
	ar -crv $(TARGET) $(OBJECTS)
	mv $(TARGET) ../../build/release

clean:
	rm -rf *.o
	rm -rf $(TARGET)
