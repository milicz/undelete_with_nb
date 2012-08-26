CXXFLAGS = -O2 -g -mwindows -Wall -fmessage-length=0

TARGET = undelete.exe

$(TARGET): milan.o
	$(CXX) -o $(TARGET) milan.o
# above, we are saying undelete.exe depends on milan.o
# and to create undelete.exet we give the g++ command as shown on the next line

milan.o : milan.cpp milan.h NTFSDrive.h
	$(CXX) -g -c milan.cpp


clean:
	rm *.o undelete.exe
 
 
all : $(TARGET)


#An Example :
#Lets say we have 3 .cpp files and 2 .h files that make up our program. They are:
#main1.cpp
#mylib.cpp
#mylib.h
#openfile.cpp
#openfile.h

#We decide we want to create an executable program named lab1.out
#So, we set up a make file. HOW? We use our favorite text editor (vi or pico or emacs) and we create our make file, lab1.make. We name it lab1.make so we know its a make file and not a source code or header or executable file. If you have separate directories for each assignment(a good idea) then naming the file "makefile" is perfectly logical, since it will be in the directory of the files to "make".

#Once in our editor we write the make file which is really a group of targets, rules, and dependencies, here is a make file named lab1.make created using an editor.

# lab1.make -- this is a comment line, ignored by make utility

#lab1.out : main1.o mylib.o openfile.o
#      g++ -o lab1.out main1.o mylib.o openfile.o
# above, we are saying lab1.out depends on main1.o, mylib.o and openfile.o
# and to create lab1.out we give the g++ command as shown on the next line

# which starts with a TAB although you cannot see that .
# note that the command : g++ -o lab1.out main1.o mylib.o openfile.o
# creates an executable file named lab1.out from the 3 object files respectively.
#main1.o: main1.cpp openfile.h mylib.h
#      g++ -c main1.cpp
# above we are saying main1.o depends on main1.cpp openfile.h and mylib.h
# and to compile only main1.cpp if and only if main1.cpp or openfile.h or mylib.h
# have changed since the last creation of main1.o
#mylib.o: mylib.cpp mylib.h
#     g++ -c mylib.cpp
# above we are saying mylib.o depends on mylib.cpp and mylib.h
# so if either mylib.cpp or mylib.h CHANGED since creating mylib.o
# comple mylib.cpp again
#openfile.o: openfile.cpp openfile.h
#      g++ -c openfile.cpp
# above we are saying openfile.o depends on openfile.cpp and openfile.h
# so if either openfile.cpp or openfile.h has CHANGED since creating
# openfile.o, compile only (again) openfile.cpp
#clean:
# &nbps    rm *.o lab1.out
# above we are stating how to run the rule for clean, no dependencies,
# what we want is when we ask to do a "make -f lab1.make clean"
# that will not do anything except remove executable and object files
# so we can "clean out" our directory of unneeded large files.
# we only do a make clean when we want to clean up the files.

# END OF MAKE FILE 