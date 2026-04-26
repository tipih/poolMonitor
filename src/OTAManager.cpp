#include "OTAManager.h"

OTAManager::OTAManager()
  : _initialized(false) {
}

void OTAManager::begin(const char* hostname, const char* password) {
  // Set hostname if provided
  if (hostname != nullptr) {
    ArduinoOTA.setHostname(hostname);
  }

  // Set password if provided
  if (password != nullptr) {
    ArduinoOTA.setPassword(password);
  }

  // Configure OTA callbacks
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {
      type = "filesystem";
    }
    Serial.println("OTA: Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA: Update complete");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });

  // Start OTA service
  ArduinoOTA.begin();
  _initialized = true;

  Serial.println("OTA Manager initialized and ready");
}

void OTAManager::handle() {
  if (_initialized) {
    ArduinoOTA.handle();
  }
}
