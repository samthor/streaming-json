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

#include <ctype.h>
#include <emscripten.h>
#include <string.h>

#include "parser.h"

int token_callback(int start, int end);
int depth_callback(int start, int end, int depth);

int parser_peek(parser_t *p, int off) {
  if (p->at + off >= p->len) {
    if (!p->complete) {
      return -1;  // more data pending
    }
    return 0;
  }
  return p->buf[p->at + off];
}

int parser_next(parser_t *p) {
  int curr = parser_peek(p, 0);

  // consume number (whole at a time)
  if (isdigit(curr) || curr == '.') {
    int seen_dot = (curr == '.');
    int len = 1;
    for (;;) {
      int next = parser_peek(p, len);
      if (isdigit(next) || next == 'E' || next == '+' || next == '-') {
        // ok
      } else if (!seen_dot && next == '.') {
        seen_dot = 1;
      } else if (next < 0) {
        return 0;  // incomplete token
      } else {
        return len;
      }
      ++len;
    }
    // should never get here
  }

  switch (curr) {
    case -1:
    case 0:
      return 0;  // done

    case '{': {
      stack_t q = {p->at, 0};
      p->stack[++p->depth] = q;
      return 1;
    }
    case '[': {
      stack_t q = {p->at, 1};
      p->stack[++p->depth] = q;
      return 1;
    }

    case '}': 
      if (!p->depth || p->stack[p->depth--].square != 0) {
        return ERROR__STACK;
      }
      return 1;
    
    case ']': 
      if (!p->depth || p->stack[p->depth--].square == 0) {
        return ERROR__STACK;
      }
      return 1;

    case ',':
    case ':':
      return 1;

    case '"':
      break;

    default: {
      // look for any alpha words: should only be null/false, but whatever
      int count = 0;
      while (isalpha(curr)) {
        curr = parser_peek(p, ++count);
      }
      if (count > 0) {
        return count;
      }
      return ERROR__SYNTAX;
    }
  }

  // consume strings
  // TODO: partial strings
  int len = 0;
  for (;;) {
    int next = parser_peek(p, ++len);
    if (next == '"') {
      ++len;
      break;
    } else if (next < 0) {
      return 0;  // incomplete token
    } else if (!next) {
      return ERROR__SYNTAX;  // invalid string
    } else if (next == '\\') {
      // completely ignore next char
      ++len;
    }
  }
  return len;
}

static parser_t p;

EMSCRIPTEN_KEEPALIVE
int streaming_init(char *buf) {
  p.buf = buf;
  p.at = 0;
  p.complete = 0;
  p.len = 0;
  p.depth = 0;
  return 0;
}

EMSCRIPTEN_KEEPALIVE
int streaming_next(int len) {
  if (p.complete) {
    return ERROR__DONE;
  } else if (len == 0) {
    p.complete = 1;
  } else if (len < p.len) {
    return ERROR__ARGUMENT;
  } else {
    p.len = len;
  }

  int last = -1;

  for (;;) {
    // eat whitespace
    while (p.at < p.len && isspace(p.buf[p.at])) {
      ++p.at;
    }

    // store positions for comparisons
    int depth = p.depth;
    int at = p.at;

    // sanity checking that we always move forward
    if (at <= last) {
      return ERROR__INTERNAL;
    }
    last = at;

    // grab next token
    int part = parser_next(&p);
    if (part < 0) {
      return part;  // error
    } else if (part == 0) {
      return p.at;  // consumed this much
    }

    // check depth change
    if (depth - 1 == p.depth) {
      int from = p.stack[depth].at;
      depth_callback(from, p.at + part, depth);
    }

    // yield token itself
    p.at += part;  // move past it
    token_callback(at, p.at);
  }
}