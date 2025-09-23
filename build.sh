gcc -fsanitize=address,undefined -O1 -Wall -Wextra -Wno-unused-function -lm -D __DEBUG__ -I./src src/main.c src/logging/logging.c -o dist/main.out && ./dist/main.out
