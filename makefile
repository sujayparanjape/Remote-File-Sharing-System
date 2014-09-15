UNAME = $(shell uname)
ifeq ($(UNAME), SunOS) # Sun OS
MY_LIBS = -lresolv -lsocket -lnsl
endif
ifeq ($(UNAME), Linux) # Linux
MY_LIBS = -lresolv -lnsl -lpthread
endif
ifeq ($(UNAME), Darwin) # Mac OS
MY_LIBS =
endif

CC := g++

all: sujaypar_proj1.o

sujaypar_proj1.o : CustomStructs.h ClientServerBase.h ClientServerBase.cpp Client.h Client.cpp Server.h Server.cpp 
	${CC} sujaypar_proj1.cpp ClientServerBase.cpp Client.cpp Server.cpp -o sujaypar_proj1

##==========================================================================
clean:
	@- $(RM) sujaypar_proj1
	@- echo “Data Cleansing Done.Ready to Compile”
