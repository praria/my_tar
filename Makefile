CC = gcc
CFLAGS = -Wall -Wextra -Werror

SRCS = main.c tar_operations.c
OBJS = $(SRCS:.c=.o)
TARGET = my_tar

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(TARGET)

re: fclean all
