// Copyright 2014 Google Inc. All Rights Reserved.

#include "oemcrypto_logging.h"

#include <stdio.h>

namespace wvoec_mock {

int logging_category_setting = 0x00;

void SetLoggingSettings(int level, int categories) {
  SetLoggingLevel(level);
  TurnOffLoggingForAllCategories();
  AddLoggingForCategories(categories);
}

void TurnOffLoggingForAllCategories() {
  logging_category_setting = 0;
}

void SetLoggingLevel(int level){;
  wvcdm::g_cutoff =  static_cast<wvcdm::LogPriority>(level);
}

void SetLoggingLevel(wvcdm::LogPriority level) {
  wvcdm::g_cutoff = level;
}

void AddLoggingForCategories(int categories) {
  logging_category_setting |= categories;
}

void RemoveLoggingForCategories(int categories) {
  logging_category_setting &= ~categories;
}

bool LogCategoryEnabled(int categories) {
  return ( (logging_category_setting & categories) !=0 );
}

void dump_hex_helper(std::string& buffer, std::string name,
                            const uint8_t* vector, size_t length) {
  buffer += name + " = ";
  if (vector == NULL) {
    buffer +="NULL;\n";
    LOGE(buffer.c_str());
    return;
  }
  int a, b;
  char int_to_hexcar[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
            '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  for (size_t i = 0; i < length; i++) {
    if (i == 0) {
      buffer += "\n     wvcdm::a2b_hex(\"";
    } else if (i % 32 == 0) {
      buffer += "\"\n                    \"";
    }
    a = vector[i] % 16;
    b = (vector[i] - a) / 16;
    buffer += int_to_hexcar[b];
    buffer += int_to_hexcar[a];
  }
  buffer += "\");\n";
}

void dump_hex(std::string name, const uint8_t* vector, size_t length) {
  std::string buffer="";
  dump_hex_helper(buffer, name, vector, length);
  LOGV(buffer.c_str());
}

void dump_array_part_helper(std::string& buffer, std::string array,
                            size_t index, std::string name,
                            const uint8_t* vector, size_t length) {
  char index_str[256];

  snprintf(index_str, sizeof index_str, "%zu", index);

  if (vector == NULL) {
    buffer += array.c_str();
    buffer += "[";
    buffer += index_str;
    buffer += "].";
    buffer += name.c_str();
    buffer += " = NULL;\n";
    LOGW(buffer.c_str());
    return;
  }
  buffer += "std::string s";
  buffer += index_str;
  buffer+= "_";
  dump_hex_helper(buffer, name, vector, length);
  buffer += array.c_str();
  buffer += "[";
  buffer += index_str;
  buffer += "]." + name + " = message_ptr + message.find(s";
  buffer += index_str;
  buffer += "_" + name + ".data());\n";
}

void dump_array_part(std::string array, size_t index,
                     std::string name, const uint8_t* vector, size_t length) {
  std::string buffer ="";
  dump_array_part_helper(buffer, array, index, name, vector, length);
  LOGV(buffer.c_str());
}

} // namespace wvoec_mock

