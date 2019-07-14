#!/bin/sh

valgrind --leak-check=full ./a.out
#         --show-leak-kinds=all \
#         --track-origins=yes \
#         --verbose \
#         --log-file=valgrind-out.txt \
