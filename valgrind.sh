#!/bin/sh

valgrind --leak-check=full \
         --show-leak-kinds=all \
         ./a.out input output
#         --show-leak-kinds=all \
#         --track-origins=yes \
#         --verbose \
#         --log-file=valgrind-out.txt \

cat output
