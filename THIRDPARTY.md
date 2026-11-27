# Third-Party Software Notices and Licenses

GrnGame Engine pre-compiled runtimes and source distributions incorporate the following third-party libraries and
compiler runtimes. Their respective copyright notices and license terms are listed below.

---

## 1. Libraries under the zlib License

* **SDL3, SDL3_image, SDL3_mixer** — Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

**zlib License Text:**
This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable
for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it
and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If
   you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not
   required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original
   software.
3. This notice may not be removed or altered from any source distribution.

---

## 2. Libraries under the MIT License

* **Wren** — Copyright (c) 2013-2021 Robert Nystrom and Wren Contributors
* **cglm** — Copyright (c) 2017 Recp (Recep Aslantas)
* **klib** — Copyright (c) 2008-2018 by Attractive Chaos <attrchaos@live.co.uk>
* **cJSON** — Copyright (c) 2009-2017 Dave Gamble and cJSON contributors
* **haclog** — Copyright (c) 2020-2026 MuggleWei

**MIT License Text:**
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

---

## 3. Libraries under the BSD Licenses

* **Tracy Profiler (3-Clause BSD)** — Copyright (c) 2017-2026, Bartosz Taudul <wolf@nereid.pl>
* **TinyDir (2-Clause Simplified BSD)** — Copyright (c) 2013-2019, Cong Xu

**BSD 3-Clause License Text:**
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
   disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

---

## 4. FreeType (FreeType License - FTL)

* **FreeType** — Portions of this software are copyright © 1996-2026 The FreeType Project (www.freetype.org). All rights
  reserved.

*Note: FreeType is distributed under the FreeType License (FTL), a BSD-style license with an attribution clause recited
above.*

---

## 5. Highway (Apache License 2.0)

* **Highway** — Copyright 2020 Google LLC

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the
License. You may obtain a copy of the License at:
http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific
language governing permissions and limitations under the License.

---

## 6. SQLite3 (Public Domain)

* **SQLite** — The author or authors of this code dedicate any and all copyright interest in this code to the public
  domain. Anyone is free to copy, modify, publish, use, compile, sell, or distribute the original SQLite code, either in
  source code form or as a compiled binary, for any purpose, commercial or non-commercial, and by any means.

---

## 7. Emscripten & WebAssembly Runtime

The WebAssembly (`.wasm` and `.js`) builds of GrnGame Engine are compiled using the **Emscripten SDK** and incorporate
runtime components governed by the following permissive licenses:

* **Emscripten Runtime** — Copyright (c) 2010-2026 Emscripten Contributors. Licensed under the MIT License and the
  University of Illinois/NCSA Open Source License.
* **musl libc (Emscripten C Standard Library)** — Copyright © 2005-2020 Rich Felker, et al. Licensed under the standard
  MIT License.
* **LLVM compiler-rt / libc++abi** — Licensed under the Apache License v2.0 with LLVM Exceptions.

---

## 8. Platform Compiler Toolchains & Standard Libraries

Pre-compiled native binaries of GrnGame Engine link against standard platform runtime libraries provided by their
respective operating system toolchains:

* **Linux & Android (GCC / Clang / Android NDK):** Links against standard runtime libraries (`libgcc`, `libstdc++`, or
  LLVM `compiler-rt`/`libc++`), covered by the **GCC Runtime Library Exception (GPLv3 with Runtime Exception)** and the
  **Apache 2.0 License with LLVM Exceptions**, both of which explicitly permit proprietary and commercial binary
  distribution. Android NDK sysroot headers and libraries are licensed under the **Apache 2.0** and **BSD** licenses.
* **Windows (MSVC Toolchain):** Compiled using Microsoft Visual C++ Build Tools and statically/dynamically linked
  against the Microsoft Visual C++ Redistributable Runtime under the Microsoft Software License Terms.
* **macOS & iOS (Apple Clang / SDK):** Compiled against Apple system frameworks and LLVM runtime libraries under the
  Apache 2.0 License with LLVM Exceptions and Apple SDK License Agreements.