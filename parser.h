/*
 * Copyright 2017 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

#define ERROR__DONE     -1
#define ERROR__ARGUMENT -2
#define ERROR__INTERNAL -3
#define ERROR__SYNTAX   -4
#define ERROR__STACK    -5

#define __DEPTH_BITS 8
#define __STACK_SIZE (1<<__DEPTH_BITS)

typedef struct {
  int at;
  unsigned square : 1;
} stack_t;

typedef struct {
  char *buf;
  int at;
  int len;
  unsigned complete : 1;
  unsigned depth : __DEPTH_BITS;
  stack_t stack[__STACK_SIZE];
} parser_t;

int parser_peek(parser_t *p, int off);