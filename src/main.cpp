#include <Arduino.h>
#include <EventManager.hpp>

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Evt::on({"QWE"}, [](std::string data) -> Evt::EvSignal {

    Serial.println(data.c_str());

    return Evt::CONTINUE;
  });

  Evt::on({"QWE"}, [](std::string data) -> Evt::EvSignal {

    Serial.println(data.c_str());

    return Evt::CONTINUE;
  });
}

void loop() {
  Evt::run();
  Evt::emit("QWE", "data");
  delay(2000);
}