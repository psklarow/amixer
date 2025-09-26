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

/// @file amixer.cpp
/// @brief mixer interface and volume control classes implementation.
/// This file defines the IVolume and IMixer interfaces for volume control.
/// It also provides the ALSA implementation of these interfaces.
///
/// Note: this code has been developed with AI assistance.
/// Note: this file uses C++23 features, such as std::clamp and std::ranges.
///      Ensure your compiler supports C++23 and is configured accordingly.

#include "amixer.hpp"

#include <string>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <alsa/asoundlib.h>
#include <print>
#include <cmath>

/// @brief Volume controller base class for different volume control methods (Decibel, Linear, Dummy).
/// Each channel (left, right, mono) has its own controller instance.
/// The controller is created based on the capabilities of the ALSA mixer element.
/// The controller provides methods to set and get volume in percentage (0..100).
/// The actual implementation depends on the capabilities of the ALSA mixer element.
/// For example, if the element supports dB volume control, the VolumeControllerDb is used.
/// If it supports only linear volume control, the VolumeControllerLinear is used.
/// If it supports neither, the VolumeControllerDummy is used, which does nothing.
/// The VolumeController class is an abstract base class with pure virtual methods.
/// The derived classes implement the actual volume control logic.
///
/// @see VolumeControllerDb, VolumeControllerLinear, VolumeControllerDummy
class VolumeController {
protected:
    snd_mixer_elem_t *mixer_elem;
    snd_mixer_selem_channel_id_t channel;
    
    explicit VolumeController(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t ch) : mixer_elem(elem), channel(ch) {}

public:
    static std::shared_ptr<VolumeController> create(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t ch);

    virtual ~VolumeController() = default;
    virtual void setVolume(int volume) = 0; // 0..100 percentage
    virtual int getVolume() = 0;            // 0..100 percentage
};

/// @brief Volume controller using Decibel scale.
/// This controller maps volume percentage (0..100) to dB range supported by the ALSA mixer element.
/// It uses linear interpolation between the minimum and maximum dB values.
class VolumeControllerDb : public VolumeController {
private:
    long dbMin;
    long dbMax;
    long dbRange;
public:
    explicit VolumeControllerDb(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t ch) : VolumeController(elem, ch), dbMin(0), dbMax(0), dbRange(0) {
        if (mixer_elem) {
            if (snd_mixer_selem_get_playback_dB_range(elem, &dbMin, &dbMax) == 0) {
                dbRange = dbMax - dbMin;
            } else {
                dbMin = dbMax = dbRange = 0;
            }
        }
    }

    void setVolume(int volume) override {
        if (!mixer_elem || dbRange <= 0)
            return;
        double volumeNorm = std::clamp(volume/100.0, 0.0, 1.0); // 0..1
        long dB = dbMin + lround(volumeNorm * dbRange);
        snd_mixer_selem_set_playback_dB(mixer_elem, channel, dB, 0);
    }

    int getVolume() override {
        if (!mixer_elem || dbRange <= 0)
            return 0;
        long dB;
        if (snd_mixer_selem_get_playback_dB(mixer_elem, channel, &dB) != 0)
            return 0;
        double volumeNorm = static_cast<double>(dB - dbMin) / static_cast<double>(dbRange);
        int volume = std::clamp(static_cast<int>(lround(volumeNorm * 100.0)), 0, 100);
        return volume;
    }

};

/// @brief Volume controller using linear scale.
/// This controller maps volume percentage (0..100) to linear volume range supported by the ALSA mixer element.
/// It uses linear interpolation between the minimum and maximum volume values.
/// Note: Linear volume control may not correspond to perceived loudness as well as dB control.
/// It is recommended to use dB control if available.
///      However, some ALSA elements may not support dB control, in which case linear control is used as a fallback.
class VolumeControllerLinear : public VolumeController {
private:
    long volMin;
    long volMax;
    long volRange;
public:
    explicit VolumeControllerLinear(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t ch) : VolumeController(elem, ch), volMin(0), volMax(0), volRange(0) {
        if (mixer_elem) {
            if (snd_mixer_selem_get_playback_volume_range(elem, &volMin, &volMax) == 0) {
                volRange = volMax - volMin;
            } else {
                volMin = volMax = volRange = 0;
            }
        }
    }

    void setVolume(int volume) override {
        if (!mixer_elem || volRange <= 0)
            return;
        double volumeNorm = std::clamp(volume/100.0, 0.0, 1.0); // 0..1
        long vol = volMin + lround(volumeNorm * volRange);
        snd_mixer_selem_set_playback_volume(mixer_elem, channel, vol);
    }

    int getVolume() override {
        if (!mixer_elem || volRange <= 0)
            return 0;
        long vol;
        if (snd_mixer_selem_get_playback_volume(mixer_elem, channel, &vol) != 0)
            return 0;
        double volumeNorm = static_cast<double>(vol - volMin) / static_cast<double>(volRange);
        int volume = std::clamp(static_cast<int>(lround(volumeNorm * 100.0)), 0, 100);
        return volume;
    }
};

/// @brief Dummy volume controller.
/// This controller does nothing and always returns 0 volume.
/// It is used when the ALSA mixer element does not support any volume control (neither dB nor linear).
/// This allows the rest of the code to work without special cases for unsupported elements.
/// Note: This controller is not very useful in practice, but it provides a fallback mechanism.
class VolumeControllerDummy : public VolumeController {
public:
    explicit VolumeControllerDummy(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t ch) : VolumeController(elem, ch) {}
    void setVolume([[maybe_unused]] int volume) override {}
    int getVolume() override { 
        return 0; 
    }
};

/// @brief Factory method to create appropriate VolumeController based on ALSA mixer element capabilities.
/// @param elem ALSA mixer element
/// @param ch ALSA channel ID
/// @return Shared pointer to VolumeController instance (e.g. VolumeControllerDb, VolumeControllerLinear, or VolumeControllerDummy)
std::shared_ptr<VolumeController> VolumeController::create(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t ch) {
    if (!elem)
        return std::shared_ptr<VolumeController>(new VolumeControllerDummy(elem, ch));
    long dBMin, dBMax;
    if (snd_mixer_selem_get_playback_dB_range(elem, &dBMin, &dBMax) == 0 && dBMax > dBMin) {
        return std::shared_ptr<VolumeController>(new VolumeControllerDb(elem, ch));
    }
    long volMin, volMax;
    if (snd_mixer_selem_get_playback_volume_range(elem, &volMin, &volMax) == 0 && volMax > volMin) {
        return std::shared_ptr<VolumeController>(new VolumeControllerLinear(elem, ch));
    }
    return std::shared_ptr<VolumeController>(new VolumeControllerDummy(elem, ch));
}

/// @brief ALSA Volume implementation
/// This class implements the IVolume interface using ALSA mixer elements.
/// It supports setting and getting volume and balance for stereo and mono channels.
/// It uses VolumeController instances for each channel (left, right, mono) to handle the actual volume control.
class AMVolume : public IVolume {
private:
    std::string name;
    bool hasLeft{false};
    bool hasRight{false};

    int card;

    std::shared_ptr<VolumeController> leftVolumeController;
    std::shared_ptr<VolumeController> rightVolumeController;
    std::shared_ptr<VolumeController> monoVolumeController;

public:
    AMVolume(int card, snd_mixer_elem_t *elem) : card(card) {
        name=snd_mixer_selem_get_name(elem);
        hasLeft = snd_mixer_selem_has_playback_channel(elem, SND_MIXER_SCHN_FRONT_LEFT);
        hasRight = snd_mixer_selem_has_playback_channel(elem, SND_MIXER_SCHN_FRONT_RIGHT);
        leftVolumeController = VolumeController::create(elem, SND_MIXER_SCHN_FRONT_LEFT);
        rightVolumeController = VolumeController::create(elem, SND_MIXER_SCHN_FRONT_RIGHT);
        monoVolumeController = VolumeController::create(elem, SND_MIXER_SCHN_MONO);
    }

    /// @brief Set volume for the channel.
    /// If the channel is stereo (has both left and right), the volume is set according to the current balance.
    /// If the channel is mono, the volume is set directly.
    /// @param volume Volume percentage (0..100)
    void setVolume(int volume) override {

        volume = std::clamp(volume, 0, 100);
        if (hasLeft && hasRight) {
            auto balance = getBalance();                         // -100..100
            double balanceNorm = 1.0 - (abs(balance) / 100.0); // 0..1

            if (balance < 0) {
                // left louder
                leftVolumeController->setVolume(volume);
                rightVolumeController->setVolume(lround(volume * balanceNorm));
            }
            else if (balance > 0) {
                // right louder
                leftVolumeController->setVolume(lround(volume * balanceNorm));
                rightVolumeController->setVolume(volume);
            } else {
                // balanced
                leftVolumeController->setVolume(volume);
                rightVolumeController->setVolume(volume);
            }

        } else {
            monoVolumeController->setVolume(volume);
        }
    }

    const std::string getName() override {
        return name;
    }

    /// @brief Get current volume for the channel.
    /// If the channel is stereo (has both left and right), the maximum of left and right volumes is returned.
    /// If the channel is mono, the mono volume is returned.
    /// @return Current volume (0..100)
    int getVolume() override {
        int left = leftVolumeController->getVolume();
        int right = rightVolumeController->getVolume();
        int mono = monoVolumeController->getVolume();

        int resultInt = std::max(std::max(left, right), mono);

        return resultInt;
    }

    /// @brief Set balance for the channel.
    /// Balance is only applicable for stereo channels (has both left and right).
    /// Balance is in the range -100 (left only) to +100 (right only), with 0 being centered.
    /// The volume is adjusted accordingly to maintain the overall volume level
    /// @param balance Balance value (-100..100)
    void setBalance(int balance) override {
        // balance: -100 (left only) .. 0 (center) .. +100 (right only)
        if (!hasLeft || !hasRight)
            return;

        balance = std::clamp(balance, -100, 100);

        double balanceNorm = (100.0 - abs(balance)) / 100.0; // 0..1
        int volume = getVolume(); // current volume 0..100

        if (balance < 0) {
            // left louder
            leftVolumeController->setVolume(volume);
            rightVolumeController->setVolume(lround(volume * balanceNorm));
        }
        else if (balance > 0) {
            // right louder
            leftVolumeController->setVolume(lround(volume * balanceNorm));
            rightVolumeController->setVolume(volume);
        } else {
            // balanced
            leftVolumeController->setVolume(volume);
            rightVolumeController->setVolume(volume);
        }
    }

    /// @brief Get current balance for the channel.
    /// Balance is only applicable for stereo channels (has both left and right).
    /// Balance is in the range -100 (left only) to +100 (right only), with 0 being centered.
    /// @return Current balance (-100..100)
    int getBalance() override {
        if (!hasLeft || !hasRight)
            return 0;

        auto left = leftVolumeController->getVolume();
        auto right = rightVolumeController->getVolume();

        auto delta = right - left; // -100..100
        return delta;
    }
};

/// @brief ALSA Mixer implementation
/// This class implements the IMixer interface using ALSA mixer elements.
/// It enumerates all available ALSA mixer elements and creates AMVolume instances for each.
/// It provides access to the list of available volume channels.
/// It keeps the ALSA mixers alive to ensure the volume controls remain valid.
/// Note: The ALSA mixers are opened in the constructor and closed in the destructor.
///       This ensures that the volume controls remain valid as long as the AMixer instance exists.
class AMixer : public IMixer {
public:

    /// @brief Constructor
    /// Enumerates all ALSA cards and their mixer elements.
    /// Creates AMVolume instances for each mixer element that supports playback volume.
    AMixer()
    {
        int card = -1;
        if (snd_card_next(&card) < 0)
            return;
        while (card >= 0)
        {
            std::string hwname = std::format("hw:{}", card);

            snd_mixer_t *mixer = nullptr;
            if (snd_mixer_open(&mixer, 0) == 0 && mixer)
            {
                bool attached = (snd_mixer_attach(mixer, hwname.c_str()) == 0);
                if (attached &&
                    snd_mixer_selem_register(mixer, nullptr, nullptr) == 0 &&
                    snd_mixer_load(mixer) == 0)
                {
                    // Keep mixer alive
                    mixers.push_back(mixer);

                    for (snd_mixer_elem_t *elem = snd_mixer_first_elem(mixer); elem; elem = snd_mixer_elem_next(elem))
                    {
                        if (!snd_mixer_selem_is_active(elem))
                            continue;
                        if (!snd_mixer_selem_has_playback_volume(elem))
                            continue;

                        auto vol = std::shared_ptr<AMVolume>(new AMVolume(card, elem));
                        channelsList.push_back(vol);
                    }
                }
                else
                {
                    if (mixer)
                    {
                        snd_mixer_close(mixer);
                    }
                }
            }

            if (snd_card_next(&card) < 0)
                break;
        }
    }

    /// @brief Destructor
    /// Closes all ALSA mixers to release resources.
    ~AMixer() override
    {
        for (auto *m : mixers)
        {
            if (m)
                snd_mixer_close(m);
        }
        mixers.clear();
    }

    /// @brief Get list of available volume channels.
    /// @return List of shared pointers to IVolume instances.
    const std::list<std::shared_ptr<IVolume>> &channels() const override {
        return const_cast<std::list<std::shared_ptr<IVolume>> &>(this->channelsList);
    }

private:
    std::list<std::shared_ptr<IVolume>> channelsList;
    std::vector<snd_mixer_t *> mixers; // keep mixers alive
};

/// @brief Get singleton instance of ALSA mixer.
/// This instance is created on first call and destroyed on program exit.
/// @return Reference to IMixer instance supporting ALSA.
IMixer &IMixer::alsaInstance()
{
    static AMixer alsaMixer;
    return alsaMixer;
}
