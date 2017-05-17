TARGET = handycam
CC = gcc
CXX = g++
AR = ar
LIBS = -lallegro_image-static -lpng -lz -lallegro_main-static -lallegro_memfile-static -lallegro_primitives-static -lallegro_dialog-static -lallegro_font-static -lallegro-static -lopengl32 -luser32 -lcomdlg32 -lgdi32 -lshell32 -lshlwapi -lpsapi -lkernel32 -lole32 -lwinmm
CXXFLAGS = -mwindows -static-libgcc -static-libstdc++ -static

.PHONY: default all clean

default: $(TARGET)
all: default


OBJECTS = $(patsubst %.cpp, %.o, $(wildcard imgui/*.cpp)) $(patsubst %.cpp, %.o, $(wildcard *.cpp)) handycam.rc.o
HEADERS = $(wildcard *.h)

%.o: %.cpp $(HEADERS) $(wildcard data/*.h)
	$(CXX) $(CXXFLAGS) $(LIBS) -c $< -o $@

handycam.rc : ;
%.rc.o: %.rc
	windres $^ -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o $(TARGET) $(DATAFILES)
