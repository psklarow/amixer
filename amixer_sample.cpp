/*

Copyright (c) 2025 Pawel Sklarow https://github.com/psklarow

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

/// @file amixer_sample.cpp
/// @brief Example usage of ALSA mixer interface defined in amixer.hpp.
/// This example finds the "Speaker" channel and decreases its volume by 2 percentage points.
/// If the "Speaker" channel is not found, the program exits with code 1.
///
/// Note: this code has been developed with AI assistance.
/// Note: this file uses C++23 features, such as std::clamp and std::ranges.
///
/// To build use e.g.:
///     c++ -std=c++23 amixer_sample.cpp amixer.cpp -o amixer -lasound -Wall -Wextra -Wpedantic -Werror

#include "amixer.hpp"
#include <print>
#include <cstdlib>

int main()
{
    std::println("Hello, World!");
    IMixer &mixer = IMixer::alsaInstance();

    auto vi = std::ranges::find_if(mixer.channels(), 
        [](const auto &vol) {
            return vol->getName()=="Speaker";
        }
    );

    if (vi == mixer.channels().end()) return 1;

    auto v = *vi;

    std::println("Name: '{}' volume: {} balance: {}",
        v->getName(), v->getVolume(), v->getBalance());

    v->setVolume(v->getVolume()-2);

    std::println("Name: '{}' volume: {} balance: {}",
        v->getName(), v->getVolume(), v->getBalance());

    return 0;
}
