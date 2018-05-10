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
#include "proxy-gpio.hpp"

// #include <cmath>
#include <fstream>
#include <iostream>
#include <cstring>
#include <vector>
// #include <array>
// #include <sstream>
#include <string>
//#include <ctime>
//#include <chrono>

float Gpio::decode(const std::string &data) noexcept {
    float temp = std::stof(data);
    return temp;
}

Gpio::Gpio(bool VERBOSE, uint32_t id)
    //: TimeTriggeredConferenceClientModule(argc, argv, "proxy-miniature-gpio")
    : m_debug(VERBOSE)
    , m_bbbId(id)
    , m_senderStampOffsetGpio(id*1000)
    , m_initialised()
    , m_initialValuesDirections()
    , m_path()
    , m_pins()
    , m_gpioValueFileIn()
    , m_gpioValueFileOut()
{
	Gpio::setUp();
}

Gpio::~Gpio() 
{
}

void Gpio::setUp()
{
  //odcore::base::KeyValueConfiguration kv = getKeyValueConfiguration();

  m_path = "/sys/class/gpio";
  std::vector<std::string> pinsVector = {"26","27","44","45","46","47","49","61","62","65","66", "67", "68", "69", "112", "115"};
  std::vector<std::string> initialValuesVector = {"0","0","0","0","0","0","0","0","0","0", "0", "0", "0", "0", "0", "0"};
  std::vector<std::string> initialDirectionsVector = {"out","out","out","out","out","out","in","out","out","out","out", "out", "out", "out", "in", "in"};

  if (pinsVector.size() == initialValuesVector.size() 
      && pinsVector.size() == initialDirectionsVector.size()) {
    for (uint32_t i = 0; i < pinsVector.size(); i++) {
      uint16_t pin = std::stoi(pinsVector.at(i));
      bool value = static_cast<bool>(std::stoi(initialValuesVector.at(i)));
      std::string direction = initialDirectionsVector.at(i);
      if (direction.compare("out") == 0 || direction.compare("in") == 0) {
        m_pins.push_back(pin);
        m_initialValuesDirections.push_back(std::make_pair(value, direction));
      } else {
        std::cerr << "[GPIO] " << "Invalid direction for pin " 
            << pin << "." << std::endl;
      }
    }
    if (m_debug) {
      std::cout << "[GPIO] " << "Initialised pins: ";
      for (auto pin : m_pins) {
         std::cout << pin << " ";
      }
      std::cout << "(Value, direction): ";
      for (auto pair : m_initialValuesDirections) {
        std::cout << "(" << pair.first << "," << pair.second << ") ";
      }
      std::cout << std::endl;
    }
  } else {
    std::cerr << "[GPIO] Number of pins do not equals to number of values or directions" 
        << std::endl;
  }

  OpenGpio();

  m_initialised = true;
}

void Gpio::tearDown()
{
  CloseGpio();
}

void Gpio::body(cluon::OD4Session &od4)
{


    cluon::data::TimeStamp sampleTime = cluon::time::now();
    opendlv::proxy::SwitchStateReading msg;

    for (uint16_t i = 0; i < m_pins.size(); i++) {
        if((m_initialValuesDirections[i].second).compare("out") != 0){
          uint16_t pin = m_pins[i];
          bool pinState = GetValue(pin);
          int16_t senderStamp = (int16_t) pin + m_senderStampOffsetGpio;
          msg.state(pinState);
          od4.send(msg, sampleTime, senderStamp);
        }
    }
    if (false) {
      std::cout << "Number of pins: " << m_pins.size() << std::endl;
      for (auto pin : m_pins) {
        std::cout << "[GPIO] Pin: " << pin 
            << " Direction: " << GetDirection(pin) 
            << " Value: " << GetValue(pin) 
            << "." << std::endl;
      }
    }
}
/*
void Gpio::callOnReceive(cluon::data::Envelope data){
    if (!m_initialised) {
        return;
    }
    if (data.dataType() == static_cast<int32_t>(opendlv::proxy::SwitchStateRequest::ID())) {
        opendlv::proxy::SwitchStateRequest gpioState = cluon::extractMessage<opendlv::proxy::SwitchStateRequest>(std::move(data));
        uint16_t pin = data.senderStamp()-m_senderStampOffsetGpio;
        bool value = gpioState.state();

        if (m_debug){
          std::cout << "[GPIO-WRITE] Recieved request:\tpin: " << pin << "\tvalue: " << value << std::endl;
        }
        
        if (GetDirection(pin).compare("out") == 0) {
        SetValue(pin, value);
        } else {
        std::cout << "[GPIO-WRITE] The requested pin " << pin
            << " is read-only." 
            << std::endl;
        }

        //std::cout << "Test function: State:" << value << " Pin:" << pin << std::endl;
    }else if (m_debug && data.dataType() == static_cast<int32_t>(opendlv::proxy::SwitchStateReading::ID())) {
        opendlv::proxy::SwitchStateReading gpioState = cluon::extractMessage<opendlv::proxy::SwitchStateReading>(std::move(data));
        uint16_t pin = data.senderStamp()-m_senderStampOffsetGpio;
        bool value = gpioState.state();

        std::cout << "[GPIO-READ] The read pin: " << pin
            << " state:"
 	    << value
            << std::endl;
     }

}*/

void Gpio::OpenGpio()
{
  std::string filename = m_path + "/export";
  std::ofstream exportFile(filename, std::ofstream::out);
  
  if (exportFile.is_open()) {
    for (auto pin : m_pins) {
      exportFile << pin;
      exportFile.flush();
      std::cout << "[GPIO] Exported pin: " << pin << std::endl;

      std::string gpioValueFilename =  m_path + "/gpio" + std::to_string(pin) + "/value";
      m_gpioValueFileIn[pin] = new std::ifstream(gpioValueFilename, std::fstream::in);
      m_gpioValueFileOut[pin] = new std::ofstream(gpioValueFilename, std::fstream::out);
    if ((*m_gpioValueFileIn[pin]).is_open() && (*m_gpioValueFileOut[pin]).is_open()) {
    } else {
      std::cout << "[GPIO] Could not open " << gpioValueFilename << "." << std::endl;
    }


    }
    Reset();
  } else {
    std::cout << "[GPIO] Could not open " << filename << "." 
        << std::endl;
  }
  exportFile.close();
}

void Gpio::CloseGpio()
{
  std::string filename = m_path + "/unexport";
  std::ofstream unexportFile(filename, std::ofstream::out);
  
  if (unexportFile.is_open()) {
    for (auto pin : m_pins) {
      unexportFile << pin;
      unexportFile.flush();
      m_gpioValueFileIn[pin]->close();
      m_gpioValueFileOut[pin]->close();
    }
  } else {
    std::cout << "[GPIO] Could not open " << filename << "." 
        << std::endl;
  }
  unexportFile.close();
}

void Gpio::Reset()
{
  for (uint16_t i = 0; i < m_pins.size(); i++) {
    uint16_t pin = m_pins[i];
    bool initialValue = m_initialValuesDirections[i].first;
    std::string initialDirection = m_initialValuesDirections[i].second;
    SetDirection(pin, initialDirection);

    if (initialDirection.compare("out") == 0) {
      SetValue(pin, initialValue);
    }
  }
}

void Gpio::SetDirection(uint16_t const a_pin, std::string const a_str)
{
  std::string gpioDirectionFilename = m_path + "/gpio" + std::to_string(a_pin) 
      + "/direction";

  std::ofstream gpioDirectionFile(gpioDirectionFilename, std::ofstream::out);
  if (gpioDirectionFile.is_open()) {
    gpioDirectionFile << a_str;
    gpioDirectionFile.flush();
  } else {
    std::cout << "[GPIO] Could not open " << gpioDirectionFilename 
        << "." << std::endl;
  }

  gpioDirectionFile.close();
}

std::string Gpio::GetDirection(uint16_t const a_pin) const
{
  std::string gpioDirectionFilename = m_path + "/gpio" + std::to_string(a_pin) 
      + "/direction";
  std::string line;

  std::ifstream gpioDirectionFile(gpioDirectionFilename, std::ifstream::in);
  if (gpioDirectionFile.is_open()) {
    std::getline(gpioDirectionFile, line);
    std::string direction = line;
    gpioDirectionFile.close();
    return direction;
  } else {
    std::cout << "[GPIO] Could not open " << gpioDirectionFilename 
        << "." << std::endl;
    gpioDirectionFile.close();
    return "";
  }
}

void Gpio::SetValue(uint16_t const a_pin, bool const a_value)
{
    if (!m_initialised) {
        return;
    }

    if (GetDirection(a_pin).compare("out") == 0) {
    } else {
      std::cout << "[GPIO-WRITE] The requested pin " << a_pin
        << " is read-only." 
        << std::endl;
      return;
    }
  //std::string gpioValueFilename =       m_path + "/gpio" + std::to_string(a_pin) + "/value";

  //std::ofstream gpioValueFile(gpioValueFilename, std::ofstream::out);
  if ((*m_gpioValueFileOut[a_pin]).is_open()) {
    (*m_gpioValueFileOut[a_pin]).seekp(0);
    (*m_gpioValueFileOut[a_pin]) << static_cast<uint16_t>(a_value);
    (*m_gpioValueFileOut[a_pin]).flush();
  } else {
    std::cout << "[GPIO] Could not open " << a_pin 
        << "." << std::endl;
  }
}

bool Gpio::GetValue(uint16_t const a_pin)
{
  //std::string gpioValueFilename = m_path + "/gpio" + std::to_string(a_pin) + "/value";
  std::string line;
  //char c;
  //std::ifstream gpioValueFile(gpioValueFilename, std::ifstream::in);
  if ((*m_gpioValueFileIn[a_pin]).is_open()) {
    (*m_gpioValueFileIn[a_pin]).sync();
    (*m_gpioValueFileIn[a_pin]).seekg(0);
    //if (m_debug){
    //  std::cout << "[GPIO-READ] " << " Pin: " << a_pin << std::endl; 
    //}
    std::getline((*m_gpioValueFileIn[a_pin]), line);
    bool value = (std::stoi(line) == 1);
    //c = (*m_gpioValueFileIn[a_pin]).peek();
    //bool value = ((c - '0') == 1);
    //if (m_debug){
    //  std::cout << "[GPIO-READ] Value: " << value << " Pin: " << a_pin << std::endl; 
    //}
    return value;
  } else {
    std::cout << "[GPIO] Could not open " << a_pin 
        << "." << std::endl;
    return NULL;
  }
}


uint32_t Gpio::getSenderStampOffsetGpio(){
  return m_senderStampOffsetGpio;
}
bool Gpio::getInitialised(){
  return m_initialised;
}
