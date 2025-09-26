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

 // Note: this code has been developed with AI assistance.

/// @file amixer.hpp
/// @brief mixer interface and volume control classes.
/// This file defines the IVolume and IMixer interfaces for volume control.
/// It also provides the ALSA implementation of these interfaces.

#ifndef __AMIXER_HPP__
#define __AMIXER_HPP__

#include <list>
#include <memory>
#include <string>
#include <ranges>

/// @brief Interface for volume control of a single channel (e.g., "Speaker", "Headphone").
/// The channel may be mono (only mono controller) or stereo (left and right controllers).
/// The interface provides methods to set and get volume and balance.
class IVolume {
public:
    virtual ~IVolume() = default;
    virtual void setVolume(int volume) =0; // 0..100 percentage
    virtual void setBalance(int balance) =0; // -100 (left only) .. 0 (center) .. +100 (right only)
    virtual const std::string getName() = 0; 
    virtual int getVolume() = 0; // 0..100 percentage
    virtual int getBalance() = 0; // -100 (left only) .. 0 (center) .. +100 (right only
};

/// @brief Interface for mixer providing access to available volume channels.
class IMixer{
public:
    virtual ~IMixer()=default;

    /// @brief Get list of available volume channels.
    /// @return List of shared pointers to IVolume instances.
    virtual const std::list<std::shared_ptr<IVolume>> &channels() const = 0;

    /// @brief Get singleton instance of ALSA mixer
    /// @return Reference to IMixer instance supporting ALSA
    static IMixer& alsaInstance();
};


 #endif // __AMIXER_HPP__