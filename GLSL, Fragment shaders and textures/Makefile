# Makefile pour Glfw avec GLAD et stb_image
#
# CC BY-SA Edouard.Thiel@univ-amu.fr - 27/01/2025
#
# Installation des packages :
#   sudo apt install libglfw3-dev libgl1-mesa-dev
#
# Pour tout compiler, tapez : make all
# Pour tout compiler en parallèle, tapez : make -j all
# pour supprimer les .o et exécutables : make clean
# Pour tout recompiler : make clean all

SHELL    = /bin/bash
RM       = rm -f
CPP      = g++
CPPFLAGS = -Wall -O2 -fno-strict-aliasing --std=c++17  # -g pour gdb
LIBS     = -lglfw -lGLU -lGL -lm -ldl
CC       = gcc
CFLAGS   = -Wall -O2

# Fichiers à compiler :
# chaque fichier .cpp produira un exécutable du même nom
CFILES  := $(wildcard *.cpp)
EXECS   := $(CFILES:%.cpp=%)

# Règle pour fabriquer les .o à partir des .cpp
%.o : %.cpp
	$(CPP) $(CPPFLAGS) -c $*.cpp

# Règle pour fabriquer les .o à partir des .c
%.o : %.c
	$(CC) $(CFLAGS) -c $*.c

# Déclaration des cibles factices
.PHONY : all clean

# Règle pour produire tous les exécutables.
all : $(EXECS)

# Règle de production de chaque exécutable
$(EXECS) : % : %.o glad.o stb_image.o
	$(CPP) -o $@ $^ $(LIBS)

# Règle de nettoyage - AUTOCLEAN
clean :
	$(RM) *.o *~ $(EXECS) tmp*.*

