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

async function create(bytes, callbacks) {
  const memory = new WebAssembly.Memory({initial: 256, maximum: 256});

  const importObject = (function() {
    const env = {
      'abortStackOverflow': () => { throw new Error('abortStackOverflow'); },
      'table': new WebAssembly.Table({initial: 0, maximum: 0, element: 'anyfunc'}),
      'tableBase': 0,
      'memory': memory,
      'memoryBase': 1024,
      'STACKTOP': 0,
      'STACK_MAX': memory.buffer.byteLength,
      'DYNAMICTOP_PTR': 0,
    };
    for (const key in callbacks) {
      env['_' + key] = callbacks[key];
    }
    return {env};
  }());

  const buffer = new Int8Array(memory.buffer);
  return {wa: await WebAssembly.instantiate(bytes, importObject), buffer};
}

/**
 * Converts buffer to string. Only supports UTF-8.
 *
 * @param {!TypedArray} buffer
 * @return {string}
 */
function bufferToString(buffer) {
  const uint8 = new Uint8Array(buffer);
  return String.fromCodePoint(...uint8);
}
