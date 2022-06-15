.PHONY: all clean fclean re

NAME			= proxy

SOURCE			= src/tcp_proxy.cpp

HEADERS			= src/tcp_proxy.hpp

OBJECT			=	$(SOURCE:.cpp=.o)

CXX				= 	c++

CXXFLAGS		= 	-std=c++14

#-Wall -Wextra -Werror

LD_FLAGS 		= -lpthread

RESET			= 	\033[0m
GREEN 			= 	\033[38;5;46m
WHITE 			= 	\033[38;5;15m
GREY 			= 	\033[38;5;8m
ORANGE 			= 	\033[38;5;202m
RED 			= 	\033[38;5;160m

all: $(NAME)

%.o: %.cpp $(HEADERS)
	@echo "$(GREY)Compiling...				$(WHITE)$<"
	@$(CXX) $(CXXFLAGS) $< -o $@ 

$(NAME): $(OBJECT)
	@echo "$(GREEN)----------------------------------------------------"
	@$(CXX) $(CXXFLAGS) $(OBJECT) $(LD_FLAGS) -v -o $(NAME) 
	@echo "Executable				./$(NAME) $(RESET)"

clean:
	@echo "$(RED)----------------------------------------------------"
	@/bin/rm -f $(OBJECT)
	@echo "$(WHITE)REMOVED O-FILES $(RESET)"

fclean: clean
	@echo "$(RED)----------------------------------------------------"
	@/bin/rm -f $(NAME)
	@echo "$(WHITE)REMOVED EXECUTABLE FILE $(RESET)"

re: fclean all