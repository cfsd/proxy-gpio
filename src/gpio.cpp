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

#include "cluon-complete.hpp"
#include "opendlv-standard-message-set.hpp"

#include "proxy-gpio.hpp"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <cmath>
#include <ctime>
#include <chrono>


int32_t main(int32_t argc, char **argv) {
    int32_t retCode{0};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ((0 == commandlineArguments.count("cid"))) {
        std::cerr << argv[0] << " is a module for reading and writing to GPIOs on the BeagleBone Black" << std::endl;
        std::cerr << "Usage:   " << argv[0] << "--cid=<OpenDaVINCI session> [--id=<Identifier in case of multiple beaglebone units>] [--verbose]" << std::endl;
        std::cerr << "Example: " << argv[0] << "--cid=220 --id=1 --verbose=1 --freq=30" << std::endl;
        retCode = 1;
    } else {
        const uint32_t ID{(commandlineArguments["id"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["id"])) : 0};
        const bool VERBOSE{commandlineArguments.count("verbose") != 0};
	    const float FREQ{std::stof(commandlineArguments["freq"])};

        std::cout << "Micro-Service ID:" << ID << std::endl;

        // Interface to a running OpenDaVINCI session.
        Gpio gpio(VERBOSE, ID);

        cluon::data::Envelope data;
        cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))};

        uint32_t errorCount = 0;
        
        auto onSwitchStateRequest{[&gpio, &VERBOSE, &errorCount](cluon::data::Envelope &&envelope)
        {   
            if (!gpio.getInitialised()){
                return;
            }
            auto const gpioState = cluon::extractMessage<opendlv::proxy::SwitchStateRequest>(std::move(envelope));
            int16_t pin = envelope.senderStamp()-gpio.getSenderStampOffsetGpio();
            bool value = gpioState.state();
            if (pin == 27){ //Heartbeat
                errorCount = 0;
            }else{
                gpio.SetValue(pin, value);
            }
            if (VERBOSE){
                std::cout << "[GPIO-WRITE] Recieved request:\tpin: " << pin << "\tvalue: " << value << std::endl;
            }
        }};
        od4.dataTrigger(opendlv::proxy::SwitchStateRequest::ID(), onSwitchStateRequest);
        
        auto heartbeatThread{[&gpio, &od4, &errorCount]()
        {
            if (!gpio.getInitialised()){
                return;
            }
            using namespace std::literals::chrono_literals;
            bool heatbeat = 0;

            std::chrono::system_clock::time_point threadTime = std::chrono::system_clock::now();


            while (true) {
                if (errorCount <= 2){
                    errorCount++;
                    heatbeat = !heatbeat;
                    gpio.SetValue(27, heatbeat);
                    std::this_thread::sleep_until(std::chrono::duration<double>(0.033)+threadTime);
                    threadTime = std::chrono::system_clock::now();
                }
            }

        }};
        std::thread hbThread(heartbeatThread);
        
        auto atFrequency{[&od4, &gpio]() -> bool
        {            
            gpio.body(od4);
            return true;
        }};

        od4.timeTrigger(FREQ, atFrequency);
    }
    return retCode;
}

