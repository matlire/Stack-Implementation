gcc  -O1 -Wall -Wextra -Wno-unused-function -lm -D __DEBUG__ -I./src src/logging/logging.c src/stack/stack.c src/main.c -o dist/main.out && ./dist/main.out
