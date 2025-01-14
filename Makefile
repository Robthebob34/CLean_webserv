# Compiler and flags
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -fsanitize=address -g

# Target name
NAME = webserv

# Directories
SRCDIR = src
INCDIR = include
UPLOADDIR = www/uploads

# Source files
SRC = $(SRCDIR)/main.cpp \
      $(SRCDIR)/Servers.cpp \
	  $(SRCDIR)/Config.cpp \
	  $(SRCDIR)/utils.cpp \

# Object files
OBJ = $(SRC:.cpp=.o)

# Include directory
INCLUDES = -I$(INCDIR)

# Rules
all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(NAME) $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean clean_uploads
	rm -f $(NAME)

clean_uploads:
	rm -rf $(UPLOADDIR)/*

re: fclean all

.PHONY: all clean fclean re
