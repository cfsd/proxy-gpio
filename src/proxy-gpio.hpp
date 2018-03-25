/*
 * Copyright (C) 2018  Christian Berger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GPIO
#define GPIO

#include "opendlv-standard-message-set.hpp"

#include <memory>
#include <string>
#include <vector>
#include <utility>

class Gpio {
   private:
    //Gpio(const Gpio &) = delete;
    //Gpio(Gpio &&)      = delete;
    //Gpio &operator=(const Gpio &) = delete;
    //Gpio &operator=(Gpio &&) = delete;

   public:
    Gpio(bool VERBOSE, uint32_t ssOffsetGpio);
    ~Gpio();

   public:
    float decode(const std::string &data) noexcept;
    void callOnReceive(cluon::data::Envelope data);
    void body(cluon::OD4Session &od4);

   private:
    void setUp();
    void tearDown();

    void OpenGpio();
    void CloseGpio();
    void Reset();
    void SetDirection(uint16_t const, std::string);
    std::string GetDirection(uint16_t const) const;
    void SetValue(uint16_t const, bool const);
    bool GetValue(uint16_t const) const;

    bool m_debug;
    uint32_t m_bbbId;
    uint32_t m_senderStampOffsetGpio;
    bool m_initialised;
    std::vector<std::pair<bool, std::string>> m_initialValuesDirections;
    std::string m_path;
    std::vector<uint16_t> m_pins;
};

#endif

