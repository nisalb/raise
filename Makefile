
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Raise Library

vector_test: vector_test.c vector.c
	gcc -Wall -std=c11 -o $@ $^
